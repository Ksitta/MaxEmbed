#include "mapper.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include "timer.h"
#include <folly/container/F14Set.h>

template <typename K>
using FastSet = folly::F14FastSet<K, std::hash<K>, std::equal_to<K>,
                                  folly::f14::DefaultAlloc<K>>;

static std::mutex lock;
static Counter<double> tot_prepare;
static Counter<double> tot_calc;
static Counter<double> tot_tot;

static int GLOBAL_VAL_SIZE;

std::atomic<uint64_t> total_cnt = 0;
std::atomic<uint64_t> hit_cnt = 0;
std::atomic<uint64_t> in_queue = 0;
std::atomic<uint64_t> cache_write_cnt = 0;
std::atomic<uint64_t> cache_cnt = 0;

std::vector<int> eff_emb_cnt_threads[64];

Mapping::Mapping(std::string mapping_file, uint64_t cache_size, int value_size, int thread_num, index_type num_items, int ssd_num)
    : write_queue(1 << 20), write_queue_free(1 << 20), _value_size(value_size), ssd_num(ssd_num) , thread_num(thread_num) {
  CHECK(thread_num <= MAX_QUEUE_NUM);
  ssd_ = SpdkWrapper::create(thread_num, ssd_num);
  ssd_->Init();
  GLOBAL_VAL_SIZE = _value_size;
  uint64_t key_cnt = cache_size / value_size;
  if(cache_size == 0){
    use_cache = false;
  } else {
    use_cache = true;
    Cache::Config config;
    config
      .setCacheSize(cache_size)
      .setCacheName("cache") // unique identifier for the cache
      .setAccessConfig(key_cnt)
      .validate();
    cache.reset(new Cache(config));
    default_pool =
        cache->addPool("default", cache->getCacheMemoryStats().ramCacheSize);
    printf("cache size %ld entry_size %ld\n", cache_size, key_cnt);
    cache_write_thread = std::thread([this]() { cache_write(); });
    for(size_t i = 0; i < 1 << 20; i++){
      QueueEntry *entry = (QueueEntry *)malloc(sizeof(data_type) + GLOBAL_VAL_SIZE);
      write_queue_free.push(entry);
    }
  }

  data_type max_block = 0;
  FILE *fin = fopen(mapping_file.c_str(), "rb");
  struct stat st;
  fstat(fileno(fin), &st);
  char *raw_data_ptr = (char *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fileno(fin), 0);
  assert(raw_data_ptr != MAP_FAILED);
  item_cnt = *(uint64_t *)raw_data_ptr;
  uint64_t all_blocks_cnt = *(uint64_t *)(raw_data_ptr + sizeof(uint64_t));
  block_idx = (data_type *)(raw_data_ptr + 2 * sizeof(uint64_t));
  block_cnt = (int *)(raw_data_ptr + 2 * sizeof(uint64_t) + sizeof(data_type) * item_cnt);
  all_blocks = (data_type *)(raw_data_ptr + 2 * sizeof(uint64_t) + sizeof(data_type) * item_cnt + sizeof(int) * item_cnt);

  CHECK(item_cnt == num_items) << "item_cnt " << item_cnt << " num_items " << num_items;
  for(int i = 0; i < all_blocks_cnt; i++){
    max_block = std::max(max_block, all_blocks[i]);
  }
  max_block++;
  block_to_elem.resize(max_block);
  for(size_t i = 0; i < item_cnt; i++){
    int start = block_idx[i];
    int cnt = block_cnt[i];
    for(int j = start; j < start + cnt; j++){
      int block = all_blocks[j];
      block_to_elem[block].push_back(i);
    }
  }
  stop = false;
  init();
}

extern void bind_core(int core);

void Mapping::cache_write() {
  bind_core(thread_num);
  while (!stop) {
    QueueEntry *entry;
    if(write_queue.try_pop(entry)){
      auto handle = cache->allocate(default_pool, std::to_string(entry->key), _value_size);
      while(!handle){
        handle = cache->allocate(default_pool, std::to_string(entry->key), _value_size);
      }
      std::memcpy(handle->getMemory(), entry->value, _value_size);
      cache->insert(handle);
      cache_write_cnt.fetch_add(1, std::memory_order_release);
      write_queue_free.push(entry);
    }
  }
}

