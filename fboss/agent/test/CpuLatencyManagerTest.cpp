// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/CpuLatencyManager.h"

#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include <gtest/gtest.h>
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

namespace {

// Mark a port as eligible for monitoring by giving it a neighbor reachability
// entry.
void makeEligiblePort(std::shared_ptr<SwitchState>& state, PortID portId) {
  auto portMap = state->getPorts()->modify(&state);
  auto port = portMap->getNodeIf(portId)->modify(&state);
  cfg::PortNeighbor neighbor;
  neighbor.remoteSystem() = "router1";
  neighbor.remotePort() = "eth0/1/1";
  port->setExpectedNeighborReachability({neighbor});
}

} // namespace

class CpuLatencyManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // testStateAWithPortsUp() gives us 20 UP INTERFACE_PORT ports.
    // Ports 1-10 are on Interface 1 (VLAN 1), ports 11-20 on Interface 55.
    auto state = testStateAWithPortsUp();
    // Mark ports 1 and 2 as eligible (with neighbor reachability).
    makeEligiblePort(state, PortID(1));
    makeEligiblePort(state, PortID(2));
    handle_ = createTestHandle(state);
    sw_ = handle_->getSw();

    // Add NDP neighbor entries via updateStateBlocking so
    // sendProbePackets() can resolve neighbor MACs without test mode.
    sw_->updateStateBlocking(
        "add NDP neighbors", [](const std::shared_ptr<SwitchState>& state) {
          auto newState = state->clone();
          auto ndpTable = newState->getInterfaces()
                              ->getNode(InterfaceID(1))
                              ->getNdpTable()
                              ->modify(InterfaceID(1), &newState);
          ndpTable->addEntry(
              folly::IPAddressV6("2401:db00:2110:3001::22"),
              folly::MacAddress("02:09:00:00:00:22"),
              PortDescriptor(PortID(1)),
              InterfaceID(1),
              NeighborState::REACHABLE);
          ndpTable->addEntry(
              folly::IPAddressV6("2401:db00:2110:3001::33"),
              folly::MacAddress("02:09:00:00:00:33"),
              PortDescriptor(PortID(2)),
              InterfaceID(1),
              NeighborState::REACHABLE);
          return newState;
        });
  }

  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_{nullptr};
};

// Verify the 8-byte probe payload (timestamp) is at the correct byte offset.
// Packet layout with VLAN: Ethernet(14) + VLAN(4) + IPv6(40) + ICMPv6(4) +
// unused(4) = 66 header bytes, then:
//   [0..7]   txTimestampNs (uint64_t, big-endian — any non-zero value)
TEST_F(CpuLatencyManagerTest, PacketFormat_PayloadFieldsAtCorrectOffsets) {
  auto mgr = std::make_unique<CpuLatencyManager>(sw_);

  const folly::IPAddressV6 dstIp("2001:db8::1");
  const folly::MacAddress neighborMac("02:00:00:00:00:01");
  const folly::MacAddress srcMac("02:00:00:00:00:02");

  // Create packet with VLAN to match production path.
  auto pkt = mgr->createLatencyPacket(dstIp, neighborMac, srcMac, VlanID(1));

  ASSERT_NE(pkt, nullptr);

  folly::io::Cursor cursor(pkt->buf());
  // Ethernet(14) + VLAN(4) + IPv6(40) + ICMPv6 hdr(4) + unused(4) = 66
  cursor.skip(66);

  const uint64_t txTs = cursor.readBE<uint64_t>();
  EXPECT_GT(txTs, 0u); // timestamp must be non-zero
}

// sendProbePackets() walks switch state and sends on eligible ports.
// Stats entries are not created until receive (handlePacket), so after
// a send cycle the stats map should still be empty.
TEST_F(CpuLatencyManagerTest, PortDiscovery_EligiblePortsRegistered) {
  auto mgr = std::make_unique<CpuLatencyManager>(sw_);
  mgr->start();
  mgr->sendProbePackets();

  // Stats are created on receive, not send — map is empty after send only.
  const auto stats = mgr->getAllCpuLatencyPortStats();
  EXPECT_EQ(stats.size(), 0u);
  mgr->stop();
}

// sendProbePackets() completes without error on eligible ports.
TEST_F(CpuLatencyManagerTest, TxPath_SendProbePackets) {
  auto mgr = std::make_unique<CpuLatencyManager>(sw_);
  mgr->start();

  // Should not throw — sends probes on ports 1 and 2 (eligible),
  // skips ports 3-20 (no neighbor reachability).
  EXPECT_NO_THROW(mgr->sendProbePackets());
  mgr->stop();
}

// ---------------------------------------------------------------------------
// RX path helpers and tests
// ---------------------------------------------------------------------------

