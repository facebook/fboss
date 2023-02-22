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

namespace facebook {
namespace fboss {

class Wedge40PlatformMapping : public PlatformMapping {
 public:
  Wedge40PlatformMapping();
  explicit Wedge40PlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Wedge40PlatformMapping(Wedge40PlatformMapping const&) = delete;
  Wedge40PlatformMapping& operator=(Wedge40PlatformMapping const&) = delete;
};
} // namespace fboss
} // namespace facebook
