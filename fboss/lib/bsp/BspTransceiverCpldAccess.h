// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspTransceiverAccessImpl.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

class BspTransceiverCpldAccess : public BspTransceiverAccessImpl {
 public:
  BspTransceiverCpldAccess(uint32_t tcvr, BspTransceiverMapping& tcvrMapping);

  bool isPresent() override;
  void init(bool force) override;
  void holdReset() override;
  void releaseReset() override;
};

} // namespace fboss
} // namespace facebook
