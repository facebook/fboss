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

#include "fboss/agent/LookupClassRouteUpdater.h"
#include "fboss/agent/NeighborUpdater.h"
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

template <typename AddrT>
class LookupClassRouteUpdaterTest : public ::testing::Test {
 public:
  using Func = std::function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;

  void SetUp() override {
    handle_ = createTestHandle(testStateAWithLookupClasses());
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
      return RoutePrefix<AddrT>{folly::IPAddressV6{"2803:6080:d038:3066::"},
                                64};
    }
  }

  RoutePrefix<AddrT> kroutePrefix2() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return RoutePrefix<AddrT>{folly::IPAddressV4{"11.1.4.0"}, 24};
    } else {
      return RoutePrefix<AddrT>{folly::IPAddressV6{"2803:6080:d038:3067::"},
                                64};
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

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForStateUpdates(sw_);
  }

  void unresolveNeighbor(folly::IPAddress ipAddress) {
    sw_->getNeighborUpdater()->flushEntry(kVlan(), ipAddress);

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
  }

  void verifyClassIDHelper(
      RoutePrefix<AddrT> routePrefix,
      std::optional<cfg ::AclLookupClass> classID) {
    this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
      auto state = sw_->getState();
      auto vlan = state->getVlans()->getVlan(kVlan());
      auto routeTableRib = state->getRouteTables()
                               ->getRouteTable(kRid())
                               ->template getRib<AddrT>();

      auto route = routeTableRib->routes()->getRouteIf(routePrefix);
      XLOG(DBG) << route->str();
      EXPECT_EQ(route->getClassID(), classID);
    });
  }

  void addRoute(RoutePrefix<AddrT> routePrefix, std::vector<AddrT> nextHops) {
    this->updateState(
        "Add new route", [=](const std::shared_ptr<SwitchState>& state) {
          auto newState = state->clone();
          auto routeTables = state->getRouteTables();

          RouteNextHopSet nexthops;
          for (const auto& nextHop : nextHops) {
            nexthops.emplace(UnresolvedNextHop(nextHop, UCMP_DEFAULT_WEIGHT));
          }

          RouteUpdater updater(routeTables);
          updater.addRoute(
              this->kRid(),
              routePrefix.network,
              routePrefix.mask,
              this->kClientID(),
              RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));

          auto newRouteTables = updater.updateDone();
          newRouteTables->publish();
          newState->resetRouteTables(newRouteTables);

          return newState;
        });

    waitForStateUpdates(this->sw_);
    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(this->sw_);
    waitForStateUpdates(this->sw_);
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

    waitForStateUpdates(this->sw_);
    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
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

using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_CASE(LookupClassRouteUpdaterTest, TestTypes);

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

template <typename AddrType, bool RouteFix>
struct WarmbootTestT {
  using AddrT = AddrType;
  static constexpr auto hasRouteFix = RouteFix;
};

using NoRouteFixV4 = WarmbootTestT<folly::IPAddressV4, false>;
using RouteFixV4 = WarmbootTestT<folly::IPAddressV4, true>;
using NoRouteFixV6 = WarmbootTestT<folly::IPAddressV6, false>;
using RouteFixV6 = WarmbootTestT<folly::IPAddressV6, true>;

using TestTypesWarmboot =
    ::testing::Types<NoRouteFixV4, RouteFixV4, NoRouteFixV6, RouteFixV6>;

template <typename WarmbootTestT>
class LookupClassRouteUpdaterWarmbootTest
    : public LookupClassRouteUpdaterTest<typename WarmbootTestT::AddrT> {
  using AddrT = typename WarmbootTestT::AddrT;
  static constexpr auto hasRouteFix = WarmbootTestT::hasRouteFix;

 public:
  void SetUp() override {
    using NeighborTableT = std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    if (hasRouteFix) {
      gflags::SetCommandLineOptionWithMode(
          "queue_per_host_route_fix", "true", gflags::SET_FLAGS_VALUE);
    } else {
      gflags::SetCommandLineOptionWithMode(
          "queue_per_host_route_fix", "false", gflags::SET_FLAGS_VALUE);
    }

    auto newState = testStateAWithLookupClasses();

    auto vlan = newState->getVlans()->getVlanIf(this->kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    neighborTable->addEntry(NeighborEntryFields(
        this->kIpAddressA(),
        this->kMacAddressA(),
        PortDescriptor(this->kPortID()),
        this->kInterfaceID(),
        NeighborState::PENDING));

    neighborTable->updateEntry(
        this->kIpAddressA(),
        this->kMacAddressA(),
        PortDescriptor(this->kPortID()),
        this->kInterfaceID(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    RouteUpdater updater(newState->getRouteTables());
    updater.addInterfaceAndLinkLocalRoutes(newState->getInterfaces());

    RouteNextHopSet nexthops;
    // resolved by intf 1
    nexthops.emplace(
        UnresolvedNextHop(this->kIpAddressA(), UCMP_DEFAULT_WEIGHT));
    nexthops.emplace(
        UnresolvedNextHop(this->kIpAddressB(), UCMP_DEFAULT_WEIGHT));

    updater.addRoute(
        this->kRid(),
        this->kroutePrefix1().network,
        this->kroutePrefix1().mask,
        this->kClientID(),
        RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));

    auto newRt = updater.updateDone();
    newState->resetRouteTables(newRt);

    auto routeTableRib = newState->getRouteTables()
                             ->getRouteTable(this->kRid())
                             ->template getRib<AddrT>();
    auto route = routeTableRib->routes()->getRouteIf(this->kroutePrefix1());
    route->updateClassID(cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    this->handle_ = createTestHandle(newState);
    this->sw_ = this->handle_->getSw();
  }
};

TYPED_TEST_CASE(LookupClassRouteUpdaterWarmbootTest, TestTypesWarmboot);

/*
 * Initialize the Setup() SwitchState to contain:
 *  - resolved neighbor n1 with classID c1.
 *  - route with n1 and n2 as nexthop.
 * (this mimics warmboot)
 *
 * If queue_per_host_route_fix is true:
 *     Verify if route inherits classID of resolved nexthop viz. c1.
 *     Resolve n2 => gets classID c2.
 *     Verify if route classID is unchanged.
 *     Unresolve n1.
 *     Verify if route now inherits classID from next resolved nexthop i.e. c2.
 *
 * If queue_per_host_route_fix is false:
 *     Verify route does not get classID.
 */
TYPED_TEST(LookupClassRouteUpdaterWarmbootTest, VerifyClassID) {
  auto routeClassID0 = TypeParam::hasRouteFix
      ? std::optional(cfg::AclLookupClass{
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0})
      : std::nullopt;
  auto routeClassID1 = TypeParam::hasRouteFix
      ? std::optional(cfg::AclLookupClass{
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1})
      : std::nullopt;

  this->verifyClassIDHelper(this->kroutePrefix1(), routeClassID0);

  // neighbor should get next classID: CLASS_QUEUE_PER_HOST_QUEUE_1
  // (if route fix is enabled).
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

  // route classID is unchanged
  this->verifyClassIDHelper(this->kroutePrefix1(), routeClassID0);

  // unresolve the nexthop route inherited classID from
  this->unresolveNeighbor(this->kIpAddressA());

  // route gets classID of next resolved neighbor: CLASS_QUEUE_PER_HOST_QUEUE_1
  // (if route fix is enabled).
  this->verifyClassIDHelper(this->kroutePrefix1(), routeClassID1);
}

} // namespace facebook::fboss
