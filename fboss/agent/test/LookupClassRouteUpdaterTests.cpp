/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/LookupClassRouteUpdater.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

using folly::IPAddressV4;
using folly::IPAddressV6;

namespace facebook::fboss {

template <typename AddressT>
class LookupClassRouteUpdaterTest : public ::testing::Test {
 public:
  using Func = std::function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;
  using AddrT = AddressT;
  static constexpr bool hasStandAloneRib = true;

  cfg::SwitchConfig getConfig() const {
    return testConfigAWithLookupClasses();
  }
  void SetUp() override {
    auto config = getConfig();
    handle_ = createTestHandle(&config, SwitchFlags::ENABLE_STANDALONE_RIB);
    sw_ = handle_->getSw();
  }

  void verifyStateUpdate(Func func) {
    runInUpdateEventBaseAndWait(std::move(func));
  }

  void verifyStateUpdateAfterNeighborCachePropagation(Func func) {
    // internally, calls through neighbor updater are proxied to the
    // NeighborCache thread, which can eventually end up scheduling a
    // new state update back on to the update evb. This helper waits
    // until the change has made it through all these proxy layers.
    runInUpdateEvbAndWaitAfterNeighborCachePropagation(std::move(func));
  }

  void TearDown() override {
    waitForRibUpdates(sw_);
    schedulePendingTestStateUpdates();
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, func);
  }

  VlanID kVlan() const {
    return VlanID(1);
  }

  PortID kPortID() const {
    return PortID(1);
  }

  ClientID kClientID() const {
    return ClientID(1001);
  }

  RouterID kRid() const {
    return RouterID{0};
  }

  InterfaceID kInterfaceID() const {
    return InterfaceID(1);
  }

  MacAddress kMacAddressA() const {
    return MacAddress("01:02:03:04:05:06");
  }

  MacAddress kMacAddressB() const {
    return MacAddress("01:02:03:04:05:07");
  }

