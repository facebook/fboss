// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspTransceiverAccessImpl.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

class BspTransceiverAccessError : public std::exception {
 public:
  explicit BspTransceiverAccessError(const std::string& what) : what_(what) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

 private:
  std::string what_;
};

class BspTransceiverAccess {
 public:
  BspTransceiverAccess(uint32_t tcvr, BspTransceiverMapping& tcvrMapping);

  bool isPresent();
  void init(bool force);
  void holdReset();
  void releaseReset();

 private:
  BspTransceiverMapping tcvrMapping_;
  int tcvrID_;
  std::unique_ptr<BspTransceiverAccessImpl> impl_;
};

} // namespace fboss
} // namespace facebook
