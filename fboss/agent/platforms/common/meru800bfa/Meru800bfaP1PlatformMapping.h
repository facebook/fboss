// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook::fboss {

class Meru800bfaP1PlatformMapping : public PlatformMapping {
 public:
  Meru800bfaP1PlatformMapping();
  explicit Meru800bfaP1PlatformMapping(const std::string& platformMappingStr);
  explicit Meru800bfaP1PlatformMapping(bool multiNpuPlatformMapping);

 private:
  // Forbidden copy constructor and assignment operator
  Meru800bfaP1PlatformMapping(Meru800bfaP1PlatformMapping const&) = delete;
  Meru800bfaP1PlatformMapping& operator=(Meru800bfaP1PlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
