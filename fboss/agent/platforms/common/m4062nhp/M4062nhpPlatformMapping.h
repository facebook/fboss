/*
 *  Copyright (c) 2026 Nexthop Systems Inc.
 *  SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook::fboss {

class M4062nhpPlatformMapping : public PlatformMapping {
 public:
  M4062nhpPlatformMapping();
  explicit M4062nhpPlatformMapping(const std::string& platformMappingStr);

 private:
  M4062nhpPlatformMapping(M4062nhpPlatformMapping const&) = delete;
  M4062nhpPlatformMapping& operator=(M4062nhpPlatformMapping const&) = delete;
};

} // namespace facebook::fboss
