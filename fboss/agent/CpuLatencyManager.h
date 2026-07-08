// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/Synchronized.h>
#include <folly/io/async/AsyncTimeout.h>
#include <gflags/gflags.h>
#include <gtest/gtest_prod.h>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"

DECLARE_int32(cpu_latency_monitoring_interval_ms);

namespace facebook::fboss {

class SwSwitch;
class RxPacket;
class TxPacket;
class Port;

// CpuLatencyManager sends periodic ICMPv6 probe packets out ethernet
// ports and measures CPU round-trip latency via the IP2ME (CPU_IS_NHOP)
// trap mechanism. Modeled after FabricLinkMonitoringManager.
//
// Packet flow:
//   TX: ICMPv6 probe sent out ethernet port P, dst_ip = switch's own
//       interface IP on port P, so the router neighbor will forward it back.
//   RX: ASIC traps it via SAI_HOSTIF_TRAP_TYPE_IP2ME ->
//       IPv6Handler::handleICMPv6Packet() dispatches by type ->
//       CpuLatencyManager::handlePacket(), which computes round-trip latency.
//
// Identification: ICMPv6 type ICMPV6_TYPE_CPU_LATENCY_PROBE (160).
class CpuLatencyManager : private folly::AsyncTimeout {
 public:
  explicit CpuLatencyManager(SwSwitch* sw);
  ~CpuLatencyManager() override;

  CpuLatencyManager(const CpuLatencyManager&) = delete;
  CpuLatencyManager& operator=(const CpuLatencyManager&) = delete;
  CpuLatencyManager(CpuLatencyManager&&) = delete;
  CpuLatencyManager& operator=(CpuLatencyManager&&) = delete;

  // Schedule the periodic probe timer.
  void start();

  // Cancel the probe timer and wait for any in-flight callback to complete.
  void stop();

  // Process a received probe packet and update per-port latency stats.
  // Called from IPv6Handler::handleICMPv6Packet() when type matches.
  // Parses the full ICMPv6 payload internally from the packet buffer.
  void handlePacket(std::unique_ptr<RxPacket> pkt);

  // Return a snapshot of stats for one port.
  CpuLatencyPortStats getCpuLatencyPortStats(const PortID& portId) const;

  // Return snapshots for all monitored ports (used for fb303 export).
  std::unordered_map<PortID, CpuLatencyPortStats> getAllCpuLatencyPortStats()
      const;

 private:
  void timeoutExpired() noexcept override;
  void sendProbePackets();

  // Build a single ICMPv6 probe packet with an 8-byte timestamp payload.
  std::unique_ptr<TxPacket> createLatencyPacket(
      const folly::IPAddressV6& dstIp,
      const folly::MacAddress& neighborMac,
      const folly::MacAddress& srcMac,
      const std::optional<VlanID>& vlanId);

  bool isEligiblePort(const std::shared_ptr<Port>& port) const;

  SwSwitch* sw_;
  std::chrono::milliseconds intervalMsecs_;

  folly::Synchronized<std::unordered_map<PortID, CpuLatencyPortStats>>
      portStats_;

  FRIEND_TEST(CpuLatencyManagerTest, PortDiscovery_EligiblePortsRegistered);
  FRIEND_TEST(
      CpuLatencyManagerTest,
      PacketFormat_PayloadFieldsAtCorrectOffsets);
  FRIEND_TEST(CpuLatencyManagerTest, TxPath_SendProbePackets);
  FRIEND_TEST(CpuLatencyManagerTest, HandlePacket_LatencyUpdated);
  FRIEND_TEST(CpuLatencyManagerTest, HandlePacket_PortDownDropped);
  FRIEND_TEST(CpuLatencyManagerTest, HandlePacket_NoVlanLatencyUpdated);
  FRIEND_TEST(CpuLatencyManagerTest, HandlePacket_BogusTimestampDropped);
};

} // namespace facebook::fboss
