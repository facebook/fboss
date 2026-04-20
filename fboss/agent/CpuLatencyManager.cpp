// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/CpuLatencyManager.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
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

std::unique_ptr<TxPacket> CpuLatencyManager::createLatencyPacket(
    const folly::IPAddressV6&,
    const folly::MacAddress&,
    const folly::MacAddress&,
    const std::optional<VlanID>&) {
  return nullptr;
}

void CpuLatencyManager::handlePacket(std::unique_ptr<RxPacket>) {}

} // namespace facebook::fboss
