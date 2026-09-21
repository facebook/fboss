// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspLedContainer.h"
#include "fboss/lib/led/LedIO.h"
#include "fboss/lib/led/gen-cpp2/led_mapping_types.h"

namespace facebook {
namespace fboss {

BspLedContainer::BspLedContainer(LedMapping& ledMapping)
    : ledMapping_(ledMapping) {
  ledID_ = *ledMapping.id();
  ledIO_ = std::make_unique<LedIO>(ledMapping);
}

} // namespace fboss
} // namespace facebook
