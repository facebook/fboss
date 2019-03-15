/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/platforms/FakeBcmTestPlatform.h"

#include "fboss/agent/state/PortQueue.h"

namespace facebook {
namespace fboss {
const PortQueue& FakeBcmTestPlatform::getDefaultPortQueueSettings(
    cfg::StreamType /*streamType*/) const {
  static const PortQueue kQueue;
  return kQueue;
}

const PortQueue& FakeBcmTestPlatform::getDefaultControlPlaneQueueSettings(
    cfg::StreamType /*streamType*/) const {
  static const PortQueue kQueue;
  return kQueue;
}
} // namespace fboss
} // namespace facebook
