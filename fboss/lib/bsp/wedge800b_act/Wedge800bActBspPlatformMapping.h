// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Wedge800bActBspPlatformMapping : public BspPlatformMapping {
 public:
  Wedge800bActBspPlatformMapping();
  explicit Wedge800bActBspPlatformMapping(
      const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
