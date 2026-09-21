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

class Yangra2PlatformMapping : public PlatformMapping {
 public:
  Yangra2PlatformMapping();
  explicit Yangra2PlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Yangra2PlatformMapping(Yangra2PlatformMapping const&) = delete;
  Yangra2PlatformMapping& operator=(Yangra2PlatformMapping const&) = delete;
};
} // namespace facebook::fboss
