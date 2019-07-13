/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/Wedge40Platform.h"

#include <folly/Range.h>

namespace facebook {
namespace fboss {

folly::ByteRange Wedge40Platform::defaultLed0Code() {
  return folly::ByteRange();
}

folly::ByteRange Wedge40Platform::defaultLed1Code() {
  return defaultLed0Code();
}

const PortQueue& Wedge40Platform::getDefaultPortQueueSettings(
    cfg::StreamType /*streamType*/) const {
  throw FbossError("PortQueue setting is not supported");
}

const PortQueue& Wedge40Platform::getDefaultControlPlaneQueueSettings(
    cfg::StreamType /*streamType*/) const {
  throw FbossError("ControlPlaneQueue setting is not supported");
}
} // namespace fboss
} // namespace facebook
