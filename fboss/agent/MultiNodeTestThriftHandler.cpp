/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/MultiNodeTestThriftHandler.h"

#include "fboss/agent/Utils.h"

namespace facebook::fboss {

MultiNodeTestThriftHandler::MultiNodeTestThriftHandler(SwSwitch* sw)
    : ThriftHandler(sw) {}

void MultiNodeTestThriftHandler::gracefullyRestartService(
    std::unique_ptr<std::string> serviceName) {
  XLOG(INFO) << __func__;

  static std::set<std::string> kServicesSupportingGracefulRestart = {
      "wedge_agent_multinode_test",
      "fsdb",
      "qsfp_service",
      "bgp",
  };
  if (kServicesSupportingGracefulRestart.find(*serviceName) ==
      kServicesSupportingGracefulRestart.end()) {
    throw std::runtime_error(folly::to<std::string>(
        "Failed to restart gracefully. Unsupported service: ", *serviceName));
  }

  auto cmd = folly::to<std::string>("systemctl restart ", *serviceName);
  runShellCmd(cmd);
}

void MultiNodeTestThriftHandler::ungracefullyRestartService(
    std::unique_ptr<std::string> serviceName) {
  XLOG(INFO) << __func__;

  static std::set<std::string> kServicesSupportingUngracefulRestart = {
      "wedge_agent_multinode_test",
      "qsfp_service",
  };
  if (kServicesSupportingUngracefulRestart.find(*serviceName) ==
      kServicesSupportingUngracefulRestart.end()) {
    throw std::runtime_error(folly::to<std::string>(
        "Failed to restart ungracefully. Unsupported service: ", *serviceName));
  }

  std::string fileToCreate;
  if (*serviceName == "wedge_agent_multinode_test") {
    fileToCreate = "/dev/shm/fboss/warm_boot/cold_boot_once_0";
  } else if (*serviceName == "qsfp_service") {
    fileToCreate = "/dev/shm/fboss/qsfp_service/cold_boot_once_qsfp_service";
  }

  auto cmd = folly::to<std::string>(
      "touch ", fileToCreate, " && systemctl restart ", *serviceName);
  runShellCmd(cmd);
}

void MultiNodeTestThriftHandler::gracefullyRestartServiceWithDelay(
    std::unique_ptr<std::string> serviceName,
    int32_t delayInSeconds) {
  XLOG(INFO) << __func__;

  if (*serviceName != "wedge_agent_multinode_test") {
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

} // namespace facebook::fboss
