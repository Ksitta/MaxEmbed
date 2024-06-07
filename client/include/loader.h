#pragma once

#include "mytype.h"
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cassert>
class RequestReader {
public:
  class iterator {
  public:
    iterator(index_type *ptr) : ptr_(ptr) {}

    iterator &operator++() {
      ++ptr_;
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      ++ptr_;
      return tmp;
    }

    bool operator<(const iterator &other) const { return ptr_ < other.ptr_; }

    iterator &operator+=(int n) {
      ptr_ += n;
      return *this;
    }

    iterator operator+(int n) const { return iterator(ptr_ + n); }

    int operator-(const iterator &other) const { return ptr_ - other.ptr_; }

    bool operator!=(const iterator &other) const { return ptr_ != other.ptr_; }

    bool operator==(const iterator &other) const { return ptr_ == other.ptr_; }

    bool operator<=(const iterator &other) const { return ptr_ <= other.ptr_; }

    bool operator>=(const iterator &other) const { return ptr_ >= other.ptr_; }

    bool operator>(const iterator &other) const { return ptr_ > other.ptr_; }

    const index_type &operator*() const { return *ptr_; }

  private:
    index_type *ptr_;
  };

  RequestReader(std::string filename) {
    fin = fopen(filename.c_str(), "rb");
    if (fin == NULL) {
      std::cout << "File not found" << std::endl;
      exit(1);
    }
    struct stat st;
    fstat(fileno(fin), &st);
    raw_data_ptr = (char *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fileno(fin), 0);
    assert(raw_data_ptr != MAP_FAILED);
    file_size = st.st_size;
    index = (uint64_t *)(raw_data_ptr + 2 * sizeof(uint64_t));
    index_len = index[-2];
    item_cnt = index[-1];
    data_len = index[index_len];
    uint64_t dataoffset = (sizeof(size_t) * (index_len + 3));
    data = (data_type *)(raw_data_ptr + dataoffset);
  }

  ~RequestReader(){
    munmap(raw_data_ptr, file_size);
    fclose(fin);
  }

  std::vector<data_type> get(iterator iter, int batch_size) {
    auto back = iter + batch_size;
    if(back > end()) {
      back = end();
    }
    std::vector<data_type>result(data + *iter, data + *back);
    return result;
  }

  index_type size() { return index_len; }

  iterator begin() { return iterator(index); }

  iterator end() { return iterator(index + index_len); }

  index_type get_num_items() { return item_cnt; }

  index_type item_cnt;
  index_type data_len;
  index_type index_len;
  data_type *data;
  char *raw_data_ptr;
  index_type *index;
  FILE *fin;
  uint64_t file_size;
private:
};
