// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Ladakh800bclsBspPlatformMapping : public BspPlatformMapping {
 public:
  Ladakh800bclsBspPlatformMapping();
  explicit Ladakh800bclsBspPlatformMapping(
      const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
