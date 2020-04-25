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

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/LookupClassUpdater.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>

using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;

namespace facebook::fboss {

/*
 * TODO(skhare)
 *
 * Diff stack D21099268 rewrites queue-per-host + route fix from 'inherit
 * classID from first nexthop' to 'inherit classID from first *reachable*
 * nexthop. The rewrite is spread across several stacked diffs where first few
 * diffs remove the old implementation and latter diffs gradual add new
 * implementation. To avoid test failures in on-diff testing when old
 * implementation is removed, disable these tests.
 * After new implementation is added (diff latter in same stack), this diff is
 * reverted to re-enable the tests.
 */

template <typename AddrT>
class DISABLED_LookupClassUpdaterTest : public ::testing::Test {
 public:
  using Func = folly::Function<void()>;
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

  PortID kPortID2() const {
    return PortID(2);
  }

  IPAddressV4 kIp4Addr() const {
    return IPAddressV4("10.0.0.22");
  }

  IPAddressV6 kIp6Addr() const {
    return IPAddressV6("2401:db00:2110:3001::0002");
  }

  IPAddressV4 kIp4Addr2() const {
    return IPAddressV4("10.0.0.33");
  }

  IPAddressV4 kIp4Addr3() const {
    return IPAddressV4("10.0.0.44");
  }

  IPAddressV6 kIp6Addr2() const {
    return IPAddressV6("2401:db00:2110:3001::0003");
  }

  IPAddressV6 kIp6Addr3() const {
    return IPAddressV6("2401:db00:2110:3001::0004");
  }

  MacAddress kMacAddress() const {
    return MacAddress("01:02:03:04:05:06");
  }

  MacAddress kMacAddress2() const {
    return MacAddress("01:02:03:04:05:07");
  }

  RouterID kRid() const {
    return RouterID{0};
  }

  RoutePrefix<IPAddress> kRouteV4Prefix() const {
    return RoutePrefix<folly::IPAddress>{folly::IPAddressV4{"10.1.1.0"}, 24};
  }

  RoutePrefix<IPAddress> kRouteV4Prefix2() const {
    return RoutePrefix<folly::IPAddress>{folly::IPAddressV4{"10.1.2.0"}, 24};
  }

  RoutePrefix<IPAddress> kRouteV4Prefix3() const {
    return RoutePrefix<folly::IPAddress>{folly::IPAddressV4{"10.1.3.0"}, 24};
  }

  RoutePrefix<IPAddress> kRouteV6Prefix() const {
    return RoutePrefix<folly::IPAddress>{
        folly::IPAddressV6{"2803:6080:d038:3063::"}, 64};
  }

  RoutePrefix<IPAddress> kRouteV6Prefix2() const {
    return RoutePrefix<folly::IPAddress>{
        folly::IPAddressV6{"2803:6080:d038:3064::"}, 64};
  }

  RoutePrefix<IPAddress> kRouteV6Prefix3() const {
    return RoutePrefix<folly::IPAddress>{
        folly::IPAddressV6{"2803:6080:d038:3065::"}, 64};
  }

