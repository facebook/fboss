// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Wedge800BACTBspPlatformMapping : public BspPlatformMapping {
 public:
  Wedge800BACTBspPlatformMapping();
  explicit Wedge800BACTBspPlatformMapping(
      const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
