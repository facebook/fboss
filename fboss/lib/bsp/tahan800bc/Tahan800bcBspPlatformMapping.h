// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Tahan800bcBspPlatformMapping : public BspPlatformMapping {
 public:
  Tahan800bcBspPlatformMapping();
  explicit Tahan800bcBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
