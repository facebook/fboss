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

class Wedge800BACTPlatformMapping : public PlatformMapping {
 public:
  Wedge800BACTPlatformMapping();
  explicit Wedge800BACTPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Wedge800BACTPlatformMapping(Wedge800BACTPlatformMapping const&) = delete;
  Wedge800BACTPlatformMapping& operator=(Wedge800BACTPlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
