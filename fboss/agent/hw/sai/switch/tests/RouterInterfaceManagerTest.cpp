/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSystemPortManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

HwSwitchMatcher scope() {
  return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(0)}));
}
/*
 * In these tests, we will assume 4 ports with one lane each, with IDs
 */

class RouterInterfaceManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    intf1 = testInterfaces[1];
    sysPort1 =
        saiManagerTable->systemPortManager().addSystemPort(makeSystemPort());
    sysPort2 = saiManagerTable->systemPortManager().addSystemPort(
        makeSystemPort(std::nullopt, 2));
  }

  void checkRouterInterface(
      RouterInterfaceSaiId saiRouterInterfaceId,
      VlanSaiId expectedSaiVlanId,
      const folly::MacAddress& expectedSrcMac,
      int expectedMtu = 1500) const {
    auto saiVlanIdGot = saiApiTable->routerInterfaceApi().getAttribute(
        saiRouterInterfaceId,
        SaiVlanRouterInterfaceTraits::Attributes::VlanId{});
    auto srcMacGot = saiApiTable->routerInterfaceApi().getAttribute(
        saiRouterInterfaceId,
        SaiVlanRouterInterfaceTraits::Attributes::SrcMac{});
    auto vlanIdGot = saiApiTable->vlanApi().getAttribute(
        VlanSaiId{saiVlanIdGot}, SaiVlanTraits::Attributes::VlanId{});
    auto mtuGot = saiApiTable->routerInterfaceApi().getAttribute(
        saiRouterInterfaceId, SaiVlanRouterInterfaceTraits::Attributes::Mtu{});
    EXPECT_EQ(VlanSaiId{vlanIdGot}, expectedSaiVlanId);
    EXPECT_EQ(srcMacGot, expectedSrcMac);
    EXPECT_EQ(mtuGot, expectedMtu);
  }
  void checkRouterInterface(
      InterfaceID intfId,
      VlanSaiId expectedSaiVlanId,
      const folly::MacAddress& expectedSrcMac,
      int expectedMtu = 1500) const {
    auto saiId = saiManagerTable->routerInterfaceManager()
                     .getRouterInterfaceHandle(intfId)
                     ->adapterKey();
    checkRouterInterface(saiId, expectedSaiVlanId, expectedSrcMac, expectedMtu);
  }
  void checkPortRouterInterface(
      RouterInterfaceSaiId saiRouterInterfaceId,
      SystemPortSaiId expectedSaiPortId,
      const folly::MacAddress& expectedSrcMac,
      int expectedMtu = 1500) const {
    SystemPortSaiId saiPortIdGot{saiApiTable->routerInterfaceApi().getAttribute(
        saiRouterInterfaceId,
        SaiPortRouterInterfaceTraits::Attributes::PortId{})};
    auto srcMacGot = saiApiTable->routerInterfaceApi().getAttribute(
        saiRouterInterfaceId,
        SaiPortRouterInterfaceTraits::Attributes::SrcMac{});
    auto mtuGot = saiApiTable->routerInterfaceApi().getAttribute(
        saiRouterInterfaceId, SaiPortRouterInterfaceTraits::Attributes::Mtu{});
    EXPECT_EQ(saiPortIdGot, expectedSaiPortId);
    EXPECT_EQ(srcMacGot, expectedSrcMac);
    EXPECT_EQ(mtuGot, expectedMtu);
  }

  void checkPortRouterInterface(
      InterfaceID intfId,
      SystemPortSaiId expectedSaiPortId,
      const folly::MacAddress& expectedSrcMac,
      int expectedMtu = 1500) const {
    auto saiId = saiManagerTable->routerInterfaceManager()
                     .getRouterInterfaceHandle(intfId)
                     ->adapterKey();
    checkPortRouterInterface(
        saiId, expectedSaiPortId, expectedSrcMac, expectedMtu);
  }
  void checkAdapterKey(
      RouterInterfaceSaiId saiRouterInterfaceId,
      InterfaceID intfId) const {
    EXPECT_EQ(
        saiManagerTable->routerInterfaceManager()
            .getRouterInterfaceHandle(intfId)
            ->adapterKey(),
        saiRouterInterfaceId);
  }
  void checkType(
      RouterInterfaceSaiId saiRouterInterfaceId,
      InterfaceID intfId,
      cfg::InterfaceType expectedType) const {
    EXPECT_EQ(
        saiManagerTable->routerInterfaceManager()
            .getRouterInterfaceHandle(intfId)
            ->type(),
        expectedType);
  }

  void checkSubnets(
      const std::vector<SaiRouteTraits::RouteEntry>& subnetRoutes,
      bool shouldExist) const {
    for (const auto& route : subnetRoutes) {
      auto& store = saiStore->get<SaiRouteTraits>();
      auto routeObj = store.get(route);
      if (shouldExist) {
        EXPECT_NE(routeObj, nullptr);
      } else {
        EXPECT_EQ(routeObj, nullptr);
      }
    }
  }

  std::vector<SaiRouteTraits::RouteEntry> getSubnetKeys(InterfaceID id) const {
    std::vector<SaiRouteTraits::RouteEntry> keys;
    auto toMeRoutes = saiManagerTable->routerInterfaceManager()
                          .getRouterInterfaceHandle(id)
                          ->toMeRoutes;
    for (const auto& toMeRoute : toMeRoutes) {
      keys.emplace_back(toMeRoute->adapterHostKey());
    }
    return keys;
  }

  TestInterface intf0;
  TestInterface intf1;
  SystemPortSaiId sysPort1, sysPort2;
};

