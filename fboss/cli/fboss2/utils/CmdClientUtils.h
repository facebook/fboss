/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/led_service/if/gen-cpp2/LedService.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

#include <chrono>

namespace facebook::fboss::utils {

using RunForHwAgentFn = std::function<void(
    apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client)>;

using RunForAgentFn =
    std::function<void(apache::thrift::Client<FbossCtrl>& client)>;

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createAgentClient(
    const HostInfo& hostInfo);

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createAgentClient(
    const HostInfo& hostInfo,
    const std::chrono::milliseconds& timeout);

std::unique_ptr<apache::thrift::Client<FbossCtrl>> createAgentClient(
    const HostInfo& hostInfo,
    int switchIndex);

std::unique_ptr<apache::thrift::Client<FbossHwCtrl>> createHwAgentClient(
    const HostInfo& hostInfo,
    int switchIndex);

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> createQsfpClient(
    const HostInfo& hostInfo);

std::unique_ptr<
    apache::thrift::Client<facebook::fboss::led_service::LedService>>
createLedClient(const HostInfo& hostInfo);

int getNumHwSwitches(const HostInfo& hostInfo);

void runOnAllHwAgents(const HostInfo& hostInfo, RunForHwAgentFn fn);
void runOnAllHwAgents(const HostInfo& hostInfo, RunForAgentFn fn);

} // namespace facebook::fboss::utils
