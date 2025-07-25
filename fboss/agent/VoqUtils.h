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

} // namespace facebook::fboss
