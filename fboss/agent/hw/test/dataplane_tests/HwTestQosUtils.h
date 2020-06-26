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
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

#include <map>
#include <vector>

namespace facebook::fboss {
class HwSwitchEnsemble;
struct HwPortStats;
class HwSwitch;
namespace utility {

bool verifyQueueMappings(
    const HwPortStats& portStatsBefore,
    const std::map<int, std::vector<uint8_t>>& q2dscps,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::PortID egressPort);

void disableTTLDecrements(
    HwSwitch* hw,
    RouterID routerId,
    InterfaceID intf,
    const folly::IPAddress& nhop);
} // namespace utility
} // namespace facebook::fboss
