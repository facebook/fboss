// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Meru400biuBspPlatformMapping : public BspPlatformMapping {
 public:
  Meru400biuBspPlatformMapping();
  explicit Meru400biuBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
