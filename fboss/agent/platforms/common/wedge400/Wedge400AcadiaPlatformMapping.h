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
class Wedge400AcadiaPlatformMapping : public PlatformMapping {
 public:
  Wedge400AcadiaPlatformMapping();
  explicit Wedge400AcadiaPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Wedge400AcadiaPlatformMapping(Wedge400AcadiaPlatformMapping const&) = delete;
  Wedge400AcadiaPlatformMapping& operator=(
      Wedge400AcadiaPlatformMapping const&) = delete;
};
} // namespace facebook::fboss
