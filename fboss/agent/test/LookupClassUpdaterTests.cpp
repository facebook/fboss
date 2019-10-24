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
  const VlanID kVlan1{1};
  const PortID kPort1{1};
  auto kIp4Addr = IPAddressV4("10.0.0.2");
  auto kIp6Addr = IPAddressV6("2401:db00:2110:3001::0002");

  using NeighborTableT = std::conditional_t<
      std::is_same<TypeParam, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  /*
   * Cause a neighbor entry to resolve by receiving appropriate ARP/NDP, and
   * assert if valid CLASSID is associated with the newly resolved neighbor.
   */

  if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
    this->sw_->getNeighborUpdater()->receivedArpMine(
        kVlan1,
        kIp4Addr,
        MacAddress("01:02:03:04:05:06"),
        PortDescriptor(kPort1),
        ArpOpCode::ARP_OP_REPLY);
  } else {
    this->sw_->getNeighborUpdater()->receivedNdpMine(
        kVlan1,
        kIp6Addr,
        MacAddress("01:02:03:04:05:06"),
        PortDescriptor(kPort1),
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
        0);
  }

  this->sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForStateUpdates(this->sw_);

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto vlan = state->getVlans()->getVlan(kVlan1);
    auto neighborTable = vlan->template getNeighborTable<NeighborTableT>();

    if constexpr (std::is_same<TypeParam, folly::IPAddressV4>::value) {
      auto entry = neighborTable->getEntry(kIp4Addr);

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac()
                << " class: " << static_cast<int>(entry->getClassID().value());
      EXPECT_EQ(
          entry->getClassID(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
    } else {
      auto entry = neighborTable->getEntry(kIp6Addr);

      XLOG(DBG) << "ip: " << entry->getIP() << " mac " << entry->getMac()
                << " class: " << static_cast<int>(entry->getClassID().value());
      EXPECT_EQ(
          entry->getClassID(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);
    }
  });
}
} // namespace fboss
} // namespace facebook
