/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/GalaxyPlatform.h"


namespace facebook { namespace fboss {

std::map<std::string, std::string> GalaxyPlatform::loadConfig() {
  std::map<std::string, std::string> config;
  return config;
}

folly::ByteRange GalaxyPlatform::defaultLed0Code() {
  return folly::ByteRange();
}

folly::ByteRange GalaxyPlatform::defaultLed1Code() {
  return defaultLed0Code();
}

}} // facebook::fboss
