/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/saintpaul/SaintpaulPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
}
)";
} // namespace

namespace facebook::fboss {

SaintpaulPlatformMapping::SaintpaulPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

SaintpaulPlatformMapping::SaintpaulPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

std::map<uint32_t, std::pair<uint32_t, uint32_t>>
SaintpaulPlatformMapping::getCpuPortsCoreAndPortIdx() const {
  // TODO: Replace this stub (copied from j4sim) with SaintPaul's actual
  // CPU port -> (core ID, port ID) mapping. Derive from the recycle/CPU
  // port entries in stpaul_static_mapping.csv or get from hardware team.
  static const std::map<uint32_t, std::pair<uint32_t, uint32_t>>
      kSingleStageCpuPortsCoreAndPortIdx = {
          // {CPU System Port, {Core ID, Port ID/PP_DSP}}
          {0, {0, 0}},
          {1, {1, 200}},
          {2, {2, 202}},
          {3, {3, 203}},
      };
  return kSingleStageCpuPortsCoreAndPortIdx;
}

} // namespace facebook::fboss
