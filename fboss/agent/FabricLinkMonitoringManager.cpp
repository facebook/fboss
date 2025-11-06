// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricLinkMonitoringManager.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"

#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include <folly/lang/Bits.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <cstring>
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"

DEFINE_int32(
    fabric_link_monitoring_interval_ms,
    1000,
    "Interval in milliseconds for sending fabric link monitoring packets");

using folly::MacAddress;
using folly::io::RWPrivateCursor;

namespace facebook::fboss {

// Cell size of 512B - header size os 32B
constexpr int kFabricLinkMonitoringPacketSize{480};

// Initialize the FabricLinkMonitoringManager with a reference to SwSwitch and
// configure the monitoring interval. Automatically determines the maximum
// outstanding packets based on switch type (VOQ switches: 160 packets per port
// group, Fabric switches: 40 packets per port group).
FabricLinkMonitoringManager::FabricLinkMonitoringManager(SwSwitch* sw)
    : folly::AsyncTimeout(sw->getBackgroundEvb()),
      sw_(sw),
      intervalMsecs_(FLAGS_fabric_link_monitoring_interval_ms) {}

FabricLinkMonitoringManager::~FabricLinkMonitoringManager() {}

// Start fabric link monitoring by clearing all existing state, discovering
// fabric ports from switch state, and grouping them by virtual device ID.
// Filters ports based on switch topology - VOQ switches monitor all fabric
// ports in both single and dual stage topologies. In dual stage topology,
// L1 fabric switches will monitor fabric ports connected to L2 switches.
// L2 switch will not send any fabric link monitoring packets. Periodic
// packet transmission is scheduled on the background event base.
void FabricLinkMonitoringManager::start() {
  portGroupToPortsMap_.clear();
  portToGroupMap_.clear();
  auto portStatsLocked = portStats_.wlock();
  portStatsLocked->clear();

  // Expect fabric ports to not change once the system is configured
  std::shared_ptr<SwitchState> state = sw_->getState();
  for (const auto& portMap : std::as_const(*state->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap.second)) {
      if (port->getPortType() == cfg::PortType::FABRIC_PORT) {
        const PortID portId = port->getID();
        int groupId = getPortGroup(portId);
        portGroupToPortsMap_[groupId].push_back(portId);
        portToGroupMap_[portId] = groupId;
        portStatsLocked->emplace(
            portId, folly::Synchronized<FabricLinkMonPortStats>{});
      }
    }
  }

  sw_->getBackgroundEvb()->runInFbossEventBaseThread(
      [this] { this->timeoutExpired(); });
}

// Stop fabric link monitoring by canceling the periodic timeout on the
// background event base. Ensures no new packets are sent after stop is
// called.
void FabricLinkMonitoringManager::stop() {
  sw_->getBackgroundEvb()->runInFbossEventBaseThreadAndWait(
      [this] { this->cancelTimeout(); });
}

// Timeout callback that executes periodic monitoring cycles. Called
// automatically by AsyncTimeout at configured intervals to send
// monitoring packets on all fabric ports and reschedule itself for
// the next cycle.
void FabricLinkMonitoringManager::timeoutExpired() noexcept {
  try {
    sendPacketsOnAllFabricPorts();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "FabricLinkMonitoring: Failed to send fabric link "
              << "monitoring packets. Error: " << folly::exceptionStr(ex);
  }
  scheduleTimeout(intervalMsecs_);
}

// Get port group ID for a given port. Groups ports by their virtual device
// (VD) ID for outstanding packet management. For switches without virtual
// devices, all ports are considered to be in group 0.
int FabricLinkMonitoringManager::getPortGroup(PortID portId) const {
  int portGroup = 0;
  const auto& pPort = sw_->getPlatformMapping()->getPlatformPort(portId);
  if (pPort.mapping()->virtualDeviceId().has_value()) {
    portGroup = *pPort.mapping()->virtualDeviceId();
  }
  return portGroup;
}

// Get expected payload pattern for a sequence number. Returns alternating
// patterns based on sequence number parity for validation - even and odd
// sequence numbers return a different alternating sequence of 0s and 1s.
// The goal here is to validate integrity of evey bit in the packet in the
// pipeline.
uint32_t FabricLinkMonitoringManager::getPayloadPattern(uint64_t sequenceNum) {
  if (sequenceNum % 2) {
    return 0xA5A5A5A5;
  } else {
    return 0x5A5A5A5A;
  }
}

void FabricLinkMonitoringManager::sendPacketsOnAllFabricPorts() {
  XLOG(DBG4) << "FabricLinkMonitoring: Found " << portToGroupMap_.size()
             << " fabric ports in " << portGroupToPortsMap_.size() << " groups";
  // TODO: Implementation pending
}

// Create a 480-byte monitoring packet with: 8 bytes sequence number
// (big-endian), 4 bytes port ID (big-endian), and rest of the bytes payload
// pattern with alternating 0s and 1s based on sequence number parity.
std::unique_ptr<TxPacket> FabricLinkMonitoringManager::createMonitoringPacket(
    const PortID& portId,
    uint64_t sequenceNumber) {
  uint32_t frameLen = kFabricLinkMonitoringPacketSize;
  auto pkt = sw_->allocatePacket(frameLen);

  RWPrivateCursor cursor(pkt->buf());
  cursor.writeBE<uint64_t>(sequenceNumber);
  cursor.writeBE<uint32_t>(static_cast<uint32_t>(portId));

  uint32_t fillPattern = getPayloadPattern(sequenceNumber);

  // Payload is the space after seq num and port ID
  size_t payloadSize =
      kFabricLinkMonitoringPacketSize - sizeof(uint64_t) - sizeof(uint32_t);
  for (size_t i = 0; i < payloadSize; i += 4) {
    cursor.writeBE<uint32_t>(fillPattern);
  }

  if (cursor.length() > 0) {
    memset(cursor.writableData(), 0, cursor.length());
  }

  return pkt;
}

void FabricLinkMonitoringManager::handlePacket(
    std::unique_ptr<RxPacket> pkt,
    folly::MacAddress /*dst*/,
    folly::MacAddress /*src*/,
    folly::io::Cursor /*cursor*/) {
  // Placeholder for packet handling
  XLOG(DBG4) << "FabricLinkMonitoring: Received packet on port "
             << pkt->getSrcPort();
}

} // namespace facebook::fboss
