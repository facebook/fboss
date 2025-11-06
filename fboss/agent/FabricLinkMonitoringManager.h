// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/MacAddress.h>
#include <folly/Synchronized.h>
#include <folly/io/async/AsyncTimeout.h>
#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <vector>

#include "fboss/agent/types.h"

namespace folly {
namespace io {
class Cursor;
}
} // namespace folly

namespace facebook::fboss {

class SwSwitch;
class RxPacket;
class TxPacket;
class Port;

// FabricLinkMonitoringManager sends periodic monitoring packets on fabric ports
// to verify link health and track packet loss
class FabricLinkMonitoringManager : private folly::AsyncTimeout {
 public:
  explicit FabricLinkMonitoringManager(SwSwitch* sw);
  ~FabricLinkMonitoringManager() override;

  // Disable copy and move operations
  FabricLinkMonitoringManager(const FabricLinkMonitoringManager&) = delete;
  FabricLinkMonitoringManager& operator=(const FabricLinkMonitoringManager&) =
      delete;
  FabricLinkMonitoringManager(FabricLinkMonitoringManager&&) = delete;
  FabricLinkMonitoringManager& operator=(FabricLinkMonitoringManager&&) =
      delete;

  // Start sending monitoring packets
  void start();

  // Stop sending monitoring packets
  void stop();

  // Handle received monitoring packet
  void handlePacket(
      std::unique_ptr<RxPacket> pkt,
      folly::MacAddress dst,
      folly::MacAddress src,
      folly::io::Cursor cursor);

  // Get the payload pattern for a sequence number
  static uint32_t getPayloadPattern(uint64_t sequenceNum);

 private:
  void timeoutExpired() noexcept override;
  void sendPacketsOnAllFabricPorts();
  std::unique_ptr<TxPacket> createMonitoringPacket(
      const PortID& portId,
      uint64_t sequenceNumber);

  // Get port group ID for a given port (ports are grouped by portID % 4)
  int getPortGroup(PortID portId) const;

  // Per-port statistics for tracking packet transmission and reception
  struct FabricLinkMonPortStats {
    uint64_t txCount{0};
    uint64_t rxCount{0};
    uint64_t droppedCount{0};
    uint64_t invalidPayloadCount{0};
    uint64_t noPendingSeqNumCount{0};
    std::deque<uint64_t> pendingSequenceNumbers;
  };

  SwSwitch* sw_;
  std::chrono::milliseconds intervalMsecs_;

  folly::Synchronized<
      std::map<PortID, folly::Synchronized<FabricLinkMonPortStats>>>
      portStats_;
  std::map<PortID, int> portToGroupMap_;
  std::map<int, std::vector<PortID>> portGroupToPortsMap_;
};

} // namespace facebook::fboss