TEST_F(RouterInterfaceManagerTest, addRouterInterface) {
  auto swInterface = makeInterface(intf0);
  auto state = programmedState;
  auto rifMap = state->getInterfaces()->modify(&state);
  rifMap->addNode(swInterface, scope());
  applyNewState(state);
  checkRouterInterface(
      swInterface->getID(), static_cast<VlanSaiId>(intf0.id), intf0.routerMac);
}

TEST_F(RouterInterfaceManagerTest, addPortRouterInterface) {
  auto test = [this](bool isRemote) {
    auto origState = programmedState;
    auto swSysPort = makeSystemPort();
    auto swInterface = makeInterface(*swSysPort, {intf0.subnet});
    auto state = programmedState;
    auto rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                           : state->getInterfaces()->modify(&state);
    rifMap->addNode(swInterface, scope());
    applyNewState(state);
    checkPortRouterInterface(
        swInterface->getID(), sysPort1, swInterface->getMac());
    applyNewState(origState);
  };
  test(false /*local*/);
  test(true /*remote*/);
}

TEST_F(RouterInterfaceManagerTest, addTwoRouterInterfaces) {
  auto swInterface0 = makeInterface(intf0);
  auto state = programmedState;
  auto rifMap = state->getInterfaces()->modify(&state);
  rifMap->addNode(swInterface0, scope());
  applyNewState(state);
  checkRouterInterface(
      swInterface0->getID(), static_cast<VlanSaiId>(intf0.id), intf0.routerMac);
  auto swInterface1 = makeInterface(intf1);
  rifMap = state->getInterfaces()->modify(&state);
  rifMap->addNode(swInterface1, scope());
  applyNewState(state);
  checkRouterInterface(
      swInterface1->getID(), static_cast<VlanSaiId>(intf1.id), intf1.routerMac);
}

TEST_F(RouterInterfaceManagerTest, addTwoPortRouterInterfaces) {
  auto test = [this](bool isRemote) {
    auto origState = programmedState;
    auto swSysPort0 = makeSystemPort();
    auto swInterface0 = makeInterface(*swSysPort0, {intf0.subnet});
    auto state = programmedState;
    auto rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                           : state->getInterfaces()->modify(&state);
    rifMap->addNode(swInterface0, scope());
    applyNewState(state);
    checkPortRouterInterface(
        swInterface0->getID(), sysPort1, swInterface0->getMac());
    auto swSysPort1 = makeSystemPort(std::nullopt, 2);
    auto swInterface1 = makeInterface(*swSysPort1, {intf0.subnet});
    rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                      : state->getInterfaces()->modify(&state);
    rifMap->addNode(swInterface1, scope());
    applyNewState(state);
    checkPortRouterInterface(
        swInterface1->getID(), sysPort2, swInterface1->getMac());
    applyNewState(origState);
  };
  test(false /*local*/);
  test(true /*remote*/);
}

