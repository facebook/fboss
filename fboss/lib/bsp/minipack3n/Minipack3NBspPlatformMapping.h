// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Minipack3NBspPlatformMapping : public BspPlatformMapping {
 public:
  Minipack3NBspPlatformMapping();
  explicit Minipack3NBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
