/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <map>
extern "C" {
#include <bcm/port.h>
}

namespace facebook::fboss {

class PlatformMapping;

using PortSpeed2TransmitterTechAndMode =
    std::map<cfg::PortSpeed, std::map<TransmitterTechnology, bcm_port_if_t>>;

const PortSpeed2TransmitterTechAndMode& getSpeedToTransmitterTechAndMode();

uint32_t getDesiredPhyLaneConfig(
    TransmitterTechnology tech,
    cfg::PortSpeed speed);

} // namespace facebook::fboss

namespace facebook::fboss::utility {

bcm_port_phy_fec_t phyFecModeToBcmPortPhyFec(facebook::fboss::phy::FecMode fec);

facebook::fboss::phy::FecMode bcmPortPhyFecToPhyFecMode(bcm_port_phy_fec_t fec);

uint32_t getDesiredPhyLaneConfig(const phy::ProfileSideConfig& profileCfg);

bcm_gport_t getPortGport(int unit, int port);

int getPortItm(utility::BcmChip chip, BcmPort* bcmPort);

bcm_port_loopback_t fbToBcmLoopbackMode(cfg::PortLoopbackMode inMode);

int getBcmPfcDeadlockDetectionTimerGranularity(int timerMsec);

int getAdjustedPfcDeadlockDetectionTimerValue(int timerMsec);

int getAdjustedPfcDeadlockRecoveryTimerValue(cfg::AsicType type, int timerMsec);

int getDefaultPfcDeadlockDetectionTimer(cfg::AsicType type);

int getDefaultPfcDeadlockRecoveryTimer(cfg::AsicType type);

} // namespace facebook::fboss::utility