  AddrT kIpAddressA() {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return IPAddressV4("10.0.0.2");
    } else {
      return IPAddressV6("2401:db00:2110:3001::0002");
    }
  }

  AddrT kIpAddressB() {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return IPAddressV4("10.0.0.3");
    } else {
      return IPAddressV6("2401:db00:2110:3001::0003");
    }
  }

  AddrT kIpAddressC() {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return IPAddressV4("10.0.0.4");
    } else {
      return IPAddressV6("2401:db00:2110:3001::0004");
    }
  }

  RoutePrefix<AddrT> kroutePrefix1() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return RoutePrefix<AddrT>{folly::IPAddressV4{"10.1.4.0"}, 24};
    } else {
      return RoutePrefix<AddrT>{
          folly::IPAddressV6{"2803:6080:d038:3066::"}, 64};
    }
  }

  RoutePrefix<AddrT> kroutePrefix2() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return RoutePrefix<AddrT>{folly::IPAddressV4{"11.1.4.0"}, 24};
    } else {
      return RoutePrefix<AddrT>{
          folly::IPAddressV6{"2803:6080:d038:3067::"}, 64};
    }
  }

  RoutePrefix<AddrT> kroutePrefix3() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return RoutePrefix<AddrT>{folly::IPAddressV4{"12.1.4.0"}, 24};
    } else {
      return RoutePrefix<AddrT>{
          folly::IPAddressV6{"3803:6080:d038:3067::"}, 64};
    }
  }
  void resolveNeighbor(AddrT ipAddress, MacAddress macAddress) {
    /*
     * Cause a neighbor entry to resolve by receiving appropriate ARP/NDP, and
     * assert if valid CLASSID is associated with the newly resolved neighbor.
     */
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      sw_->getNeighborUpdater()->receivedArpMine(
          kVlan(),
          ipAddress,
          macAddress,
          PortDescriptor(kPortID()),
          ArpOpCode::ARP_OP_REPLY);
    } else {
      sw_->getNeighborUpdater()->receivedNdpMine(
          kVlan(),
          ipAddress,
          macAddress,
          PortDescriptor(kPortID()),
          ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
          0);
    }

    // Wait for neighbor update to be done, this should
    // queue up state updates
    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForStateUpdates(sw_);
    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForStateUpdates(sw_);
    // Neighbor updates may in turn cause route class Id updates
    // wait for them
    waitForRibUpdates(sw_);
    waitForStateUpdates(sw_);
  }

  void unresolveNeighbor(folly::IPAddress ipAddress) {
    sw_->getNeighborUpdater()->flushEntry(kVlan(), ipAddress);
    // Wait for neighbor update to be done, this should
    // queue up state updates
    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForStateUpdates(sw_);
    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForStateUpdates(sw_);
    // Neighbor updates may in turn cause route class Id updates
    // wait for them
    waitForRibUpdates(sw_);
    waitForStateUpdates(sw_);
  }

  void verifyClassIDHelper(
      RoutePrefix<AddrT> routePrefix,
      std::optional<cfg ::AclLookupClass> classID) {
    this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
      auto route = findRoute<AddrT>(
          sw_->isStandaloneRibEnabled(),
          kRid(),
          {folly::IPAddress(routePrefix.network), routePrefix.mask},
          sw_->getState());
      EXPECT_EQ(route->getClassID(), classID);
    });
  }

  void addRoute(RoutePrefix<AddrT> routePrefix, std::vector<AddrT> nextHops) {
    CHECK(nextHops.size());
    addDelRouteImpl(routePrefix, nextHops);
  }

  void removeRoute(RoutePrefix<AddrT> routePrefix) {
    addDelRouteImpl(routePrefix, {});
  }

  void removeNeighbor(const AddrT& ip) {
    this->updateState(
        "Add new route", [=](const std::shared_ptr<SwitchState>& state) {
          auto newState = state->clone();

          VlanID vlanId = newState->getInterfaces()
                              ->getInterface(this->kInterfaceID())
                              ->getVlanID();
          Vlan* vlan = newState->getVlans()->getVlanIf(VlanID(vlanId)).get();
          auto* neighborTable =
              vlan->template getNeighborEntryTable<AddrT>().get()->modify(
                  &vlan, &newState);
          neighborTable->removeEntry(ip);
          return newState;
        });
    waitForStateUpdates(this->sw_);
    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForRibUpdates(sw_);
    waitForBackgroundThread(this->sw_);
    waitForStateUpdates(this->sw_);
  }

  void updateLookupClasses(
      const std::vector<cfg::AclLookupClass>& lookupClasses) {
    this->updateState(
        "Remove lookupclasses", [=](const std::shared_ptr<SwitchState>& state) {
          auto newState = state->clone();
          auto newPortMap = newState->getPorts()->modify(&newState);

          for (auto port : *newPortMap) {
            auto newPort = port->clone();
            newPort->setLookupClassesToDistributeTrafficOn(lookupClasses);
            newPortMap->updatePort(newPort);
          }
          return newState;
        });

    // Wait for neighbors to notice the port change
    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForStateUpdates(this->sw_);
    waitForBackgroundThread(this->sw_);
    waitForStateUpdates(this->sw_);
    waitForRibUpdates(sw_);
    waitForStateUpdates(this->sw_);
  }

 private:
  void addDelRouteImpl(
      RoutePrefix<AddrT> routePrefix,
      std::vector<AddrT> nextHops) {
    auto routeUpdater = sw_->getRouteUpdater();
    if (nextHops.size()) {
      RouteNextHopSet nexthops;
      for (const auto& nextHop : nextHops) {
        nexthops.emplace(UnresolvedNextHop(nextHop, UCMP_DEFAULT_WEIGHT));
      }
      routeUpdater.addRoute(
          this->kRid(),
          routePrefix.network,
          routePrefix.mask,
          this->kClientID(),
          RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));
    } else {
      routeUpdater.delRoute(
          this->kRid(),
          routePrefix.network,
          routePrefix.mask,
          this->kClientID());
    }
    routeUpdater.program();
    waitForStateUpdates(this->sw_);
    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForRibUpdates(sw_);
    waitForBackgroundThread(this->sw_);
    waitForStateUpdates(this->sw_);
  }

 protected:
  void runInUpdateEventBaseAndWait(Func func) {
    auto* evb = sw_->getUpdateEvb();
    evb->runInEventBaseThreadAndWait(std::move(func));
  }

  void runInUpdateEvbAndWaitAfterNeighborCachePropagation(Func func) {
    schedulePendingTestStateUpdates();
    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    runInUpdateEventBaseAndWait(std::move(func));
  }

  void schedulePendingTestStateUpdates() {
    runInUpdateEventBaseAndWait([]() {});
  }

  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

