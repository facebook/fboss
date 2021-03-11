/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestWedgeTomahawk3Platform.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"
#include "fboss/agent/hw/bcm/BcmPortIngressBufferManager.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"

namespace facebook::fboss {

const PortQueue& BcmTestWedgeTomahawk3Platform::getDefaultPortQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultPortQueueSettings(
      utility::BcmChip::TOMAHAWK3, streamType);
}

const PortQueue&
BcmTestWedgeTomahawk3Platform::getDefaultControlPlaneQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultControlPlaneQueueSettings(
      utility::BcmChip::TOMAHAWK3, streamType);
}

const PortPgConfig& BcmTestWedgeTomahawk3Platform::getDefaultPortPgSettings()
    const {
  return BcmPortIngressBufferManager::getDefaultChipPgSettings(
      utility::BcmChip::TOMAHAWK3);
}

const BufferPoolCfg&
BcmTestWedgeTomahawk3Platform::getDefaultPortIngressPoolSettings() const {
  return BcmPortIngressBufferManager::getDefaultChipIngressPoolSettings(
      utility::BcmChip::TOMAHAWK3);
}

int BcmTestWedgeTomahawk3Platform::getPortItm(BcmPort* bcmPort) const {
  return utility::getPortItm(utility::BcmChip::TOMAHAWK3, bcmPort);
}

} // namespace facebook::fboss
