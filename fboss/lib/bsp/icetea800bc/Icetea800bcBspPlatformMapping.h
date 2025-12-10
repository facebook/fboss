// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class Icetea800bcBspPlatformMapping : public BspPlatformMapping {
 public:
  Icetea800bcBspPlatformMapping();
  explicit Icetea800bcBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
