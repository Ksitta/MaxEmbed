#pragma once
#include "spdk/stdinc.h"

#include "spdk/nvme.h"
#include "spdk/vmd.h"
#include "spdk/nvme_zns.h"
#include "spdk/env.h"
#include "spdk/string.h"
#include "spdk/log.h"

#include <string>
#include <unordered_map>
#include <memory>


class SpdkWrapper {
 public:
  static std::unique_ptr<SpdkWrapper> create(int thread_num, int ssd_num);
  virtual void Init() = 0;

  virtual void SubmitReadCommand(void *pinned_dst, const int64_t bytes, const int64_t lba,
                                 spdk_nvme_cmd_cb func, void *ctx, int qp_id, int ssd_id) = 0;

  virtual int SubmitWriteCommand(const void *pinned_src, const int64_t bytes,
                                 const int64_t lba, spdk_nvme_cmd_cb func, void *ctx, int qp_id, int ssd_id) = 0;

  virtual void SyncRead(void *pinned_dst, const int64_t bytes, const int64_t lba, int qp_id, int ssd_id) = 0;
  
  virtual void SyncWrite(const void *pinned_src, const int64_t bytes,
                         const int64_t lba, int qp_id, int ssd_id) = 0;

  virtual void PollCompleteQueue(int qp_id, int ssd_id) = 0;
  virtual int GetLBASize() const = 0;
  virtual uint64_t GetLBANumber() const = 0;
  virtual ~SpdkWrapper() {}
};
