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
#include "fboss/lib/platforms/PlatformMode.h"

namespace facebook {
namespace fboss {

class MinipackPlatformMapping : public PlatformMapping {
 public:
  explicit MinipackPlatformMapping(
      ExternalPhyVersion xphyVersion,
      const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  MinipackPlatformMapping(MinipackPlatformMapping const&) = delete;
  MinipackPlatformMapping& operator=(MinipackPlatformMapping const&) = delete;
};
} // namespace fboss
} // namespace facebook
