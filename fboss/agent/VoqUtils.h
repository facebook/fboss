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

#include <boost/container/flat_map.hpp>
#include <folly/IPAddress.h>

namespace facebook::fboss {

class Interface;
class SwitchState;

using IntfAddress = std::pair<facebook::fboss::InterfaceID, folly::IPAddress>;
using IntfRoute = boost::container::flat_map<folly::CIDRNetwork, IntfAddress>;
using IntfRouteTable =
    boost::container::flat_map<facebook::fboss::RouterID, IntfRoute>;
using RouterIDToPrefixes = boost::container::
    flat_map<facebook::fboss::RouterID, std::vector<folly::CIDRNetwork>>;

constexpr auto k2StageEdgePodClusterId = 200;

// Due to a HW bug in J3/R3, we are restricted to using switch-ids in the
// range of 0-4064.
// For 2-Stage,
//   - Num RDSWs = 512. SwitchIds consumed = 2048.
//   - Num EDSWs = 128, Switch Ids consumed = 512. These ids will come after
//     2048. So 2560 (2.5K total). The last EDSW will take switch-ids from
//     2556-2559.
//   - Num FDSWs = 200. We now start FDSWs at 2560. Taking 4 switch IDs each,
//     FDSWs will use 800 switch IDs in the range 2560-3359.
//   - Num SDSWs = 128. We start SDSW switch-ids from 3360. Taking 4 switch
//     IDs each, this will use up 128*4 switch IDs in the range 3360-3871.
// Now, MAX switch ID can only be set in multiples of 32 minus 1. So we set
// the max switch ID to be the next multiple of 32 which is 3904-1 => 3903.
constexpr auto kDualStageMaxGlobalSwitchId{3903};

// Single stage FAP-ID on J3/R3 are limited to 1K. With 4 cores we are
// limited to 1K switch-ids. Then with 80 R3 chips we get 160 more switch
// IDs, so we are well within the 2K (vendor) recommended limit.
constexpr auto kSingleStageMaxGlobalSwitchId{2048};

// Max number of switchIDs that can be allocated for the various layers
// for fabric link monitoring.
constexpr auto kSingleStageMaxLeafL1FabricLinkMonitoringSwitchIds{256};
constexpr auto kDualStageMaxLeafL1FabricLinkMonitoringSwitchIds{160};
constexpr auto kDualStageMaxL1L2FabricLinkMonitoringSwitchIds{200};

// Define the starting switch ID for use with fabric link mon between
// FDSW-SDSW
constexpr auto kFabricLinkMonitoringL1L2BaseSwitchId{4096};

// Max usable switchID in VoQ switch
constexpr auto kMaxUsableVoqSwitchId{4064};

// Keep track of the max possible links per VD either local or remote
constexpr auto kDsfMaxLeafFabricLinksPerVd{40};
constexpr auto kDsfR192F40E32MaxL1ToLeafLinksPerVd{224};
constexpr auto kDsfR128F40E16MaxL1ToLeafLinksPerVd{144};
constexpr auto kDsfMtiaMaxL1ToLeafLinksPerVd{256};
constexpr auto kDsfDualStageMaxL1ToLeafLinksPerVd{128};
constexpr auto kDsfMaxL1ToL2SwitchLinksPerVd{128};
constexpr auto kDsfMaxL2ToL1SwitchLinksPerVd{200};

int getLocalPortNumVoqs(cfg::PortType portType, cfg::Scope portScope);

int getRemotePortNumVoqs(
    HwAsic::InterfaceNodeRole intfRole,
    cfg::PortType portType);

void processRemoteInterfaceRoutes(
    const std::shared_ptr<Interface>& remoteIntf,
    const std::shared_ptr<SwitchState>& state,
    bool add,
    IntfRouteTable& remoteIntfRoutesToAdd,
    RouterIDToPrefixes& remoteIntfRoutesToDel);

bool isConnectedToVoqSwitch(
    const cfg::SwitchConfig* config,
    const SwitchID& remoteSwitchId);
} // namespace facebook::fboss
