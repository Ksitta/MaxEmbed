#pragma once

#if 1
#include <folly/Format.h>
#include <folly/GLog.h>
#include <folly/Likely.h>

static void init_folly() {
  google::InitGoogleLogging("test");
}

#else 

#include <iostream>
#include <fstream>
#include <vector>
#include <atomic>

static void init_folly() {}

struct NormalLog {
  NormalLog() {}
  ~NormalLog() { std::cout << std::endl; }
};
template <class T> NormalLog &operator<<(NormalLog &out, T data) {
  std::cout << data;
  return out;
}

struct AssertLog {
  AssertLog(bool _exp, std::string statement): _exp(_exp) {
    if (!_exp) {
      std::cout << "Assertion failed: " << statement << " ";
    }
  }
  template <class T>
  friend AssertLog &operator<<(AssertLog &out, T data) {
    if (!out._exp) {
      std::cout << data;
    }
    return out;
  }
  ~AssertLog() {
    if (!_exp) {
      std::cout << std::endl;
      exit(-1);
    }
  }

private:
  bool _exp;
};

#define LOG(...) std::cout
#define CHECK(exp) assert(exp); std::fstream("/dev/null")
#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_NE(a, b) CHECK((a) != (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))
#define CHECK_LT(a, b) CHECK((a) < (b))

#define CHECK_(...) std::fstream("/dev/null")
#define FB_LOG_EVERY_MS(...) std::fstream("/dev/null")

#define FOLLY_ALWAYS_INLINE
#define FOLLY_UNLIKELY(a) (a)
#endif
