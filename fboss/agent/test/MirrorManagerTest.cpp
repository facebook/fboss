#include "fboss/agent/MirrorManager.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

#include "fboss/agent/GtestDefs.h"

using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;

namespace facebook::fboss {

namespace {
constexpr auto kMirrorName = "mirror";
constexpr AdminDistance DISTANCE = AdminDistance::STATIC_ROUTE;
const PortID kMirrorEgressPort{5};

template <typename AddrT>
struct MirrorManagerTestParams {
  AddrT mirrorDestination;
  AddrT mirrorSource;
  std::array<AddrT, 2> neighborIPs;
  std::array<MacAddress, 2> neighborMACs;
  std::array<PortID, 2> neighborPorts;
  std::array<InterfaceID, 2> interfaces;
  RoutePrefix<AddrT> longerPrefix;
  RoutePrefix<AddrT> shorterPrefix;

  MirrorManagerTestParams(
      const AddrT& mirrorDestination,
      const AddrT& mirrorSource,
      std::array<AddrT, 2>&& neighborIPs,
      std::array<MacAddress, 2>&& neighborMACs,
      std::array<PortID, 2>&& neighborPorts,
      std::array<InterfaceID, 2>&& interfaces,
      const RoutePrefix<AddrT>& longerPrefix,
      const RoutePrefix<AddrT>& shorterPrefix)
      : mirrorDestination(mirrorDestination),
        mirrorSource(mirrorSource),
        neighborIPs(std::move(neighborIPs)),
        neighborMACs(std::move(neighborMACs)),
        neighborPorts(std::move(neighborPorts)),
        interfaces(std::move(interfaces)),
        longerPrefix(longerPrefix),
        shorterPrefix(shorterPrefix) {}

  const UnresolvedNextHop nextHop(int neighborIndex) const {
    return UnresolvedNextHop(neighborIPs[neighborIndex % 2], NextHopWeight(80));
  }
  static MirrorManagerTestParams<AddrT> getParams();
};

using MirrorManagerV4TestParams = MirrorManagerTestParams<folly::IPAddressV4>;
using MirrorManagerV6TestParams = MirrorManagerTestParams<folly::IPAddressV6>;

template <>
MirrorManagerV4TestParams
MirrorManagerTestParams<folly::IPAddressV4>::getParams() {
  return MirrorManagerV4TestParams(
      IPAddressV4("10.0.10.101"),
      IPAddressV4("10.0.11.101"),
      {IPAddressV4("10.0.0.111"), IPAddressV4("10.0.55.111")},
      {MacAddress("10:0:0:0:1:11"), MacAddress("10:0:0:55:1:11")},
      {PortID(6), PortID(7)},
      {InterfaceID(1), InterfaceID(55)},
      {IPAddressV4("10.0.10.100"), 31},
      {IPAddressV4("10.0.0.0"), 16});
}

template <>
MirrorManagerV6TestParams
MirrorManagerTestParams<folly::IPAddressV6>::getParams() {
  return MirrorManagerV6TestParams(
      IPAddressV6("2401:db00:2110:10::1001"),
      IPAddressV6("2401:db00:2110:11::1001"),
      {IPAddressV6("2401:db00:2110:3001::0111"),
       IPAddressV6("2401:db00:2110:3055::0111")},
      {MacAddress("10:0:0:0:1:11"), MacAddress("10:0:0:55:1:11")},
      {PortID(6), PortID(7)},
      {InterfaceID(1), InterfaceID(55)},
      {IPAddressV6("2401:db00:2110:10::1000"), 127},
      {IPAddressV6("2401:db00:2110:10::0000"), 64});
}
} // namespace

template <typename AddrT>
class MirrorManagerTest : public ::testing::Test {
 public:
  using Func = folly::Function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;

  void SetUp() override {
    handle_ = createTestHandle(testStateA());
    sw_ = handle_->getSw();
  }

  void verifyStateUpdate(Func func) {
    runInUpdateEventBaseAndWait(std::move(func));
  }

  void TearDown() override {
    schedulePendingTestStateUpdates();
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, func);
  }

