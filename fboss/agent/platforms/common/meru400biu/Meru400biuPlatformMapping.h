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

class Meru400biuPlatformMapping : public PlatformMapping {
 public:
  Meru400biuPlatformMapping();
  Meru400biuPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Meru400biuPlatformMapping(Meru400biuPlatformMapping const&) = delete;
  Meru400biuPlatformMapping& operator=(Meru400biuPlatformMapping const&) =
      delete;
};
} // namespace fboss
} // namespace facebook
