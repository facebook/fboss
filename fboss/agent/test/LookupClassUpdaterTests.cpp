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

namespace facebook {
namespace fboss {

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
  }

  void unresolveNeighbor(IPAddress ipAddress) {
    sw_->getNeighborUpdater()->flushEntry(kVlan(), ipAddress);

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
  }

  void verifyClassIDHelper(
      folly::IPAddress ipAddress,
      cfg ::AclLookupClass classID) {
    using NeighborTableT = std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    auto state = sw_->getState();
    auto vlan = state->getVlans()->getVlan(kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      auto entry = neighborTable->getEntry(ipAddress.asV4());

      EXPECT_TRUE(entry->getClassID().has_value());
      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac()
                << " class: " << static_cast<int>(entry->getClassID().value());
      EXPECT_EQ(entry->getClassID(), classID);
    } else {
      auto entry = neighborTable->getEntry(ipAddress.asV6());

      EXPECT_TRUE(entry->getClassID().has_value());
      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac()
                << " class: " << static_cast<int>(entry->getClassID().value());
      EXPECT_EQ(entry->getClassID(), classID);
    }
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

TYPED_TEST_CASE(LookupClassUpdaterTest, TestTypes);

TYPED_TEST(LookupClassUpdaterTest, VerifyClassID) {
  using NeighborTableT = std::conditional_t<
      std::is_same<TypeParam, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  IPAddress ipAddress;
  if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
    ipAddress = IPAddress(this->kIp4Addr());
  } else {
    ipAddress = IPAddress(this->kIp6Addr());
  }

  this->resolveNeighbor(ipAddress, this->kMacAddress());

  this->verifyStateUpdate([=]() {
    this->verifyClassIDHelper(
        ipAddress, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  });
}

TYPED_TEST(LookupClassUpdaterTest, VerifyClassIDPortDown) {
  using NeighborTableT = std::conditional_t<
      std::is_same<TypeParam, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  IPAddress ipAddress;
  if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
    ipAddress = IPAddress(this->kIp4Addr());
  } else {
    ipAddress = IPAddress(this->kIp6Addr());
  }

  this->resolveNeighbor(ipAddress, this->kMacAddress());

  /*
   * Force a port down, this will cause previously resolved neighbor to go to
   * pending state, and the classID should no longer be associated with the
   * port. Assert for it.
   */

  this->sw_->linkStateChanged(this->kPortID(), false);

  waitForStateUpdates(this->sw_);
  this->sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(this->sw_);
  waitForStateUpdates(this->sw_);

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto vlan = state->getVlans()->getVlan(this->kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
      auto entry = neighborTable->getEntry(this->kIp4Addr());

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac();
      EXPECT_FALSE(entry->getClassID().has_value());
    } else {
      auto entry = neighborTable->getEntry(this->kIp6Addr());

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac();
      EXPECT_FALSE(entry->getClassID().has_value());
    }
  });
}

TYPED_TEST(LookupClassUpdaterTest, LookupClassesToNoLookupClasses) {
  using NeighborTableT = std::conditional_t<
      std::is_same<TypeParam, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  IPAddress ipAddress;
  if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
    ipAddress = IPAddress(this->kIp4Addr());
  } else {
    ipAddress = IPAddress(this->kIp6Addr());
  }

  this->resolveNeighbor(ipAddress, this->kMacAddress());

  this->updateState(
      "Remove lookupclasses", [=](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        auto newPortMap = newState->getPorts()->modify(&newState);

        for (auto port : *newPortMap) {
          auto newPort = port->clone();
          newPort->setLookupClassesToDistributeTrafficOn({});
          newPortMap->updatePort(newPort);
        }
        return newState;
      });

  waitForStateUpdates(this->sw_);
  this->sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(this->sw_);
  waitForStateUpdates(this->sw_);

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto vlan = state->getVlans()->getVlan(this->kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
      auto entry = neighborTable->getEntry(this->kIp4Addr());

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac();
      EXPECT_FALSE(entry->getClassID().has_value());
    } else {
      auto entry = neighborTable->getEntry(this->kIp6Addr());

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac();
      EXPECT_FALSE(entry->getClassID().has_value());
    }
  });
}

TYPED_TEST(LookupClassUpdaterTest, LookupClassesChange) {
  using NeighborTableT = std::conditional_t<
      std::is_same<TypeParam, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  IPAddress ipAddress;
  if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
    ipAddress = IPAddress(this->kIp4Addr());
  } else {
    ipAddress = IPAddress(this->kIp6Addr());
  }

  this->resolveNeighbor(ipAddress, this->kMacAddress());

  this->updateState(
      "Remove lookupclasses", [=](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        auto newPortMap = newState->getPorts()->modify(&newState);

        for (auto port : *newPortMap) {
          auto newPort = port->clone();
          newPort->setLookupClassesToDistributeTrafficOn(
              {cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3,
               cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4});

          newPortMap->updatePort(newPort);
        }
        return newState;
      });

  waitForStateUpdates(this->sw_);
  this->sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(this->sw_);
  waitForStateUpdates(this->sw_);

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto vlan = state->getVlans()->getVlan(this->kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
      auto entry = neighborTable->getEntry(this->kIp4Addr());

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac();
      EXPECT_TRUE(entry->getClassID().has_value());
      EXPECT_TRUE(
          entry->getClassID().value() ==
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3 ||
          entry->getClassID().value() ==
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4);

    } else {
      auto entry = neighborTable->getEntry(this->kIp6Addr());

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac();
      EXPECT_TRUE(entry->getClassID().has_value());
      EXPECT_TRUE(
          entry->getClassID().value() ==
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3 ||
          entry->getClassID().value() ==
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4);
    }
  });
}

