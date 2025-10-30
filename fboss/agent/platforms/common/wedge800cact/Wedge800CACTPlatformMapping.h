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

class Wedge800CACTPlatformMapping : public PlatformMapping {
 public:
  Wedge800CACTPlatformMapping();
  explicit Wedge800CACTPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Wedge800CACTPlatformMapping(Wedge800CACTPlatformMapping const&) = delete;
  Wedge800CACTPlatformMapping& operator=(Wedge800CACTPlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
