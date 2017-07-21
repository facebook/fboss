/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/Wedge100Platform.h"

#include <folly/Range.h>

namespace facebook { namespace fboss {

std::map<std::string, std::string> Wedge100Platform::loadConfig() {
  std::map<std::string, std::string> config;
  return config;
}

folly::ByteRange Wedge100Platform::defaultLed0Code() {
  return folly::ByteRange();
}

folly::ByteRange Wedge100Platform::defaultLed1Code() {
  return defaultLed0Code();
}

void Wedge100Platform::setPciPreemphasis(int /*unit*/) const {
  //stubbed out
}

}} // facebook::fboss
