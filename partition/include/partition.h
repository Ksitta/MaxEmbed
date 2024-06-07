#ifndef __PARTITION_H__
#define __PARTITION_H__
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>
#include <folly/GLog.h>
#include <sys/stat.h>
#include <sys/mman.h>

using IndexType = uint64_t;
using NodeType = uint32_t;

struct mapping_info{
  NodeType *mapping;
  int *idx;
  int *cnt;
  int part_cnt;
  int vertex_cnt;
  int file_size;
  int total_cnt;
  void *raw_ptr;
};

mapping_info read_mapping(char *filepath, int &part_cnt);

constexpr float p = 0.5;
constexpr int MAX_ITER = 8;
using edge = std::pair<IndexType, IndexType>;
IndexType partition(IndexType c, IndexType parts, NodeType *pins,
                    std::vector<edge> *related_edge, IndexType *part,
                    int longest_edge, int cnt_per_part);
void read_binary_hypergraph(char *filename, IndexType &c, IndexType &n,
                            IndexType *&xpins, NodeType *&pins);
void read_hypergraph(char *filename, IndexType &c, IndexType &n,
                     IndexType *&xpins, NodeType *&pins);

#endif