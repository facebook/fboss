/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/TestThriftHandler.h"

#include "fboss/agent/NdpCache.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/Utils.h"

using facebook::network::toIPAddress;

namespace {

const std::set<std::string> kServicesSupportingRestart() {
  static const std::set<std::string> servicesSupportingRestart = {
      "wedge_agent_test",
      "fsdb",
      "qsfp_service",
      "bgp",
  };

  return servicesSupportingRestart;
}

} // namespace

namespace facebook::fboss {

TestThriftHandler::TestThriftHandler(SwSwitch* sw) : ThriftHandler(sw) {}

void TestThriftHandler::gracefullyRestartService(
    std::unique_ptr<std::string> serviceName) {
  XLOG(INFO) << __func__;

  if (kServicesSupportingRestart().find(*serviceName) ==
      kServicesSupportingRestart().end()) {
    throw std::runtime_error(folly::to<std::string>(
        "Failed to restart gracefully. Unsupported service: ", *serviceName));
  }

  auto cmd = folly::to<std::string>("systemctl restart ", *serviceName);
  runShellCmd(cmd);
}

void TestThriftHandler::ungracefullyRestartService(
    std::unique_ptr<std::string> serviceName) {
  XLOG(INFO) << __func__;

  if (kServicesSupportingRestart().find(*serviceName) ==
      kServicesSupportingRestart().end()) {
    throw std::runtime_error(folly::to<std::string>(
        "Failed to restart ungracefully. Unsupported service: ", *serviceName));
  }

  if (*serviceName == "wedge_agent_test" || *serviceName == "qsfp_service") {
    std::string fileToCreate = (*serviceName == "wedge_agent_test")
        ? "/dev/shm/fboss/warm_boot/cold_boot_once_0"
        : "/dev/shm/fboss/qsfp_service/cold_boot_once_qsfp_service";

    auto cmd = folly::to<std::string>(
        "touch ", fileToCreate, " && systemctl restart ", *serviceName);
    runShellCmd(cmd);
  } else {
    auto cmd = folly::to<std::string>("pkill -9 ", *serviceName);
    runShellCmd(cmd);
  }
}

void TestThriftHandler::gracefullyRestartServiceWithDelay(
    std::unique_ptr<std::string> serviceName,
    int32_t delayInSeconds) {
  XLOG(INFO) << __func__;

  if (*serviceName != "wedge_agent_test") {
    throw std::runtime_error(folly::to<std::string>(
        "Failed to restart with delay. Unsupported service: ", *serviceName));
  }

  auto unitName = folly::to<std::string>(*serviceName, "_delayed_start");
  // Schedule a restart of service after delay seconds
  auto cmd = folly::to<std::string>(
      "systemd-run --on-active=",
      delayInSeconds,
      "s --unit=",
      unitName,
      "systemctl start ",
      *serviceName);
  std::system(cmd.c_str());

  // Then stop wedge_agent immediately
  cmd = folly::to<std::string>("systemctl stop ", *serviceName);
  std::system(cmd.c_str());
}

void TestThriftHandler::addNeighbor(
    int32_t interfaceID,
    std::unique_ptr<BinaryAddress> ip,
    std::unique_ptr<std::string> mac,
    int32_t portID) {
  ensureConfigured(__func__);

  if (!FLAGS_intf_nbr_tables) {
    throw std::runtime_error(
        "Thrift API addNeighbor is only supported for Interface Neighbor Tables");
  }

  auto neighborIP = toIPAddress(*ip);

  if (!neighborIP.isV6()) {
    throw std::runtime_error(folly::to<std::string>(
        "Thrift API addNeighbor supports IPv6 neighbors only. Neighbor to add:",
        neighborIP));
  }

  auto neighborMac = folly::MacAddress::tryFromString(*mac);
  if (!neighborMac.hasValue()) {
    throw std::runtime_error(folly::to<std::string>(
        "Thrift API addNeighbor: invalid MAC address provided: ", *mac));
  }

  // To resolve a neighbor, mimic receiving NDP Response from neighbor
  getSw()->getNeighborUpdater()->receivedNdpMineForIntf(
      InterfaceID(interfaceID),
      neighborIP.asV6(),
      neighborMac.value(),
      PortDescriptor(PortID(portID)),
      ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
      0 /* flags */);
}

} // namespace facebook::fboss
