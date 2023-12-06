// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Janga800bicBspPlatformMapping : public BspPlatformMapping {
 public:
  Janga800bicBspPlatformMapping();
  explicit Janga800bicBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