std::pair<uint64_t, uint64_t> Mapping::Statistic(){
  return std::make_pair(load_page_cnt.load(), eff_emb_cnt.load());
}

void Mapping::init(){
  for (int i = 0; i < thread_num; i++) {
    bouncedBuffer_[i] = (char *) spdk_malloc(kBouncedBuffer_ * ssd_->GetLBASize(), 0x1000, NULL,
                                SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
    CHECK_NOTNULL(bouncedBuffer_[i]);
  }
}


struct SyncReadCtx {
  std::atomic<int> *read_cnt;
  int emb_cnt;
  char *src;
  char *dst;
  Mapping *map;
  data_type idx[64];
  int offset[64];
};

void ReadCompleteCB(void *ctx, const struct spdk_nvme_cpl *cpl) {
  if (UNLIKELY(spdk_nvme_cpl_is_error(cpl))) {
    LOG(FATAL) << "I/O error status: "
                << spdk_nvme_cpl_get_status_string(&cpl->status);
  }
  SyncReadCtx *read_ctx = (SyncReadCtx *)ctx;
  class Mapping * mapping = read_ctx->map;
  if(mapping->use_cache){
    for(int i = 0; i < read_ctx->emb_cnt; i++){
      data_type key = read_ctx->idx[i];
      int offset = read_ctx->offset[i];
      QueueEntry *entry;
      cache_cnt.fetch_add(1, std::memory_order_relaxed);
      if(!mapping->write_queue_free.try_pop(entry)){
        break;
      }
      entry->key = key;
      memcpy(entry->value, read_ctx->src + GLOBAL_VAL_SIZE * offset, GLOBAL_VAL_SIZE);
      mapping->write_queue.push(entry);
    }
  }
  for(int i = 0; i < read_ctx->emb_cnt; i++){
    int offset = read_ctx->offset[i];
    memcpy(read_ctx->dst + i * GLOBAL_VAL_SIZE, read_ctx->src + GLOBAL_VAL_SIZE * offset, GLOBAL_VAL_SIZE);
  }
  read_ctx->read_cnt->fetch_add(1);
}

std::atomic<uint64_t> throughput = 0;
#define PIPELINE
std::vector<int> valid_emb_cnt[64];

GetResult Mapping::BatchGet(base::ConstArray<data_type> keys_array, char *results, int tid) {
  int ssd_id = tid % ssd_num;
  int queue_id = tid / ssd_num;
  static thread_local std::vector<SyncReadCtx> read_keys_vec;
  static thread_local std::atomic<int> read_cnt;
  static thread_local bool init = false;
  static thread_local std::vector<int> blocks_read;
  if(UNLIKELY(!init)){
    for(int i = 0; i < kBouncedBuffer_; i++){
      read_keys_vec.push_back(SyncReadCtx());
    }
    for(int i = 0; i < kBouncedBuffer_; i++){
      read_keys_vec[i].read_cnt = &read_cnt;
      read_keys_vec[i].map = this;
    }
    init = true;
  }

  folly::F14FastSet<data_type> &req_set = req_sets[tid];

  CHECK_LE(keys_array.Size(), kBouncedBuffer_);
  CHECK_LE(_value_size, ssd_->GetLBASize()) << "KISS";
  CHECK(_value_size % sizeof(float) == 0);

  int copy_cnt = 0;
  GetResult stats;    
  Timer timer;
  Timer timer_prepare, timer_calc;
  int ssd_request_cnt = 0;
  read_keys_vec.reserve(10000);
  int load_emb_cnt = 0;
  auto submit = [&](int block, SyncReadCtx *ctx){
    valid_emb_cnt[tid].push_back(ctx->emb_cnt);
    load_emb_cnt += ctx->emb_cnt;
    eff_emb_cnt_threads[tid].push_back(ctx->emb_cnt);
    ctx->src = bouncedBuffer_[tid] + ssd_request_cnt * ssd_->GetLBASize();
    ssd_->SubmitReadCommand(ctx->src,
                            ssd_->GetLBASize(), block,
                            ReadCompleteCB, 
                            ctx, queue_id, ssd_id);
    ssd_request_cnt++;
  };

  req_set.clear();
  read_cnt = 0;


  timer_prepare.Start();
  std::vector<data_type> order(keys_array.begin(), keys_array.end());
  std::sort(order.begin(), order.end(), [&](auto x, auto y) {
    return block_cnt[x] < block_cnt[y] ||
           (block_cnt[x] == block_cnt[y] && x < y);
  });
  order.erase(std::unique(order.begin(), order.end()), order.end());
  req_set.insert(order.begin(), order.end());

  int hit_cnt = 0;
  int all_cnt = order.size();

  if(use_cache){
    for (auto elem : order) {
        auto handle = cache->findFast(std::to_string(elem));
        if(handle){
          memcpy(results + copy_cnt * _value_size, handle->getMemory(), _value_size);
          copy_cnt++;
          hit_cnt++;
          req_set.erase(elem);
        }
    }
  }
  timer_prepare.End();

  timer_calc.Start();

#ifndef PIPELINE
  blocks_read.clear();
#endif
  int ctx_cnt = 0;
  for (auto elem : order) {
    if (!req_set.count(elem)){
      continue;
    }
    int i = 0;
    const int max_visit = 5;
    int start = block_idx[elem];
    int max = 0;
    int maxp = all_blocks[start];
    int end = start + block_cnt[elem];
    if (block_cnt[elem] != 1) {
      for (int pos = start; pos < end; pos++) {
        data_type block = all_blocks[pos];
        int count = 0;
        for (auto item : block_to_elem[block]) {
          count += req_set.count(item);
        }
        if (count > max) {
          max = count;
          maxp = block;
        }
        if (++i == max_visit){
          break;
        }
      }
    }
    int real_block = maxp;
    SyncReadCtx *ctx = &read_keys_vec[ctx_cnt];
    ctx->emb_cnt = 0;
    ctx->dst = results + copy_cnt * _value_size;
    for (size_t offset = 0; offset < block_to_elem[maxp].size(); offset++){
      auto item = block_to_elem[maxp][offset];
      if (req_set.erase(item)) {
        ctx->offset[ctx->emb_cnt] = offset;
        ctx->idx[ctx->emb_cnt] = item;
        ctx->emb_cnt++;
      }
    }
    ctx_cnt++;
    copy_cnt += ctx->emb_cnt;
#ifdef PIPELINE
    submit(real_block, ctx);
#else
    blocks_read.push_back(real_block);
#endif
  }
  timer_calc.End();

  timer.Start();

  for(int i = 0;i < blocks_read.size(); i++){
    submit(blocks_read[i], &read_keys_vec[i]);
  }

  while (read_cnt != ssd_request_cnt) {
    ssd_->PollCompleteQueue(queue_id, ssd_id);
  }

  stats.load_emb_cnt = load_emb_cnt;
  stats.load_cnt = ssd_request_cnt;

  //timer_kvell_pollCQ.end();
  timer.End();
  stats.timer_tot = timer.PeriodReportTotal();
  stats.timer_prepare1 = timer_prepare.PeriodReportTotal();
  stats.timer_prepare2 = timer_calc.PeriodReportTotal();
  ::hit_cnt.fetch_add(hit_cnt);
  ::total_cnt.fetch_add(all_cnt);
  return stats;
}

Stats Mapping::Report(bool overall) {
  std::lock_guard<std::mutex> _(lock);

  Stats result;

  result.prepare_tot = tot_prepare.PeriodReportSum();
  result.calculate_tot = tot_calc.PeriodReportSum();
  result.tot = tot_tot.PeriodReportSum();

  result.prepare_cnt = tot_prepare.PeriodReportCnt();
  result.calculate_cnt = tot_prepare.PeriodReportCnt();
  result.cnt = tot_prepare.PeriodReportCnt();
  
  tot_prepare.PeriodClear();
  tot_calc.PeriodClear();
  tot_tot.PeriodClear();  

  return result;
}

Mapping::~Mapping() {
  stop = true;
  if(cache_write_thread.joinable()){
    cache_write_thread.join();
  }
  while(!write_queue_free.was_empty()){
    QueueEntry *entry;
    entry = write_queue_free.pop();
    free(entry);
  }
}