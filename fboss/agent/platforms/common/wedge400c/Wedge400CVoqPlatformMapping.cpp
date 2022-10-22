/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/wedge400c/Wedge400CVoqPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{}
)";
} // namespace

namespace facebook {
namespace fboss {
Wedge400CVoqPlatformMapping::Wedge400CVoqPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}
} // namespace fboss
} // namespace facebook
