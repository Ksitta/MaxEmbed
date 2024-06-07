#include "partition.h"
#include <folly/GLog.h>
#include <folly/container/F14Map.h>
#include <folly/container/F14Set.h>
#include <iostream>
#include <omp.h>
#include <string.h>
#include <tbb/parallel_sort.h>
#include <unordered_set>
#include <vector>
#include <sys/stat.h>
#include <sys/mman.h>

template <typename K, typename V>
using FastMap =
    folly::F14FastMap<K, V, std::hash<K>, std::equal_to<K>,
                      folly::f14::DefaultAlloc<std::pair<K const, V>>>;

template <typename K>
using FastSet = folly::F14FastSet<K, std::hash<K>, std::equal_to<K>,
                                  folly::f14::DefaultAlloc<K>>;

int remove_low_freq(IndexType c, IndexType n, IndexType *xpins,
                    NodeType *pins) {
  IndexType new_cnt = 0;
  int *appear_cnt = new int[c];
  memset(appear_cnt, 0, sizeof(int) * c);
  for (IndexType i = 0; i < xpins[n]; i++) {
    appear_cnt[pins[i]]++;
  }
  for (IndexType i = 0; i < n; i++) {
    IndexType start = xpins[i];
    IndexType end = xpins[i + 1];
    xpins[i] = new_cnt;
    for (IndexType j = start; j < end; j++) {
      if (appear_cnt[pins[j]] > 1) {
        pins[new_cnt] = pins[j];
        new_cnt++;
      }
    }
  }
  xpins[n] = new_cnt;
  IndexType new_edge_cnt = 0;
  IndexType move_offset = 0;
  // remove edges that have only one node or no node
  for (IndexType i = 0; i < n; i++) {
    IndexType start = xpins[i];
    IndexType end = xpins[i + 1];
    if (end - start > 1) {
      xpins[new_edge_cnt] = xpins[i] - move_offset;
      for (IndexType j = start; j < end; j++) {
        pins[j - move_offset] = pins[j];
      }
      new_edge_cnt++;
    } else {
      move_offset += end - start;
    }
  }
  xpins[new_edge_cnt] = xpins[n] - move_offset;

  return new_edge_cnt;
}


void get_node2edge(IndexType c, IndexType n, IndexType *xpins, NodeType *pins,
                   IndexType *&node2edge_idx, IndexType *&node2edge_data) {
  node2edge_data = new IndexType[xpins[n]];
  node2edge_idx = new IndexType[c + 1];
  int *node2edge_cnt = new int[c];

  memset(node2edge_idx, 0, sizeof(IndexType) * (c + 1));
  memset(node2edge_cnt, 0, sizeof(int) * c);

  std::atomic_int *atomic_cnt_ptr = (std::atomic_int *)node2edge_cnt;

#pragma omp parallel for
  for (IndexType i = 0; i < xpins[n]; i++) {
    atomic_cnt_ptr[pins[i]].fetch_add(1);
  }

  for (IndexType i = 1; i < c + 1; i++) {
    node2edge_idx[i] = node2edge_idx[i - 1] + node2edge_cnt[i - 1];
  }

  memset(node2edge_cnt, 0, sizeof(int) * c);

#pragma omp parallel for
  for (IndexType i = 0; i < n; i++) {
    for (IndexType j = xpins[i]; j < xpins[i + 1]; j++) {
      IndexType pos = atomic_cnt_ptr[pins[j]].fetch_add(1);
      node2edge_data[pos + node2edge_idx[pins[j]]] = i;
    }
  }

  delete[] node2edge_cnt;
}

