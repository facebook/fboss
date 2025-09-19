// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Tahansb800bcBspPlatformMapping : public BspPlatformMapping {
 public:
  Tahansb800bcBspPlatformMapping();
  explicit Tahansb800bcBspPlatformMapping(
      const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
