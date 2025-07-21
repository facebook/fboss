// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Icecube800bcBspPlatformMapping : public BspPlatformMapping {
 public:
  Icecube800bcBspPlatformMapping();
  explicit Icecube800bcBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
