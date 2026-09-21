// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Meru800bfaBspPlatformMapping : public BspPlatformMapping {
 public:
  Meru800bfaBspPlatformMapping();
  explicit Meru800bfaBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
