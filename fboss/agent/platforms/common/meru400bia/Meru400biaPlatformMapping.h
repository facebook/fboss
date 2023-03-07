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

class Meru400biaPlatformMapping : public PlatformMapping {
 public:
  Meru400biaPlatformMapping();
  Meru400biaPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Meru400biaPlatformMapping(Meru400biaPlatformMapping const&) = delete;
  Meru400biaPlatformMapping& operator=(Meru400biaPlatformMapping const&) =
      delete;
};
} // namespace fboss
} // namespace facebook
