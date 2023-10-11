
/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/sandia/Sandia8DDPimPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"()";
} // namespace

namespace facebook {
namespace fboss {
Sandia8DDPimPlatformMapping::Sandia8DDPimPlatformMapping()
    : MultiPimPlatformMapping(kJsonPlatformMappingStr) {}

Sandia8DDPimPlatformMapping::Sandia8DDPimPlatformMapping(
    const std::string& platformMappingStr)
    : MultiPimPlatformMapping(platformMappingStr) {}

} // namespace fboss
} // namespace facebook
