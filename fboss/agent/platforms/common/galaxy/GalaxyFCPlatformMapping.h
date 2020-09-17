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

DECLARE_string(netwhoami);

namespace facebook {
namespace fboss {

class GalaxyFCPlatformMapping : public PlatformMapping {
 public:
  GalaxyFCPlatformMapping(const std::string& linecardName);

  static std::string getFabriccardName();

 private:
  // Forbidden copy constructor and assignment operator
  GalaxyFCPlatformMapping(GalaxyFCPlatformMapping const&) = delete;
  GalaxyFCPlatformMapping& operator=(GalaxyFCPlatformMapping const&) = delete;
};
} // namespace fboss
} // namespace facebook
