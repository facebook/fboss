// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/CpuLatencyManager.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

DEFINE_int32(
    cpu_latency_monitoring_interval_ms,
    1000,
    "Interval in milliseconds between CPU latency probe packets per port");

namespace facebook::fboss {

CpuLatencyManager::CpuLatencyManager(SwSwitch* sw)
    : folly::AsyncTimeout(sw->getBackgroundEvb()),
      sw_(sw),
      intervalMsecs_(FLAGS_cpu_latency_monitoring_interval_ms) {}

CpuLatencyManager::~CpuLatencyManager() = default;

// Schedule the periodic probe timer. Port discovery happens at send time
// by walking switch state each tick (LldpManager pattern). This means
// ports that come up after start(), or ports that flex/merge, are handled
// automatically without restart.
void CpuLatencyManager::start() {
  portStats_.wlock()->clear();

  sw_->getBackgroundEvb()->runInFbossEventBaseThread(
      [this] { this->scheduleTimeout(intervalMsecs_); });
}

void CpuLatencyManager::stop() {
  sw_->getBackgroundEvb()->runInFbossEventBaseThreadAndWait(
      [this] { this->cancelTimeout(); });
}

void CpuLatencyManager::timeoutExpired() noexcept {
  XLOG(INFO) << "CpuLatency: timeoutExpired fired, interval="
             << intervalMsecs_.count() << "ms";
  sendProbePackets();
  scheduleTimeout(intervalMsecs_);
}

// A port qualifies for monitoring if it is an INTERFACE_PORT, operationally
// UP, and has expected neighbor reachability entries.
bool CpuLatencyManager::isEligiblePort(
    const std::shared_ptr<Port>& port) const {
  if (port->getPortType() != cfg::PortType::INTERFACE_PORT) {
    XLOG(INFO) << "CpuLatency: port " << port->getID()
               << " skipped — not INTERFACE_PORT (type="
               << static_cast<int>(port->getPortType()) << ")";
    return false;
  }
  if (!port->isPortUp()) {
    XLOG(INFO) << "CpuLatency: port " << port->getID()
               << " skipped — port not UP";
    return false;
  }
  if (port->getExpectedNeighborValues()->empty()) {
    XLOG(INFO) << "CpuLatency: port " << port->getID()
               << " skipped — no expectedNeighborReachability";
    return false;
  }
  return true;
}

// ---------- Accessors ----------

CpuLatencyPortStats CpuLatencyManager::getCpuLatencyPortStats(
    const PortID& portId) const {
  auto locked = portStats_.rlock();
  auto it = locked->find(portId);
  if (it == locked->end()) {
    throw FbossError("No CPU latency stats for port ", portId);
  }
  return it->second;
}

std::unordered_map<PortID, CpuLatencyPortStats>
CpuLatencyManager::getAllCpuLatencyPortStats() const {
  return *portStats_.rlock();
}

// ---------- Stubs — implemented in later diffs ----------

void CpuLatencyManager::sendProbePackets() {}

// Probe packet layout (8-byte ICMPv6 payload, big-endian):
//   [0..7]   uint64_t txTimestampNs   — steady_clock nanoseconds at send time
//
// ICMPv6 type = ICMPV6_TYPE_CPU_LATENCY_PROBE (160), code = 0.
// The 4-byte unused field between the ICMPv6 header and the payload is
// set to zero (standard ICMPv6 layout).
//
// IP layout: srcIp = dstIp = the switch's own interface IP on this port.
// Using the interface IP as source passes uRPF checks on the router neighbor.
// dstIp triggers IP2ME (CPU_IS_NHOP) trap on return.
std::unique_ptr<TxPacket> CpuLatencyManager::createLatencyPacket(
    const folly::IPAddressV6& dstIp,
    const folly::MacAddress& neighborMac,
    const folly::MacAddress& srcMac,
    const std::optional<VlanID>& vlanId) {
  // 8-byte payload: txTimestampNs(8)
  constexpr uint32_t kPayloadLen = 8;
  // ICMPv6 body = 4-byte unused/reserved + 8-byte payload
  constexpr uint32_t kIcmpBodyLen = 4 + kPayloadLen;

  IPv6Hdr ipv6;
  ipv6.version = 6;
  ipv6.trafficClass = 0;
  ipv6.flowLabel = 0;
  ipv6.payloadLength = ICMPHdr::SIZE + kIcmpBodyLen;
  ipv6.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
  ipv6.hopLimit = 2;
  ipv6.srcAddr = dstIp;
  ipv6.dstAddr = dstIp;

  ICMPHdr icmp6(
      static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_CPU_LATENCY_PROBE), 0, 0);

  auto totalLen =
      ICMPHdr::computeTotalLengthV6(kIcmpBodyLen, vlanId.has_value());
  auto pkt = sw_->allocatePacket(totalLen);
  folly::io::RWPrivateCursor cursor(pkt->buf());

  icmp6.serializeFullPacket(
      &cursor,
      neighborMac,
      srcMac,
      vlanId,
      ipv6,
      kIcmpBodyLen,
      [&](folly::io::RWPrivateCursor* c) {
        // 4 bytes unused/reserved (standard ICMPv6 layout)
        c->writeBE<uint32_t>(0);
        // 8-byte probe payload: send timestamp in nanoseconds
        c->writeBE<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count());
      });

  return pkt;
}

void CpuLatencyManager::handlePacket(std::unique_ptr<RxPacket>) {}

} // namespace facebook::fboss
