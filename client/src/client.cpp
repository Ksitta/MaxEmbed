#include <atomic>
#include <chrono>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "mapper.h"
#include <pthread.h>
#include <sched.h>

#include "argparse/argparse.hpp"

#include "folly_wrapper.h"
#include "loader.h"

extern std::vector<int> eff_emb_cnt_threads[64];

void parse_args(int argc, char **argv, argparse::ArgumentParser &program) {

  program.add_argument("query_file");
  program.add_argument("mapping_file");
  program.add_argument("--batch_size", "-b").scan<'i', int>().default_value(32);
  program.add_argument("--num_threads", "-n").scan<'i', int>().default_value(1);
  program.add_argument("--embedding_dim", "-d")
      .scan<'i', int>()
      .default_value(32);
  program.add_argument("--output_csv", "-o");
  program.add_argument("--delay").scan<'i', int>().default_value(150);
  program.add_argument("--time", "-t").scan<'i', int>().default_value(60);
  program.add_argument("--cache_ratio", "-c")
      .scan<'f', float>()
      .default_value(0.1f);
  program.add_argument("--ssd_num", "-s").scan<'d', int>().default_value(1);
  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    exit(-1);
  }
}

void bind_core(int core_num) {
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(core_num, &set);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &set) != 0) {
    perror("pthread_setaffinity_np");
    exit(-1);
  }
}

extern std::atomic<uint64_t> total_cnt;
extern std::atomic<uint64_t> hit_cnt;
extern std::atomic<uint64_t> cache_write_cnt, cache_cnt;
extern std::vector<int> valid_emb_cnt[64];

