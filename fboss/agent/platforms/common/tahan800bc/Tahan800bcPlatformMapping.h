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

class Tahan800bcPlatformMapping : public PlatformMapping {
 public:
  Tahan800bcPlatformMapping();
  explicit Tahan800bcPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Tahan800bcPlatformMapping(Tahan800bcPlatformMapping const&) = delete;
  Tahan800bcPlatformMapping& operator=(Tahan800bcPlatformMapping const&) =
      delete;
  static const std::string getPlatformMappingStr();
};
} // namespace facebook::fboss
