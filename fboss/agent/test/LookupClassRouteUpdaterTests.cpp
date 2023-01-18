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
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {
constexpr auto kQphMultiNextHopCounter = "qph.multinexthop.route";
} // namespace

namespace facebook::fboss {

template <typename AddressT>
class LookupClassRouteUpdaterTest : public ::testing::Test {
 public:
  using Func = std::function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;
  using AddrT = AddressT;

  virtual cfg::SwitchConfig getConfig() const {
    return testConfigAWithLookupClasses();
  }

  void SetUp() override {
    auto config = getConfig();
    handle_ = createTestHandle(&config);
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

  VlanID kVlan2() const {
    return VlanID(55);
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

  folly::CIDRNetwork kInterfaceAddress() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return std::make_pair(folly::IPAddress("10.0.0.1"), 24);
    } else {
      return std::make_pair(folly::IPAddress("2401:db00:2110:3001::1"), 64);
    }
  }

  MacAddress kMacAddressA() const {
    return MacAddress("01:02:03:04:05:06");
  }

  MacAddress kMacAddressB() const {
    return MacAddress("01:02:03:04:05:07");
  }

  MacAddress kMacAddressC() const {
    return MacAddress("01:02:03:04:05:09");
  }

  MacAddress kMacAddressD() const {
    return MacAddress("01:02:03:04:05:08");
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

  AddrT kIpAddressD() {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return IPAddressV4("10.0.55.2");
    } else {
      return IPAddressV6("2401:db00:2110:3055::0002");
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

  RoutePrefix<AddrT> kroutePrefix4() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return RoutePrefix<AddrT>{folly::IPAddressV4{"13.1.4.0"}, 24};
    } else {
      return RoutePrefix<AddrT>{
          folly::IPAddressV6{"4803:6080:d038:3067::"}, 64};
    }
  }

