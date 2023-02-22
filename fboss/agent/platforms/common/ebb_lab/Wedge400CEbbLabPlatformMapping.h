// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook::fboss {

class Wedge400CEbbLabPlatformMapping : public PlatformMapping {
 public:
  Wedge400CEbbLabPlatformMapping();
  explicit Wedge400CEbbLabPlatformMapping(
      const std::string& platformMappingStr);

 private:
  Wedge400CEbbLabPlatformMapping(Wedge400CEbbLabPlatformMapping const&) =
      delete;
  Wedge400CEbbLabPlatformMapping& operator=(
      Wedge400CEbbLabPlatformMapping const&) = delete;
};

} // namespace facebook::fboss