  void resolveNeighbor(IPAddress ipAddress, MacAddress macAddress) {
    /*
     * Cause a neighbor entry to resolve by receiving appropriate ARP/NDP, and
     * assert if valid CLASSID is associated with the newly resolved neighbor.
     */
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      sw_->getNeighborUpdater()->receivedArpMine(
          kVlan(),
          ipAddress.asV4(),
          macAddress,
          PortDescriptor(kPortID()),
          ArpOpCode::ARP_OP_REPLY);
    } else {
      sw_->getNeighborUpdater()->receivedNdpMine(
          kVlan(),
          ipAddress.asV6(),
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

  void unresolveNeighbor(IPAddress ipAddress) {
    sw_->getNeighborUpdater()->flushEntry(kVlan(), ipAddress);

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
  }

  void verifyNeighborClassIDHelper(
      folly::IPAddress ipAddress,
      RoutePrefix<IPAddress> routePrefix,
      std::optional<cfg ::AclLookupClass> classID = std::nullopt) {
    using NeighborTableT = std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;
    auto state = sw_->getState();
    auto vlan = state->getVlans()->getVlan(kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();
    auto routeTableRib = state->getRouteTables()
                             ->getRouteTable(kRid())
                             ->template getRib<AddrT>();

    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      auto entry = neighborTable->getEntry(ipAddress.asV4());
      XLOG(DBG) << entry->str();
      EXPECT_EQ(entry->getClassID(), classID);

      RoutePrefix<folly::IPAddressV4> routePrefixV4{routePrefix.network.asV4(),
                                                    routePrefix.mask};
      auto route = routeTableRib->routes()->getRouteIf(routePrefixV4);
      XLOG(DBG) << route->str();
      EXPECT_EQ(route->getClassID(), classID);
    } else {
      auto entry = neighborTable->getEntry(ipAddress.asV6());
      XLOG(DBG) << entry->str();
      EXPECT_EQ(entry->getClassID(), classID);

      RoutePrefix<folly::IPAddressV6> routePrefixV6{routePrefix.network.asV6(),
                                                    routePrefix.mask};
      auto route = routeTableRib->routes()->getRouteIf(routePrefixV6);
      XLOG(DBG) << route->str();
      EXPECT_EQ(route->getClassID(), classID);
    }
  }

  void resolve(IPAddress ipAddress, MacAddress macAddress) {
    if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
      resolveMac(macAddress);
    } else {
      this->resolveNeighbor(ipAddress, macAddress);
    }
  }

  void resolveMac(MacAddress macAddress) {
    auto l2Entry = L2Entry(
        macAddress,
        this->kVlan(),
        PortDescriptor(this->kPortID()),
        L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING);

    this->sw_->l2LearningUpdateReceived(
        l2Entry, L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD);

    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(this->sw_);
    waitForStateUpdates(this->sw_);
  }

  void verifyClassIDHelper(
      const folly::IPAddress& ipAddress,
      const RoutePrefix<IPAddress>& routePrefix,
      const folly::MacAddress& macAddress,
      std::optional<cfg::AclLookupClass> classID = std::nullopt) {
    if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
      verifyMacClassIDHelper(macAddress, classID);
    } else {
      this->verifyNeighborClassIDHelper(ipAddress, routePrefix, classID);
    }
  }

  void verifyMacClassIDHelper(
      const folly::MacAddress& macAddress,
      std::optional<cfg::AclLookupClass> classID = std::nullopt) {
    auto state = this->sw_->getState();
    auto vlan = state->getVlans()->getVlanIf(this->kVlan()).get();
    auto* macTable = vlan->getMacTable().get();

    auto entry = macTable->getNode(macAddress);
    XLOG(DBG) << entry->str();
    EXPECT_EQ(entry->getClassID(), classID);
  }

  IPAddress getIpAddress() {
    IPAddress ipAddress;
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      ipAddress = IPAddress(this->kIp4Addr());
    } else {
      ipAddress = IPAddress(this->kIp6Addr());
    }

    return ipAddress;
  }

  IPAddress getIpAddress2() {
    IPAddress ipAddress;
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      ipAddress = IPAddress(this->kIp4Addr2());
    } else {
      ipAddress = IPAddress(this->kIp6Addr2());
    }

    return ipAddress;
  }

  IPAddress getIpAddress3() {
    IPAddress ipAddress;
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      ipAddress = IPAddress(this->kIp4Addr3());
    } else {
      ipAddress = IPAddress(this->kIp6Addr3());
    }

    return ipAddress;
  }

  RoutePrefix<IPAddress> getRoutePrefix() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return this->kRouteV4Prefix();
    } else {
      return this->kRouteV6Prefix();
    }
  }

  RoutePrefix<IPAddress> getRoutePrefix2() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return this->kRouteV4Prefix2();
    } else {
      return this->kRouteV6Prefix2();
    }
  }

  RoutePrefix<IPAddress> getRoutePrefix3() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return this->kRouteV4Prefix3();
    } else {
      return this->kRouteV6Prefix3();
    }
  }

  void bringPortDown(PortID portID) {
    this->sw_->linkStateChanged(portID, false);

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

using TestTypes =
    ::testing::Types<folly::IPAddressV4, folly::IPAddressV6, folly::MacAddress>;
using TestTypesNeighbor =
    ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_CASE(DISABLED_LookupClassUpdaterTest, TestTypes);

TYPED_TEST(DISABLED_LookupClassUpdaterTest, VerifyClassID) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->getRoutePrefix(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  });
}