  SwSwitch* getSw() const {
    return sw_;
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
          kRid(),
          {folly::IPAddress(routePrefix.network()), routePrefix.mask()},
          sw_->getState());
      EXPECT_EQ(route->getClassID(), classID);
    });
  }

  void verifyNumRoutesWithMultiNextHops(int numExpectedRoute) {
    auto lookupClassRouteUpdater = this->sw_->getLookupClassRouteUpdater();
    EXPECT_EQ(
        lookupClassRouteUpdater->getNumPrefixesWithMultiNextHops(),
        numExpectedRoute);

    CounterCache counters(this->sw_);
    counters.update();
    EXPECT_EQ(
        counters.value(SwitchStats::kCounterPrefix + kQphMultiNextHopCounter),
        numExpectedRoute);
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
            auto newPort = port.second->clone();
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

  void addRouteResolveNeighborBlockUnblockHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);

    updateBlockedNeighbor(
        this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);

    updateBlockedNeighbor(this->getSw(), {{}});

    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);
  }

  void resolveNeighborBlockAddRouteUnblockHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    updateBlockedNeighbor(
        this->getSw(), {{this->kVlan(), this->kIpAddressA()}});

    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);

    updateBlockedNeighbor(this->getSw(), {{}});
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);
  }

  void blockResolveNeighborAddRouteUnblockHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    updateBlockedNeighbor(
        this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());

    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);

    updateBlockedNeighbor(this->getSw(), {{}});
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);
  }

  // block traffic to MAC helpers
  void addRouteResolveNeighborBlockUnblockMacHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);

    updateMacAddrsToBlock(
        this->getSw(), {{this->kVlan(), this->kMacAddressA()}});
    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);

    updateMacAddrsToBlock(this->getSw(), {});

    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);
  }

  void resolveNeighborBlockMacAddRouteUnblockMacHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    updateMacAddrsToBlock(
        this->getSw(), {{this->kVlan(), this->kMacAddressA()}});

    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);

    updateMacAddrsToBlock(this->getSw(), {});
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);
  }

  void blockMacResolveNeighborAddRouteUnblockHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    updateMacAddrsToBlock(
        this->getSw(), {{this->kVlan(), this->kMacAddressA()}});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());

    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);

    updateMacAddrsToBlock(this->getSw(), {});
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);
  }

  void applySameMacBlockListTwiceHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);

    updateMacAddrsToBlock(
        this->getSw(), {{this->kVlan(), this->kMacAddressA()}});
    auto subnetCacheAfterBlocking1 = getSubnetCache();

    updateMacAddrsToBlock(
        this->getSw(), {{this->kVlan(), this->kMacAddressA()}});
    auto subnetCacheAfterBlocking2 = getSubnetCache();

    EXPECT_EQ(subnetCacheAfterBlocking1, subnetCacheAfterBlocking2);
  }

  void setAndClearMacBlockListHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);

    updateMacAddrsToBlock(
        this->getSw(), {{this->kVlan(), this->kMacAddressA()}});
    auto subnetCacheAfterBlocking = getSubnetCache();

    updateMacAddrsToBlock(this->getSw(), {});
    auto subnetCacheAfterUnblocking = getSubnetCache();

    auto subnetCacheAfterBlockingForVlan =
        subnetCacheAfterBlocking.find(kVlan())->second;
    auto subnetCacheAfterUnblockingForVlan =
        subnetCacheAfterUnblocking.find(kVlan())->second;

    std::vector<folly::CIDRNetwork> addedEntries;
    std::set_difference(
        subnetCacheAfterBlockingForVlan.begin(),
        subnetCacheAfterBlockingForVlan.end(),
        subnetCacheAfterUnblockingForVlan.begin(),
        subnetCacheAfterUnblockingForVlan.end(),
        std::inserter(addedEntries, addedEntries.end()));

    bool noLookupClasses = !expectedClassID.has_value();
    if (noLookupClasses) {
      EXPECT_EQ(addedEntries.size(), 1);
      EXPECT_EQ(addedEntries.front(), kInterfaceAddress());
    } else {
      // Entries are already cached due to non-empty lookupclasses and remain
      // cached even after clearing block list
      EXPECT_EQ(addedEntries.size(), 0);
      EXPECT_EQ(subnetCacheAfterBlocking, subnetCacheAfterUnblocking);
    }
  }

  void blockMacForFirstNextHopHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    this->addRoute(
        this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);
    /*
     * In the current implementation, route inherits classID of the first
     * reachable nexthop. Thus, if the MAC corresponding to that nexthop is
     * blocked, traffic to that route stops.
     */
    updateMacAddrsToBlock(
        this->getSw(), {{this->kVlan(), this->kMacAddressA()}});
    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);

    updateMacAddrsToBlock(this->getSw(), {});
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);
  }

  void multipleRoutesBlockUnblockNexthopMacHelper(
      std::optional<cfg::AclLookupClass> expectedClassID1,
      std::optional<cfg::AclLookupClass> expectedClassID2) {
    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());

    this->addRoute(this->kroutePrefix2(), {this->kIpAddressB()});
    this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

    // route1's nexthop's mac unblocked, route2's nexthop's mac unblocked
    updateMacAddrsToBlock(this->getSw(), {});
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID1);
    this->verifyClassIDHelper(this->kroutePrefix2(), expectedClassID2);

    // route1's nexthop's mac blocked, route2's nexthop's mac unblocked
    updateMacAddrsToBlock(
        this->getSw(), {{this->kVlan(), this->kMacAddressA()}});
    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);
    this->verifyClassIDHelper(this->kroutePrefix2(), expectedClassID2);

    // route1's nexthop's mac blocked, route2's nexthop's mac blocked
    updateMacAddrsToBlock(
        this->getSw(),
        {{this->kVlan(), this->kMacAddressA()},
         {this->kVlan(), this->kMacAddressB()}});

    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);
    this->verifyClassIDHelper(
        this->kroutePrefix2(), cfg::AclLookupClass::CLASS_DROP);

    // route1's nexthop's mac unblocked, route2's nexthop's mac blocked
    updateMacAddrsToBlock(
        this->getSw(), {{this->kVlan(), this->kMacAddressB()}});
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID1);
    this->verifyClassIDHelper(
        this->kroutePrefix2(), cfg::AclLookupClass::CLASS_DROP);

    // route1's nexthop's mac unblocked, route2's nexthop's mac unblocked
    updateMacAddrsToBlock(this->getSw(), {});
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID1);
    this->verifyClassIDHelper(this->kroutePrefix2(), expectedClassID2);
  }

  void blockMacThenIPResolveToSameMacHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());

    updateMacAddrsToBlock(this->getSw(), {});
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);

    updateMacAddrsToBlock(
        this->getSw(), {{this->kVlan(), this->kMacAddressA()}});
    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);

    // Another IP address resolved to the same MAC address
    this->addRoute(this->kroutePrefix2(), {this->kIpAddressB()});
    this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressA());

    this->verifyClassIDHelper(
        this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);
    this->verifyClassIDHelper(
        this->kroutePrefix2(), cfg::AclLookupClass::CLASS_DROP);

    updateMacAddrsToBlock(this->getSw(), {});
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);
    this->verifyClassIDHelper(this->kroutePrefix2(), expectedClassID);
  }

  const boost::container::
      flat_map<VlanID, folly::F14FastSet<folly::CIDRNetwork>>&
      getSubnetCache() const {
    auto lookupClassRouteUpdater = this->sw_->getLookupClassRouteUpdater();
    return lookupClassRouteUpdater->getVlan2SubnetCache();
  }

  folly::F14FastSet<folly::CIDRNetwork> getSubnetCacheForVlan(
      const boost::container::flat_map<
          VlanID,
          folly::F14FastSet<folly::CIDRNetwork>>& vlanSubnetCache,
      VlanID vlanID) const {
    folly::F14FastSet<folly::CIDRNetwork> subnetCacheForVlan = {};
    auto it = vlanSubnetCache.find(vlanID);
    if (it != vlanSubnetCache.end()) {
      subnetCacheForVlan = it->second;
    }

    return subnetCacheForVlan;
  }

  void printSubnetCache(
      const boost::container::flat_map<
          VlanID,
          folly::F14FastSet<folly::CIDRNetwork>>& vlan2SubnetCache,
      const std::string& str) const {
    for (const auto& [vlanID, cidrSet] : vlan2SubnetCache) {
      for (const auto& [prefix, mask] : cidrSet) {
        XLOG(DBG2) << str << " vlanID: " << vlanID
                   << " prefix: " << prefix.str()
                   << " mask: " << static_cast<int>(mask);
      }
    }
  }

  void applySameBlockListTwiceHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);

    updateBlockedNeighbor(
        this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
    auto subnetCacheAfterBlocking1 = getSubnetCache();

    updateBlockedNeighbor(
        this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
    auto subnetCacheAfterBlocking2 = getSubnetCache();

    printSubnetCache(subnetCacheAfterBlocking1, "subnetCacheAfterBlocking1");
    printSubnetCache(subnetCacheAfterBlocking2, "subnetCacheAfterBlocking2");

    EXPECT_EQ(subnetCacheAfterBlocking1, subnetCacheAfterBlocking2);
  }

  void setAndClearBlockListHelper(
      std::optional<cfg::AclLookupClass> expectedClassID) {
    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID);

    updateBlockedNeighbor(
        this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
    auto subnetCacheAfterBlocking = getSubnetCache();

    updateBlockedNeighbor(this->getSw(), {{}});
    auto subnetCacheAfterUnblocking = getSubnetCache();

    printSubnetCache(subnetCacheAfterBlocking, "subnetCacheAfterBlocking");
    printSubnetCache(subnetCacheAfterUnblocking, "subnetCacheAfterUnblocking");

    auto subnetCacheAfterBlockingForVlan =
        subnetCacheAfterBlocking.find(kVlan())->second;
    auto subnetCacheAfterUnblockingForVlan =
        subnetCacheAfterUnblocking.find(kVlan())->second;

    std::vector<folly::CIDRNetwork> addedEntries;
    std::set_difference(
        subnetCacheAfterBlockingForVlan.begin(),
        subnetCacheAfterBlockingForVlan.end(),
        subnetCacheAfterUnblockingForVlan.begin(),
        subnetCacheAfterUnblockingForVlan.end(),
        std::inserter(addedEntries, addedEntries.end()));

    bool noLookupClasses = !expectedClassID.has_value();
    if (noLookupClasses) {
      EXPECT_EQ(addedEntries.size(), 1);
      EXPECT_EQ(addedEntries.front(), kInterfaceAddress());
    } else {
      // Entries are already cached due to non-empty lookupclasses and remain
      // cached even after clearing block list
      EXPECT_EQ(addedEntries.size(), 0);
      EXPECT_EQ(subnetCacheAfterBlocking, subnetCacheAfterUnblocking);
    }
  }

  void modifyBlockListSameSubnetHelper(
      std::optional<cfg::AclLookupClass> expectedClassID1,
      std::optional<cfg::AclLookupClass> expectedClassID2) {
    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->addRoute(this->kroutePrefix2(), {this->kIpAddressB()});

    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID1);
    this->verifyClassIDHelper(this->kroutePrefix2(), expectedClassID2);

    auto subnetCacheBeforeBlocking = getSubnetCache();

    // neighborA blocked, neighborB unblocked
    updateBlockedNeighbor(
        this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
    auto subnetCacheNeighborABlockedNeighborBUnblocked = getSubnetCache();

    // neighborA blocked, neighborB blocked
    updateBlockedNeighbor(
        this->getSw(),
        {{this->kVlan(), this->kIpAddressA()},
         {this->kVlan(), this->kIpAddressB()}});
    auto subnetCacheNeighborABlockedNeighborBBlocked = getSubnetCache();

    // neighborA unblocked, neighborB blocked
    updateBlockedNeighbor(
        this->getSw(), {{this->kVlan(), this->kIpAddressB()}});
    auto subnetCacheNeighborAUnBlockedNeighborBBlocked = getSubnetCache();

    // neighborA unblocked, neighborB unblocked
    updateBlockedNeighbor(this->getSw(), {{}});
    auto subnetCacheNeighborAUnBlockedNeighborBUnBlocked = getSubnetCache();

    bool noLookupClasses = !expectedClassID1.has_value();
    if (noLookupClasses) {
      EXPECT_EQ(
          getSubnetCacheForVlan(subnetCacheBeforeBlocking, kVlan()),
          getSubnetCacheForVlan(
              subnetCacheNeighborAUnBlockedNeighborBUnBlocked, kVlan()));
      EXPECT_EQ(
          getSubnetCacheForVlan(
              subnetCacheNeighborABlockedNeighborBUnblocked, kVlan()),
          getSubnetCacheForVlan(
              subnetCacheNeighborABlockedNeighborBBlocked, kVlan()));
      EXPECT_NE(
          getSubnetCacheForVlan(subnetCacheBeforeBlocking, kVlan()),
          getSubnetCacheForVlan(
              subnetCacheNeighborABlockedNeighborBBlocked, kVlan()));
    } else {
      // Entries are already cached due to non-empty lookupclasses and remain
      // cached even after modifications to the block list
      EXPECT_EQ(
          subnetCacheBeforeBlocking,
          subnetCacheNeighborABlockedNeighborBUnblocked);
      EXPECT_EQ(
          subnetCacheBeforeBlocking,
          subnetCacheNeighborABlockedNeighborBBlocked);
      EXPECT_EQ(
          subnetCacheBeforeBlocking,
          subnetCacheNeighborAUnBlockedNeighborBUnBlocked);
    }
  }

  void modifyBlockListDifferentSubnetHelper(
      std::optional<cfg::AclLookupClass> expectedClassID1,
      std::optional<cfg::AclLookupClass> expectedClassID2) {
    this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
    this->addRoute(this->kroutePrefix4(), {this->kIpAddressD()});

    this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
    this->resolveNeighbor(this->kIpAddressD(), this->kMacAddressD());

    this->verifyClassIDHelper(this->kroutePrefix1(), expectedClassID1);
    this->verifyClassIDHelper(this->kroutePrefix4(), expectedClassID2);

    auto subnetCacheBeforeBlocking = getSubnetCache();

    // neighborA blocked, neighborD unblocked
    updateBlockedNeighbor(
        this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
    auto subnetCacheNeighborABlockedNeighborDUnblocked = getSubnetCache();

    // neighborA blocked, neighborD blocked
    updateBlockedNeighbor(
        this->getSw(),
        {{this->kVlan(), this->kIpAddressA()},
         {this->kVlan2(), this->kIpAddressD()}});
    auto subnetCacheNeighborABlockedNeighborDBlocked = getSubnetCache();

    // neighborA unblocked, neighborD blocked
    updateBlockedNeighbor(
        this->getSw(), {{this->kVlan2(), this->kIpAddressD()}});
    auto subnetCacheNeighborAUnBlockedNeighborDBlocked = getSubnetCache();

    // neighborA unblocked, neighborD unblocked
    updateBlockedNeighbor(this->getSw(), {{}});
    auto subnetCacheNeighborAUnBlockedNeighborDUnBlocked = getSubnetCache();

    bool noLookupClasses = !expectedClassID1.has_value();
    if (noLookupClasses) {
      EXPECT_EQ(
          getSubnetCacheForVlan(subnetCacheBeforeBlocking, kVlan()),
          getSubnetCacheForVlan(
              subnetCacheNeighborAUnBlockedNeighborDUnBlocked, kVlan()));
      EXPECT_EQ(
          getSubnetCacheForVlan(subnetCacheBeforeBlocking, kVlan2()),
          getSubnetCacheForVlan(
              subnetCacheNeighborAUnBlockedNeighborDUnBlocked, kVlan2()));

      EXPECT_EQ(
          getSubnetCacheForVlan(
              subnetCacheNeighborABlockedNeighborDUnblocked, kVlan()),
          getSubnetCacheForVlan(
              subnetCacheNeighborABlockedNeighborDBlocked, kVlan()));
      // Neighbor D belongs kVlan2(), and thus subnet cache would be updated
      // when Neighbor D is in the block list.
      EXPECT_NE(
          getSubnetCacheForVlan(
              subnetCacheNeighborABlockedNeighborDUnblocked, kVlan2()),
          getSubnetCacheForVlan(
              subnetCacheNeighborABlockedNeighborDBlocked, kVlan2()));

      EXPECT_NE(
          getSubnetCacheForVlan(subnetCacheBeforeBlocking, kVlan()),
          getSubnetCacheForVlan(
              subnetCacheNeighborABlockedNeighborDBlocked, kVlan()));
      EXPECT_NE(
          getSubnetCacheForVlan(subnetCacheBeforeBlocking, kVlan2()),
          getSubnetCacheForVlan(
              subnetCacheNeighborABlockedNeighborDBlocked, kVlan2()));
    } else {
      EXPECT_EQ(
          subnetCacheBeforeBlocking,
          subnetCacheNeighborABlockedNeighborDUnblocked);
      EXPECT_EQ(
          subnetCacheBeforeBlocking,
          subnetCacheNeighborAUnBlockedNeighborDBlocked);
      EXPECT_EQ(
          subnetCacheBeforeBlocking,
          subnetCacheNeighborAUnBlockedNeighborDUnBlocked);
    }
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
          routePrefix.network(),
          routePrefix.mask(),
          this->kClientID(),
          RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));
    } else {
      routeUpdater.delRoute(
          this->kRid(),
          routePrefix.network(),
          routePrefix.mask(),
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
    waitForStateUpdates(this->sw_);
    sw_->getRib()->waitForRibUpdates();
    waitForStateUpdates(this->sw_);
    runInUpdateEventBaseAndWait(std::move(func));
  }

  void schedulePendingTestStateUpdates() {
    runInUpdateEventBaseAndWait([]() {});
  }

  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
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
  this->verifyNumRoutesWithMultiNextHops(0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ResolveNextHopThenAddRoute) {
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyNumRoutesWithMultiNextHops(0);
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
  this->verifyNumRoutesWithMultiNextHops(1);
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
  this->verifyNumRoutesWithMultiNextHops(1);
}

TYPED_TEST(LookupClassRouteUpdaterTest, AddRouteThenResolveSecondNextHopOnly) {
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyNumRoutesWithMultiNextHops(0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ResolveSecondNextHopOnlyThenAddRoute) {
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyNumRoutesWithMultiNextHops(0);
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
  this->verifyNumRoutesWithMultiNextHops(1);

  this->unresolveNeighbor(this->kIpAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
  this->verifyNumRoutesWithMultiNextHops(0);
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
  this->verifyNumRoutesWithMultiNextHops(1);

  this->removeNeighbor(this->kIpAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
  this->verifyNumRoutesWithMultiNextHops(0);
}

TYPED_TEST(
    LookupClassRouteUpdaterTest,
    ResolveOneNextHopAndAddRouteThenResolveSecondNextHop) {
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyNumRoutesWithMultiNextHops(1);

  this->unresolveNeighbor(this->kIpAddressB());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyNumRoutesWithMultiNextHops(0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, AddTwoRouteThenResolveNextHop) {
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});
  this->addRoute(
      this->kroutePrefix2(), {this->kIpAddressB(), this->kIpAddressC()});
  auto lookupClassRouteUpdater = this->sw_->getLookupClassRouteUpdater();

  EXPECT_EQ(lookupClassRouteUpdater->getNumPrefixesWithMultiNextHops(), 0);

  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());
  EXPECT_EQ(lookupClassRouteUpdater->getNumPrefixesWithMultiNextHops(), 1);
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyClassIDHelper(
      this->kroutePrefix2(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

  this->resolveNeighbor(this->kIpAddressC(), this->kMacAddressC());
  EXPECT_EQ(lookupClassRouteUpdater->getNumPrefixesWithMultiNextHops(), 2);

  this->unresolveNeighbor(this->kIpAddressC());
  EXPECT_EQ(lookupClassRouteUpdater->getNumPrefixesWithMultiNextHops(), 1);

  this->resolveNeighbor(this->kIpAddressC(), this->kMacAddressC());
  this->unresolveNeighbor(this->kIpAddressB());
  EXPECT_EQ(lookupClassRouteUpdater->getNumPrefixesWithMultiNextHops(), 0);
  this->verifyClassIDHelper(
      this->kroutePrefix2(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ChangeNextHopSet) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->verifyClassIDHelper(this->kroutePrefix1(), std::nullopt);

  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyNumRoutesWithMultiNextHops(0);
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
  this->verifyNumRoutesWithMultiNextHops(1);

  // unresolve ipA.
  // r1 inherits ipB's classID.
  this->unresolveNeighbor(this->kIpAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
  this->verifyClassIDHelper(
      this->kroutePrefix2(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
  this->verifyNumRoutesWithMultiNextHops(0);

  // unresolve ipB.
  // None of the nexthops are resolved, thus, no classID for r1 or r2.
  this->unresolveNeighbor(this->kIpAddressB());
  this->verifyClassIDHelper(this->kroutePrefix1(), std::nullopt);
  this->verifyClassIDHelper(this->kroutePrefix2(), std::nullopt);
  this->verifyNumRoutesWithMultiNextHops(0);
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
      this->kroutePrefix2().network(),
      this->kroutePrefix2().mask(),
      ClientID::BGPD,
      RouteNextHopEntry(nexthops, AdminDistance::EBGP));
  // SyncFib for BGP
  updater.program(
      {{{this->kRid(), ClientID::BGPD}},
       RouteUpdateWrapper::SyncFibInfo::SyncFibType::IP_ONLY});
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
  this->verifyNumRoutesWithMultiNextHops(0);
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
  this->verifyNumRoutesWithMultiNextHops(0);
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

TYPED_TEST(LookupClassRouteUpdaterTest, BlockNeighborAddRouteResolveToBlocked) {
  updateBlockedNeighbor(this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);
}

TYPED_TEST(LookupClassRouteUpdaterTest, AddRouteResolveNextHopThenBlock) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  updateBlockedNeighbor(this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);
}

TYPED_TEST(
    LookupClassRouteUpdaterTest,
    AddRouteThenResolveNextHopBlockFistNextHop) {
  this->addRoute(
      this->kroutePrefix1(), {this->kIpAddressA(), this->kIpAddressB()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  /*
   * In the current implementation, route inherits classID of the first
   * reachable nexthop. Thus, if that nexthop is blocked, traffic to that route
   * stops.
   */
  updateBlockedNeighbor(this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);
}

TYPED_TEST(LookupClassRouteUpdaterTest, MultipleRoutesBlockUnblockNexthop) {
  this->addRoute(this->kroutePrefix1(), {this->kIpAddressA()});
  this->resolveNeighbor(this->kIpAddressA(), this->kMacAddressA());

  this->addRoute(this->kroutePrefix2(), {this->kIpAddressB()});
  this->resolveNeighbor(this->kIpAddressB(), this->kMacAddressB());

  // route1's nexthop unblocked, route2's nexthop unblocked
  updateBlockedNeighbor(this->getSw(), {{}});
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyClassIDHelper(
      this->kroutePrefix2(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

  // route1's nexthop blocked, route2's nexthop unblocked
  updateBlockedNeighbor(this->getSw(), {{this->kVlan(), this->kIpAddressA()}});
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);
  this->verifyClassIDHelper(
      this->kroutePrefix2(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

  // route1's nexthop blocked, route2's nexthop blocked
  updateBlockedNeighbor(
      this->getSw(),
      {{this->kVlan(), this->kIpAddressA()},
       {this->kVlan(), this->kIpAddressB()}});

  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_DROP);
  this->verifyClassIDHelper(
      this->kroutePrefix2(), cfg::AclLookupClass::CLASS_DROP);

  // route1's nexthop unblocked, route2's nexthop blocked
  updateBlockedNeighbor(this->getSw(), {{this->kVlan(), this->kIpAddressB()}});
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyClassIDHelper(
      this->kroutePrefix2(), cfg::AclLookupClass::CLASS_DROP);

  // route1's nexthop unblocked, route2's nexthop unblocked
  updateBlockedNeighbor(this->getSw(), {{}});
  this->verifyClassIDHelper(
      this->kroutePrefix1(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  this->verifyClassIDHelper(
      this->kroutePrefix2(), cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
}

TYPED_TEST(LookupClassRouteUpdaterTest, AddRouteResolveNeighborBlockUnblock) {
  this->addRouteResolveNeighborBlockUnblockHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ResolveNeighborBlockAddRouteUnblock) {
  this->resolveNeighborBlockAddRouteUnblockHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, BlockResolveNeighborAddRouteUnblock) {
  this->blockResolveNeighborAddRouteUnblockHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ApplySameBlockListTwice) {
  this->applySameBlockListTwiceHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, SetAndClearBlockList) {
  this->setAndClearBlockListHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ModifyBlockListSameSubnet) {
  this->modifyBlockListSameSubnetHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ModifyBlockListDifferentSubnet) {
  this->modifyBlockListDifferentSubnetHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0, std::nullopt);
}

// MAC addrs to block unit tests
TYPED_TEST(
    LookupClassRouteUpdaterTest,
    AddRouteResolveNeighborBlockUnblockMac) {
  this->addRouteResolveNeighborBlockUnblockMacHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(
    LookupClassRouteUpdaterTest,
    ResolveNeighborBlockMacAddRouteUnblockMac) {
  this->resolveNeighborBlockMacAddRouteUnblockMacHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(
    LookupClassRouteUpdaterTest,
    BlockMacResolveNeighborAddRouteUnblock) {
  this->blockMacResolveNeighborAddRouteUnblockHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, ApplySameMacBlockListTwice) {
  this->applySameMacBlockListTwiceHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, SetAndClearMacBlockList) {
  this->setAndClearMacBlockListHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(
    LookupClassRouteUpdaterTest,
    AddRouteThenResolveNextHopBlockMacForFirstNextHop) {
  this->blockMacForFirstNextHopHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

TYPED_TEST(LookupClassRouteUpdaterTest, MultipleRoutesBlockUnblockNexthopMac) {
  this->multipleRoutesBlockUnblockNexthopMacHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
}

TYPED_TEST(LookupClassRouteUpdaterTest, BlockMacThenIPResolveToSameMac) {
  this->blockMacThenIPResolveToSameMacHelper(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
}

template <typename AddressT>
class LookupClassRouteUpdaterNoLookupClassTest
    : public LookupClassRouteUpdaterTest<AddressT> {
  cfg::SwitchConfig getConfig() const override {
    return testConfigA();
  }
};

TYPED_TEST_CASE(
    LookupClassRouteUpdaterNoLookupClassTest,
    TypeTypesLookupClassRouteUpdater);

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    AddRouteResolveNeighborBlock) {
  this->addRouteResolveNeighborBlockUnblockHelper(std::nullopt);
}

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    ResolveNeighborBlockAddRouteUnblock) {
  this->resolveNeighborBlockAddRouteUnblockHelper(std::nullopt);
}

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    BlockResolveNeighborAddRouteUnblock) {
  this->blockResolveNeighborAddRouteUnblockHelper(std::nullopt);
}

TYPED_TEST(LookupClassRouteUpdaterNoLookupClassTest, ApplySameBlockListTwice) {
  this->applySameBlockListTwiceHelper(std::nullopt);
}

TYPED_TEST(LookupClassRouteUpdaterNoLookupClassTest, SetAndClearBlockList) {
  this->setAndClearBlockListHelper(std::nullopt);
}

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    ModifyBlockListSameSubnet) {
  this->modifyBlockListSameSubnetHelper(std::nullopt, std::nullopt);
}

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    ModifyBlockListDifferentSubnet) {
  this->modifyBlockListDifferentSubnetHelper(std::nullopt, std::nullopt);
}

// MAC addrs to block unit tests
TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    AddRouteResolveNeighborBlockUnblockMac) {
  this->addRouteResolveNeighborBlockUnblockMacHelper(std::nullopt);
}

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    ResolveNeighborBlockMacAddRouteUnblockMac) {
  this->resolveNeighborBlockMacAddRouteUnblockMacHelper(std::nullopt);
}

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    BlockMacResolveNeighborAddRouteUnblock) {
  this->blockMacResolveNeighborAddRouteUnblockHelper(std::nullopt);
}

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    ApplySameMacBlockListTwice) {
  this->applySameMacBlockListTwiceHelper(std::nullopt);
}

TYPED_TEST(LookupClassRouteUpdaterNoLookupClassTest, SetAndClearMacBlockList) {
  this->setAndClearMacBlockListHelper(std::nullopt);
}

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    AddRouteThenResolveNextHopBlockMacForFirstNextHop) {
  this->blockMacForFirstNextHopHelper(std::nullopt);
}

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    MultipleRoutesBlockUnblockNexthopMac) {
  this->multipleRoutesBlockUnblockNexthopMacHelper(std::nullopt, std::nullopt);
}

TYPED_TEST(
    LookupClassRouteUpdaterNoLookupClassTest,
    BlockMacThenIPResolveToSameMac) {
  this->blockMacThenIPResolveToSameMacHelper(std::nullopt);
}

} // namespace facebook::fboss