  std::shared_ptr<SwitchState> addNeighbor(
      const std::shared_ptr<SwitchState>& state,
      InterfaceID interfaceId,
      const AddrT& ip,
      const MacAddress mac,
      PortID port) {
    auto newState = state->isPublished() ? state->clone() : state;
    VlanID vlanId =
        newState->getInterfaces()->getInterface(interfaceId)->getVlanID();
    Vlan* vlan = newState->getVlans()->getVlanIf(VlanID(vlanId)).get();
    auto* neighborTable =
        vlan->template getNeighborEntryTable<AddrT>().get()->modify(
            &vlan, &newState);
    neighborTable->addEntry(ip, mac, PortDescriptor(port), interfaceId);
    return newState;
  }

  std::shared_ptr<SwitchState> delNeighbor(
      const std::shared_ptr<SwitchState>& state,
      InterfaceID interfaceId,
      const AddrT& ip) {
    auto newState = state->isPublished() ? state->clone() : state;
    VlanID vlanId =
        newState->getInterfaces()->getInterface(interfaceId)->getVlanID();
    Vlan* vlan = newState->getVlans()->getVlanIf(VlanID(vlanId)).get();
    auto* neighborTable =
        vlan->template getNeighborEntryTable<AddrT>().get()->modify(
            &vlan, &newState);
    neighborTable->removeEntry(ip);
    return newState;
  }

  std::shared_ptr<SwitchState> addRoute(
      const std::shared_ptr<SwitchState>& state,
      const RoutePrefix<AddrT>& prefix,
      RouteNextHopSet nexthops) {
    auto routeTables = state->getRouteTables();
    RouteUpdater updater(routeTables);
    updater.addRoute(
        RouterID(0),
        prefix.network,
        prefix.mask,
        ClientID(1000),
        RouteNextHopEntry(nexthops, DISTANCE));
    auto newRouteTables = updater.updateDone();
    EXPECT_NE(newRouteTables, nullptr);
    newRouteTables->publish();
    auto newState = state->isPublished() ? state->clone() : state;
    newState->resetRouteTables(newRouteTables);
    return newState;
  }

  std::shared_ptr<SwitchState> delRoute(
      const std::shared_ptr<SwitchState>& state,
      const RoutePrefix<AddrT>& prefix) {
    auto routeTables = state->getRouteTables();
    RouteUpdater updater(routeTables);
    updater.delRoute(RouterID(0), prefix.network, prefix.mask, ClientID(1000));
    auto newRouteTables = updater.updateDone();
    EXPECT_NE(newRouteTables, nullptr);
    newRouteTables->publish();
    auto newState = state->isPublished() ? state->clone() : state;
    newState->resetRouteTables(newRouteTables);
    return newState;
  }

  std::shared_ptr<SwitchState> addSpanMirror(
      const std::shared_ptr<SwitchState>& state,
      const std::string& name,
      PortID egressPort) {
    auto mirror = std::make_shared<Mirror>(
        name, std::optional<PortID>(egressPort), std::optional<IPAddress>());
    auto newState = state->isPublished() ? state->clone() : state;
    auto mirrors = newState->getMirrors()->modify(&newState);
    mirrors->addMirror(mirror);
    return newState;
  }

  std::shared_ptr<SwitchState> addErspanMirror(
      const std::shared_ptr<SwitchState>& state,
      const std::string& name,
      AddrT remoteIp) {
    auto mirror = std::make_shared<Mirror>(
        name, std::optional<PortID>(), std::optional<IPAddress>(remoteIp));

    auto newState = state->isPublished() ? state->clone() : state;
    auto mirrors = newState->getMirrors()->modify(&newState);
    mirrors->addMirror(mirror);
    return newState;
  }

  std::shared_ptr<SwitchState> addErspanMirror(
      const std::shared_ptr<SwitchState>& state,
      const std::string& name,
      AddrT remoteIp,
      PortID egressPort) {
    auto mirror = std::make_shared<Mirror>(
        name,
        std::optional<PortID>(PortID(egressPort)),
        std::optional<IPAddress>(remoteIp));

    auto newState = state->isPublished() ? state->clone() : state;
    auto mirrors = newState->getMirrors()->modify(&newState);
    mirrors->addMirror(mirror);
    return newState;
  }

  std::shared_ptr<SwitchState> addSflowMirror(
      const std::shared_ptr<SwitchState>& state,
      const std::string& name,
      AddrT remoteIp,
      uint16_t srcPort,
      uint16_t dstPort) {
    auto mirror = std::make_shared<Mirror>(
        name,
        std::optional<PortID>(),
        std::optional<IPAddress>(remoteIp),
        std::optional<IPAddress>(),
        std::make_optional<TunnelUdpPorts>(srcPort, dstPort));
    auto newState = state->isPublished() ? state->clone() : state;
    auto mirrors = newState->getMirrors()->modify(&newState);
    mirrors->addMirror(mirror);
    return newState;
  }

