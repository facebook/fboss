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
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

extern "C" {
#include <sai.h>
}
namespace facebook::fboss::utility {

sai_port_flow_control_mode_t getSaiPortPauseMode(cfg::PortPause pause);

sai_port_internal_loopback_mode_t getSaiPortInternalLoopbackMode(
    cfg::PortLoopbackMode loopbackMode);

sai_port_media_type_t getSaiPortMediaType(
    TransmitterTechnology transmitterTech,
    cfg::PortSpeed speed);

sai_port_fec_mode_t getSaiPortFecMode(phy::FecMode fec);

cfg::PortLoopbackMode getCfgPortInternalLoopbackMode(
    sai_port_internal_loopback_mode_t mode);

} // namespace facebook::fboss::utility
