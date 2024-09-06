// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

using facebook::fboss::HwSwitch;
using facebook::fboss::InterfaceID;
using facebook::fboss::SaiManagerTable;
using facebook::fboss::SaiNeighborHandle;
using facebook::fboss::SaiRouterInterfaceHandle;
using facebook::fboss::SaiSwitch;

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
  facebook::fboss::SaiNeighborTraits::NeighborEntry entry(
      managerTable->switchManager().getSwitchSaiId(),
      rintfHandle->adapterKey(),
      ip);
  return managerTable->neighborManager().getNeighborHandle(entry);
}

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
  if (!nbrHandle || !nbrHandle->neighbor) {
    XLOG(DBG2) << "Neighbor does not exist for " << intf << " " << ip;
    return std::nullopt;
  }
  return facebook::fboss::SaiApiTable::getInstance()
      ->neighborApi()
      .getAttribute(
          nbrHandle->neighbor->adapterKey(),
          facebook::fboss::SaiNeighborTraits::Attributes::Metadata{});
}

} // namespace

namespace facebook {
namespace fboss {
namespace utility {

void HwTestThriftHandler::getNeighborInfo(
    NeighborInfo& neighborInfo,
    std::unique_ptr<::facebook::fboss::IfAndIP> neighbor) {
  auto classId = getNbrClassId(
      hwSwitch_,
      InterfaceID(*neighbor->interfaceID()),
      facebook::network::toIPAddress(*neighbor->ip()));
  if (classId.has_value()) {
    neighborInfo.classId() = *classId;
  }
  neighborInfo.isProgrammedToCpu() = nbrProgrammedToCpu(
      hwSwitch_,
      InterfaceID(*neighbor->interfaceID()),
      facebook::network::toIPAddress(*neighbor->ip()));
  neighborInfo.exists() = nbrExists(
      hwSwitch_,
      InterfaceID(*neighbor->interfaceID()),
      facebook::network::toIPAddress(*neighbor->ip()));
}

} // namespace utility
} // namespace fboss
} // namespace facebook
