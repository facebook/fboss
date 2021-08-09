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

std::vector<int> kLosslessPgs();
int kPgMinLimitCells();
int kGlobalSharedBufferCells();
int kGlobalHeadroomBufferCells();
int kPgHeadroomLimitCells();
int kPgResumeLimitCells();

void addPfcConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports);
} // namespace utility
} // namespace facebook::fboss
