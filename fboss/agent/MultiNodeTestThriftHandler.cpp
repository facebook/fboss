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

void MultiNodeTestThriftHandler::triggerGracefulRestart() {
  XLOG(INFO) << __func__;

  // TODO: Get this to work on monolithic and split binary
  auto cmd =
      folly::to<std::string>("systemctl restart wedge_agent_multinode-test");
  runShellCmd(cmd);
}

void MultiNodeTestThriftHandler::triggerUngracefulRestart() {
  XLOG(INFO) << __func__;

  // TODO: Get this to work on monolithic and split binary
  auto cmd = folly::to<std::string>(
      "touch /dev/shm/fboss/warm_boot/cold_boot_once_0 && sudo systemctl restart wedge_agent_multinode-test");
  runShellCmd(cmd);
}

void MultiNodeTestThriftHandler::restartWithDelay(int32_t delayInSeconds) {
  XLOG(INFO) << __func__;

  // Schedule a restart of wedge_agent after delay seconds
  auto cmd = folly::to<std::string>(
      "systemd-run --on-active=",
      delayInSeconds,
      "s --unit=wedge_agent_multinode-test_delayed_start systemctl start wedge_agent_multinode-test");
  std::system(cmd.c_str());

  // Then stop wedge_agent immediately
  cmd = folly::to<std::string>("systemctl stop wedge_agent_multinode-test");
  std::system(cmd.c_str());
}

void MultiNodeTestThriftHandler::gracefullyRestartService(
    std::unique_ptr<std::string> serviceName) {
  return;
}

void MultiNodeTestThriftHandler::ungracefullyRestartService(
    std::unique_ptr<std::string> serviceName) {
  return;
}

void MultiNodeTestThriftHandler::gracefullyRestartServiceWithDelay(
    std::unique_ptr<std::string> serviceName,
    int32_t delayInSeconds) {
  return;
}

} // namespace facebook::fboss