template <typename AddrType, bool StandAloneRib>
struct LookupClassRouteUpdaterTypeT {
  using AddrT = AddrType;
  static constexpr auto hasStandAloneRib = StandAloneRib;
};

using TypeTypesLookupClassRouteUpdater =
    ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_CASE(LookupClassRouteUpdaterTest, TypeTypesLookupClassRouteUpdater);

// Test cases verifying Route changes

TYPED_TEST(LookupClassRouteUpdaterTest, AddRouteThenResolveNextHop) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ResolveNextHopThenAddRoute) {
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, AddRouteThenResolveTwoNextHop) {
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

  /*
   * In the current implementation, route inherits classID of the first
   * reachable nexthop. This test verifies that.
   * An accidental change in those semantics would be caught by this test
   * failure.
   */
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ResolveTwoNextHopThenAddRoute) {
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});

  /*
   * In the current implementation, route inherits classID of the first
   * reachable nexthop. This test verifies that.
   * An accidental change in those semantics would be caught by this test
   * failure.
   */
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, AddRouteThenResolveSecondNextHopOnly) {
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ResolveSecondNextHopOnlyThenAddRoute) {
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(
    LookupClassRouteUpdaterTest,
    AddRouteWithTwoNextHopsThenUnresolveFirstNextHop) {
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

  /*
   * In the current implementation, route inherits classID of the first
   * reachable nexthop. This test verifies that.
   * An accidental change in those semantics would be caught by this test
   * failure.
   */
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  this->unresolveNeighbor(this->kIpAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
}

TYPED_TEST(
    LookupClassRouteUpdaterTest,
    AddRouteWithTwoNextHopsThenRemoveFirstNextHop) {
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

  /*
   * In the current implementation, route inherits classID of the first
   * reachable nexthop. This test verifies that.
   * An accidental change in those semantics would be caught by this test
   * failure.
   */
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  this->removeNeighbor(this->kIpAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ChangeNextHopSet) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->verifyClassIDHelper(this->kroutePrefix1(), std::nullopt);

  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

// Test cases verifying Neighbor changes

TYPED_TEST(LookupClassRouteUpdaterTest, VerifyNeighborAddAndRemove) {
  // route r1 has ipA and ipB as nexthop.
  // route r2 has ipB and ipC as nexthop.
  // None of the nexthops are resolved, thus, no classID for r1 or r2.
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});
  this->addRoute(
      this->kroutePrefix2(), {this->kIpAddressB(), this->kIpAddressC()});

  this->verifyClassIDHelper(this->kroutePrefix1(), std::nullopt);
  this->verifyClassIDHelper(this->kroutePrefix2(), std::nullopt);

  // resolve ipA.
  // route r1 inherits ipA's classID, r2 gets none.
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyClassIDHelper(this->kroutePrefix2(), std::nullopt);

  // resolve ipB.
  // route r1 continues to inherit ipA's classID, r2 inherits ipB's classID.
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyClassIDHelper(
      this->kroutePrefix2(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

  // unresolve ipA.
  // r1 inherits ipB's classID.
  this->unresolveNeighbor(this->kIpAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
  this->verifyClassIDHelper(
      this->kroutePrefix2(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

  // unresolve ipB.
  // None of the nexthops are resolved, thus, no classID for r1 or r2.
  this->unresolveNeighbor(this->kIpAddressB());
  this->verifyClassIDHelper(this->kroutePrefix1(), std::nullopt);
  this->verifyClassIDHelper(this->kroutePrefix2(), std::nullopt);
}

TYPED_TEST(LookupClassRouteUpdaterTest, VerifyInteractionWithSyncFib) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  auto updater = this->sw_->getRouteUpdater();
  RouteNextHopSet nexthops;
  nexthops.emplace(UnresolvedNextHop(this->kIpAddressB(), UCMP_DEFAULT_WEIGHT));

  updater.addRoute(
      this->kRid(),
      this->kroutePrefix2().network,
      this->kroutePrefix2().mask,
      ClientID::BGPD,
      RouteNextHopEntry(nexthops, AdminDistance::EBGP));
  // SyncFib for BGP
  updater.program({{this->kRid(), ClientID::BGPD}});
  waitForStateUpdates(this->sw_);
  // ClassID should be unchanged
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, VerifyInteractionWithAddRoute) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  // Add another route
  this->addRoute(this->kroutePrefix2(), {this->kIpAddressB()});
  waitForStateUpdates(this->sw_);
  // ClassID should be unchanged
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, VerifyInteractionWithDelRoute) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  // Add, remove another route
  this->addRoute(this->kroutePrefix2(), {this->kIpAddressB()});
  this->removeRoute(this->kroutePrefix2());
  waitForStateUpdates(this->sw_);
  // ClassID should be unchanged
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

// Test cases verifying Port changes

TYPED_TEST(LookupClassRouteUpdaterTest, PortLookupClassesToNoLookupClasses) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  this->updateLookupClasses({});

  this->verifyClassIDHelper(this->kroutePrefix1(), std::nullopt);
}

TYPED_TEST(LookupClassRouteUpdaterTest, PortNoLookupClassesToLookupClasses) {
  this->updateLookupClasses({});
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());

  this->verifyClassIDHelper(this->kroutePrefix1(), std::nullopt);

  this->updateLookupClasses(
      {cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
       cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1,
       cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2,
       cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3,
       cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4});

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, PortLookupClassesChange) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  this->updateLookupClasses(
      {cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3});

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3);
}

