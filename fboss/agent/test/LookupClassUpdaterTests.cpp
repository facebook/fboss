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

  IPAddressV4 kIp4LinkLocalAddr() const {
    return IPAddressV4("169.254.0.1");
  }

  IPAddressV6 kIp6LinkLocalAddr() const {
    return IPAddressV6("fe80::302:03ff:fe04:0506");
  }
  IPAddressV4 kNonMacIp4LinkLocalAddr() const {
    return IPAddressV4("169.254.0.10");
  }

  IPAddressV6 kNonMacIp6LinkLocalAddr() const {
    return IPAddressV6("fe80::991a:885b:34ad:d94a");
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
  SwSwitch* getSw() const {
    return sw_;
  }

  void resolveNeighbor(
      IPAddress ipAddress,
      MacAddress macAddress,
      bool wait = true) {
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
          ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION,
          0);
    }
    if (wait) {
      sw_->getNeighborUpdater()->waitForPendingUpdates();
      waitForBackgroundThread(sw_);
      waitForStateUpdates(sw_);
      sw_->getNeighborUpdater()->waitForPendingUpdates();
      waitForStateUpdates(sw_);
    }
  }

  void unresolveNeighbor(IPAddress ipAddress) {
    sw_->getNeighborUpdater()->flushEntry(kVlan(), ipAddress);

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
  }

  void verifyNeighborClassIDHelper(
      folly::IPAddress ipAddress,
      std::optional<cfg ::AclLookupClass> ipClassID,
      std::optional<cfg ::AclLookupClass> macClassID) const {
    using NeighborTableT = std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    auto state = sw_->getState();
    auto vlan = state->getVlans()->getVlan(kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    auto verifyNeighbor =
        [this, ipClassID, macClassID, neighborTable](auto ipAddr) {
          auto entry = neighborTable->getEntry(ipAddr);
          XLOG(DBG) << entry->str();
          EXPECT_EQ(entry->getClassID(), ipClassID);
          if (entry->isReachable() && !entry->getMac().isBroadcast()) {
            // We assume here that class ID of mac matches that of
            // neighbor. That's true for our tests, since for neighbor
            // entries we add a Mac entry in sequence. And since Mac
            // entries round robin over the same sequence of classIDs
            // the paired MAC entry gets a identical classID.
            verifyMacClassIDHelper(
                entry->getMac(), macClassID, MacEntryType::STATIC_ENTRY);
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

  void resolveMac(MacAddress macAddress, bool wait = true) {
    auto l2Entry = L2Entry(
        macAddress,
        this->kVlan(),
        PortDescriptor(this->kPortID()),
        L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING);

    this->sw_->l2LearningUpdateReceived(
        l2Entry, L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD);
    if (wait) {
      this->sw_->getNeighborUpdater()->waitForPendingUpdates();
      waitForBackgroundThread(this->sw_);
      waitForStateUpdates(this->sw_);
    }
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

  void verifyClassIDHelper(
      const folly::IPAddress& ipAddress,
      const folly::MacAddress& macAddress,
      std::optional<cfg::AclLookupClass> ipClassID,
      std::optional<cfg::AclLookupClass> macClassID) const {
    if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
      // Learned Mac entries always get added as dynamic entries
      verifyMacClassIDHelper(
          macAddress, macClassID, MacEntryType::DYNAMIC_ENTRY);
    } else {
      this->verifyNeighborClassIDHelper(ipAddress, ipClassID, macClassID);
    }
  }

  auto getMacEntry(folly::MacAddress mac) const {
    auto vlan = sw_->getState()->getVlans()->getVlanIf(this->kVlan());
    return vlan->getMacTable()->getMacIf(mac);
  }

  void verifyMacClassIDHelper(
      const folly::MacAddress& macAddress,
      std::optional<cfg::AclLookupClass> classID,
      MacEntryType type) const {
    auto entry = getMacEntry(macAddress);
    XLOG(DBG) << entry->str();
    EXPECT_EQ(entry->getClassID(), classID);
    EXPECT_EQ(entry->getType(), type);
  }

  IPAddress getIpAddress() const {
    IPAddress ipAddress;
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      ipAddress = IPAddress(this->kIp4Addr());
    } else {
      ipAddress = IPAddress(this->kIp6Addr());
    }

    return ipAddress;
  }

  IPAddress getLinkLocalIpAddress() const {
    IPAddress ipAddress;
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      ipAddress = IPAddress(this->kIp4LinkLocalAddr());
    } else {
      ipAddress = IPAddress(this->kIp6LinkLocalAddr());
    }
    return ipAddress;
  }

  IPAddress getNonMacLinkLocalIpAddress() const {
    IPAddress ipAddress;
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      ipAddress = IPAddress(this->kNonMacIp4LinkLocalAddr());
    } else {
      ipAddress = IPAddress(this->kNonMacIp6LinkLocalAddr());
    }
    return ipAddress;
  }

  std::vector<IPAddress> getLinkLocalIpAddresses() const {
    return {getLinkLocalIpAddress(), getNonMacLinkLocalIpAddress()};
  }

  void resolveLinkLocals(folly::MacAddress mac) {
    for (const auto& ip : getLinkLocalIpAddresses()) {
      this->resolve(ip, mac);
    }
  }
  std::optional<cfg::AclLookupClass> getExpectedLinkLocalClassID(
      cfg::AclLookupClass classID) const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return classID;
    } else {
      // IPv6 Link local has noHostRoute set, so no classID
      return std::nullopt;
    }
  }

  IPAddress getIpAddress2() const {
    IPAddress ipAddress;
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      ipAddress = IPAddress(this->kIp4Addr2());
    } else {
      ipAddress = IPAddress(this->kIp6Addr2());
    }

    return ipAddress;
  }

  IPAddress getIpAddress3() const {
    IPAddress ipAddress;
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      ipAddress = IPAddress(this->kIp4Addr3());
    } else {
      ipAddress = IPAddress(this->kIp6Addr3());
    }

    return ipAddress;
  }

  void bringPortDown(PortID portID) const {
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

          for (auto port : std::as_const(*newPortMap)) {
            auto newPort = port.second->clone();
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
  this->resolveLinkLocals(this->kMacAddress());
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);
  });

  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    for (const auto& ip : this->getLinkLocalIpAddresses()) {
      this->verifyClassIDHelper(
          ip,
          this->kMacAddress(),
          this->getExpectedLinkLocalClassID(
              cfg::AclLookupClass::
                  CLASS_QUEUE_PER_HOST_QUEUE_0) /* ipClassID */,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);
    }
  });
}