 protected:
  void runInUpdateEventBaseAndWait(Func func) {
    auto* evb = sw_->getUpdateEvb();
    evb->runInEventBaseThreadAndWait(std::move(func));
  }

  void schedulePendingTestStateUpdates() {
    runInUpdateEventBaseAndWait([]() {});
  }

  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_SUITE(MirrorManagerTest, TestTypes);

TYPED_TEST(MirrorManagerTest, CanNotUpdateMirrors) {
  auto params = MirrorManagerTestParams<TypeParam>::getParams();

  this->updateState(
      "CanNotUpdateMirrors", [=](const std::shared_ptr<SwitchState>& state) {
        return this->addErspanMirror(
            state, kMirrorName, params.mirrorDestination);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_FALSE(mirror->isResolved());
  });
}

TYPED_TEST(MirrorManagerTest, ResolveNoMirrorWithoutRoutes) {
  auto params = MirrorManagerTestParams<TypeParam>::getParams();
  this->updateState(
      "ResolveNoMirrorWithoutRoutes",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState =
            this->addErspanMirror(state, kMirrorName, params.mirrorDestination);
        return this->addNeighbor(
            updatedState,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_FALSE(mirror->isResolved());
  });
}

TYPED_TEST(MirrorManagerTest, ResolveNoMirrorWithoutArpEntry) {
  auto params = MirrorManagerTestParams<TypeParam>::getParams();
  this->updateState(
      "ResolveNoMirrorWithoutArpEntry",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState =
            this->addErspanMirror(state, kMirrorName, params.mirrorDestination);
        RouteNextHopSet nextHops = {params.nextHop(0), params.nextHop(1)};
        return this->addRoute(updatedState, params.longerPrefix, nextHops);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_FALSE(mirror->isResolved());
  });
}

TYPED_TEST(MirrorManagerTest, LocalMirrorAlreadyResolved) {
  this->updateState(
      "LocalMirrorAlreadyResolved",
      [=](const std::shared_ptr<SwitchState>& state) {
        return this->addSpanMirror(state, kMirrorName, kMirrorEgressPort);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
  });
}

TYPED_TEST(MirrorManagerTest, ResolveMirrorWithoutEgressPort) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();

  this->updateState(
      "ResolveMirrorWithoutEgressPort",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState =
            this->addErspanMirror(state, kMirrorName, params.mirrorDestination);
        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);

        RouteNextHopSet nextHops = {params.nextHop(0)};
        return this->addRoute(updatedState, params.longerPrefix, nextHops);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    ASSERT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort().value();
    EXPECT_EQ(egressPort, params.neighborPorts[0]);
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[0]);
    const auto& interface =
        state->getInterfaces()->getInterfaceIf(params.interfaces[0]);
    const auto& iter = interface->getAddressToReach(params.neighborIPs[0]);
    ASSERT_TRUE(iter != interface->getAddresses().end());
    EXPECT_EQ(tunnel.srcIp, iter->first);
    EXPECT_EQ(tunnel.srcMac, interface->getMac());
  });
}

TYPED_TEST(MirrorManagerTest, ResolveMirrorWithEgressPort) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();

  this->updateState(
      "ResolveMirrorWithEgressPort",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->addErspanMirror(
            state,
            kMirrorName,
            params.mirrorDestination,
            params.neighborPorts[1]);
        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[1],
            params.neighborIPs[1],
            params.neighborMACs[1],
            params.neighborPorts[1]);

        RouteNextHopSet nextHops = {params.nextHop(0), params.nextHop(1)};
        return this->addRoute(updatedState, params.longerPrefix, nextHops);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    ASSERT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort();
    EXPECT_EQ(egressPort.value(), params.neighborPorts[1]);
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[1]);

    const auto& interface =
        state->getInterfaces()->getInterfaceIf(params.interfaces[1]);
    const auto& iter = interface->getAddressToReach(params.neighborIPs[1]);

    ASSERT_TRUE(iter != interface->getAddresses().end());

    EXPECT_EQ(tunnel.srcIp, iter->first);
    EXPECT_EQ(tunnel.srcMac, MacAddress(interface->getMac()));
  });
}

