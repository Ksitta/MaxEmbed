#pragma once

#include <chrono>
#include <functional>
#include <atomic>

template <class T, class R = double> class Counter {
  using C = int64_t;
  T tot_value = 0, period_value = 0;
  C tot_cnt = 0, period_cnt = 0;
public:
  void Add(T value) {
    tot_value += value;
    period_value += value;
    tot_cnt += 1;
    period_cnt += 1;
  }
  R ReportAvg() {
    return static_cast<R>(tot_value) / tot_cnt;
  }
  T ReportSum() { return tot_value; }
  C ReportCnt() { return tot_cnt; }

  R PeriodReportAvg() {
    return static_cast<R>(period_value) / period_cnt;
  }
  T PeriodReportSum() { return period_value; }
  T PeriodReportCnt() { return period_cnt; }
  void PeriodClear() {
    period_value = 0;
    period_cnt = 0;
  }
};

class Timer {
  Counter<double> counter;
  decltype(std::chrono::high_resolution_clock::now()) start;
public:
  void Start() {
    start = std::chrono::high_resolution_clock::now();
  }
  void End() {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur = end - start;
    counter.Add(dur.count());
  }
  double ReportAvg() {
    return counter.ReportAvg();
  }
  double ReportTotal() { return counter.ReportSum(); }
  int64_t ReportCnt() { return counter.ReportCnt(); }
  double PeriodReportAvg() {
    return counter.PeriodReportAvg();
  }
  double PeriodReportTotal() { return counter.PeriodReportSum(); }
  int64_t PeriodReportCnt() { return counter.PeriodReportCnt(); }
  void PeriodClear() { counter.PeriodClear(); }
};

class Event {
  decltype(std::chrono::high_resolution_clock::now()) start;
  double interval;
  std::function<void()> func;
public:
  Event(double interval, std::function<void()> func): interval(interval), func(func) {
    start = std::chrono::high_resolution_clock::now();
  }
  void CheckOut() {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur = end - start;
    if (dur.count() > interval) {
      func();
      start = end;
    }
  }
};
