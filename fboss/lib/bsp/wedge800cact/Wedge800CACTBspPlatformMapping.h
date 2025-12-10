// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Wedge800CACTBspPlatformMapping : public BspPlatformMapping {
 public:
  Wedge800CACTBspPlatformMapping();
  explicit Wedge800CACTBspPlatformMapping(
      const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
