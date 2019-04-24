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


#include <memory>
#include <utility>

#include <folly/logging/xlog.h>

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/TestPacketFactory.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/HwTestHandle.h"


using namespace facebook::fboss;
using std::make_shared;
using std::make_pair;

using ::testing::_;

namespace {

const folly::MacAddress kPlatformMac("02:01:02:03:04:05");
const folly::IPAddressV4 kIPv4Addr1("10.0.0.1");
const folly::IPAddressV4 kIPv4Addr2("10.0.0.2");
const folly::IPAddressV6 kIPv6Addr1("face:b00c::1");
const folly::IPAddressV6 kIPv6Addr2("face:b00c::2");
}

class ColdBootPacketHandlingFixture : public ::testing::Test {
 public:
  void SetUp() override {
    handle_ = createTestHandle(getColdBootState(), kPlatformMac, ENABLE_TUN);
  }
  void TearDown() override {
    handle_.reset();
  }
  SwSwitch* getSw() { return handle_->getSw(); }
 protected:
  void packetSendUnknownInterface(const folly::IOBuf& buf) {
    CounterCache counters(getSw());
    auto pkt = createTxPacket(getSw(), buf);

    EXPECT_HW_CALL(getSw(), sendPacketSwitchedAsync_(_)).Times(0);
    getSw()->sendL3Packet(std::move(pkt), InterfaceID(2000));
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  }

 private:
  std::shared_ptr<SwitchState> getColdBootState() const {
    auto coldBootState = make_shared<SwitchState>();
    std::array<std::shared_ptr<Port>, 2> ports{
        make_shared<Port>(PortID(1), "port1"),
        make_shared<Port>(PortID(2), "port2")};
    // On cold boot all ports are in Vlan 1
    auto vlan = make_shared<Vlan>(VlanID(1), "InitVlan");
    Vlan::MemberPorts memberPorts;
    for (const auto& port : ports) {
      coldBootState->addPort(port);
      memberPorts.insert(make_pair(port->getID(), false));
    }
    vlan->setPorts(memberPorts);
    coldBootState->addVlan(vlan);
    return coldBootState;
  }
  std::unique_ptr<HwTestHandle> handle_{nullptr};
};

TEST_F(ColdBootPacketHandlingFixture, v4PacketUnknownInterface) {
  packetSendUnknownInterface(
      createV4Packet(kIPv4Addr1, kIPv4Addr2, kPlatformMac, kPlatformMac));
}

TEST_F(ColdBootPacketHandlingFixture, v6PacketUnknownInterface) {
  packetSendUnknownInterface(
      createV6Packet(kIPv6Addr1, kIPv6Addr2, kPlatformMac, kPlatformMac));
}