TYPED_TEST(LookupClassUpdaterTest, VerifyClassIDPortDown) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolveLinkLocals(this->kMacAddress());
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
      this->verifyClassIDHelper(
          this->getIpAddress(),
          this->kMacAddress(),
          std::nullopt /* ipClassID */,
          std::nullopt /* macClassID */);
      for (const auto& ip : this->getLinkLocalIpAddresses()) {
        this->verifyClassIDHelper(
            ip,
            this->kMacAddress(),
            std::nullopt /* ipClassID */,
            std::nullopt /* macClassID */);
        // Mac entry should get pruned with neighbor entry
        EXPECT_EQ(this->getMacEntry(this->kMacAddress()), nullptr);
      }
    }
  });
}

TYPED_TEST(LookupClassUpdaterTest, LookupClassesToNoLookupClasses) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolveLinkLocals(this->kMacAddress());
  this->updateLookupClasses({});
  this->verifyClassIDHelper(
      this->getIpAddress(),
      this->kMacAddress(),
      std::nullopt /* ipClassID */,
      std::nullopt /* macClassID */);
  for (const auto& ip : this->getLinkLocalIpAddresses()) {
    this->verifyClassIDHelper(
        ip,
        this->kMacAddress(),
        std::nullopt /* ipClassID */,
        std::nullopt /* macClassID */);
  }
}