std::vector<int>& get_valid_emb_cnt();
int main(int argc, char **argv) {
  init_folly();
  argparse::ArgumentParser args("test");
  parse_args(argc, argv, args);

  const int embedding_dim = args.get<int>("--embedding_dim");
  const int batch_size = args.get<int>("--batch_size");
  const int num_threads = args.get<int>("--num_threads");

  const std::string mapping_file = args.get("mapping_file");
  const std::string query_file = args.get("query_file");

  const int delay = args.get<int>("delay");
  const int run_time = args.get<int>("time");

  const float cache_ratio = args.get<float>("cache_ratio");
  const int ssd_num = args.get<int>("ssd_num");

  FILE *output = nullptr;
  if (auto fn = args.present("--output_csv")) {
    output = fopen(fn->c_str(), "w");
  }

  auto loader = std::make_unique<RequestReader>(query_file);
  auto items = loader->get_num_items();
  printf("total items %ld\n", items);
  uint64_t cache_size = items * cache_ratio * embedding_dim * sizeof(float);
  auto server = std::make_unique<Mapping>(mapping_file, cache_size,
                                          embedding_dim * sizeof(float),
                                          num_threads, items, ssd_num);
  // printf("!!\n");

  std::atomic<int64_t> global_load_cnt(0);
  std::atomic<int64_t> global_request_cnt(0);
  std::atomic<int64_t> global_batch_cnt(0);
  std::atomic<int> finished(0);
  std::atomic<int> stop(0);

  typedef std::vector<std::pair<double, int64_t>> CountStats;
  CountStats overall_s, prepare_s, calc_s, ssd_s,
      thr_overall_s, thr_eff_s, batch_s;

  CountStats latency[num_threads];
  CountStats latency_ssd[num_threads];
  CountStats latency_cache[num_threads];
  CountStats latency_index[num_threads];
  CountStats latency_model[num_threads];

  Counter<double> latency_counter[num_threads];
  Counter<double> latency_ssd_counter[num_threads];
  Counter<double> latency_cache_counter[num_threads];
  Counter<double> latency_index_counter[num_threads];
  Counter<double> latency_model_counter[num_threads];

  std::mutex latency_lock;
  Counter<double> latency_second;
  Counter<double> latency_ssd_second;
  Counter<double> latency_cache_second;
  Counter<double> latency_index_second;
  Counter<double> latency_model_second;

  auto worker = [&](int thread_id) {
    bind_core(thread_id);

    auto start_iter = loader->begin() + thread_id * batch_size;
    auto end_iter = loader->end();
    std::chrono::time_point start = std::chrono::high_resolution_clock::now();
    for (auto iter = start_iter;; iter += num_threads * batch_size) {
      if (iter >= end_iter) {
        if (run_time == 0) {
          break;
        }
        iter = start_iter;
      }
      auto request = loader->get(iter, batch_size);
      std::vector<float> result(request.size() * embedding_dim);
      Timer timer, model_timer;
      timer.Start();
      auto ret = server->BatchGet(request, (char *)result.data(), thread_id);
      model_timer.Start();
      if (delay) {
        auto target = std::chrono::high_resolution_clock::now() +
                      std::chrono::microseconds(delay);
        while (std::chrono::high_resolution_clock::now() < target)
          ;
      }
      model_timer.End();
      timer.End();
      global_load_cnt.fetch_add(ret.load_cnt);
      global_request_cnt.fetch_add(ret.load_emb_cnt);
      global_batch_cnt.fetch_add(1);
      std::chrono::time_point end = std::chrono::high_resolution_clock::now();
      if(std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > 1){
        latency[thread_id].push_back({latency_counter[thread_id].PeriodReportSum(), latency_counter[thread_id].PeriodReportCnt()});
        latency_ssd[thread_id].push_back({latency_ssd_counter[thread_id].PeriodReportSum(), latency_ssd_counter[thread_id].PeriodReportCnt()});
        latency_index[thread_id].push_back({latency_index_counter[thread_id].PeriodReportSum(), latency_index_counter[thread_id].PeriodReportCnt()});
        latency_cache[thread_id].push_back({latency_cache_counter[thread_id].PeriodReportSum(), latency_cache_counter[thread_id].PeriodReportCnt()});
        latency_model[thread_id].push_back({latency_model_counter[thread_id].PeriodReportSum(), latency_cache_counter[thread_id].PeriodReportCnt()});
        {
          std::lock_guard<std::mutex> _(latency_lock);
          latency_second.Add(latency_counter[thread_id].PeriodReportAvg());
          latency_ssd_second.Add(latency_ssd_counter[thread_id].PeriodReportAvg());
          latency_model_second.Add(latency_model_counter[thread_id].PeriodReportAvg());
          latency_index_second.Add(latency_index_counter[thread_id].PeriodReportAvg());
          latency_cache_second.Add(latency_cache_counter[thread_id].PeriodReportAvg());
        }
        latency_cache_counter[thread_id].PeriodClear();
        latency_index_counter[thread_id].PeriodClear();
        latency_ssd_counter[thread_id].PeriodClear();
        latency_counter[thread_id].PeriodClear();
        latency_model_counter[thread_id].PeriodClear();
      }
      latency_cache_counter[thread_id].Add(ret.timer_prepare1);
      latency_index_counter[thread_id].Add(ret.timer_prepare2);
      latency_ssd_counter[thread_id].Add(ret.timer_tot);
      latency_counter[thread_id].Add(timer.PeriodReportTotal());
      latency_model_counter[thread_id].Add(model_timer.PeriodReportTotal());
      if (stop) {
        break;
      }
    }
    stop.store(1);
    finished.fetch_add(1);
  };
  auto monitor = [&]() {
    // bind_core(num_threads + 1);
    int64_t last_load_cnt = 0;
    int64_t last_request_cnt = 0;
    int64_t last_batch_cnt = 0;
    Timer timer;
    timer.Start();
    int cnt = 0;
    while (1) {
      sleep(1);
      timer.End();
      int64_t current_load_cnt =
          global_load_cnt.load();
      int64_t current_request_cnt =
          global_request_cnt.load();
      int64_t current_batch_cnt =
          global_batch_cnt.load();

      int64_t delta_load = current_load_cnt - last_load_cnt;
      int64_t delta_request = current_request_cnt - last_request_cnt;
      int64_t delta_batch = current_batch_cnt - last_batch_cnt;

      double overall_bw = 1.0 * delta_load * 4 / timer.PeriodReportTotal();
      double effective_bw = 1.0 * delta_request * sizeof(float) *
                            embedding_dim / 1024 / timer.PeriodReportTotal();

      printf(
          "Load %ld blocks, Requests: %ld, Batch %ld, Overall b/w: %lf KiB/s, "
          "Effective b/w: %lf KiB/s (%lf %%), %ld, %lf\n",
          delta_load, delta_request, delta_batch, overall_bw, effective_bw,
          effective_bw / overall_bw * 100, delta_load,
          timer.PeriodReportTotal());

      auto stats = Mapping::Report();
      double lat_all, lat_cache, lat_ssd, lat_index, lat_model;
      {
        std::lock_guard<std::mutex> _(latency_lock);
        lat_all = latency_second.ReportAvg();
        lat_cache = latency_cache_second.ReportAvg();
        lat_index = latency_index_second.ReportAvg();
        lat_ssd = latency_ssd_second.ReportAvg();
        lat_model = latency_model_second.ReportAvg();

        // lat_all = latency_second.PeriodReportAvg();
        // lat_cache = latency_cache_second.PeriodReportAvg();
        // lat_index = latency_index_second.PeriodReportAvg();
        // lat_ssd = latency_ssd_second.PeriodReportAvg();
        // lat_model = latency_model_second.PeriodReportAvg();
        // latency_second.PeriodClear();
        // latency_cache_second.PeriodClear();
        // latency_ssd_second.PeriodClear();
        // latency_model_second.PeriodClear();
        // latency_second.PeriodClear();
      }
      printf("All %lf us, Cache %lf us, Index %lf us, SSD %lf us\n", lat_all * 1e6, lat_cache * 1e6, lat_index * 1e6, lat_ssd * 1e6);

      prepare_s.emplace_back(stats.prepare_tot, stats.prepare_cnt);
      calc_s.emplace_back(stats.calculate_tot, stats.calculate_cnt);
      ssd_s.emplace_back(stats.tot, stats.cnt);
      thr_overall_s.emplace_back(overall_bw, 1);
      thr_eff_s.emplace_back(effective_bw, 1);
      batch_s.emplace_back(delta_batch * batch_size, 1);
      
      timer.PeriodClear();
      timer.Start();
      last_load_cnt = current_load_cnt;
      last_request_cnt = current_request_cnt;
      last_batch_cnt = current_batch_cnt;

      if (finished.load() == num_threads)
        break;
      if (++cnt == run_time) {
        stop = 1;
        break;
      }
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back(worker, i);
  }
  monitor();

  for (auto &t : threads) {
    t.join();
  }

  auto process_stats = [](const CountStats &s) {
    double tot = 0;
    int64_t cnt = 0;
    for (int i = 5; i < (int)s.size() - 2; i++) {
      tot += s[i].first;
      cnt += s[i].second;
    }
    return tot / cnt;
  };

  auto process_lat_stats = [](const std::vector<double> &s) {
    double tot = 0;
    int64_t cnt = 0;
    for (int i = 2; i < (int)s.size() - 2; i++) {
      tot += s[i];
      cnt++;
    }
    return tot / cnt;
  };

  auto process_stats_n = [](const CountStats s[], int n) {
    double tot = 0;
    int64_t cnt = 0;
    size_t end = s[0].size();
    for (int i = 1; i < n; i++) {
      end = std::min(end, s[i].size());
    }
    end -= 2;
    for (int j = 0; j < n; j++) {
      for (int i = 5; i < end; i++) {
        tot += s[j][i].first;
        cnt += s[j][i].second;
      }
    }
    return tot / cnt;
  };

  printf(
      "Result: \n"
      "Overall %lf KiB/s, Effective %lf KiB/s (%lf%%), Request %lf Ops\n"
      "Overall %lf us, Prepare %lf us, Calc %lf us, SSD %lf us, Model %lf us\n",
      process_stats(thr_overall_s), process_stats(thr_eff_s),
      process_stats(thr_eff_s) / process_stats(thr_overall_s) * 100,
      process_stats(batch_s), process_stats_n(latency, num_threads) * 1e6,
      process_stats_n(latency_cache, num_threads) * 1e6,
      process_stats_n(latency_index, num_threads) * 1e6,
      process_stats_n(latency_ssd, num_threads) * 1e6,
      process_stats_n(latency_model, num_threads) * 1e6);
  printf("%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n", process_stats(thr_overall_s),
         process_stats(thr_eff_s), process_stats(batch_s),
         process_stats_n(latency, num_threads) * 1e6,
         process_stats_n(latency_cache, num_threads) * 1e6,
         process_stats_n(latency_index, num_threads) * 1e6,
         process_stats_n(latency_ssd, num_threads) * 1e6,
         process_stats_n(latency_model, num_threads) * 1e6);
  if(output){
    for(int i = 0; i < 64; i++){
      for(auto each : eff_emb_cnt_threads[i]){
        fprintf(output, "%d\n", each);
      }
    }
    fclose(output);
  }
  return 0;
}
