// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricLinkMonitoringManager.h"

#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include <folly/lang/Bits.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <cstring>
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"

DEFINE_int32(
    fabric_link_monitoring_interval_ms,
    1000,
    "Interval in milliseconds for sending fabric link monitoring packets");

using folly::MacAddress;
using folly::io::RWPrivateCursor;

namespace facebook::fboss {

FabricLinkMonitoringManager::FabricLinkMonitoringManager(SwSwitch* sw)
    : folly::AsyncTimeout(sw->getBackgroundEvb()),
      sw_(sw),
      intervalMsecs_(FLAGS_fabric_link_monitoring_interval_ms) {}

FabricLinkMonitoringManager::~FabricLinkMonitoringManager() {}

void FabricLinkMonitoringManager::start() {
  sw_->getBackgroundEvb()->runInFbossEventBaseThread(
      [this] { this->timeoutExpired(); });
}

void FabricLinkMonitoringManager::stop() {
  sw_->getBackgroundEvb()->runInFbossEventBaseThreadAndWait(
      [this] { this->cancelTimeout(); });
}

void FabricLinkMonitoringManager::timeoutExpired() noexcept {
  try {
    // Placeholder for sending packets
    XLOG(DBG4) << "FabricLinkMonitoring timeout expired";
  } catch (const std::exception& ex) {
    XLOG(ERR) << "FabricLinkMonitoring: Failed to send fabric link "
              << "monitoring packets. Error: " << folly::exceptionStr(ex);
  }
  scheduleTimeout(intervalMsecs_);
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