TYPED_TEST(DISABLED_LookupClassUpdaterTest, VerifyClassIDPortDown) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->bringPortDown(this->kPortID());
  /*
   * On port down, ARP/NDP behavior differs from L2 entries:
   *  - ARP/NDP neighbors go to pending state, and classID is disassociated.
   *  - L2 entries remain in L2 table with classID associated as before.
   */
  this->verifyStateUpdate([=]() {
    if constexpr (std::is_same_v<TypeParam, folly::MacAddress>) {
      this->verifyMacClassIDHelper(
          this->kMacAddress(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    } else {
      this->verifyClassIDHelper(
          this->getIpAddress(), this->getRoutePrefix(), this->kMacAddress());
    }
  });
}

TYPED_TEST(DISABLED_LookupClassUpdaterTest, LookupClassesToNoLookupClasses) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->updateLookupClasses({});
  this->verifyClassIDHelper(
      this->getIpAddress(), this->getRoutePrefix(), this->kMacAddress());
}

TYPED_TEST(DISABLED_LookupClassUpdaterTest, LookupClassesChange) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->updateLookupClasses(
      {cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3});
  this->verifyClassIDHelper(
      this->getIpAddress(),
      this->getRoutePrefix(),
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3);
}

TYPED_TEST(DISABLED_LookupClassUpdaterTest, MacMove) {
  if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
    return;
  } else if constexpr (std::is_same<TypeParam, folly::IPAddressV6>::value) {
    return;
  }

  this->resolve(this->getIpAddress(), this->kMacAddress());

  this->updateState(
      "Trigger MAC Move", [=](const std::shared_ptr<SwitchState>& state) {
        std::shared_ptr<SwitchState> newState{state};

        auto vlan = state->getVlans()->getVlanIf(this->kVlan()).get();
        auto* macTable = vlan->getMacTable().get();
        auto node = macTable->getNodeIf(this->kMacAddress());

        EXPECT_NE(node, nullptr);

        macTable = macTable->modify(&vlan, &newState);

        macTable->removeEntry(this->kMacAddress());
        auto macEntry = std::make_shared<MacEntry>(
            this->kMacAddress(), PortDescriptor(this->kPortID2()));
        macTable->addEntry(macEntry);

        return newState;
      });

  waitForStateUpdates(this->sw_);
  this->sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(this->sw_);
  waitForStateUpdates(this->sw_);

  auto state = this->sw_->getState();

  auto vlan = state->getVlans()->getVlanIf(this->kVlan());
  auto* macTable = vlan->getMacTable().get();
  auto node = macTable->getNodeIf(this->kMacAddress());

  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->getPort(), PortDescriptor(this->kPortID2()));
}

/*
 * Tests that are valid for arp/ndp neighbors only and not for Mac addresses
 */