TYPED_TEST(LookupClassUpdaterTest, VerifyClassIDSameMacDifferentIPs) {
  IPAddress ipAddress;
  IPAddress ipAddress2;
  if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
    ipAddress = IPAddress(this->kIp4Addr());
    ipAddress2 = IPAddress(this->kIp4Addr2());
  } else {
    ipAddress = IPAddress(this->kIp6Addr());
    ipAddress2 = IPAddress(this->kIp6Addr2());
  }

  this->resolveNeighbor(ipAddress, this->kMacAddress());
  this->resolveNeighbor(ipAddress2, this->kMacAddress());

  // Two IPs with same MAC get same classID
  this->verifyStateUpdate([=]() {
    this->verifyClassIDHelper(
        ipAddress, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    this->verifyClassIDHelper(
        ipAddress2, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    // Verify that refCnt is 2 = 1 for ipAddress + 1 for ipAddress2
    auto lookupClassUpdater = this->sw_->getLookupClassUpdater();
    EXPECT_EQ(
        lookupClassUpdater->getRefCnt(
            this->kPortID(),
            this->kMacAddress(),
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0),
        2);
  });
}

TYPED_TEST(LookupClassUpdaterTest, ResolveUnresolveResolve) {
  using NeighborTableT = std::conditional_t<
      std::is_same<TypeParam, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  IPAddress ipAddress;
  IPAddress ipAddress2;
  if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
    ipAddress = IPAddress(this->kIp4Addr());
    ipAddress2 = IPAddress(this->kIp4Addr2());
  } else {
    ipAddress = IPAddress(this->kIp6Addr());
    ipAddress2 = IPAddress(this->kIp6Addr2());
  }

  auto lookupClassUpdater = this->sw_->getLookupClassUpdater();

  this->resolveNeighbor(ipAddress, this->kMacAddress());
  this->resolveNeighbor(ipAddress2, this->kMacAddress());

  // Two IPs with same MAC get same classID
  this->verifyStateUpdate([=]() {
    this->verifyClassIDHelper(
        ipAddress, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    this->verifyClassIDHelper(
        ipAddress2, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    // Verify that refCnt is 2 = 1 for ipAddress + 1 for ipAddress2
    EXPECT_EQ(
        lookupClassUpdater->getRefCnt(
            this->kPortID(),
            this->kMacAddress(),
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0),
        2);
  });

  this->unresolveNeighbor(ipAddress);

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto vlan = state->getVlans()->getVlan(this->kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
      EXPECT_EQ(neighborTable->getEntryIf(ipAddress.asV4()), nullptr);
    } else {
      EXPECT_EQ(neighborTable->getEntryIf(ipAddress.asV6()), nullptr);
    }

    // Verify that refCnt is 1 = 1 for ipAddress2 as ipAddress is unersolved
    EXPECT_EQ(
        lookupClassUpdater->getRefCnt(
            this->kPortID(),
            this->kMacAddress(),
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0),
        1);

  });

  // Resolve the IP with same MAC, gets same classID as other IP with same MAC
  this->resolveNeighbor(ipAddress, this->kMacAddress());

  this->verifyStateUpdate([=]() {
    this->verifyClassIDHelper(
        ipAddress, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    this->verifyClassIDHelper(
        ipAddress2, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    // Verify that refCnt is 2 = 1 for ipAddress + 1 for ipAddress2
    EXPECT_EQ(
        lookupClassUpdater->getRefCnt(
            this->kPortID(),
            this->kMacAddress(),
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0),
        2);
  });
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
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return this->kIp4Addr();
    } else {
      return this->kIp6Addr();
    }
  }
};

TYPED_TEST_CASE(LookupClassUpdaterWarmbootTest, TestTypes);

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
  IPAddress ipAddress;
  IPAddress ipAddress2;
  IPAddress ipAddress3;

  if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
    ipAddress = IPAddress(this->kIp4Addr());
    ipAddress2 = IPAddress(this->kIp4Addr2());
    ipAddress3 = IPAddress(this->kIp4Addr3());
  } else {
    ipAddress = IPAddress(this->kIp6Addr());
    ipAddress2 = IPAddress(this->kIp6Addr2());
    ipAddress3 = IPAddress(this->kIp6Addr3());
  }

  this->resolveNeighbor(ipAddress2, this->kMacAddress2());
  this->resolveNeighbor(ipAddress3, this->kMacAddress());

  this->verifyStateUpdate([=]() {
    this->verifyClassIDHelper(
        ipAddress, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

    this->verifyClassIDHelper(
        ipAddress2, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);

    this->verifyClassIDHelper(
        ipAddress3, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
  });
}

} // namespace fboss
} // namespace facebook
