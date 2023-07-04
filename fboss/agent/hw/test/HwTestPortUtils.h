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
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {
class HwSwitch;
namespace utility {

void setPortLoopbackMode(
    const HwSwitch* hw,
    PortID port,
    cfg::PortLoopbackMode lbMode);

TransceiverInfo getTransceiverInfo(cfg::PortProfileID profileID);

void setCreditWatchdogAndPortTx(const HwSwitch* hw, PortID port, bool enable);

void enableTransceiverProgramming(bool enable);
int getLoopbackMode(cfg::PortLoopbackMode loopbackMode);

} // namespace utility
} // namespace facebook::fboss