TYPED_TEST(MirrorManagerTest, ResolveNoMirrorWithEgressPort) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();

  this->updateState(
      "ResolveNoMirrorWithEgressPort",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->addErspanMirror(
            state, kMirrorName, params.mirrorDestination, PortID(9));
        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
        updatedState = this->addNeighbor(
            updatedState,
            InterfaceID(55),
            params.neighborIPs[1],
            params.neighborMACs[1],
            params.neighborPorts[1]);

        RouteNextHopSet nextHops = {params.nextHop(0), params.nextHop(1)};
        return this->addRoute(updatedState, params.longerPrefix, nextHops);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_FALSE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    EXPECT_EQ(mirror->getEgressPort().value(), PortID(9));
    EXPECT_FALSE(mirror->getMirrorTunnel().has_value());
  });
}

TYPED_TEST(MirrorManagerTest, ResolveMirrorWithDirectlyConnectedRoute) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();

  this->updateState(
      "ResolveMirrorWithDirectlyConnectedRoute",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState =
            this->addErspanMirror(state, kMirrorName, params.neighborIPs[0]);
        return this->addNeighbor(
            updatedState,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
      });
  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort().value();
    EXPECT_EQ(egressPort, params.neighborPorts[0]);
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.neighborIPs[0]));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[0]);

    const auto& interface =
        state->getInterfaces()->getInterfaceIf(params.interfaces[0]);
    const auto& iter = interface->getAddressToReach(params.neighborIPs[0]);

    ASSERT_TRUE(iter != interface->getAddresses().end());

    EXPECT_EQ(tunnel.srcIp, iter->first);
    EXPECT_EQ(tunnel.srcMac, interface->getMac());
  });
}

TYPED_TEST(MirrorManagerTest, ResolveNoMirrorWithDirectlyConnectedRoute) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();
  this->updateState(
      "ResolveNoMirrorWithDirectlyConnectedRoute",
      [=](const std::shared_ptr<SwitchState>& state) {
        return this->addErspanMirror(state, kMirrorName, params.neighborIPs[0]);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    EXPECT_NE(state, nullptr);
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_FALSE(mirror->isResolved());
    EXPECT_FALSE(mirror->getEgressPort().has_value());
    ASSERT_FALSE(mirror->getMirrorTunnel().has_value());
  });
}

TYPED_TEST(MirrorManagerTest, UpdateMirrorOnRouteDelete) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();
  this->updateState(
      "UpdateMirrorOnRouteDelete: addMirror",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState =
            this->addErspanMirror(state, kMirrorName, params.mirrorDestination);
        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[1],
            params.neighborIPs[1],
            params.neighborMACs[1],
            params.neighborPorts[1]);

        RouteNextHopSet nextHopsLonger = {params.nextHop(0)};
        updatedState =
            this->addRoute(updatedState, params.longerPrefix, nextHopsLonger);
        RouteNextHopSet nextHopsShorter = {params.nextHop(1)};
        return this->addRoute(
            updatedState, params.shorterPrefix, nextHopsShorter);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort().value();
    EXPECT_EQ(egressPort, params.neighborPorts[0]);
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[0]);

    const auto& interface0 =
        state->getInterfaces()->getInterfaceIf(params.interfaces[0]);
    const auto& iter0 = interface0->getAddressToReach(params.neighborIPs[0]);

    ASSERT_TRUE(iter0 != interface0->getAddresses().end());

    EXPECT_EQ(tunnel.srcIp, iter0->first);
    EXPECT_EQ(tunnel.srcMac, interface0->getMac());
  });

  this->updateState(
      "UpdateMirrorOnRouteDelete: delNeighbor",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->delNeighbor(
            state, params.interfaces[0], params.neighborIPs[0]);
        return this->delRoute(updatedState, params.longerPrefix);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort().value();
    EXPECT_EQ(egressPort, params.neighborPorts[1]);
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[1]);

    const auto& interface1 =
        state->getInterfaces()->getInterfaceIf(params.interfaces[1]);
    const auto& iter1 = interface1->getAddressToReach(params.neighborIPs[1]);

    ASSERT_TRUE(iter1 != interface1->getAddresses().end());

    EXPECT_EQ(tunnel.srcIp, iter1->first);
    EXPECT_EQ(tunnel.srcMac, interface1->getMac());
  });
}

