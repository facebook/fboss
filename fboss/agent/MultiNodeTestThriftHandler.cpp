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

void MultiNodeTestThriftHandler::triggerGracefulExit() {
  XLOG(INFO) << __func__;

  // TODO: Get this to work on monolithic and split binary
  auto cmd =
      folly::to<std::string>("systemctl restart wedge_agent_multinode-test");
  runShellCmd(cmd);
}

void MultiNodeTestThriftHandler::triggerUngracefulExit() {
  XLOG(INFO) << __func__;

  // TODO: Get this to work on monolithic and split binary
  auto cmd = folly::to<std::string>(
      "touch /dev/shm/fboss/warm_boot/cold_boot_once_0 && sudo systemctl restart wedge_agent_multinode-test");
  runShellCmd(cmd);
}

} // namespace facebook::fboss
