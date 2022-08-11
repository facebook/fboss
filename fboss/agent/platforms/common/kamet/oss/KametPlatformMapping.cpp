/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/kamet/KametPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
}
)";
} // namespace

namespace facebook {
namespace fboss {
KametPlatformMapping::KametPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

} // namespace fboss
} // namespace facebook
