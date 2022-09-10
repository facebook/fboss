/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/makalu/MakaluPlatformMapping.h"

// TODO(skhare) add platform mapping

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
}
)";
} // namespace

namespace facebook {
namespace fboss {
MakaluPlatformMapping::MakaluPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

} // namespace fboss
} // namespace facebook