namespace {

// Build a synthetic RxPacket that looks like one of our probes came back.
// handlePacket detects VLAN presence via ethertype at offset 12, so the
// buffer must include full headers:
//   No VLAN:   Ethernet(14) + IPv6(40) + ICMPv6(4) + unused(4) + timestamp(8) =
//   70 With VLAN: Ethernet(14) + VLAN(4) + IPv6(40) + ICMPv6(4) + unused(4) +
//   timestamp(8) = 74
std::unique_ptr<RxPacket> makeCpuLatencyRxPacket(
    const PortID& portId,
    uint64_t txTimestampNs,
    bool withVlan = true) {
  const size_t kHeaderBytes = withVlan ? 66 : 62;
  const size_t kTotalBytes = kHeaderBytes + 8;
  std::vector<uint8_t> data(kTotalBytes, 0);

  if (withVlan) {
    // Ethernet: dst(6) + src(6) + VLAN ethertype(2)
    data[12] = 0x81;
    data[13] = 0x00; // VLAN ethertype 0x8100
    // VLAN TCI(2) + IPv6 ethertype(2)
    data[16] = 0x86;
    data[17] = 0xDD; // IPv6 ethertype 0x86DD
  } else {
    // Ethernet: dst(6) + src(6) + IPv6 ethertype(2)
    data[12] = 0x86;
    data[13] = 0xDD; // IPv6 ethertype 0x86DD
  }

  // Timestamp payload at the end of the header
  uint64_t ts = htobe64(txTimestampNs);
  std::memcpy(data.data() + kHeaderBytes, &ts, 8);

  auto buf = folly::IOBuf::copyBuffer(data.data(), data.size());
  auto pkt = std::make_unique<MockRxPacket>(std::move(buf));
  pkt->setSrcPort(portId);
  return pkt;
}

} // namespace

// After one send + one matching RX, latencyUs must be > 0.
TEST_F(CpuLatencyManagerTest, HandlePacket_LatencyUpdated) {
  auto mgr = std::make_unique<CpuLatencyManager>(sw_);
  mgr->start();
  mgr->sendProbePackets();

  // Simulate the probe returning on port 1: set txTimestamp 1ms in the past.
  const uint64_t txTs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count() -
      1'000'000ULL; // 1ms ago in nanoseconds
  auto rxPkt = makeCpuLatencyRxPacket(PortID(1), txTs);
  mgr->handlePacket(std::move(rxPkt));

  const auto stats = mgr->getCpuLatencyPortStats(PortID(1));
  EXPECT_GT(*stats.latencyUs(), 0.0);
  mgr->stop();
}

// A probe returning on a port that went down should be silently dropped.
TEST_F(CpuLatencyManagerTest, HandlePacket_PortDownDropped) {
  auto mgr = std::make_unique<CpuLatencyManager>(sw_);
  mgr->start();
  mgr->sendProbePackets();

  // Bring port 1 down.
  sw_->updateStateBlocking(
      "bring port down", [](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        auto portMap = newState->getPorts()->modify(&newState);
        auto port = portMap->getNodeIf(PortID(1))->clone();
        port->setOperState(false);
        portMap->updateNode(
            port,
            SwitchIdScopeResolver(
                utility::getFirstNodeIf(newState->getSwitchSettings())
                    ->getSwitchIdToSwitchInfo())
                .scope(port));
        return newState;
      });

  const uint64_t txTs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count() -
      1'000'000ULL;
  auto rxPkt = makeCpuLatencyRxPacket(PortID(1), txTs);
  mgr->handlePacket(std::move(rxPkt));

  // No stats entry should exist — the packet was dropped.
  const auto allStats = mgr->getAllCpuLatencyPortStats();
  EXPECT_EQ(allStats.count(PortID(1)), 0u);
  mgr->stop();
}

// RX packet without VLAN tag should also produce valid latency.
TEST_F(CpuLatencyManagerTest, HandlePacket_NoVlanLatencyUpdated) {
  auto mgr = std::make_unique<CpuLatencyManager>(sw_);
  mgr->start();
  mgr->sendProbePackets();

  const uint64_t txTs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count() -
      1'000'000ULL;
  auto rxPkt = makeCpuLatencyRxPacket(PortID(1), txTs, false /* withVlan */);
  mgr->handlePacket(std::move(rxPkt));

  const auto stats = mgr->getCpuLatencyPortStats(PortID(1));
  EXPECT_GT(*stats.latencyUs(), 0.0);
  mgr->stop();
}

// A packet with txTimestamp in the future should be silently dropped.
TEST_F(CpuLatencyManagerTest, HandlePacket_BogusTimestampDropped) {
  auto mgr = std::make_unique<CpuLatencyManager>(sw_);
  mgr->start();
  mgr->sendProbePackets();

  const uint64_t futureTs =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count() +
      60'000'000'000ULL; // 60s in the future
  auto rxPkt = makeCpuLatencyRxPacket(PortID(1), futureTs);
  mgr->handlePacket(std::move(rxPkt));

  const auto allStats = mgr->getAllCpuLatencyPortStats();
  EXPECT_EQ(allStats.count(PortID(1)), 0u);
  mgr->stop();
}

} // namespace facebook::fboss
