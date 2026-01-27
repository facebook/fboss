// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Icecube800banwBspPlatformMapping : public BspPlatformMapping {
 public:
  Icecube800banwBspPlatformMapping();
  explicit Icecube800banwBspPlatformMapping(
      const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
