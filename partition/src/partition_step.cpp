#include "partition.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <folly/container/F14Map.h>
#include <omp.h>

std::vector<IndexType> part_element;
std::vector<std::unordered_set<IndexType>> related_point;

template <typename K, typename V>
using FastMap =
    folly::F14FastMap<K, V, std::hash<K>, std::equal_to<K>,
                      folly::f14::DefaultAlloc<std::pair<K const, V>>>;

int main(int argc, char *argv[]) {
  if (argc != 5) {
    printf(
        "Usage: %s <cnt_per_part> <input_file> <output_file> <binary_graph>\n",
        argv[0]);
    return 1;
  }
  auto t1 = std::chrono::high_resolution_clock::now();
  int cnt_per_part = atoi(argv[1]);

  FILE *f = fopen(argv[3], "wb");

  IndexType c, n;
  NodeType *pins;
  IndexType *xpins;
  bool binary_graph = atoi(argv[4]);

  if (binary_graph) {
    read_binary_hypergraph(argv[2], c, n, xpins, pins);
  } else {
    read_hypergraph(argv[2], c, n, xpins, pins);
  }

  IndexType * part_info = new IndexType[c];
  memset(part_info, -1, sizeof(IndexType) * c);

  auto t2 = std::chrono::high_resolution_clock::now();

  std::cout
      << "Read Time: "
      << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
      << " ms" << std::endl;


  std::vector<edge> *related_edge = new std::vector<edge>;
  int longest_edge = 0;
  for (IndexType i = 0; i < n; i++) {
    if (xpins[i + 1] - xpins[i] < 2) {
      continue;
    }
    related_edge->push_back({xpins[i], xpins[i + 1]});
    if(xpins[i + 1] - xpins[i] > longest_edge){
      longest_edge = xpins[i + 1] - xpins[i];
    }
  }
  free(xpins);
  part_element.resize(c);
  for (IndexType i = 0; i < c; i++) {
    part_element[i] = i;
  }
  std::chrono::high_resolution_clock::time_point t3 =
      std::chrono::high_resolution_clock::now();

  IndexType part_cnt = (c + cnt_per_part - 1) / cnt_per_part;

  IndexType result = partition(c, part_cnt, pins, related_edge, part_info, longest_edge, cnt_per_part);
  free(pins);
  fwrite(&c, sizeof(IndexType), 1, f);
  fwrite(&c, sizeof(IndexType), 1, f);
  for(int i = 0; i < c; i++){
    fwrite(&i, sizeof(int), 1, f);
  }
  int len = 1;
  for(int i = 0; i < c; i++){
    fwrite(&len, sizeof(int), 1, f);
  }
  for(int i = 0; i < c; i++){
    fwrite(&part_info[i], sizeof(int), 1, f);
  }
  fclose(f);

  part_cnt = result;
  printf("part_cnt: %ld\n", part_cnt);

  std::chrono::high_resolution_clock::time_point t4 =
      std::chrono::high_resolution_clock::now();

  std::cout
      << "Partition Time: "
      << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count()
      << " ms" << std::endl;

  std::cout
      << "Time: "
      << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t1).count()
      << " ms" << std::endl;

  delete [] part_info;

  return 0;
}
