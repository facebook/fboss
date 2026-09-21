/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/tahan800bc/Tahan800bcPlatformMapping.h"

namespace facebook::fboss {
Tahan800bcPlatformMapping::Tahan800bcPlatformMapping()
    : PlatformMapping(getPlatformMappingStr()) {}

Tahan800bcPlatformMapping::Tahan800bcPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
