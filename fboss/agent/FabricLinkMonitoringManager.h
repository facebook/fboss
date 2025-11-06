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

 private:
  void timeoutExpired() noexcept override;

  SwSwitch* sw_;
  std::chrono::milliseconds intervalMsecs_;
};

} // namespace facebook::fboss