TYPED_TEST(MirrorManagerTest, UpdateMirrorOnRouteAdd) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();
  this->updateState(
      "UpdateMirrorOnRouteAdd: addMirror",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState =
            this->addErspanMirror(state, kMirrorName, params.mirrorDestination);
        updatedState = this->addNeighbor(
            updatedState,
            InterfaceID(55),
            params.neighborIPs[1],
            params.neighborMACs[1],
            params.neighborPorts[1]);
        RouteNextHopSet nextHopsShorter = {params.nextHop(1)};
        return this->addRoute(
            updatedState, params.shorterPrefix, nextHopsShorter);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort().value();
    EXPECT_EQ(egressPort, params.neighborPorts[1]);
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[1]);

    const auto& interface1 =
        state->getInterfaces()->getInterfaceIf(params.interfaces[1]);
    const auto& iter1 = interface1->getAddressToReach(params.neighborIPs[1]);

    ASSERT_TRUE(iter1 != interface1->getAddresses().end());

    EXPECT_EQ(tunnel.srcIp, iter1->first);
    EXPECT_EQ(tunnel.srcMac, interface1->getMac());
  });

  this->updateState(
      "UpdateMirrorOnRouteAdd: addRoute",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->addNeighbor(
            state,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
        RouteNextHopSet nextHopsLonger = {params.nextHop(0)};
        return this->addRoute(
            updatedState, params.longerPrefix, nextHopsLonger);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort().value();
    EXPECT_EQ(egressPort, params.neighborPorts[0]);
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[0]);

    const auto& interface0 =
        state->getInterfaces()->getInterfaceIf(params.interfaces[0]);
    const auto& iter0 = interface0->getAddressToReach(params.neighborIPs[0]);

    ASSERT_TRUE(iter0 != interface0->getAddresses().end());

    EXPECT_EQ(tunnel.srcIp, iter0->first);
    EXPECT_EQ(tunnel.srcMac, interface0->getMac());
  });
}

TYPED_TEST(MirrorManagerTest, UpdateNoMirrorWithEgressPortOnRouteDel) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();

  this->updateState(
      "UpdateNoMirrorWithEgressPortOnRouteDel",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->addErspanMirror(
            state,
            kMirrorName,
            params.mirrorDestination,
            params.neighborPorts[0]);
        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
        updatedState = this->addNeighbor(
            updatedState,
            InterfaceID(55),
            params.neighborIPs[1],
            params.neighborMACs[1],
            params.neighborPorts[1]);

        RouteNextHopSet nextHopsLonger = {params.nextHop(0), params.nextHop(1)};
        return this->addRoute(
            updatedState, params.longerPrefix, nextHopsLonger);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort().value();
    EXPECT_EQ(egressPort, params.neighborPorts[0]);
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[0]);

    const auto& interface =
        state->getInterfaces()->getInterfaceIf(params.interfaces[0]);
    const auto& iter = interface->getAddressToReach(params.neighborIPs[0]);

    ASSERT_TRUE(iter != interface->getAddresses().end());

    EXPECT_EQ(tunnel.srcIp, iter->first);
    EXPECT_EQ(tunnel.srcMac, interface->getMac());
  });

  this->updateState(
      "UpdateNoMirrorWithEgressPortOnRouteDel: delNeighbor",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->delNeighbor(
            state, params.interfaces[0], params.neighborIPs[0]);
        return this->delRoute(updatedState, params.longerPrefix);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_FALSE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    EXPECT_EQ(mirror->getEgressPort().value(), params.neighborPorts[0]);
    EXPECT_FALSE(mirror->getMirrorTunnel().has_value());
  });
}

TYPED_TEST(MirrorManagerTest, UpdateNoMirrorWithEgressPortOnRouteAdd) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();

  this->updateState(
      "UpdateNoMirrorWithEgressPortOnRouteAdd",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->addErspanMirror(
            state,
            kMirrorName,
            params.mirrorDestination,
            params.neighborPorts[0]);

        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[1]);
        updatedState = this->addNeighbor(
            updatedState,
            InterfaceID(55),
            params.neighborIPs[1],
            params.neighborMACs[1],
            params.neighborPorts[1]);

        RouteNextHopSet nextHopsLonger = {params.nextHop(0), params.nextHop(1)};
        return this->addRoute(
            updatedState, params.longerPrefix, nextHopsLonger);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_FALSE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort().value();
    EXPECT_EQ(egressPort, params.neighborPorts[0]);
    EXPECT_FALSE(mirror->getMirrorTunnel().has_value());
  });
}

