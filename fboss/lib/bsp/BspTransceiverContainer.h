// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <folly/logging/xlog.h>
#include <memory>
#include <unordered_map>
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

class BspTransceiverContainer {
 public:
  explicit BspTransceiverContainer(BspTransceiverMapping& tcvrMapping);

 private:
  BspTransceiverMapping tcvrMapping_;
  int tcvrID_;
};

} // namespace fboss
} // namespace facebook
