// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Meru800biaBspPlatformMapping : public BspPlatformMapping {
 public:
  Meru800biaBspPlatformMapping();
  explicit Meru800biaBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
