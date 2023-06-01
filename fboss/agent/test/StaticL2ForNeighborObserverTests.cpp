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
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/StaticL2ForNeighborObserver.h"
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
class StaticL2ForNeighorObserverTest : public ::testing::Test {
 public:
  using Func = folly::Function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;

  void SetUp() override {
    handle_ = createTestHandle(testStateAWithPortsUp());
    sw_ = handle_->getSw();
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

  IPAddress kIp4Addr() const {
    return IPAddressV4("10.0.0.2");
  }
  IPAddress kIp4Addr2() const {
    return IPAddressV4("10.0.0.3");
  }

  IPAddress kIp6Addr() const {
    return IPAddressV6("2401:db00:2110:3001::0002");
  }
  IPAddress kIp6Addr2() const {
    return IPAddressV6("2401:db00:2110:3001::0003");
  }
  MacAddress kMacAddress() const {
    return MacAddress("01:02:03:04:05:06");
  }

  void resolveNeighbor(IPAddress ipAddress, MacAddress macAddress) {
    /*
     * Cause a neighbor entry to resolve by receiving appropriate ARP/NDP, and
     * assert if valid CLASSID is associated with the newly resolved neighbor.
     */
    if (ipAddress.isV4()) {
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

  void resolve(IPAddress ipAddress, MacAddress macAddress) {
    if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
      resolveMac(macAddress);
    } else {
      this->resolveNeighbor(ipAddress, macAddress);
    }
  }

  void resolveMac(
      MacAddress macAddress,
      std::optional<PortID> port = std::nullopt) {
    auto l2Entry = L2Entry(
        macAddress,
        this->kVlan(),
        PortDescriptor(port ? port.value() : this->kPortID()),
        L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING);

    this->sw_->l2LearningUpdateReceived(
        l2Entry, L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD);

    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(this->sw_);
    waitForStateUpdates(this->sw_);
  }

  void unresolveMac(MacAddress macAddress) {
    auto l2Entry = L2Entry(
        macAddress,
        this->kVlan(),
        PortDescriptor(this->kPortID()),
        L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);

    this->sw_->l2LearningUpdateReceived(
        l2Entry, L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE);

    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(this->sw_);
    waitForStateUpdates(this->sw_);
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

  void bringPortDown(PortID portID) {
    this->sw_->linkStateChanged(portID, false);

    waitForStateUpdates(this->sw_);
    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(this->sw_);
    waitForStateUpdates(this->sw_);
  }

 protected:
  void verifyMacEntryExists(
      MacEntryType type,
      std::optional<PortID> port = std::nullopt) const {
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
    auto macEntry = getMacEntry();
    ASSERT_NE(macEntry, nullptr);
    EXPECT_EQ(macEntry->getType(), type);
    if (port) {
      EXPECT_EQ(macEntry->getPort(), port);
    }
  }
  void verifyMacEntryDoesNotExist() const {
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
    EXPECT_EQ(getMacEntry(), nullptr);
  }

 private:
  auto getMacEntry() const {
    auto vlan = sw_->getState()->getVlans()->getNode(kVlan());
    return vlan->getMacTable()->getMacIf(kMacAddress());
  }
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

TYPED_TEST_SUITE(StaticL2ForNeighorObserverTest, TestTypes);

TYPED_TEST(
    StaticL2ForNeighorObserverTest,
    noStaticL2EntriesForUnResolvedNeighbor) {
  this->verifyMacEntryDoesNotExist();
}

TYPED_TEST(StaticL2ForNeighorObserverTest, staticL2EntriesForResolvedNeighbor) {
  this->verifyMacEntryDoesNotExist();
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
}

TYPED_TEST(
    StaticL2ForNeighorObserverTest,
    staticL2EntriesForUnresolvedToResolvedNeighbor) {
  this->verifyMacEntryDoesNotExist();
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  this->unresolveNeighbor(this->getIpAddress());
  this->verifyMacEntryDoesNotExist();
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
}

TYPED_TEST(StaticL2ForNeighorObserverTest, dynamicMacEntryIfNoNeighbor) {
  this->resolveMac(this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::DYNAMIC_ENTRY);
}

TYPED_TEST(
    StaticL2ForNeighorObserverTest,
    dynamicToStaticL2MacEntryOnNeighborResolution) {
  this->resolveMac(this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::DYNAMIC_ENTRY);
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
}

TYPED_TEST(
    StaticL2ForNeighorObserverTest,
    staticL2MacEntryForMultipleNeighbors) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolve(this->getIpAddress2(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  // Unresolve one neighbor Static mac entry should continue to
  // exist
  this->unresolveNeighbor(this->getIpAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  // Unresolve second neighbor, now static MAC should go away
  this->unresolveNeighbor(this->getIpAddress2());
  this->verifyMacEntryDoesNotExist();
}

TYPED_TEST(
    StaticL2ForNeighorObserverTest,
    staticL2MacEntryForMultipleAddrFamilyNeighbors) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  auto secondAddr =
      this->getIpAddress().isV6() ? this->kIp4Addr() : this->kIp6Addr();
  this->resolve(secondAddr, this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  // Unresolve one neighbor Static mac entry should continue to
  // exist
  this->unresolveNeighbor(this->getIpAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  // Unresolve second neighbor, now static MAC should go away
  this->unresolveNeighbor(secondAddr);
  this->verifyMacEntryDoesNotExist();
}

TYPED_TEST(StaticL2ForNeighorObserverTest, bringPortDownSingleNeighbor) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  this->bringPortDown(this->kPortID());
  // Neighbor would get unreolved as a result of port down
  // taking down MAC entryy with it
  this->verifyMacEntryDoesNotExist();
}

TYPED_TEST(StaticL2ForNeighorObserverTest, bringPortDownMultipleNeighbors) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  auto secondAddr =
      this->getIpAddress().isV6() ? this->kIp4Addr() : this->kIp6Addr();
  this->resolve(secondAddr, this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  this->bringPortDown(this->kPortID());
  // All neighbor would get unreolved as a result of port down
  // taking down MAC entryy with it
  this->verifyMacEntryDoesNotExist();
}

TYPED_TEST(StaticL2ForNeighorObserverTest, learnMacPostNeighborResolution) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolveMac(this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
}

TYPED_TEST(StaticL2ForNeighorObserverTest, unrelatedPortDown) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  this->bringPortDown(this->kPortID2());
  // MAC entryy is unaffected, since the port down above did
  // not take away any neighbors
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
}

TYPED_TEST(StaticL2ForNeighorObserverTest, ageMacWhileNeighborResolved) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolveMac(this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  this->unresolveMac(this->kMacAddress());
  // MAC entry still exists due to the fact that a neigbor still
  // refers to it.
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
}

TYPED_TEST(StaticL2ForNeighorObserverTest, macMovePostNeighborResolution) {
  // Since we create static entries for resolved neighbor, from there on
  // the mac entry is owned by the neighbor. So in the event MAC moves
  // to a different port make sure that we still defer to the neighbor
  // entry's port information. In practice, when a MAC moves a neighbor
  // resolution over the new port will follow soon and get things
  // to converge
  this->resolveMac(this->kMacAddress());
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  this->resolveMac(this->kMacAddress(), this->kPortID2());
  // Neighbor entry port ID trumps MAC learning event
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY, this->kPortID());
}
} // namespace facebook::fboss
