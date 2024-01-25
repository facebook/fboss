/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

namespace facebook::fboss::utils {

template <>
std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createClient(
    const HostInfo& hostInfo) {
  return utils::createAgentClient(hostInfo);
}

template <>
std::unique_ptr<apache::thrift::Client<FbossHwCtrl>> createClient(
    const HostInfo& hostInfo,
    int switchIndex) {
  return utils::createHwAgentClient(hostInfo, switchIndex);
}

template <>
std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> createClient(
    const HostInfo& hostInfo) {
  return utils::createQsfpClient(hostInfo);
}

int getNumHwSwitches(const HostInfo& hostInfo) {
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  MultiSwitchRunState runState;
  client->sync_getMultiSwitchRunState(runState);
  return runState.hwIndexToRunState()->size();
}

void runOnAllHwAgents(const HostInfo& hostInfo, RunForHwAgentFn fn) {
  auto numHwSwitches = getNumHwSwitches(hostInfo);
  for (int i = 0; i < numHwSwitches; i++) {
    auto client =
        utils::createClient<apache::thrift::Client<FbossHwCtrl>>(hostInfo, i);
    try {
      fn(*client);
    } catch (const std::exception& ex) {
      // skip switches that cannot be reached
    }
  }
}
} // namespace facebook::fboss::utils