TYPED_TEST(LookupClassRouteUpdaterTest, CompeteClassIdUpdatesWithRouteUpdates) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  std::thread classIdUpdates([this]() {
    for (auto i = 0; i < 1000; ++i) {
      // Induce class ID updates
      this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
      this->unresolveNeighbor(this->kIpAddressA());
    }
  });

  std::thread routeAddDel([this]() {
    for (auto i = 0; i < 1000; ++i) {
      // Induce both route and classID updates
      this->addRoute(
          this->kroutePrefix2(), {this->kIpAddressA(), this->kIpAddressB()});
      this->removeRoute(this->kroutePrefix2());
    }
  });
  for (auto i = 0; i < 1000; ++i) {
    // Induce just route updates
    this->addRoute(this->kroutePrefix3(), {this->kIpAddressC()});
    this->removeRoute(this->kroutePrefix3());
  }
  classIdUpdates.join();
  routeAddDel.join();
}

TYPED_TEST(LookupClassRouteUpdaterTest, CompeteClassIdUpdatesWithApplyConfig) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  std::thread classIdUpdates([this]() {
    for (auto i = 0; i < 1000; ++i) {
      // Induce class ID updates
      this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
      this->unresolveNeighbor(this->kIpAddressA());
    }
  });

  std::thread routeAddDel([this]() {
    for (auto i = 0; i < 1000; ++i) {
      // Induce just route updates
      this->addRoute(this->kroutePrefix3(), {this->kIpAddressC()});
      this->removeRoute(this->kroutePrefix3());
    }
  });
  for (auto i = 0; i < 1000; ++i) {
    this->sw_->applyConfig("Reconfigure", this->getConfig());
  }
  classIdUpdates.join();
  routeAddDel.join();
}

template <typename AddrType, bool StandAloneRib, bool RouteFix>
struct LookupClassRouteUpdaterWarmbootTypeT
    : public LookupClassRouteUpdaterTypeT<AddrType, StandAloneRib> {
  using Base = LookupClassRouteUpdaterTypeT<AddrType, StandAloneRib>;
  static constexpr auto hasRouteFix = RouteFix;
};
} // namespace facebook::fboss
