#pragma once

#include "timer.h"
#include "cachelib/allocator/CacheAllocator.h"
#include "mytype.h"
#include "array.h"
#include <functional>
#include <string>
#include <vector>
#include <atomic_queue/atomic_queue.h>
#include "spdk_wrapper.h"
#include <queue>

#define USE_CACHE

struct Stats {
  double prepare_tot;
  double calculate_tot;
  double tot;
  uint64_t prepare_cnt;
  uint64_t calculate_cnt;
  uint64_t cnt;
};

struct QueueEntry {
  data_type key;
  char value[0];
};

struct GetResult {
  int load_cnt;
  int load_emb_cnt;
  double timer_prepare1;
  double timer_prepare2;
  double timer_tot;
};

class Mapping {
public:
  static const int buffer_size = 4096;

  Mapping(std::string mapping_file, uint64_t cache_size, int value_size, int thread_num, index_type num_items, int ssd_num);
  GetResult BatchGet(base::ConstArray<data_type> keys_array, char *results, int tid);
  static Stats Report(bool overall = false);
  std::pair<uint64_t, uint64_t> Statistic();
  ~Mapping();

  facebook::cachelib::PoolId default_pool;
  using Cache = facebook::cachelib::LruAllocator;
  using WriteHandle = Cache::WriteHandle;

  std::unique_ptr<Cache> cache;

  atomic_queue::AtomicQueueB<QueueEntry *> write_queue;
  atomic_queue::AtomicQueueB<QueueEntry *> write_queue_free;
  int _value_size;
  int ssd_num;
private:
  static const int MAX_QUEUE_NUM = 96;
  static constexpr int kBouncedBuffer_ = 4096;
  
  char *bouncedBuffer_[MAX_QUEUE_NUM];
  std::unique_ptr<SpdkWrapper> ssd_;
  int thread_num;

  size_t item_cnt;
  bool use_cache;
  std::atomic<bool> stop;
  data_type* block_idx;
  data_type* all_blocks;
  int* block_cnt;

  std::vector<std::vector<data_type>> block_to_elem;

  std::vector<int> permutation;
  std::thread cache_write_thread;

  std::queue<int> buffer_idx;
  folly::F14FastSet<data_type> req_sets[MAX_QUEUE_NUM];

  void cache_write();
  void init();

  std::atomic<uint64_t> load_page_cnt = 0;
  std::atomic<uint64_t> eff_emb_cnt = 0;
  friend void ReadCompleteCB(void *ctx, const struct spdk_nvme_cpl *cpl);
};

