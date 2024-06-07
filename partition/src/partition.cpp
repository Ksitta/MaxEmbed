#include "partition.h"
#include <atomic>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <omp.h>
#include <random>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>
#include <thread>
#include <unordered_set>

mapping_info read_mapping(char *filepath, int &part_cnt){
  mapping_info info;
  part_cnt = 0;
  FILE *file = fopen(filepath, "rb");
  CHECK(file) << "file " << filepath << " open failed";
  struct stat file_info;
  CHECK(stat(filepath, &file_info) == 0) << "stat failed";
  size_t file_size = file_info.st_size;
  info.raw_ptr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fileno(file), 0);
  int vertex_cnt = *(uint64_t *)info.raw_ptr;
  info.total_cnt = *(uint64_t *)((char *)info.raw_ptr + sizeof(uint64_t));
  info.idx = (int *)((char *)info.raw_ptr + sizeof(uint64_t) * 2);
  info.cnt = (int *)((char *)info.raw_ptr + sizeof(uint64_t) * 2 + vertex_cnt * sizeof(int));
  info.mapping = (NodeType *)((char *)info.raw_ptr + sizeof(uint64_t) * 2 + vertex_cnt * 2 * sizeof(int));
  info.file_size = file_size;
  for(int i = 0; i < vertex_cnt; i++){
    part_cnt = std::max(part_cnt, (int)info.mapping[i]);
  }
  part_cnt++;
  info.part_cnt = part_cnt;
  info.vertex_cnt = vertex_cnt;
  return info;
}

void read_binary_hypergraph(char *filename, IndexType &c, IndexType &n,
                            IndexType *&xpins, NodeType *&pins) {
  FILE *fp = fopen(filename, "rb");
  assert(fread(&n, sizeof(IndexType), 1, fp) == 1);
  assert(fread(&c, sizeof(IndexType), 1, fp) == 1);
  xpins = (IndexType *)malloc((n + 1) * sizeof(IndexType));
  assert(fread(xpins, sizeof(IndexType), n + 1, fp) == n + 1);
  pins = (NodeType *)malloc(xpins[n] * sizeof(NodeType));
  assert(xpins[n] == fread(pins, sizeof(NodeType), xpins[n], fp));
}

void read_hypergraph(char *filename, IndexType &c, IndexType &n,
                     IndexType *&xpins, NodeType *&pins) {

  FILE *fp = fopen(filename, "r");
  IndexType starting;
  IndexType num_pins;
  assert(fscanf(fp, "%ld%ld%ld%ld", &starting, &c, &n, &num_pins) == 4);
  assert(starting >= 0 && c >= 0 && n >= 0 && num_pins >= 0);
  xpins = (IndexType *)malloc((n + 1) * sizeof(IndexType));
  pins = (NodeType *)malloc(num_pins * sizeof(NodeType));

  IndexType current_line = 0;
  IndexType current_ele = 0;
  int ch = fgetc(fp);
  xpins[0] = 0;
  std::unordered_set<int> set;
  while (ch != EOF) {
    while (!isdigit(ch)) {
      if (ch == '\n' && current_ele != 0) {
        xpins[++current_line] = current_ele;
        assert(current_line <= n);
        set.clear();
      }
      if (ch == EOF)
        break;
      ch = fgetc(fp);
    }
    if (ch == EOF)
      break;
    IndexType num = 0;
    while (isdigit(ch)) {
      num = num * 10 + ch - '0';
      ch = fgetc(fp);
    }
    num -= starting;

    if (set.insert(num).second) {
      pins[current_ele++] = num;
    }

    assert(0 <= num && num < c);
    assert(current_ele <= num_pins);
    //    if (current_ele % 100000 == 0) printf("!!!!\n");
  }
  if (current_line == n - 1)
    xpins[++current_line] = current_ele;
}

static NodeType *p_pins;
static IndexType *idx;
static IndexType *p_partition;
static bool *part_info;
static std::atomic<IndexType> part_id = 0;
static std::random_device rd;
static std::mt19937 mt(rd());
static std::atomic<int> exist_thread = 1;
static const int THREAD_NUM = 48;
static const int OMP_THREAD_NUM = 32;
static float *gain[OMP_THREAD_NUM];
static float *table;
static uint64_t g_longest_edge;

IndexType get_part_id() { return part_id.fetch_add(1, std::memory_order_relaxed); }