TYPED_TEST(LookupClassUpdaterTest, LookupClassesChange) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolveLinkLocals(this->kMacAddress());
  this->updateLookupClasses(
      {cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3});
  this->verifyClassIDHelper(
      this->getIpAddress(),
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3 /* ipClassID */,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3 /* macClassID */);
  for (const auto& ip : this->getLinkLocalIpAddresses()) {
    this->verifyClassIDHelper(
        ip,
        this->kMacAddress(),
        this->getExpectedLinkLocalClassID(
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3) /* ipClassID */,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3 /* macClassID */);
  }
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
        auto node = macTable->getMacIf(this->kMacAddress());

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
  auto node = macTable->getMacIf(this->kMacAddress());

  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->getPort(), PortDescriptor(this->kPortID2()));
  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
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
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);
    this->verifyNeighborClassIDHelper(
        this->getIpAddress2(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);

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

  void verifyMultipleBlockedNeighborHelper(
      const std::vector<IPAddress>& ipAddresses,
      std::optional<cfg::AclLookupClass> classID1,
      std::optional<cfg::AclLookupClass> classID2) {
    std::vector<std::pair<VlanID, folly::IPAddress>> blockNeighbors;
    for (const auto& ipAddress : ipAddresses) {
      blockNeighbors.push_back(std::make_pair(this->kVlan(), ipAddress));
    }

    updateBlockedNeighbor(this->getSw(), blockNeighbors);
    this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
      this->verifyClassIDHelper(
          this->getIpAddress(),
          this->kMacAddress(),
          classID1 /* ipClassID */,
          classID1 /* macClassID */);
    });
    this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
      this->verifyClassIDHelper(
          this->getIpAddress2(),
          this->kMacAddress2(),
          classID2 /* ipClassID */,
          classID2 /* macClassID */);
    });
  }

  void verifyMultipleBlockedMacsHelper(
      const std::vector<folly::MacAddress>& macAddresses,
      std::optional<cfg::AclLookupClass> classID1,
      std::optional<cfg::AclLookupClass> classID2) {
    std::vector<std::pair<VlanID, folly::MacAddress>> macAddrsToBlock;
    for (const auto& macAddress : macAddresses) {
      macAddrsToBlock.push_back(std::make_pair(this->kVlan(), macAddress));
    }

    updateMacAddrsToBlock(this->getSw(), macAddrsToBlock);
    this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
      this->verifyClassIDHelper(
          this->getIpAddress(),
          this->kMacAddress(),
          classID1 /* ipClassID */,
          classID1 /* macClassID */);
    });
    this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
      this->verifyClassIDHelper(
          this->getIpAddress2(),
          this->kMacAddress2(),
          classID2 /* ipClassID */,
          classID2 /* macClassID */);
    });
  }
};

TYPED_TEST_SUITE(LookupClassUpdaterNeighborTest, TestTypesNeighbor);

TYPED_TEST(
    LookupClassUpdaterNeighborTest,
    ResolveUnresolveResolveVerifyClassID) {
  using NeighborTableT = std::conditional_t<
      std::is_same<TypeParam, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  auto verifyClassIDs = [this]() {
    this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
      this->verifyClassIDHelper(
          this->getIpAddress(),
          this->kMacAddress(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);
    });

    this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
      for (const auto& ip : this->getLinkLocalIpAddresses()) {
        this->verifyClassIDHelper(
            ip,
            this->kMacAddress(),
            this->getExpectedLinkLocalClassID(
                cfg::AclLookupClass::
                    CLASS_QUEUE_PER_HOST_QUEUE_0) /* ipClassID */,
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);
      }
    });
  };

  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolveLinkLocals(this->kMacAddress());

  verifyClassIDs();

  this->unresolveNeighbor(this->getIpAddress());
  this->unresolveNeighbor(this->getLinkLocalIpAddress());
  this->unresolveNeighbor(this->getNonMacLinkLocalIpAddress());

  auto state = this->sw_->getState();
  auto vlan = state->getVlans()->getVlan(this->kVlan());
  auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

  if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
    EXPECT_EQ(neighborTable->getEntryIf(this->getIpAddress().asV4()), nullptr);
    for (const auto& ip : this->getLinkLocalIpAddresses()) {
      EXPECT_EQ(neighborTable->getEntryIf(ip.asV4()), nullptr);
    }
  } else {
    EXPECT_EQ(neighborTable->getEntryIf(this->getIpAddress().asV6()), nullptr);
    for (const auto& ip : this->getLinkLocalIpAddresses()) {
      EXPECT_EQ(neighborTable->getEntryIf(ip.asV6()), nullptr);
    }
  }

  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolveLinkLocals(this->kMacAddress());

  verifyClassIDs();
}

