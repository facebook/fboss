// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

class BspTransceiverAccess {
 public:
  BspTransceiverAccess(uint32_t tcvr, BspTransceiverMapping& tcvrMapping);

  bool isPresent();
  void init(bool force);

 private:
  BspTransceiverMapping tcvrMapping_;
  int tcvrID_;
};

} // namespace fboss
} // namespace facebook
