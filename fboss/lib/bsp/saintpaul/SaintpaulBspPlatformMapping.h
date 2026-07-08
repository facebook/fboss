// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspPlatformMapping.h"

namespace facebook {
namespace fboss {

class SaintpaulBspPlatformMapping : public BspPlatformMapping {
 public:
  SaintpaulBspPlatformMapping();
  explicit SaintpaulBspPlatformMapping(const std::string& platformMappingStr);
};

} // namespace fboss
} // namespace facebook
