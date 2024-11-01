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
std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createClient(
    const HostInfo& hostInfo,
    const std::chrono::milliseconds& timeout) {
  return utils::createAgentClient(hostInfo, timeout);
}

template <>
std::unique_ptr<apache::thrift::Client<FbossCtrl>> createClient(
    const HostInfo& hostInfo,
    int switchIndex) {
  return utils::createAgentClient(hostInfo, switchIndex);
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

template <>
std::unique_ptr<
    apache::thrift::Client<facebook::fboss::led_service::LedService>>
createClient(const HostInfo& hostInfo) {
  return utils::createLedClient(hostInfo);
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
    } catch (const std::exception&) {
      // skip switches that cannot be reached
    }
  }
}

void runOnAllHwAgents(const HostInfo& hostInfo, RunForAgentFn fn) {
  auto numHwSwitches = getNumHwSwitches(hostInfo);
  for (int i = 0; i < numHwSwitches; i++) {
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo, i);
    try {
      fn(*client);
    } catch (const std::exception&) {
      // skip switches that cannot be reached
    }
  }
}
} // namespace facebook::fboss::utils
