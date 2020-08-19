/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/wedge/fuji/Fuji16QPimPlatformMapping.h"
#include <folly/logging/xlog.h>

namespace {
// TODO(daiweix): copy fuji platform mapping here
constexpr auto kJsonPlatformMappingStr = R"({})";
} // namespace

namespace facebook {
namespace fboss {
Fuji16QPimPlatformMapping::Fuji16QPimPlatformMapping()
    : MultiPimPlatformMapping(kJsonPlatformMappingStr) {}
} // namespace fboss
} // namespace facebook