TEST_F(RouterInterfaceManagerTest, addDupRouterInterface) {
  auto swInterface = makeInterface(intf0);
  saiManagerTable->routerInterfaceManager().addLocalRouterInterface(
      swInterface);
  EXPECT_THROW(
      saiManagerTable->routerInterfaceManager().addLocalRouterInterface(
          swInterface),
      FbossError);
}

TEST_F(RouterInterfaceManagerTest, addDupPortRouterInterface) {
  auto swSysPort = makeSystemPort();
  auto swInterface = makeInterface(*swSysPort, {intf0.subnet});
  std::ignore =
      saiManagerTable->routerInterfaceManager().addLocalRouterInterface(
          swInterface);
  EXPECT_THROW(
      saiManagerTable->routerInterfaceManager().addLocalRouterInterface(
          swInterface),
      FbossError);
}

TEST_F(RouterInterfaceManagerTest, changeRouterInterfaceMac) {
  auto swInterface = makeInterface(intf0);
  auto state = programmedState;
  auto rifMap = state->getInterfaces()->modify(&state);
  rifMap->addNode(swInterface, scope());
  applyNewState(state);
  checkRouterInterface(
      swInterface->getID(), static_cast<VlanSaiId>(intf0.id), intf0.routerMac);
  auto newMac = intf1.routerMac;
  CHECK_NE(swInterface->getMac(), newMac);
  auto newInterface = swInterface->clone();
  newInterface->setMac(newMac);
  rifMap = state->getInterfaces()->modify(&state);
  rifMap->updateNode(newInterface, scope());
  applyNewState(state);
  EXPECT_EQ(swInterface->getID(), newInterface->getID());
  checkRouterInterface(
      newInterface->getID(), static_cast<VlanSaiId>(intf0.id), newMac);
}

TEST_F(RouterInterfaceManagerTest, changePortRouterInterfaceMac) {
  auto test = [this](bool isRemote) {
    auto origState = programmedState;
    auto swSysPort = makeSystemPort();
    auto swInterface = makeInterface(*swSysPort, {intf0.subnet});
    auto state = programmedState;
    auto rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                           : state->getInterfaces()->modify(&state);
    rifMap->addNode(swInterface, scope());
    applyNewState(state);
    checkPortRouterInterface(
        swInterface->getID(), sysPort1, swInterface->getMac());
    auto newMac = intf0.routerMac;
    CHECK_NE(swInterface->getMac(), newMac);
    auto newInterface = swInterface->clone();
    newInterface->setMac(newMac);
    rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                      : state->getInterfaces()->modify(&state);
    rifMap->updateNode(newInterface, scope());
    applyNewState(state);
    EXPECT_EQ(swInterface->getID(), newInterface->getID());
    checkPortRouterInterface(newInterface->getID(), sysPort1, newMac);
    applyNewState(origState);
  };
  test(false /*local*/);
  test(true /*remote*/);
}

TEST_F(RouterInterfaceManagerTest, changeRouterInterfaceMtu) {
  auto swInterface = makeInterface(intf0);
  auto state = programmedState;
  auto rifMap = state->getInterfaces()->modify(&state);
  rifMap->addNode(swInterface, scope());
  applyNewState(state);
  checkRouterInterface(
      swInterface->getID(), static_cast<VlanSaiId>(intf0.id), intf0.routerMac);
  auto newMtu = intf0.mtu + 1000;
  auto newInterface = swInterface->clone();
  newInterface->setMtu(newMtu);
  rifMap = state->getInterfaces()->modify(&state);
  rifMap->updateNode(newInterface, scope());
  applyNewState(state);
  EXPECT_EQ(swInterface->getID(), newInterface->getID());
  checkRouterInterface(
      newInterface->getID(),
      static_cast<VlanSaiId>(intf0.id),
      newInterface->getMac(),
      newMtu);
}

