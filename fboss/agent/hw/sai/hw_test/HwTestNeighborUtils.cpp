/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestNeighborUtils.h"

#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

namespace {
const SaiRouterInterfaceHandle* getRouterInterfaceHandle(
    const SaiManagerTable* managerTable,
    InterfaceID intf) {
  return managerTable->routerInterfaceManager().getRouterInterfaceHandle(intf);
}

const SaiNeighborHandle* getNbrHandle(
    const SaiManagerTable* managerTable,
    InterfaceID intf,
    const folly::IPAddress& ip) {
  auto rintfHandle = getRouterInterfaceHandle(managerTable, intf);
  SaiNeighborTraits::NeighborEntry entry(
      managerTable->switchManager().getSwitchSaiId(),
      rintfHandle->routerInterface->adapterKey(),
      ip);
  return managerTable->neighborManager().getNeighborHandle(entry);
}
} // namespace

namespace utility {

bool nbrProgrammedToCpu(
    const HwSwitch* hwSwitch,
    InterfaceID intf,
    const folly::IPAddress& ip) {
  auto managerTable = static_cast<const SaiSwitch*>(hwSwitch)->managerTable();
  auto nbrHandle = getNbrHandle(managerTable, intf, ip);
  // TODO Lookup interface route and confirm that it points to CPU nhop
  return nbrHandle == nullptr;
}

bool nbrExists(
    const HwSwitch* hwSwitch,
    InterfaceID intf,
    const folly::IPAddress& ip) {
  auto managerTable = static_cast<const SaiSwitch*>(hwSwitch)->managerTable();
  auto nbrHandle = getNbrHandle(managerTable, intf, ip);

  return nbrHandle && nbrHandle->neighbor;
}

std::optional<uint32_t> getNbrClassId(
    const HwSwitch* hwSwitch,
    InterfaceID intf,
    const folly::IPAddress& ip) {
  auto managerTable = static_cast<const SaiSwitch*>(hwSwitch)->managerTable();
  auto nbrHandle = getNbrHandle(managerTable, intf, ip);
  return SaiApiTable::getInstance()->neighborApi().getAttribute(
      nbrHandle->neighbor->adapterKey(),
      SaiNeighborTraits::Attributes::Metadata{});
}

} // namespace utility
} // namespace facebook::fboss
