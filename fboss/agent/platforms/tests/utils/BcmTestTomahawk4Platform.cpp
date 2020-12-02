/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestTomahawk4Platform.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"

namespace facebook::fboss {

const PortQueue& BcmTestTomahawk4Platform::getDefaultPortQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultPortQueueSettings(
      utility::BcmChip::TOMAHAWK4, streamType);
}

const PortQueue& BcmTestTomahawk4Platform::getDefaultControlPlaneQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultControlPlaneQueueSettings(
      utility::BcmChip::TOMAHAWK4, streamType);
}

} // namespace facebook::fboss
