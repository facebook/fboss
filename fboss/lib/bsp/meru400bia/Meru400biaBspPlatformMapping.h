// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Meru400biaBspPlatformMapping : public BspPlatformMapping {
 public:
  Meru400biaBspPlatformMapping();
  explicit Meru400biaBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
