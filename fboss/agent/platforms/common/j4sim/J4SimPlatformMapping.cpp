/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/j4sim/J4SimPlatformMapping.h"

namespace facebook::fboss {

J4SimPlatformMapping::J4SimPlatformMapping() : PlatformMapping() {}

J4SimPlatformMapping::J4SimPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

std::map<uint32_t, std::pair<uint32_t, uint32_t>>
J4SimPlatformMapping::getCpuPortsCoreAndPortIdx() const {
  static const std::map<uint32_t, std::pair<uint32_t, uint32_t>>
      kSingleStageCpuPortsCoreAndPortIdx = {
          // {CPU System Port, {Core ID, Port ID/PP_DSP}}
          {0, {0, 0}},
          {1, {1, 0}},
          {2, {2, 0}},
          {3, {3, 0}},
      };
  return kSingleStageCpuPortsCoreAndPortIdx;
}

} // namespace facebook::fboss