void split_edge(const std::vector<edge> *edge_vec_raw,
                std::vector<edge> *edge_vec_1, std::vector<edge> *edge_vec_2) {
  edge_vec_1->reserve(edge_vec_raw->size());
  edge_vec_2->reserve(edge_vec_raw->size());
  std::vector<uint16_t> edge_vec_1_len(edge_vec_raw->size());
  std::vector<uint16_t> edge_vec_2_len(edge_vec_raw->size());
#pragma omp parallel for num_threads(OMP_THREAD_NUM)
  for (IndexType i = 0; i < edge_vec_raw->size(); i++) {
    std::vector<NodeType> temp_pins(g_longest_edge);
    const auto &edge = (*edge_vec_raw)[i];
    uint16_t part_0_cnt = 0;
    uint16_t part_1_cnt = 0;
    const uint16_t edge_size = edge.second - edge.first;
    for (IndexType i = edge.first; i < edge.second; i++) {
      if (part_info[p_pins[i]] == 0) {
        temp_pins[part_0_cnt] = p_pins[i];
        part_0_cnt++;
      } else {
        temp_pins[edge_size - part_1_cnt - 1] = p_pins[i];
        part_1_cnt++;
      }
    }
    for(IndexType j = 0; j < edge_size; j++){
      p_pins[j + edge.first] = temp_pins[j];
    }
    edge_vec_1_len[i] = part_0_cnt;
    edge_vec_2_len[i] = part_1_cnt;
  }
  for (size_t i = 0; i < edge_vec_raw->size(); i++) {
    uint16_t part_0_cnt = edge_vec_1_len[i];
    uint16_t part_1_cnt = edge_vec_2_len[i];
    const auto &edge = (*edge_vec_raw)[i];
    if (part_0_cnt > 1) {
      edge_vec_1->push_back({edge.first, edge.first + part_0_cnt});
    }
    if (part_1_cnt > 1) {
      edge_vec_2->push_back({edge.first + part_0_cnt, edge.second});
    }
  }
  delete edge_vec_raw;
}

void devide_core(IndexType start, IndexType mid, IndexType end,
                 const std::vector<edge> *related_edge) {
  for (IndexType i = start; i < mid; i++) {
    part_info[idx[i]] = 0;
  }

  for (IndexType i = mid; i < end; i++) {
    part_info[idx[i]] = 1;
  }
  for (int _ = 0; _ < MAX_ITER; _++) {
    if (related_edge->size() > 100000) {
#pragma omp parallel for num_threads(OMP_THREAD_NUM)
      for (int i = 0; i < OMP_THREAD_NUM; i++) {
        for (IndexType j = start; j < end; j++) {
          gain[i][idx[j]] = 0;
        }
      }
#pragma omp parallel for num_threads(OMP_THREAD_NUM)
      for (auto edge_id : *related_edge) {
        int tid = omp_get_thread_num();
        uint16_t part_0_cnt = 0, part_1_cnt = 0;
        for (IndexType indices = edge_id.first; indices < edge_id.second;
             indices++) {
          NodeType node = p_pins[indices];
          part_0_cnt += (part_info[node] == 0);
          part_1_cnt += (part_info[node] == 1);
        }

        float p1 = -(table[part_1_cnt] - table[part_0_cnt - 1]);
        float p2 = -(table[part_0_cnt] - table[part_1_cnt - 1]);

        for (IndexType indices = edge_id.first; indices < edge_id.second;
             indices++) {
          NodeType node = p_pins[indices];
          if (part_info[node] == 0) {
            gain[tid][node] += p1;
          } else {
            gain[tid][node] += p2;
          }
        }
      }
#pragma omp parallel for num_threads(OMP_THREAD_NUM)
      for (IndexType i = start; i < end; i++) {
        for (int tid = 1; tid < OMP_THREAD_NUM; tid++) {
          gain[0][idx[i]] += gain[tid][idx[i]];
        }
      }
    } else {
      for (IndexType i = start; i < end; i++) {
        gain[0][idx[i]] = 0;
      }
      for (auto edge_id : *related_edge) {
        uint16_t part_0_cnt = 0, part_1_cnt = 0;
        for (IndexType indices = edge_id.first; indices < edge_id.second;
             indices++) {
          NodeType node = p_pins[indices];
          part_0_cnt += (part_info[node] == 0);
          part_1_cnt += (part_info[node] == 1);
        }

        float p1 = -(table[part_1_cnt] - table[part_0_cnt - 1]);
        float p2 = -(table[part_0_cnt] - table[part_1_cnt - 1]);

        for (IndexType indices = edge_id.first; indices < edge_id.second;
             indices++) {
          NodeType node = p_pins[indices];
          if (part_info[node] == 0) {
            gain[0][node] += p1;
          } else {
            gain[0][node] += p2;
          }
        }
      }
    }

    tbb::parallel_sort(idx + start, idx + mid, [](IndexType a, IndexType b) {
      return gain[0][a] > gain[0][b];
    });
    tbb::parallel_sort(idx + mid, idx + end, [](IndexType a, IndexType b) {
      return gain[0][a] > gain[0][b];
    });

    IndexType i;
    for (i = start; i < mid; i++) {
      IndexType next_i = mid + i - start;
      if (next_i < end) {
        if (gain[0][idx[i]] + gain[0][idx[next_i]] > 0) {
          std::swap(idx[i], idx[next_i]);
          part_info[idx[i]] = 0;
          part_info[idx[next_i]] = 1;
        } else {
          break;
        }
      } else {
        break;
      }
    }
    if (i == start) {
      return;
    }
  }
}