TYPED_TEST(LookupClassUpdaterNeighborTest, StressNeighborEntryFlap) {
  auto verifyClassIDs = [this]() {
    this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
      this->verifyClassIDHelper(
          this->getIpAddress(),
          this->kMacAddress(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
    });
  };

  this->resolve(this->getIpAddress(), this->kMacAddress());
  verifyClassIDs();

  // unresolve neighbor entry by moving it to pending state
  this->sw_->getNeighborUpdater()->portDown(PortDescriptor(this->kPortID()));
  this->sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(this->sw_);
  waitForStateUpdates(this->sw_);

  for (int i = 0; i < 1000; i++) {
    // 1. resolve neighbor entry by moving it back to reachable state
    // run neighbor entry state update
    // lookupClassUpdater do refCnt++, schedule set neighbor entry class id
    this->resolveNeighbor(this->getIpAddress(), this->kMacAddress(), false);

    // 2. unresolve neighbor entry by moving it to pending
    // run neighbor entry state update
    // lookupClassUpdater do refCnt--, schedule unset neighbor entry class id
    this->sw_->getNeighborUpdater()->portDown(PortDescriptor(this->kPortID()));

    // 3. resolve neighbor entry by moving it back to reachable state
    // run neighbor entry state update
    // run set neighbor entry class id from step 1
    // lookupClassUpdater do refCnt++, schedule set new neighbor entry class id
    this->resolveNeighbor(this->getIpAddress(), this->kMacAddress(), false);

    // 4. unresolve neighbor entry again by moving it to pending
    // run neighbor entry state update
    // run unset neighbor entry class id from 2
    // lookupClassUpdater do refCnt--, schedule unset neighbor entry class id
    this->sw_->getNeighborUpdater()->portDown(PortDescriptor(this->kPortID()));
    /* sleep override */
    usleep(100);
  }
  this->resolve(this->getIpAddress(), this->kMacAddress());
  verifyClassIDs();
}

TYPED_TEST(
    LookupClassUpdaterNeighborTest,
    NeighborEntryFlapWithBusyUpdateThread) {
  auto verifyClassIDs = [this]() {
    this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
      this->verifyClassIDHelper(
          this->getIpAddress(),
          this->kMacAddress(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
    });
  };

  this->resolve(this->getIpAddress(), this->kMacAddress());
  verifyClassIDs();

  // first unresolve neighbor entry by moving it to pending state
  this->sw_->getNeighborUpdater()->portDown(PortDescriptor(this->kPortID()));
  this->sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(this->sw_);
  waitForStateUpdates(this->sw_);

  // emulating neighbor entry flap by resolve followed up by quick unresolve
  this->resolveNeighbor(this->getIpAddress(), this->kMacAddress(), false);

  // unresolve neighbor entry by moving it to pending state
  this->sw_->getNeighborUpdater()->portDown(PortDescriptor(this->kPortID()));

  // in the meanwhile,emulate busy state update thread by keep doing state
  // snapshot
  for (int i = 0; i < 1000; i++) {
    std::shared_ptr<SwitchState> snapshot{nullptr};
    auto snapshotUpdate = [&snapshot](const std::shared_ptr<SwitchState>& state)
        -> std::shared_ptr<SwitchState> {
      snapshot = state;
      return nullptr;
    };
    this->sw_->updateStateNoCoalescing("snapshot update", snapshotUpdate);
    usleep(100);
  }

  this->unresolveMac(this->kMacAddress());

  this->resolve(this->getIpAddress(), this->kMacAddress());
  verifyClassIDs();
}

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