template <typename AddrT>
class DISABLED_LookupClassUpdaterNeighborTest
    : public DISABLED_LookupClassUpdaterTest<AddrT> {
 public:
  void verifySameMacDifferentIpsHelper() {
    auto lookupClassUpdater = this->sw_->getLookupClassUpdater();

    this->verifyNeighborClassIDHelper(
        this->getIpAddress(),
        this->getRoutePrefix(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
    this->verifyNeighborClassIDHelper(
        this->getIpAddress2(),
        this->getRoutePrefix2(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    // Verify that refCnt is 2 = 1 for ipAddress + 1 for ipAddress2
    EXPECT_EQ(
        lookupClassUpdater->getRefCnt(
            this->kPortID(),
            this->kMacAddress(),
            this->kVlan(),
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0),
        2);
  }
};

TYPED_TEST_CASE(DISABLED_LookupClassUpdaterNeighborTest, TestTypesNeighbor);

TYPED_TEST(
    DISABLED_LookupClassUpdaterNeighborTest,
    VerifyClassIDSameMacDifferentIPs) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolve(this->getIpAddress2(), this->kMacAddress());

  // Two IPs with same MAC get same classID
  this->verifyStateUpdate([=]() { this->verifySameMacDifferentIpsHelper(); });
}

TYPED_TEST(DISABLED_LookupClassUpdaterNeighborTest, ResolveUnresolveResolve) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolve(this->getIpAddress2(), this->kMacAddress());

  // Two IPs with same MAC get same classID
  this->verifyStateUpdate([=]() { this->verifySameMacDifferentIpsHelper(); });

  this->unresolveNeighbor(this->getIpAddress());
  this->verifyStateUpdate([=]() {
    using NeighborTableT = std::conditional_t<
        std::is_same<TypeParam, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    auto state = this->sw_->getState();
    auto vlan = state->getVlans()->getVlan(this->kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
      EXPECT_EQ(
          neighborTable->getEntryIf(this->getIpAddress().asV4()), nullptr);
    } else {
      EXPECT_EQ(
          neighborTable->getEntryIf(this->getIpAddress().asV6()), nullptr);
    }

    // Verify that refCnt is 1 = 1 for ipAddress2 as ipAddress is unersolved
    auto lookupClassUpdater = this->sw_->getLookupClassUpdater();
    EXPECT_EQ(
        lookupClassUpdater->getRefCnt(
            this->kPortID(),
            this->kMacAddress(),
            this->kVlan(),
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0),
        1);
  });

  // Resolve the IP with same MAC, gets same classID as other IP with same MAC
  this->resolveNeighbor(this->getIpAddress(), this->kMacAddress());
  this->verifyStateUpdate([=]() { this->verifySameMacDifferentIpsHelper(); });
}

template <typename AddrT>
class DISABLED_LookupClassUpdaterWarmbootTest
    : public DISABLED_LookupClassUpdaterTest<AddrT> {
 public:
  void SetUp() override {
    using NeighborTableT = std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    auto newState = testStateAWithLookupClasses();

    auto vlanID = VlanID(1);
    auto vlan = newState->getVlans()->getVlanIf(vlanID);
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    neighborTable->addEntry(NeighborEntryFields(
        this->getIpAddr(),
        this->kMacAddress(),
        PortDescriptor(this->kPortID()),
        InterfaceID(1),
        NeighborState::PENDING));

    neighborTable->updateEntry(
        this->getIpAddr(),
        this->kMacAddress(),
        PortDescriptor(this->kPortID()),
        InterfaceID(1),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    auto routeTableRib = newState->getRouteTables()
                             ->getRouteTable(this->kRid())
                             ->template getRib<AddrT>();

    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      RoutePrefix<folly::IPAddressV4> routePrefixV4{
          this->getRoutePrefix().network.asV4(), this->getRoutePrefix().mask};
      auto route = routeTableRib->routes()->getRouteIf(routePrefixV4);
      route->updateClassID(cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
    } else {
      RoutePrefix<folly::IPAddressV6> routePrefixV6{
          this->getRoutePrefix().network.asV6(), this->getRoutePrefix().mask};
      auto route = routeTableRib->routes()->getRouteIf(routePrefixV6);
      route->updateClassID(cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
    }

    this->handle_ = createTestHandle(newState);
    this->sw_ = this->handle_->getSw();
  }

  AddrT getIpAddr() {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return this->kIp4Addr();
    } else {
      return this->kIp6Addr();
    }
  }
};

TYPED_TEST_CASE(DISABLED_LookupClassUpdaterWarmbootTest, TestTypesNeighbor);

/*
 * Initialize the SetUp() SwitchState to carry a neighbor with a classID.
 * LookupClassUpdater::updateStateObserverLocalCache should consume this
 * to initialize its local cache (this mimics warmboot).
 *
 * Verify if the neighbor indeed has the classID.
 * Resolve another neighbor with different MAC: it should get next classID.
 * Resolve another neighbor with same MAC as that of neighbor in SetUp().
 * Verify if it gets same classID as chosen SetUp(): classIDs are unique per
 * MAC.
 *
 * Routes inherit classID of their nexthop (if any). Thus, also verify if it
 * holds true.
 */
TYPED_TEST(DISABLED_LookupClassUpdaterWarmbootTest, VerifyClassID) {
  this->resolveNeighbor(this->getIpAddress2(), this->kMacAddress2());
  this->resolveNeighbor(this->getIpAddress3(), this->kMacAddress());

  this->verifyStateUpdate([=]() {
    this->verifyNeighborClassIDHelper(
        this->getIpAddress(),
        this->getRoutePrefix(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    this->verifyNeighborClassIDHelper(
        this->getIpAddress2(),
        this->getRoutePrefix2(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

    this->verifyNeighborClassIDHelper(
        this->getIpAddress3(),
        this->getRoutePrefix3(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  });
}

} // namespace facebook::fboss