void divide(IndexType start, IndexType end, int upper_bound,
            const std::vector<edge> *related_edge, bool fork) {
  if (end - start <= upper_bound) {
    delete related_edge;
    if (end - start == 0) {
      return;
    }
    IndexType part_id = get_part_id();
    for (IndexType i = start; i < end; i++) {
      p_partition[idx[i]] = part_id;
    }
    return;
  }

  IndexType mid = start + (end - start) / (2 * upper_bound) * upper_bound;

  if ((end - start) < 2 * upper_bound) {
    mid = start + (end - start) / 2;
  }

  devide_core(start, mid, end, related_edge);

  std::vector<edge> *related_edge_1 = new std::vector<edge>;
  std::vector<edge> *related_edge_2 = new std::vector<edge>;
  split_edge(related_edge, related_edge_1, related_edge_2);
  int thread_cnt = exist_thread.fetch_add(1);

  if (thread_cnt <= THREAD_NUM &&
      (related_edge_1->size() > 10000 || related_edge_2->size() > 10000)) {
    std::thread t1(divide, start, mid, upper_bound, related_edge_1, true);
    divide(mid, end, upper_bound, related_edge_2, false);
    exist_thread.fetch_sub(1);
    t1.join();
    exist_thread.fetch_add(1);
  } else {
    exist_thread.fetch_sub(1);
    divide(start, mid, upper_bound, related_edge_1, false);
    divide(mid, end, upper_bound, related_edge_2, false);
  }
  if (fork) {
    exist_thread.fetch_sub(1);
  }
}

void divide_start(IndexType start, IndexType end, IndexType parts,
             const std::vector<edge> *related_edge) {
  IndexType upper_bound_low = (end - start) / parts;
  IndexType upper_bound = (end - start + parts - 1) / parts;
  IndexType res = end - start - upper_bound_low * parts;
  IndexType mid = start + res * upper_bound;
  if (upper_bound == upper_bound_low) {
    mid = start + (end - start) / (2 * upper_bound) * upper_bound;
  }

  devide_core(start, mid, end, related_edge);

  std::vector<edge> *related_edge_1 = new std::vector<edge>;
  std::vector<edge> *related_edge_2 = new std::vector<edge>;
  split_edge(related_edge, related_edge_1, related_edge_2);
  int thread_cnt = exist_thread.fetch_add(1);
  if (thread_cnt <= THREAD_NUM && (related_edge_1->size() > 10000 || related_edge_2->size() > 10000)) {
    std::thread t1(divide, start, mid, upper_bound, related_edge_1, true);
    divide(mid, end, upper_bound_low, related_edge_2, false);
    exist_thread.fetch_sub(1);
    t1.join();
    exist_thread.fetch_add(1);
  } else {
    exist_thread.fetch_sub(1);
    divide(start, mid, upper_bound, related_edge_1, false);
    divide(mid, end, upper_bound_low, related_edge_2, false);
  }
}

IndexType partition(IndexType c, IndexType parts, NodeType *pins,
                    std::vector<edge> *related_edge, IndexType *part,
                    int longest_edge, int cnt_per_part) {
  table = new float[longest_edge + 1];
  g_longest_edge = longest_edge;
  table++;
  for (int i = -1; i < longest_edge; i++) {
    table[i] = powf((1 - p), i);
  }
  bool *appeared = new bool[c];
  memset(appeared, 0, sizeof(bool) * c);
  omp_set_num_threads(OMP_THREAD_NUM);
#pragma omp parallel for
  for (auto each : *related_edge) {
    for (IndexType i = each.first; i < each.second; i++) {
      appeared[pins[i]] = true;
    }
  }
  idx = new IndexType[c];
  for (int i = 0; i < OMP_THREAD_NUM; i++) {
    gain[i] = new float[c];
  }
  part_info = new bool[c];
  p_partition = part;
  p_pins = pins;
  mt.seed(1);
  part_id.store(0);
  IndexType appear_cnt = 0;
  IndexType dis_cnt = 0;
  for (IndexType i = 0; i < c; i++) {
    if (appeared[i]) {
      idx[appear_cnt++] = i;
    } else {
      idx[c - 1 - dis_cnt++] = i;
    }
  }
  delete[] appeared;
  assert(appear_cnt + dis_cnt == c);
  std::shuffle(idx, idx + appear_cnt, mt);
  int dst_parts = (appear_cnt + cnt_per_part - 1) / cnt_per_part;
  divide_start(0, appear_cnt, dst_parts, related_edge);
  int cnt_in_part = 0;
  for (IndexType i = appear_cnt; i < c; i++) {
    if (cnt_in_part == cnt_per_part) {
      cnt_in_part = 0;
      part_id++;
    }
    p_partition[idx[i]] = part_id;
    cnt_in_part++;
  }
  delete[] idx;
  for (int i = 0; i < OMP_THREAD_NUM; i++) {
    delete[] gain[i];
  }
  delete[] part_info;
  table--;
  delete[] table;
  return part_id;
}
