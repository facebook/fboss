/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"

#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Vlan.h"

namespace facebook {
namespace fboss {

void ManagerTestBase::SetUp() {
  fs = FakeSai::getInstance();
  saiApiTable = std::make_unique<SaiApiTable>();
  saiManagerTable = std::make_unique<SaiManagerTable>(saiApiTable.get());
  sai_api_initialize(0, nullptr);
}

std::shared_ptr<ArpEntry> ManagerTestBase::makeArpEntry(
    uint16_t id,
    const folly::IPAddressV4& ip,
    const folly::MacAddress& dstMac) const {
  return std::make_shared<ArpEntry>(
      ip, dstMac, PortDescriptor(PortID(id)), InterfaceID(id));
}

void ManagerTestBase::addArpEntry(
    uint16_t id,
    const folly::IPAddressV4& ip,
    const folly::MacAddress& dstMac) {
  saiManagerTable->neighborManager().addNeighbor(makeArpEntry(id, ip, dstMac));
}

std::shared_ptr<Interface> ManagerTestBase::makeInterface(
    uint16_t id,
    const folly::MacAddress& srcMac) const {
  return std::make_shared<Interface>(
      InterfaceID(id),
      RouterID(0),
      VlanID(id),
      folly::sformat("intf{}", id),
      srcMac,
      1500, // mtu
      false, // isVirtual
      false); // isStateSyncDisabled
}

sai_object_id_t ManagerTestBase::addInterface(
    uint32_t id,
    const folly::MacAddress& srcMac) {
  auto swInterface = makeInterface(id, srcMac);
  return saiManagerTable->routerInterfaceManager().addRouterInterface(
      swInterface);
}

std::shared_ptr<Port> ManagerTestBase::makePort(uint16_t id, bool enabled)
    const {
  std::string name = folly::sformat("port{}", id);
  auto swPort = std::make_shared<Port>(PortID(id), name);
  swPort->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  if (enabled) {
    swPort->setAdminState(cfg::PortState::ENABLED);
  }
  return swPort;
}

sai_object_id_t ManagerTestBase::addPort(uint16_t id, bool enabled) {
  auto swPort = makePort(id, enabled);
  return saiManagerTable->portManager().addPort(swPort);
}

std::shared_ptr<Vlan> ManagerTestBase::makeVlan(
    uint16_t id,
    const std::vector<uint16_t>& memberPorts) const {
  std::string name = folly::sformat("vlan{}", id);
  auto swVlan = std::make_shared<Vlan>(VlanID(id), "vlan42");
  VlanFields::MemberPorts mps;
  for (int memberPort : memberPorts) {
    PortID portId(memberPort);
    VlanFields::PortInfo portInfo(false);
    mps.insert(std::make_pair(portId, portInfo));
  }
  swVlan->setPorts(mps);
  return swVlan;
}

sai_object_id_t ManagerTestBase::addVlan(
    uint16_t id,
    const std::vector<uint16_t>& memberPorts) {
  auto swVlan = makeVlan(id, memberPorts);
  return saiManagerTable->vlanManager().addVlan(swVlan);
}

} // namespace fboss
} // namespace facebook
