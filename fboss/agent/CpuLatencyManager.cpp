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
#include "fboss/agent/state/PortDescriptor.h"
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

// A port qualifies for monitoring if it is an INTERFACE_PORT and operationally
// UP.
bool CpuLatencyManager::isEligiblePort(
    const std::shared_ptr<Port>& port) const {
  return port->getPortType() == cfg::PortType::INTERFACE_PORT &&
      port->isPortUp();
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

// Walk switch state each tick, check eligibility, resolve interface/IP/MAC/VLAN
// fresh, and send — all in one pass (LldpManager pattern). Ports that come up
// after start(), or ports that flex/merge, are handled automatically.
void CpuLatencyManager::sendProbePackets() {
  const auto state = sw_->getState();
  for (const auto& portMap : std::as_const(*state->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap.second)) {
      if (!isEligiblePort(port)) {
        XLOG(DBG3) << "CpuLatency: port " << port->getID()
                   << "invalid, skipped send";
        continue;
      }
      const PortID portId = port->getID();

      // Find an interface with a non-link-local IPv6 address for this port.
      std::shared_ptr<Interface> intf;
      folly::IPAddressV6 interfaceIp;
      bool foundIp = false;
      for (const auto intfId : port->getInterfaceIDs()) {
        auto candidate = state->getInterfaces()->getNodeIf(InterfaceID(intfId));
        if (!candidate) {
          continue;
        }
        for (const auto& [addr, mask] : candidate->getAddressesCopy()) {
          if (addr.isV6() && !addr.isLinkLocal()) {
            intf = candidate;
            interfaceIp = addr.asV6();
            foundIp = true;
            break;
          }
        }
        if (foundIp) {
          break;
        }
      }
      if (!foundIp) {
        continue;
      }

      const folly::MacAddress srcMac = intf->getMac();

      // Resolve VLAN via the interface type (follows NDP/RA pattern).
      std::optional<VlanID> vlanId;
      if (intf->getType() == cfg::InterfaceType::VLAN) {
        vlanId = intf->getVlanID();
      }

      // Resolve neighbor MAC from the NDP table, filtering by port so each
      // port sends to the neighbor actually reachable via that port.
      folly::MacAddress neighborMac;
      bool foundNeighbor = false;
      for (const auto& [ip, entry] : std::as_const(*intf->getNdpTable())) {
        if (entry->isReachable() &&
            entry->getPort() == PortDescriptor(portId)) {
          neighborMac = entry->getMac();
          foundNeighbor = true;
          break;
        }
      }
      if (!foundNeighbor) {
        XLOG(DBG4) << "CpuLatency: port " << portId
                   << " skipped send — NDP neighbor not yet reachable";
        continue;
      }

      auto pkt = createLatencyPacket(interfaceIp, neighborMac, srcMac, vlanId);

      if (pkt) {
        sw_->sendPacketOutOfPortAsync(std::move(pkt), portId);

        XLOG(DBG3) << "CpuLatency: port " << portId << " probe sent to "
                   << interfaceIp << " via neighbor " << neighborMac << " vlan="
                   << (vlanId ? folly::to<std::string>(*vlanId) : "none");
      }
    }
  }
}

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
