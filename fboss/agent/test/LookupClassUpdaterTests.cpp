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

  void resolveNeighbor() {
    /*
     * Cause a neighbor entry to resolve by receiving appropriate ARP/NDP, and
     * assert if valid CLASSID is associated with the newly resolved neighbor.
     */
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      sw_->getNeighborUpdater()->receivedArpMine(
          kVlan(),
          kIp4Addr(),
          MacAddress("01:02:03:04:05:06"),
          PortDescriptor(kPortID()),
          ArpOpCode::ARP_OP_REPLY);
    } else {
      sw_->getNeighborUpdater()->receivedNdpMine(
          kVlan(),
          kIp6Addr(),
          MacAddress("01:02:03:04:05:06"),
          PortDescriptor(kPortID()),
          ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
          0);
    }

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
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

  this->resolveNeighbor();

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto vlan = state->getVlans()->getVlan(this->kVlan());
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
      auto entry = neighborTable->getEntry(this->kIp4Addr());

      EXPECT_TRUE(entry->getClassID().hasValue());
      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac()
                << " class: " << static_cast<int>(entry->getClassID().value());
      EXPECT_EQ(
          entry->getClassID(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
    } else {
      auto entry = neighborTable->getEntry(this->kIp6Addr());

      EXPECT_TRUE(entry->getClassID().hasValue());
      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac()
                << " class: " << static_cast<int>(entry->getClassID().value());
      EXPECT_EQ(
          entry->getClassID(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
    }
  });
}

TYPED_TEST(LookupClassUpdaterTest, VerifyClassIDPortDown) {
  using NeighborTableT = std::conditional_t<
      std::is_same<TypeParam, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  this->resolveNeighbor();

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
      EXPECT_FALSE(entry->getClassID().hasValue());
    } else {
      auto entry = neighborTable->getEntry(this->kIp6Addr());

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac();
      EXPECT_FALSE(entry->getClassID().hasValue());
    }
  });
}

TYPED_TEST(LookupClassUpdaterTest, LookupClassesToNoLookupClasses) {
  using NeighborTableT = std::conditional_t<
      std::is_same<TypeParam, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  this->resolveNeighbor();

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
      EXPECT_FALSE(entry->getClassID().hasValue());
    } else {
      auto entry = neighborTable->getEntry(this->kIp6Addr());

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac();
      EXPECT_FALSE(entry->getClassID().hasValue());
    }
  });
}

TYPED_TEST(LookupClassUpdaterTest, LookupClassesChange) {
  using NeighborTableT = std::conditional_t<
      std::is_same<TypeParam, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  this->resolveNeighbor();

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
      EXPECT_TRUE(entry->getClassID().hasValue());
      EXPECT_TRUE(
          entry->getClassID().value() ==
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3 ||
          entry->getClassID().value() ==
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4);

    } else {
      auto entry = neighborTable->getEntry(this->kIp6Addr());

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac();
      EXPECT_TRUE(entry->getClassID().hasValue());
      EXPECT_TRUE(
          entry->getClassID().value() ==
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3 ||
          entry->getClassID().value() ==
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4);
    }
  });
}

} // namespace fboss
} // namespace facebook
