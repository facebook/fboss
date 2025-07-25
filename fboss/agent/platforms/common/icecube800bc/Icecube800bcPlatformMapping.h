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

class Icecube800bcPlatformMapping : public PlatformMapping {
 public:
  Icecube800bcPlatformMapping();
  explicit Icecube800bcPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Icecube800bcPlatformMapping(Icecube800bcPlatformMapping const&) = delete;
  Icecube800bcPlatformMapping& operator=(Icecube800bcPlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
