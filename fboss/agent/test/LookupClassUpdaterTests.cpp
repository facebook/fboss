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

#include "fboss/agent/GtestDefs.h"
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/LookupClassUpdater.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/state/Port.h"
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

template <typename AddrT>
class LookupClassUpdaterTest : public ::testing::Test {
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
    return IPAddressV4("10.0.0.2");
  }

  IPAddressV6 kIp6Addr() const {
    return IPAddressV6("2401:db00:2110:3001::0002");
  }

  IPAddressV4 kIp4Addr2() const {
    return IPAddressV4("10.0.0.3");
  }

  IPAddressV4 kIp4Addr3() const {
    return IPAddressV4("10.0.0.4");
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
      std::optional<cfg ::AclLookupClass> classID = std::nullopt) {
    using NeighborTableT = std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    auto state = sw_->getState();
    auto vlan = state->getVlans()->getVlan(kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    auto verifyNeighbor = [this, classID, neighborTable](auto ipAddr) {
      auto entry = neighborTable->getEntry(ipAddr);
      XLOG(DBG) << entry->str();
      EXPECT_EQ(entry->getClassID(), classID);
      if (entry->isReachable() && !entry->getMac().isBroadcast()) {
        // We assume here that class ID of mac matches that of
        // neighbor. That's true for our tests, since for neighbor
        // entries we add a Mac entry in sequence. And since Mac
        // entries round robin over the same sequence of classIDs
        // the paired MAC entry gets a identical classID.
        verifyMacClassIDHelper(
            entry->getMac(), classID, MacEntryType::STATIC_ENTRY);
      }
    };
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      verifyNeighbor(ipAddress.asV4());
    } else {
      verifyNeighbor(ipAddress.asV6());
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
      const folly::MacAddress& macAddress,
      std::optional<cfg::AclLookupClass> classID = std::nullopt) {
    if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
      // Learned Mac entries always get added as dynamic entries
      verifyMacClassIDHelper(macAddress, classID, MacEntryType::DYNAMIC_ENTRY);
    } else {
      this->verifyNeighborClassIDHelper(ipAddress, classID);
    }
  }

  auto getMacEntry(folly::MacAddress mac) const {
    auto vlan = sw_->getState()->getVlans()->getVlanIf(this->kVlan());
    return vlan->getMacTable()->getNodeIf(mac);
  }

  void verifyMacClassIDHelper(
      const folly::MacAddress& macAddress,
      std::optional<cfg::AclLookupClass> classID,
      MacEntryType type) {
    auto entry = getMacEntry(macAddress);
    XLOG(DBG) << entry->str();
    EXPECT_EQ(entry->getClassID(), classID);
    EXPECT_EQ(entry->getType(), type);
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
        "Reset lookupclasses", [=](const std::shared_ptr<SwitchState>& state) {
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

TYPED_TEST_SUITE(LookupClassUpdaterTest, TestTypes);

TYPED_TEST(LookupClassUpdaterTest, VerifyClassID) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  });
}

TYPED_TEST(LookupClassUpdaterTest, VerifyClassIDPortDown) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->bringPortDown(this->kPortID());
  /*
   * On port down, ARP/NDP behavior differs from L2 entries:
   *  - ARP/NDP neighbors go to pending state, and classID is disassociated.
   *  - L2 entries should get pruned with neighbor dereference
   */
  this->verifyStateUpdate([=]() {
    if constexpr (std::is_same_v<TypeParam, folly::MacAddress>) {
      this->verifyMacClassIDHelper(
          this->kMacAddress(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
          MacEntryType::DYNAMIC_ENTRY);

    } else {
      this->verifyClassIDHelper(this->getIpAddress(), this->kMacAddress());
      // Mac entry should get pruned with neighbor entry
      EXPECT_EQ(this->getMacEntry(this->kMacAddress()), nullptr);
    }
  });
}

TYPED_TEST(LookupClassUpdaterTest, LookupClassesToNoLookupClasses) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->updateLookupClasses({});
  this->verifyClassIDHelper(this->getIpAddress(), this->kMacAddress());
}

TYPED_TEST(LookupClassUpdaterTest, LookupClassesChange) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->updateLookupClasses(
      {cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3});
  this->verifyClassIDHelper(
      this->getIpAddress(),
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3);
}

TYPED_TEST(LookupClassUpdaterTest, MacMove) {
  if constexpr (!std::is_same_v<TypeParam, folly::MacAddress>) {
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
  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::DYNAMIC_ENTRY);
}

/*
 * This test case covers scenarios where a MAC address with class ID is
 * aged out and relearned. This simulates collapsing of the state changes
 * that occur in quick succession and makes sure that class ID is not lost.
 */
