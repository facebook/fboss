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
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

extern "C" {
#include <sai.h>
}
namespace facebook::fboss::utility {

sai_port_flow_control_mode_t getSaiPortPauseMode(cfg::PortPause pause);

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
sai_port_loopback_mode_t getSaiPortLoopbackMode(
    cfg::PortLoopbackMode loopbackMode);

cfg::PortLoopbackMode getCfgPortLoopbackMode(sai_port_loopback_mode_t mode);
#endif
sai_port_internal_loopback_mode_t getSaiPortInternalLoopbackMode(
    cfg::PortLoopbackMode loopbackMode);

cfg::PortLoopbackMode getCfgPortInternalLoopbackMode(
    sai_port_internal_loopback_mode_t mode);

sai_port_media_type_t getSaiPortMediaType(
    TransmitterTechnology transmitterTech,
    cfg::PortSpeed speed);

sai_port_media_type_t getSaiPortMediaFromInterfaceType(
    phy::InterfaceType interfaceType);

sai_port_fec_mode_t getSaiPortFecMode(phy::FecMode fec);

sai_port_ptp_mode_t getSaiPortPtpMode(bool enable);

phy::FecMode getFecModeFromSaiFecMode(
    sai_port_fec_mode_t fec,
    cfg::PortProfileID profileID);

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
sai_port_fec_mode_extended_t getSaiPortExtendedFecMode(phy::FecMode fec);

phy::FecMode getFecModeFromSaiExtendedFecMode(
    sai_port_fec_mode_extended_t fec,
    cfg::PortProfileID profileID);
#endif
} // namespace facebook::fboss::utility
