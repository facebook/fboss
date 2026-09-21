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
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/types.h"

/*
 * This utility is to provide utils for hw pfc tests.
 */

namespace facebook::fboss {
class HwSwitch;

namespace utility {

std::vector<int> kLosslessPgs(const HwSwitch* hwSwitch);
int kPgMinLimitCells();
int kGlobalSharedBufferCells(const HwSwitch* hwSwitch);
int kGlobalHeadroomBufferCells(const HwSwitch* hwSwitch);
int kUplinkPgHeadroomLimitCells(const HwSwitch* hwSwitch);
int kDownlinkPgHeadroomLimitCells(const HwSwitch* hwSwitch);
int kPgResumeLimitCells();

void addPfcConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports);

void addUplinkDownlinkPfcConfig(
    cfg::SwitchConfig& cfg,
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& uplinkPorts,
    const std::vector<PortID>& downllinkPorts);

} // namespace utility
} // namespace facebook::fboss