int main(int argc, char **argv) {
  if (argc != 6) {
    printf("usage: %s <mapping_file> <train_file> <rep_ratio> <cnt_per_part> "
           "<output>\n",
           argv[0]);
    exit(-1);
  }
  double rep_ratio = atof(argv[3]);
  int cnt_per_part = atoi(argv[4]);

  FILE *output = fopen(argv[5], "wb");
  CHECK(output);

  IndexType c, n;
  IndexType *xpins;
  NodeType *pins;
  int exist_part_cnt;
  auto map_info = read_mapping(argv[1], exist_part_cnt);
  NodeType *part_info = map_info.mapping;
  read_binary_hypergraph(argv[2], c, n, xpins, pins);

  // IndexType new_n = remove_low_freq(c, n, xpins, pins);
  IndexType new_n = n;
  IndexType *node2edge_idx;
  IndexType *node2edge_data;
  std::cout << "before get_node2edge\n";
  get_node2edge(c, new_n, xpins, pins, node2edge_idx, node2edge_data);

  IndexType rep_part_cnt = (rep_ratio * c - 1) / cnt_per_part + 1;
  auto t1 = std::chrono::high_resolution_clock::now();
  std::vector<NodeType> node_score_idx(c);
  {
    std::vector<uint64_t> scores(c);

#pragma omp parallel for
    for (int i = 0; i < c; i++) {
      node_score_idx[i] = i;
    }

    std::atomic_uint64_t *atomic_score_ptr =
        (std::atomic_uint64_t *)scores.data();

#pragma omp parallel for
    for (IndexType i = 0; i < new_n; i++) {
      std::unordered_set<int> part_cnt;
      for (IndexType j = xpins[i]; j < xpins[i + 1]; j++) {
        part_cnt.insert(part_info[pins[j]]);
      }
      for (IndexType j = xpins[i]; j < xpins[i + 1]; j++) {
        atomic_score_ptr[pins[j]].fetch_add(part_cnt.size() - 1);
      }
    }

    tbb::parallel_sort(
        node_score_idx.begin(), node_score_idx.end(),
        [&](NodeType a, NodeType b) { return scores[a] > scores[b]; });
  }
  auto t2 = std::chrono::high_resolution_clock::now();
  std::cout
      << "Time 1: "
      << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
      << " ms" << std::endl;
  const int num_threads = omp_get_max_threads();
  std::cout << "before rep_info\n";
  std::chrono::high_resolution_clock::time_point t3 =
      std::chrono::high_resolution_clock::now();
  FastSet<NodeType> *rep_info = new FastSet<NodeType>[rep_part_cnt];
  {

  FastMap<NodeType, int> adjacent_node_cnt_pool[num_threads];
  // std::unordered_map<int, int> adjacent_node_cnt_pool[num_threads];
  std::cout << "c " << c << std::endl;
  // for (int i = 0; i < num_threads; i++) {
  //   adjacent_node_cnt_pool[i].reserve(c / 2);
  // }


#pragma omp parallel for schedule(dynamic)
  for (IndexType i = 0; i < rep_part_cnt; i++) {
    int thread_id = omp_get_thread_num();
    auto &adjacent_node_cnt = adjacent_node_cnt_pool[thread_id];

    IndexType node = node_score_idx[i];
    adjacent_node_cnt.clear();
    IndexType edge_start = node2edge_idx[node];
    IndexType edge_end = node2edge_idx[node + 1];
    IndexType step = std::max((edge_end - edge_start) / 10000, (IndexType)1);
    for (IndexType j = edge_start; j < edge_end; j += step) {
      IndexType edge_id = node2edge_data[j];
      IndexType node_start = xpins[edge_id];
      IndexType node_end = xpins[edge_id + 1];
      for (IndexType k = node_start; k < node_end; k++) {
        NodeType aj_node = pins[k];
        adjacent_node_cnt[aj_node]++;
      }
    }
    std::vector<std::pair<NodeType, int>> sorted_candidate_list(
        adjacent_node_cnt.begin(), adjacent_node_cnt.end());

    tbb::parallel_sort(sorted_candidate_list.begin(),
                       sorted_candidate_list.end(),
                       [](auto a, auto b) { return a.second > b.second; });
    rep_info[i].insert(node);
    for (size_t order = 0; order < sorted_candidate_list.size(); order++) {
      auto each = sorted_candidate_list[order].first;
      if (rep_info[i].size() == (size_t)cnt_per_part)
        break;
      if (part_info[node] != part_info[each]) {
        rep_info[i].insert(each);
      }
    }
  }
  }

  std::chrono::high_resolution_clock::time_point t4 =
      std::chrono::high_resolution_clock::now();
  std::cout
      << "Time 2: "
      << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count()
      << " ms" << std::endl;
  std::cout << "before rep\n";
  std::vector<std::vector<NodeType>> node_info(c);
  for (size_t i = 0; i < rep_part_cnt; i++) {
    auto &rep_nodes = rep_info[i];
    for (auto each : rep_nodes) {
      node_info[each].push_back(i + exist_part_cnt);
    }
    if (rep_nodes.size() > (size_t)cnt_per_part) {
      printf("replication error\n");
      exit(1);
    }
  }
  delete [] rep_info;
  std::vector<int> final_result;
  std::vector<int> part_idx;
  std::vector<int> part_size;
  uint64_t sum = 0;
  for (IndexType i = 0; i < c; i++) {
    final_result.push_back(part_info[i]);
    for (auto each : node_info[i]) {
      final_result.push_back(each);
    }
    part_idx.push_back(sum);
    part_size.push_back(node_info[i].size() + 1);
    sum += node_info[i].size() + 1;
  }
  assert(sum == final_result.size());
  fwrite(&c, sizeof(IndexType), 1, output);
  fwrite(&sum, sizeof(IndexType), 1, output);
  fwrite(part_idx.data(), sizeof(int), c, output);
  fwrite(part_size.data(), sizeof(int), c, output);
  fwrite(final_result.data(), sizeof(int), sum, output);

  fclose(output);
  munmap(map_info.raw_ptr, map_info.file_size);
  return 0;
}