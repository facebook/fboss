// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class DarwinBspPlatformMapping : public BspPlatformMapping {
 public:
  DarwinBspPlatformMapping();
  explicit DarwinBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