TEST_F(RouterInterfaceManagerTest, changePortRouterInterfaceMtu) {
  auto test = [this](bool isRemote) {
    auto origState = programmedState;
    auto swSysPort = makeSystemPort();
    auto swInterface = makeInterface(*swSysPort, {intf0.subnet});
    auto state = programmedState;
    auto rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                           : state->getInterfaces()->modify(&state);
    rifMap->addNode(swInterface, scope());
    applyNewState(state);
    checkPortRouterInterface(
        swInterface->getID(), sysPort1, swInterface->getMac());
    auto newMtu = intf0.mtu + 1000;
    auto newInterface = swInterface->clone();
    newInterface->setMtu(newMtu);
    rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                      : state->getInterfaces()->modify(&state);
    rifMap->updateNode(newInterface, scope());
    applyNewState(state);
    EXPECT_EQ(swInterface->getID(), newInterface->getID());
    checkPortRouterInterface(
        newInterface->getID(), sysPort1, newInterface->getMac(), newMtu);
    applyNewState(origState);
  };
  test(false /*local*/);
  test(true /*remote*/);
}

TEST_F(RouterInterfaceManagerTest, removeRouterInterfaceSubnets) {
  auto swInterface = makeInterface(intf0);
  auto state = programmedState;
  auto rifMap = state->getInterfaces()->modify(&state);
  rifMap->addNode(swInterface, scope());
  applyNewState(state);
  checkRouterInterface(
      swInterface->getID(), static_cast<VlanSaiId>(intf0.id), intf0.routerMac);
  auto oldToMeRoutes = getSubnetKeys(swInterface->getID());
  auto newInterface = swInterface->clone();
  newInterface->setAddresses({});
  rifMap = state->getInterfaces()->modify(&state);
  rifMap->updateNode(newInterface, scope());
  applyNewState(state);
  checkSubnets(oldToMeRoutes, false /* should no longer exist*/);
}

TEST_F(RouterInterfaceManagerTest, removePortRouterInterfaceSubnets) {
  auto test = [this](bool isRemote) {
    auto origState = programmedState;
    auto swSysPort = makeSystemPort();
    auto swInterface = makeInterface(*swSysPort, {intf0.subnet});
    auto state = programmedState;
    auto rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                           : state->getInterfaces()->modify(&state);
    rifMap->addNode(swInterface, scope());
    applyNewState(state);
    checkPortRouterInterface(
        swInterface->getID(), sysPort1, swInterface->getMac());
    auto oldToMeRoutes = getSubnetKeys(swInterface->getID());
    auto newInterface = swInterface->clone();
    newInterface->setAddresses({});
    rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                      : state->getInterfaces()->modify(&state);
    rifMap->updateNode(newInterface, scope());
    applyNewState(state);
    checkSubnets(oldToMeRoutes, false /* should no longer exist*/);
    applyNewState(origState);
  };
  test(false /*local*/);
  test(true /*remote*/);
}

TEST_F(RouterInterfaceManagerTest, changeRouterInterfaceSubnets) {
  auto swInterface = makeInterface(intf0);
  auto state = programmedState;
  auto rifMap = state->getInterfaces()->modify(&state);
  rifMap->addNode(swInterface, scope());
  applyNewState(state);
  checkRouterInterface(
      swInterface->getID(), static_cast<VlanSaiId>(intf0.id), intf0.routerMac);
  auto oldToMeRoutes = getSubnetKeys(swInterface->getID());
  auto newInterface = swInterface->clone();
  newInterface->setAddresses({{folly::IPAddress{"100.100.100.1"}, 24}});
  rifMap = state->getInterfaces()->modify(&state);
  rifMap->updateNode(newInterface, scope());
  applyNewState(state);
  auto newToMeRoutes = getSubnetKeys(swInterface->getID());
  checkSubnets(oldToMeRoutes, false /* should no longer exist*/);
  checkSubnets(newToMeRoutes, true /* should  exist*/);
}

