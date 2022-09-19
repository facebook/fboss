// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <folly/logging/xlog.h>
#include <memory>
#include <unordered_map>
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/BspTransceiverAccess.h"
#include "fboss/lib/bsp/BspTransceiverIO.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

class BspTransceiverContainer {
 public:
  explicit BspTransceiverContainer(BspTransceiverMapping& tcvrMapping);

  bool isTcvrPresent() const;
  void initTransceiver() {}
  void clearTransceiverReset() const;
  void triggerTcvrHardReset() const;

 private:
  BspTransceiverMapping tcvrMapping_;
  int tcvrID_;
  std::unique_ptr<BspTransceiverIO> tcvrIO_;
  std::unique_ptr<BspTransceiverAccess> tcvrAccess_;
};

} // namespace fboss
} // namespace facebook
