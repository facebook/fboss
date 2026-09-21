// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Minipack3BTABspPlatformMapping : public BspPlatformMapping {
 public:
  Minipack3BTABspPlatformMapping();
  explicit Minipack3BTABspPlatformMapping(
      const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