TYPED_TEST(MirrorManagerTest, UpdateMirrorOnNeighborChange) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();
  this->updateState(
      "UpdateMirrorOnNeighborChange: addMirror",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState =
            this->addErspanMirror(state, kMirrorName, params.mirrorDestination);
        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[1],
            params.neighborIPs[1],
            params.neighborMACs[1],
            params.neighborPorts[1]);

        RouteNextHopSet nextHopsLonger = {params.nextHop(0)};
        updatedState =
            this->addRoute(updatedState, params.longerPrefix, nextHopsLonger);
        RouteNextHopSet nextHopsShorter = {params.nextHop(1)};
        return this->addRoute(
            updatedState, params.shorterPrefix, nextHopsShorter);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort().value();
    EXPECT_EQ(egressPort, params.neighborPorts[0]);
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[0]);

    const auto& interface0 =
        state->getInterfaces()->getInterfaceIf(params.interfaces[0]);
    const auto& iter0 = interface0->getAddressToReach(params.neighborIPs[0]);

    ASSERT_TRUE(iter0 != interface0->getAddresses().end());

    EXPECT_EQ(tunnel.srcIp, iter0->first);
    EXPECT_EQ(tunnel.srcMac, interface0->getMac());
  });

  this->updateState(
      "UpdateMirrorOnNeighborChange: delNeighbor",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->delNeighbor(
            state, params.interfaces[0], params.neighborIPs[0]);
        updatedState = this->delNeighbor(
            updatedState, params.interfaces[1], params.neighborIPs[1]);

        updatedState = this->addNeighbor(
            updatedState,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[1]);
        return this->addNeighbor(
            updatedState,
            params.interfaces[1],
            params.neighborIPs[1],
            params.neighborMACs[1],
            params.neighborPorts[0]);
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    EXPECT_TRUE(mirror->getEgressPort().has_value());
    auto egressPort = mirror->getEgressPort().value();
    EXPECT_EQ(egressPort, params.neighborPorts[1]);
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[0]);

    const auto& interface1 =
        state->getInterfaces()->getInterfaceIf(params.interfaces[0]);
    const auto& iter1 = interface1->getAddressToReach(params.neighborIPs[0]);

    ASSERT_TRUE(iter1 != interface1->getAddresses().end());

    EXPECT_EQ(tunnel.srcIp, iter1->first);
    EXPECT_EQ(tunnel.srcMac, interface1->getMac());
  });
}

TYPED_TEST(MirrorManagerTest, EmptyDelta) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();

  const auto oldState = this->sw_->getState();

  this->updateState(
      "EmptyDelta", [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->addNeighbor(
            state,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
        return this->addNeighbor(
            updatedState,
            params.interfaces[1],
            params.neighborIPs[1],
            params.neighborMACs[1],
            params.neighborPorts[1]);
      });

  this->verifyStateUpdate([=]() {
    const auto newState = this->sw_->getState();
    StateDelta delta(oldState, newState);
    EXPECT_FALSE(DeltaFunctions::isEmpty(delta.getVlansDelta()));
    EXPECT_TRUE(DeltaFunctions::isEmpty(delta.getRouteTablesDelta()));
  });
}

// test for gre src ip resolved
TYPED_TEST(MirrorManagerTest, GreMirrorWithSrcIp) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();

  this->updateState(
      "GreMirrorWithSrcIp", [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->addNeighbor(
            state,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
        RouteNextHopSet nextHops = {params.nextHop(0)};
        updatedState =
            this->addRoute(updatedState, params.longerPrefix, nextHops);
        updatedState = this->addErspanMirror(
            updatedState, kMirrorName, params.mirrorDestination);

        auto oldMirror = updatedState->getMirrors()->getMirrorIf(kMirrorName);
        auto mirror = std::make_shared<Mirror>(
            kMirrorName,
            oldMirror->getEgressPort(),
            oldMirror->getDestinationIp(),
            std::make_optional<folly::IPAddress>(params.mirrorSource));
        updatedState->getMirrors()->updateNode(mirror);
        return updatedState;
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[0]);
    const auto& interface =
        state->getInterfaces()->getInterfaceIf(params.interfaces[0]);
    EXPECT_EQ(tunnel.srcIp, folly::IPAddress(params.mirrorSource));
    EXPECT_EQ(tunnel.srcMac, interface->getMac());
    EXPECT_FALSE(tunnel.udpPorts.has_value());
  });
}

