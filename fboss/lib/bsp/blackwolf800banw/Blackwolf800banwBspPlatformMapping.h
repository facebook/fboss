// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Blackwolf800banwBspPlatformMapping : public BspPlatformMapping {
 public:
  Blackwolf800banwBspPlatformMapping();
  explicit Blackwolf800banwBspPlatformMapping(
      const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