TYPED_TEST(LookupClassUpdaterNeighborTest, BlockNeighborThenResolve) {
  updateBlockedNeighbor(this->getSw(), {{this->kVlan(), this->getIpAddress()}});
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
        cfg::AclLookupClass::CLASS_DROP /* macClassID */);
  });
}

TYPED_TEST(LookupClassUpdaterNeighborTest, ResolveThenBlockNeighbor) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  updateBlockedNeighbor(this->getSw(), {{this->kVlan(), this->getIpAddress()}});
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
        cfg::AclLookupClass::CLASS_DROP /* macClassID */);
  });
}

TYPED_TEST(
    LookupClassUpdaterNeighborTest,
    NoLookupClassBlockNeighborThenResolve) {
  // No lookup classes
  this->updateLookupClasses({});

  updateBlockedNeighbor(this->getSw(), {{this->kVlan(), this->getIpAddress()}});
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
        cfg::AclLookupClass::CLASS_DROP /* macClassID */);
  });
}

TYPED_TEST(
    LookupClassUpdaterNeighborTest,
    NoLookupClassResolveThenBlockNeighbor) {
  // No lookup classes
  this->updateLookupClasses({});

  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        std::nullopt /* ipClassID */,
        std::nullopt /* macClassID */);
  });

  updateBlockedNeighbor(this->getSw(), {{this->kVlan(), this->getIpAddress()}});
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
        cfg::AclLookupClass::CLASS_DROP /* macClassID */);
  });
}

TYPED_TEST(
    LookupClassUpdaterNeighborTest,
    NoLookupClassBlockThenUnblockMultipleNeighbors) {
  // No lookup classes
  this->updateLookupClasses({});

  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolve(this->getIpAddress2(), this->kMacAddress2());

  // neighbor1 unblocked, neighbor2 unblocked
  this->verifyMultipleBlockedNeighborHelper({}, std::nullopt, std::nullopt);

  // neighbor1 blocked, neighbor2 unblocked
  this->verifyMultipleBlockedNeighborHelper(
      {this->getIpAddress()}, cfg::AclLookupClass::CLASS_DROP, std::nullopt);

  // neighbor1 blocked, neighbor2 blocked
  this->verifyMultipleBlockedNeighborHelper(
      {this->getIpAddress(), this->getIpAddress2()},
      cfg::AclLookupClass::CLASS_DROP,
      cfg::AclLookupClass::CLASS_DROP);

  // neighbor1 unblocked, neighbor2 blocked
  this->verifyMultipleBlockedNeighborHelper(
      {this->getIpAddress2()}, std::nullopt, cfg::AclLookupClass::CLASS_DROP);

  // neighbor1 unblocked, neighbor2 unblocked
  this->verifyMultipleBlockedNeighborHelper({}, std::nullopt, std::nullopt);
}

TYPED_TEST(LookupClassUpdaterNeighborTest, NeighborMacChange) {
  // resolve neighbor
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);

  // resolve neighbor to different MAC address
  this->resolve(this->getIpAddress(), this->kMacAddress2());
  this->verifyMacClassIDHelper(
      this->kMacAddress2(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1 /* ipClassID */,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1 /* macClassID */);
}

TYPED_TEST(LookupClassUpdaterNeighborTest, NeighborBlockMacChange) {
  // resolve neighbor
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);

  // block neighbor
  updateBlockedNeighbor(this->getSw(), {{this->kVlan(), this->getIpAddress()}});
  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_DROP,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
      cfg::AclLookupClass::CLASS_DROP /* macClassID */);

  // resolve blocked neighbor to different MAC address
  this->resolve(this->getIpAddress(), this->kMacAddress2());
  this->verifyMacClassIDHelper(
      this->kMacAddress2(),
      cfg::AclLookupClass::CLASS_DROP,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
      cfg::AclLookupClass::CLASS_DROP /* macClassID */);

  // unblock neighbor
  updateBlockedNeighbor(this->getSw(), {});
  this->verifyMacClassIDHelper(
      this->kMacAddress2(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);
}

