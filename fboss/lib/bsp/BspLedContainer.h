// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <folly/logging/xlog.h>
#include <memory>
#include <unordered_map>
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/lib/led/LedIO.h"
#include "fboss/lib/led/gen-cpp2/led_mapping_types.h"

namespace facebook {
namespace fboss {

class BspLedContainer {
 public:
  explicit BspLedContainer(LedMapping& ledMapping);
  LedIO* getLedController() const {
    return ledIO_.get();
  }

 private:
  LedMapping ledMapping_;
  int ledID_;
  std::unique_ptr<LedIO> ledIO_;
};

} // namespace fboss
} // namespace facebook
