// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class JangaBspPlatformMapping : public BspPlatformMapping {
 public:
  JangaBspPlatformMapping();
  explicit JangaBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
