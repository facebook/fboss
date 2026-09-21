// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

class BspTransceiverAccessImpl {
 public:
  BspTransceiverAccessImpl(uint32_t tcvr, BspTransceiverMapping& tcvrMapping);
  virtual ~BspTransceiverAccessImpl() {}

  virtual bool isPresent() = 0;
  virtual void init(bool force) = 0;
  virtual void holdReset() = 0;
  virtual void releaseReset() = 0;

 protected:
  BspTransceiverMapping tcvrMapping_;
  int tcvrID_;
};

} // namespace fboss
} // namespace facebook
