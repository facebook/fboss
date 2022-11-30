/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"

#include "fboss/agent/FbossError.h"

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace facebook::fboss::utility {

void disableTTLDecrements(
    HwSwitch* hw,
    RouterID routerId,
    InterfaceID intf,
    const folly::IPAddress& nhopIp) {
  auto managerTable = static_cast<SaiSwitch*>(hw)->managerTable();
  auto rintfHandle =
      managerTable->routerInterfaceManager().getRouterInterfaceHandle(intf);
  SaiIpNextHopTraits::AdapterHostKey key(rintfHandle->adapterKey(), nhopIp);
  auto nhop = managerTable->nextHopManager().getManagedNextHop(key);
  SaiIpNextHopTraits::Attributes::DisableTtlDecrement disableTtl{true};
  SaiApiTable::getInstance()->nextHopApi().setAttribute(
      nhop->getSaiObject()->adapterKey(), disableTtl);
}

void disableTTLDecrements(HwSwitch* hw, const PortDescriptor& port) {
  if (!port.isPhysicalPort()) {
    throw FbossError("Port disable decrement not supported for LAGs yet");
  }
  auto managerTable = static_cast<SaiSwitch*>(hw)->managerTable();
  auto portHandle = managerTable->portManager().getPortHandle(port.phyPortID());
  SaiPortTraits::Attributes::DisableTtlDecrement disableTtl{true};
  SaiApiTable::getInstance()->portApi().setAttribute(
      portHandle->port->adapterKey(), disableTtl);
}

void enableTtlZeroPacketForwarding(HwSwitch* hw) {
  static HostifTrapSaiId hostIfTrap;
  if (!hostIfTrap) {
    auto managerTable = static_cast<SaiSwitch*>(hw)->managerTable();
    hostIfTrap = managerTable->hostifManager().addHostifTrap(
        cfg::PacketRxReason::TTL_0, 0 /*queueId*/, 1 /*priority*/);
  }
}

} // namespace facebook::fboss::utility