TYPED_TEST(LookupClassUpdaterTest, MacAgeAndRelearned) {
  if constexpr (!std::is_same_v<TypeParam, folly::MacAddress>) {
    return;
  }
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->updateLookupClasses(
      {cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3});

  this->updateState(
      "Trigger MAC Age & Relearn ",
      [=](const std::shared_ptr<SwitchState>& state) {
        std::shared_ptr<SwitchState> newState{state};

        auto vlan = state->getVlans()->getVlanIf(this->kVlan()).get();
        auto* macTable = vlan->getMacTable().get();
        auto node = macTable->getNodeIf(this->kMacAddress());

        EXPECT_NE(node, nullptr);

        macTable = macTable->modify(&vlan, &newState);

        macTable->removeEntry(this->kMacAddress());
        // Add the same mac entry and check the class ID
        auto macEntry = std::make_shared<MacEntry>(
            this->kMacAddress(), PortDescriptor(this->kPortID()));
        macTable->addEntry(macEntry);

        return newState;
      });

  waitForStateUpdates(this->sw_);
  this->sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(this->sw_);
  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3,
      MacEntryType::DYNAMIC_ENTRY);
}

/*
 * Tests that are valid for arp/ndp neighbors only and not for Mac addresses
 */
template <typename AddrT>
class LookupClassUpdaterNeighborTest : public LookupClassUpdaterTest<AddrT> {
 public:
  void verifySameMacDifferentIpsHelper() {
    auto lookupClassUpdater = this->sw_->getLookupClassUpdater();

    this->verifyNeighborClassIDHelper(
        this->getIpAddress(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
    this->verifyNeighborClassIDHelper(
        this->getIpAddress2(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    // Verify that refCnt is 3
    // 2 for ipAddress +  ipAddress2 and 1 for static MAC entry
    EXPECT_EQ(
        lookupClassUpdater->getRefCnt(
            this->kPortID(),
            this->kMacAddress(),
            this->kVlan(),
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0),
        3);
  }
};

TYPED_TEST_SUITE(LookupClassUpdaterNeighborTest, TestTypesNeighbor);

TYPED_TEST(LookupClassUpdaterNeighborTest, VerifyClassIDSameMacDifferentIPs) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolve(this->getIpAddress2(), this->kMacAddress());

  // Two IPs with same MAC get same classID
  this->verifyStateUpdate([=]() { this->verifySameMacDifferentIpsHelper(); });
}

TYPED_TEST(LookupClassUpdaterNeighborTest, ResolveUnresolveResolve) {
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

    // Verify that refCnt is 2 =
    // 1 for ipAddress2 as ipAddress is unersolved + 1 for Static MAC
    auto lookupClassUpdater = this->sw_->getLookupClassUpdater();
    EXPECT_EQ(
        lookupClassUpdater->getRefCnt(
            this->kPortID(),
            this->kMacAddress(),
            this->kVlan(),
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0),
        2);
    // Assert that MAC entry still retains class ID and is of
    // type static (since one neighbor is still resolved)
    this->verifyMacClassIDHelper(
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
        MacEntryType::STATIC_ENTRY);
  });

  // Resolve the IP with same MAC, gets same classID as other IP with same MAC
  this->resolveNeighbor(this->getIpAddress(), this->kMacAddress());
  this->verifyStateUpdate([=]() { this->verifySameMacDifferentIpsHelper(); });
}

TYPED_TEST(LookupClassUpdaterNeighborTest, staticL2EntriesForResolvedNeighbor) {
  this->resolveMac(this->kMacAddress());
  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::DYNAMIC_ENTRY);
  this->resolve(this->getIpAddress(), this->kMacAddress());
  // Mac entry should transform to STATIC post neighbor resolution
  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::STATIC_ENTRY);
}

template <typename AddrT>
class LookupClassUpdaterWarmbootTest : public LookupClassUpdaterTest<AddrT> {
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

TYPED_TEST_SUITE(LookupClassUpdaterWarmbootTest, TestTypesNeighbor);

/*
 * Initialize the SetUp() SwitchState to carry a neighbor with a classID.
 * LookupClassUpdater::initObserver should consume this to initialize its local
 * cache (this mimics warmboot).
 *
 * Verify if the neighbor indeed has the classID.
 * Resolve another neighbor with different MAC: it should get next classID.
 * Resolve another neighbor with same MAC as that of neighbor in SetUp().
 * Verify if it gets same classID as chosen SetUp(): classIDs are unique per
 * MAC.
 */
TYPED_TEST(LookupClassUpdaterWarmbootTest, VerifyClassID) {
  this->resolveNeighbor(this->getIpAddress2(), this->kMacAddress2());
  this->resolveNeighbor(this->getIpAddress3(), this->kMacAddress());

  this->verifyStateUpdate([=]() {
    this->verifyNeighborClassIDHelper(
        this->getIpAddress(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    this->verifyNeighborClassIDHelper(
        this->getIpAddress2(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

    this->verifyNeighborClassIDHelper(
        this->getIpAddress3(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  });
}

} // namespace facebook::fboss
