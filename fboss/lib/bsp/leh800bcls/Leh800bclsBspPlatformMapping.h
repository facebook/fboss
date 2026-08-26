// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Leh800bclsBspPlatformMapping : public BspPlatformMapping {
 public:
  Leh800bclsBspPlatformMapping();
  explicit Leh800bclsBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
