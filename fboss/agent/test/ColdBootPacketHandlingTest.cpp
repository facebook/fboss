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

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestPacketFactory.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;

using ::testing::_;

namespace {

const folly::MacAddress kMac1("02:01:02:04:06:08");
const folly::IPAddressV4 kIPv4Addr1("10.0.0.1");
const folly::IPAddressV4 kIPv4Addr2("10.0.0.2");
const folly::IPAddressV6 kIPv6Addr1("face:b00c::1");
const folly::IPAddressV6 kIPv6Addr2("face:b00c::2");
// Multicast addresses
const folly::IPAddressV4 kIPv4McastAddr("224.0.0.1");
const folly::IPAddressV6 kIPv6McastAddr("ff00::1");
// Link local addresses
const folly::IPAddressV4 kIPv4LinkLocalAddr("169.254.1.1");
const folly::IPAddressV6 kIPv6LinkLocalAddr("fe80::1");
} // namespace

class ColdBootPacketHandlingFixture : public ::testing::Test {
 public:
  void SetUp() override {
    handle_ = createTestHandle(getColdBootState(), SwitchFlags::ENABLE_TUN);
  }
  void TearDown() override {
    handle_.reset();
  }
  SwSwitch* getSw() {
    return handle_->getSw();
  }

 protected:
  void packetSendUnknownInterface(const folly::IOBuf& buf) {
    CounterCache counters(getSw());
    auto pkt = createTxPacket(getSw(), buf);

    // Cold boot state does not have any interface. Thus, we inject packet via
    // some unknown interface and assert that the packet does not get sent out.
    // This verifies that agent can handle such packets (pkts dropped) without
    // crashing.
    EXPECT_HW_CALL(getSw(), sendPacketSwitchedAsync_(_)).Times(0);
    getSw()->sendL3Packet(std::move(pkt), InterfaceID(2000));
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.tx.sum", 0);
  }

 private:
  std::shared_ptr<SwitchState> getColdBootState() const {
    auto coldBootState = make_shared<SwitchState>();
    addSwitchInfo(coldBootState);
    state::PortFields portFields1;
    portFields1.portId() = PortID(1);
    portFields1.portName() = "port1";
    auto port1 = std::make_shared<Port>(std::move(portFields1));
    state::PortFields portFields2;
    portFields2.portId() = PortID(2);
    portFields2.portName() = "port2";
    auto port2 = std::make_shared<Port>(std::move(portFields2));
    std::array<std::shared_ptr<Port>, 2> ports{port1, port2};
    // On cold boot all ports are in Vlan 1
    auto vlan = make_shared<Vlan>(VlanID(1), std::string("InitVlan"));
    Vlan::MemberPorts memberPorts;
    auto matcher = HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID{0}}));
    for (const auto& port : ports) {
      coldBootState->getPorts()->addNode(port, matcher);
      memberPorts.insert(make_pair(port->getID(), false));
    }
    vlan->setPorts(memberPorts);
    coldBootState->getVlans()->addNode(vlan, matcher);
    return coldBootState;
  }
  std::unique_ptr<HwTestHandle> handle_{nullptr};
};

TEST_F(ColdBootPacketHandlingFixture, v4PacketUnknownInterface) {
  packetSendUnknownInterface(createV4Packet(
      kIPv4Addr1,
      kIPv4Addr2,
      MockPlatform::getMockLocalMac(),
      MockPlatform::getMockLocalMac()));
}

TEST_F(ColdBootPacketHandlingFixture, v6PacketUnknownInterface) {
  packetSendUnknownInterface(createV6Packet(
      kIPv6Addr1,
      kIPv6Addr2,
      MockPlatform::getMockLocalMac(),
      MockPlatform::getMockLocalMac()));
}

TEST_F(ColdBootPacketHandlingFixture, v4McastPacketUnknownInterface) {
  packetSendUnknownInterface(createV4Packet(
      kIPv4Addr1, kIPv4McastAddr, kMac1, MockPlatform::getMockLocalMac()));
}

TEST_F(ColdBootPacketHandlingFixture, v6McastPacketUnknownInterface) {
  packetSendUnknownInterface(createV6Packet(
      kIPv6Addr1, kIPv6McastAddr, kMac1, MockPlatform::getMockLocalMac()));
}

TEST_F(ColdBootPacketHandlingFixture, v4LinkLocalPacketUnknownInterface) {
  packetSendUnknownInterface(createV4Packet(
      kIPv4Addr1, kIPv4LinkLocalAddr, kMac1, MockPlatform::getMockLocalMac()));
}

TEST_F(ColdBootPacketHandlingFixture, v6LinkLocalPacketUnknownInterface) {
  packetSendUnknownInterface(createV6Packet(
      kIPv6Addr1, kIPv6LinkLocalAddr, kMac1, MockPlatform::getMockLocalMac()));
}
