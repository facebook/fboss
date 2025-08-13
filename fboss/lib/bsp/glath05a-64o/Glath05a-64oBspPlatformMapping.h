// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Glath05a_64oBspPlatformMapping : public BspPlatformMapping {
 public:
  Glath05a_64oBspPlatformMapping();
  explicit Glath05a_64oBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
