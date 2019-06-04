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
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Vlan.h"

namespace facebook {
namespace fboss {

void ManagerTestBase::SetUp() {
  fs = FakeSai::getInstance();
  sai_api_initialize(0, nullptr);
  saiPlatform = std::make_unique<SaiPlatform>();
  saiApiTable = std::make_unique<SaiApiTable>();
  saiManagerTable =
      std::make_unique<SaiManagerTable>(saiApiTable.get(), saiPlatform.get());

  for (int i = 0; i < testInterfaces.size(); ++i) {
    if (i == 0) {
      testInterfaces[i] = TestInterface{i, 4};
    } else {
      testInterfaces[i] = TestInterface{i, 1};
    }
  }

  if (setupStage & SetupStage::PORT) {
    for (const auto& testInterface : testInterfaces) {
      for (const auto& remoteHost : testInterface.remoteHosts) {
        auto swPort = makePort(remoteHost.port);
        saiManagerTable->portManager().addPort(swPort);
      }
    }
  }
  if (setupStage & SetupStage::VLAN) {
    for (const auto& testInterface : testInterfaces) {
      auto swVlan = makeVlan(testInterface);
      saiManagerTable->vlanManager().addVlan(swVlan);
    }
  }
  if (setupStage & SetupStage::INTERFACE) {
    for (const auto& testInterface : testInterfaces) {
      auto swInterface = makeInterface(testInterface);
      saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
    }
  }
  if (setupStage & SetupStage::NEIGHBOR) {
    for (const auto& testInterface : testInterfaces) {
      for (const auto& remoteHost : testInterface.remoteHosts) {
        auto swNeighbor = makeArpEntry(testInterface.id, remoteHost);
        saiManagerTable->neighborManager().addNeighbor(swNeighbor);
      }
    }
  }
}

std::shared_ptr<Port> ManagerTestBase::makePort(
    const TestPort& testPort) const {
  std::string name = folly::sformat("port{}", testPort.id);
  auto swPort = std::make_shared<Port>(PortID(testPort.id), name);
  if (testPort.enabled) {
    swPort->setAdminState(cfg::PortState::ENABLED);
  }
  swPort->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  return swPort;
}

std::shared_ptr<Vlan> ManagerTestBase::makeVlan(
    const TestInterface& testInterface) const {
  std::string name = folly::sformat("vlan{}", testInterface.id);
  auto swVlan = std::make_shared<Vlan>(VlanID(testInterface.id), name);
  VlanFields::MemberPorts mps;
  for (const auto& remoteHost : testInterface.remoteHosts) {
    PortID portId(remoteHost.port.id);
    VlanFields::PortInfo portInfo(false);
    mps.insert(std::make_pair(portId, portInfo));
  }
  swVlan->setPorts(mps);
  return swVlan;
}

std::shared_ptr<Interface> ManagerTestBase::makeInterface(
    const TestInterface& testInterface) const {
  auto interface = std::make_shared<Interface>(
      InterfaceID(testInterface.id),
      RouterID(0),
      VlanID(testInterface.id),
      folly::sformat("intf{}", testInterface.id),
      testInterface.routerMac,
      1500, // mtu
      false, // isVirtual
      false); // isStateSyncDisabled
  Interface::Addresses addresses;
  addresses.emplace(
      testInterface.subnet.first.asV4(), testInterface.subnet.second);
  interface->setAddresses(addresses);
  return interface;
}

std::shared_ptr<ArpEntry> ManagerTestBase::makePendingArpEntry(
    int id,
    const TestRemoteHost& testRemoteHost) const {
  return std::make_shared<ArpEntry>(
      testRemoteHost.ip.asV4(), InterfaceID(id), NeighborState::PENDING);
}

std::shared_ptr<ArpEntry> ManagerTestBase::makeArpEntry(
    int id,
    const TestRemoteHost& testRemoteHost) const {
  return std::make_shared<ArpEntry>(
      testRemoteHost.ip.asV4(),
      testRemoteHost.mac,
      PortDescriptor(PortID(testRemoteHost.port.id)),
      InterfaceID(id));
}

ResolvedNextHop ManagerTestBase::makeNextHop(
    const TestInterface& testInterface) const {
  const auto& remote = testInterface.remoteHosts.at(0);
  return ResolvedNextHop{remote.ip, InterfaceID(testInterface.id), ECMP_WEIGHT};
}

std::shared_ptr<Route<folly::IPAddressV4>> ManagerTestBase::makeRoute(
    const TestRoute& route) const {
  RouteFields<folly::IPAddressV4>::Prefix destination;
  destination.network = route.destination.first.asV4();
  destination.mask = route.destination.second;
  RouteNextHopEntry::NextHopSet swNextHops{};
  for (const auto& testInterface : route.nextHopInterfaces) {
    swNextHops.emplace(makeNextHop(testInterface));
  }
  RouteNextHopEntry entry(swNextHops, AdminDistance::STATIC_ROUTE);
  auto r = std::make_shared<Route<folly::IPAddressV4>>(destination);
  r->update(ClientID{42}, entry);
  r->setResolved(entry);
  return r;
}

} // namespace fboss
} // namespace facebook
