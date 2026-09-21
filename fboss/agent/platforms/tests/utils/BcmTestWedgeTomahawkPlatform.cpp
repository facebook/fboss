/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestWedgeTomahawkPlatform.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"

namespace facebook::fboss {

const PortQueue& BcmTestWedgeTomahawkPlatform::getDefaultPortQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultPortQueueSettings(
      utility::BcmChip::TOMAHAWK, streamType);
}

const PortQueue&
BcmTestWedgeTomahawkPlatform::getDefaultControlPlaneQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultControlPlaneQueueSettings(
      utility::BcmChip::TOMAHAWK, streamType);
}

} // namespace facebook::fboss