TEST_F(RouterInterfaceManagerTest, changePortRouterInterfaceSubnets) {
  auto test = [this](bool isRemote) {
    auto origState = programmedState;
    auto swSysPort = makeSystemPort();
    auto swInterface = makeInterface(*swSysPort, {intf0.subnet});
    auto state = programmedState;
    auto rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                           : state->getInterfaces()->modify(&state);
    rifMap->addNode(swInterface, scope());
    applyNewState(state);
    checkPortRouterInterface(
        swInterface->getID(), sysPort1, swInterface->getMac());
    auto oldToMeRoutes = getSubnetKeys(swInterface->getID());
    auto newInterface = swInterface->clone();
    newInterface->setAddresses({{folly::IPAddress{"100.100.100.1"}, 24}});
    rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                      : state->getInterfaces()->modify(&state);
    rifMap->updateNode(newInterface, scope());
    applyNewState(state);
    auto newToMeRoutes = getSubnetKeys(swInterface->getID());
    checkSubnets(oldToMeRoutes, false /* should no longer exist*/);
    checkSubnets(newToMeRoutes, true /* should  exist*/);
    applyNewState(origState);
  };
  test(false /*local*/);
  test(true /*remote*/);
}

TEST_F(RouterInterfaceManagerTest, addRouterInterfaceSubnets) {
  auto swInterface = makeInterface(intf0);
  auto state = programmedState;
  auto rifMap = state->getInterfaces()->modify(&state);
  rifMap->addNode(swInterface, scope());
  applyNewState(state);
  checkRouterInterface(
      swInterface->getID(), static_cast<VlanSaiId>(intf0.id), intf0.routerMac);
  auto oldToMeRoutes = getSubnetKeys(swInterface->getID());
  auto newInterface = swInterface->clone();
  auto addresses = newInterface->getAddressesCopy();
  addresses.emplace(folly::IPAddress{"100.100.100.1"}, 24);
  EXPECT_EQ(swInterface->getAddresses()->size() + 1, addresses.size());
  rifMap = state->getInterfaces()->modify(&state);
  rifMap->updateNode(newInterface, scope());
  applyNewState(state);
  auto newToMeRoutes = getSubnetKeys(swInterface->getID());
  checkSubnets(oldToMeRoutes, true /* should exist*/);
  checkSubnets(newToMeRoutes, true /* should exist*/);
}

TEST_F(RouterInterfaceManagerTest, addPortRouterInterfaceSubnets) {
  auto test = [this](bool isRemote) {
    auto origState = programmedState;
    auto swSysPort = makeSystemPort();
    auto swInterface = makeInterface(*swSysPort, {intf0.subnet});
    auto state = programmedState;
    auto rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                           : state->getInterfaces()->modify(&state);
    rifMap->addNode(swInterface, scope());
    applyNewState(state);
    checkPortRouterInterface(
        swInterface->getID(), sysPort1, swInterface->getMac());
    auto oldToMeRoutes = getSubnetKeys(swInterface->getID());
    auto newInterface = swInterface->clone();
    auto addresses = newInterface->getAddressesCopy();
    addresses.emplace(folly::IPAddress{"100.100.100.1"}, 24);
    EXPECT_EQ(swInterface->getAddresses()->size() + 1, addresses.size());
    rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                      : state->getInterfaces()->modify(&state);
    rifMap->updateNode(newInterface, scope());
    applyNewState(state);
    auto newToMeRoutes = getSubnetKeys(swInterface->getID());
    checkSubnets(oldToMeRoutes, true /* should exist*/);
    checkSubnets(newToMeRoutes, true /* should exist*/);
    applyNewState(origState);
  };
  test(false /*local*/);
  test(true /*remote*/);
}

TEST_F(RouterInterfaceManagerTest, getRouterInterface) {
  auto oldInterface = makeInterface(intf0);
  auto saiId =
      saiManagerTable->routerInterfaceManager().addLocalRouterInterface(
          oldInterface);
  auto routerInterfaceHandle =
      saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
          InterfaceID(0));
  EXPECT_TRUE(routerInterfaceHandle);
  EXPECT_EQ(routerInterfaceHandle->adapterKey(), saiId);
}

