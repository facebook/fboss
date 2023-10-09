// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class MontblancBspPlatformMapping : public BspPlatformMapping {
 public:
  MontblancBspPlatformMapping();
  explicit MontblancBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
