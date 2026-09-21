// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Morgan800ccBspPlatformMapping : public BspPlatformMapping {
 public:
  Morgan800ccBspPlatformMapping();
  explicit Morgan800ccBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