TYPED_TEST(LookupClassUpdaterNeighborTest, BlockThenUnblockMultipleNeighbors) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolve(this->getIpAddress2(), this->kMacAddress2());

  // neighbor1 unblocked, neighbor2 unblocked
  this->verifyMultipleBlockedNeighborHelper(
      {},
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

  // neighbor1 blocked, neighbor2 unblocked
  this->verifyMultipleBlockedNeighborHelper(
      {this->getIpAddress()},
      cfg::AclLookupClass::CLASS_DROP,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

  // neighbor1 blocked, neighbor2 blocked
  this->verifyMultipleBlockedNeighborHelper(
      {this->getIpAddress(), this->getIpAddress2()},
      cfg::AclLookupClass::CLASS_DROP,
      cfg::AclLookupClass::CLASS_DROP);

  // neighbor1 unblocked, neighbor2 blocked
  this->verifyMultipleBlockedNeighborHelper(
      {this->getIpAddress2()},
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      cfg::AclLookupClass::CLASS_DROP);

  // neighbor1 unblocked, neighbor2 unblocked
  this->verifyMultipleBlockedNeighborHelper(
      {},
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
}

// MAC addrs to block unit tests
TYPED_TEST(LookupClassUpdaterNeighborTest, BlockMacThenResolveNeighbor) {
  updateMacAddrsToBlock(this->getSw(), {{this->kVlan(), this->kMacAddress()}});
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
        cfg::AclLookupClass::CLASS_DROP /* macClassID */);
  });
}

TYPED_TEST(LookupClassUpdaterNeighborTest, ResolveNeighborThenBlockMac) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  updateMacAddrsToBlock(this->getSw(), {{this->kVlan(), this->kMacAddress()}});
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
        cfg::AclLookupClass::CLASS_DROP /* macClassID */);
  });
}

TYPED_TEST(LookupClassUpdaterNeighborTest, BlockThenUnblockMultipleMacs) {
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolve(this->getIpAddress2(), this->kMacAddress2());

  // mac1 unblocked, mac2 unblocked
  this->verifyMultipleBlockedMacsHelper(
      {},
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

  // mac1 blocked, mac2 unblocked
  this->verifyMultipleBlockedMacsHelper(
      {this->kMacAddress()},
      cfg::AclLookupClass::CLASS_DROP,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

  // mac1 blocked, mac2 blocked
  this->verifyMultipleBlockedMacsHelper(
      {this->kMacAddress(), this->kMacAddress2()},
      cfg::AclLookupClass::CLASS_DROP,
      cfg::AclLookupClass::CLASS_DROP);

  // mac1 unblocked, mac2 blocked
  this->verifyMultipleBlockedMacsHelper(
      {this->kMacAddress2()},
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      cfg::AclLookupClass::CLASS_DROP);

  // mac1 unblocked, mac2 unblocked
  this->verifyMultipleBlockedMacsHelper(
      {},
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
}

TYPED_TEST(
    LookupClassUpdaterNeighborTest,
    BlockMacResolveNeighborResolveToUnblockedMac) {
  // resolve neighbor
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);

  // block MAC
  updateMacAddrsToBlock(this->getSw(), {{this->kVlan(), this->kMacAddress()}});

  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_DROP,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
      cfg::AclLookupClass::CLASS_DROP /* macClassID */);

  // resolve neighbor to a different MAC address that is NOT blocked.
  // the Neighbor is no longer blocked.
  this->resolve(this->getIpAddress(), this->kMacAddress2());
  this->verifyMacClassIDHelper(
      this->kMacAddress2(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);
}

TYPED_TEST(
    LookupClassUpdaterNeighborTest,
    BlockMacResolveNeighborResolveToAnotherBlockedMac) {
  // resolve neighbor
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);

  // block MACs
  updateMacAddrsToBlock(
      this->getSw(),
      {{this->kVlan(), this->kMacAddress()},
       {this->kVlan(), this->kMacAddress2()}});

  this->verifyMacClassIDHelper(
      this->kMacAddress(),
      cfg::AclLookupClass::CLASS_DROP,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
      cfg::AclLookupClass::CLASS_DROP /* macClassID */);

  // resolve neighbor to a different MAC address that is also blocked.
  // the Neighbor remains blocked
  this->resolve(this->getIpAddress(), this->kMacAddress2());
  this->verifyMacClassIDHelper(
      this->kMacAddress2(),
      cfg::AclLookupClass::CLASS_DROP,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
      cfg::AclLookupClass::CLASS_DROP /* macClassID */);

  // unblock MAC that the neighbor no longer resolves to: neighbor remains
  // blocked
  updateMacAddrsToBlock(this->getSw(), {{this->kVlan(), this->kMacAddress2()}});
  this->verifyMacClassIDHelper(
      this->kMacAddress2(),
      cfg::AclLookupClass::CLASS_DROP,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
      cfg::AclLookupClass::CLASS_DROP /* macClassID */);

  // unblock MAC that the neighbor no longer resolves to: neighbor unblocked
  updateMacAddrsToBlock(this->getSw(), {});
  this->verifyMacClassIDHelper(
      this->kMacAddress2(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      MacEntryType::STATIC_ENTRY);
  this->verifyNeighborClassIDHelper(
      this->getIpAddress(),
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);
}

TYPED_TEST(
    LookupClassUpdaterNeighborTest,
    NoLookupClassBlockMacThenResolveNeighbor) {
  // No lookup classes
  this->updateLookupClasses({});

  updateMacAddrsToBlock(this->getSw(), {{this->kVlan(), this->kMacAddress()}});
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
        cfg::AclLookupClass::CLASS_DROP /* macClassID */);
  });
}

