/*
 *  Copyright (c) 2023-present, Facebook, Inc.
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

class Minipack3NPlatformMapping : public PlatformMapping {
 public:
  Minipack3NPlatformMapping();
  explicit Minipack3NPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Minipack3NPlatformMapping(Minipack3NPlatformMapping const&) = delete;
  Minipack3NPlatformMapping& operator=(Minipack3NPlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
