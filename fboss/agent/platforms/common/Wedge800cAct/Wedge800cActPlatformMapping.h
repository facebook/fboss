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

class Wedge800cActPlatformMapping : public PlatformMapping {
 public:
  Wedge800cActPlatformMapping();
  explicit Wedge800cActPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Wedge800cActPlatformMapping(Wedge800cActPlatformMapping const&) = delete;
  Wedge800cActPlatformMapping& operator=(Wedge800cActPlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