TYPED_TEST(MirrorManagerTest, SflowMirrorWithSrcIp) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();

  this->updateState(
      "GreMirrorWithSrcIp", [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->addNeighbor(
            state,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
        RouteNextHopSet nextHops = {params.nextHop(0)};
        updatedState =
            this->addRoute(updatedState, params.longerPrefix, nextHops);
        updatedState = this->addSflowMirror(
            updatedState, kMirrorName, params.mirrorDestination, 10101, 20202);

        auto oldMirror = updatedState->getMirrors()->getMirrorIf(kMirrorName);
        auto mirror = std::make_shared<Mirror>(
            kMirrorName,
            oldMirror->getEgressPort(),
            oldMirror->getDestinationIp(),
            std::make_optional<folly::IPAddress>(params.mirrorSource),
            oldMirror->getTunnelUdpPorts());
        updatedState->getMirrors()->updateNode(mirror);
        return updatedState;
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[0]);
    const auto& interface =
        state->getInterfaces()->getInterfaceIf(params.interfaces[0]);
    EXPECT_EQ(tunnel.srcIp, folly::IPAddress(params.mirrorSource));
    EXPECT_EQ(tunnel.srcMac, interface->getMac());
    EXPECT_TRUE(tunnel.udpPorts.has_value());
    EXPECT_EQ(tunnel.udpPorts->udpSrcPort, 10101);
    EXPECT_EQ(tunnel.udpPorts->udpDstPort, 20202);
  });
}

TYPED_TEST(MirrorManagerTest, ResolveMirrorOnMirrorUpdate) {
  const auto params = MirrorManagerTestParams<TypeParam>::getParams();

  this->updateState(
      "add sflowMirror", [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = state->clone();
        updatedState = this->addSflowMirror(
            updatedState, kMirrorName, params.mirrorDestination, 10101, 20202);
        return updatedState;
      });

  this->updateState(
      "resolve sflowMirror", [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = this->addNeighbor(
            state,
            params.interfaces[0],
            params.neighborIPs[0],
            params.neighborMACs[0],
            params.neighborPorts[0]);
        RouteNextHopSet nextHops = {params.nextHop(0)};
        updatedState =
            this->addRoute(updatedState, params.longerPrefix, nextHops);
        return updatedState;
      });

  this->updateState(
      "update sflow mirror config",
      [=](const std::shared_ptr<SwitchState>& state) {
        auto updatedState = state->clone();
        auto oldMirror = state->getMirrors()->getNode(kMirrorName);

        // unresolve and update with non-resolving affecting update,
        // mirror so mirror manager resolves it again.
        auto mirror = std::make_shared<Mirror>(
            oldMirror->getID(),
            std::optional<PortID>(),
            oldMirror->getDestinationIp(),
            std::optional<IPAddress>(),
            oldMirror->getTunnelUdpPorts());
        mirror->setTruncate(true);

        auto mirrors = updatedState->getMirrors()->clone();
        mirrors->updateNode(mirror);
        updatedState->resetMirrors(mirrors);
        return updatedState;
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto mirror = state->getMirrors()->getMirrorIf(kMirrorName);
    EXPECT_NE(mirror, nullptr);
    EXPECT_TRUE(mirror->isResolved());
    ASSERT_TRUE(mirror->getMirrorTunnel().has_value());
    auto tunnel = mirror->getMirrorTunnel().value();
    EXPECT_EQ(tunnel.dstIp, IPAddress(params.mirrorDestination));
    EXPECT_EQ(tunnel.dstMac, params.neighborMACs[0]);
    const auto& interface =
        state->getInterfaces()->getInterfaceIf(params.interfaces[0]);
    EXPECT_EQ(tunnel.srcMac, interface->getMac());
    EXPECT_TRUE(tunnel.udpPorts.has_value());
    EXPECT_EQ(tunnel.udpPorts->udpSrcPort, 10101);
    EXPECT_EQ(tunnel.udpPorts->udpDstPort, 20202);
    EXPECT_TRUE(mirror->getTruncate());
  });
}

} // namespace facebook::fboss