TEST_F(RouterInterfaceManagerTest, getPortRouterInterface) {
  auto swSysPort = makeSystemPort();
  auto oldInterface = makeInterface(*swSysPort, {intf0.subnet});
  auto saiId =
      saiManagerTable->routerInterfaceManager().addLocalRouterInterface(
          oldInterface);
  checkPortRouterInterface(saiId, sysPort1, oldInterface->getMac());
  auto routerInterfaceHandle =
      saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
          InterfaceID(swSysPort->getID()));
  EXPECT_TRUE(routerInterfaceHandle);
  EXPECT_EQ(routerInterfaceHandle->adapterKey(), saiId);
}

TEST_F(RouterInterfaceManagerTest, getNonexistentRouterInterface) {
  auto swInterface = makeInterface(intf0);
  saiManagerTable->routerInterfaceManager().addLocalRouterInterface(
      swInterface);
  auto routerInterfaceHandle =
      saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
          InterfaceID(2));
  EXPECT_FALSE(routerInterfaceHandle);
}

TEST_F(RouterInterfaceManagerTest, removeRouterInterface) {
  auto swInterface = makeInterface(intf0);
  auto state = programmedState;
  auto rifMap = state->getInterfaces()->modify(&state);
  rifMap->addNode(swInterface, scope());
  applyNewState(state);
  checkRouterInterface(
      swInterface->getID(), static_cast<VlanSaiId>(intf0.id), intf0.routerMac);
  rifMap = state->getInterfaces()->modify(&state);
  rifMap->removeNode(swInterface->getID());
  applyNewState(state);
  auto routerInterfaceHandle =
      saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
          swInterface->getID());
  EXPECT_FALSE(routerInterfaceHandle);
}

TEST_F(RouterInterfaceManagerTest, removePortRouterInterface) {
  auto test = [this](bool isRemote) {
    auto origState = programmedState;
    auto swSysPort = makeSystemPort();
    auto swInterface = makeInterface(*swSysPort, {intf0.subnet});
    auto state = programmedState;
    auto rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                           : state->getInterfaces()->modify(&state);
    rifMap->addNode(swInterface, scope());
    applyNewState(state);
    checkPortRouterInterface(
        swInterface->getID(), sysPort1, swInterface->getMac());
    rifMap = isRemote ? state->getRemoteInterfaces()->modify(&state)
                      : state->getInterfaces()->modify(&state);
    rifMap->removeNode(swInterface->getID());
    applyNewState(state);
    auto routerInterfaceHandle =
        saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
            InterfaceID(swSysPort->getID()));
    EXPECT_FALSE(routerInterfaceHandle);
    applyNewState(origState);
  };
  test(false /*local*/);
  test(true /*remote*/);
}

TEST_F(RouterInterfaceManagerTest, removeNonexistentRouterInterface) {
  auto swInterface = makeInterface(intf1);
  saiManagerTable->routerInterfaceManager().addLocalRouterInterface(
      swInterface);
  auto& routerInterfaceManager = saiManagerTable->routerInterfaceManager();
  auto swInterface2 = makeInterface(testInterfaces[2]);
  EXPECT_THROW(
      routerInterfaceManager.removeLocalRouterInterface(swInterface2),
      FbossError);
}

TEST_F(RouterInterfaceManagerTest, adapterKeyAndTypeRouterInterface) {
  auto swInterface = makeInterface(intf1);
  auto saiId =
      saiManagerTable->routerInterfaceManager().addLocalRouterInterface(
          swInterface);
  checkAdapterKey(saiId, swInterface->getID());
  checkType(saiId, swInterface->getID(), cfg::InterfaceType::VLAN);
}

TEST_F(RouterInterfaceManagerTest, adapterKeyAndTypePortRouterInterface) {
  auto swSysPort = makeSystemPort();
  auto swInterface = makeInterface(*swSysPort, {intf0.subnet});
  auto saiId =
      saiManagerTable->routerInterfaceManager().addLocalRouterInterface(
          swInterface);
  checkAdapterKey(saiId, swInterface->getID());
  checkType(saiId, swInterface->getID(), cfg::InterfaceType::SYSTEM_PORT);
}