TYPED_TEST(
    LookupClassUpdaterNeighborTest,
    NoLookupClassResolveNeighborThenBlockMac) {
  // No lookup classes
  this->updateLookupClasses({});

  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        std::nullopt /* ipClassID */,
        std::nullopt /* macClassID */);
  });

  updateMacAddrsToBlock(this->getSw(), {{this->kVlan(), this->kMacAddress()}});
  this->verifyStateUpdateAfterNeighborCachePropagation([=]() {
    this->verifyClassIDHelper(
        this->getIpAddress(),
        this->kMacAddress(),
        cfg::AclLookupClass::CLASS_DROP /* ipClassID */,
        cfg::AclLookupClass::CLASS_DROP /* macClassID */);
  });
}

TYPED_TEST(
    LookupClassUpdaterNeighborTest,
    NoLookupClassBlockThenUnblockMultipleMacs) {
  // No lookup classes
  this->updateLookupClasses({});

  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->resolve(this->getIpAddress2(), this->kMacAddress2());

  // mac1 unblocked, mac2 unblocked
  this->verifyMultipleBlockedMacsHelper({}, std::nullopt, std::nullopt);

  // mac1 blocked, mac2 unblocked
  this->verifyMultipleBlockedMacsHelper(
      {this->kMacAddress()}, cfg::AclLookupClass::CLASS_DROP, std::nullopt);

  // mac1 blocked, mac2 blocked
  this->verifyMultipleBlockedMacsHelper(
      {this->kMacAddress(), this->kMacAddress2()},
      cfg::AclLookupClass::CLASS_DROP,
      cfg::AclLookupClass::CLASS_DROP);

  // mac1 unblocked, mac2 blocked
  this->verifyMultipleBlockedMacsHelper(
      {this->kMacAddress2()}, std::nullopt, cfg::AclLookupClass::CLASS_DROP);

  // mac1 unblocked, mac2 unblocked
  this->verifyMultipleBlockedMacsHelper({}, std::nullopt, std::nullopt);
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
        NeighborState::REACHABLE,
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
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);

    this->verifyNeighborClassIDHelper(
        this->getIpAddress2(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1 /* ipClassID */,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1 /* macClassID */);

    this->verifyNeighborClassIDHelper(
        this->getIpAddress3(),
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* ipClassID */,
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0 /* macClassID */);
  });
}

} // namespace facebook::fboss
