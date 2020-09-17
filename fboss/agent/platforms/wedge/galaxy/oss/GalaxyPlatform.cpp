/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/galaxy/GalaxyPlatform.h"

namespace {
constexpr auto kDefaultFCName = "fc001";
constexpr auto kDefaultLCName = "lc001";
} // namespace

namespace facebook::fboss {

folly::ByteRange GalaxyPlatform::defaultLed0Code() {
  return folly::ByteRange();
}

folly::ByteRange GalaxyPlatform::defaultLed1Code() {
  return defaultLed0Code();
}
} // namespace facebook::fboss
