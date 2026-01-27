/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook::fboss {

class Icecube800banwPlatformMapping : public PlatformMapping {
 public:
  Icecube800banwPlatformMapping();
  explicit Icecube800banwPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Icecube800banwPlatformMapping(Icecube800banwPlatformMapping const&) = delete;
  Icecube800banwPlatformMapping& operator=(
      Icecube800banwPlatformMapping const&) = delete;
};
} // namespace facebook::fboss
