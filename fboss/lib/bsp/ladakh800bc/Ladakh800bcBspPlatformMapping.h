// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Ladakh800bcBspPlatformMapping : public BspPlatformMapping {
 public:
  Ladakh800bcBspPlatformMapping();
  explicit Ladakh800bcBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
