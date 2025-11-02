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

#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/agent/if/gen-cpp2/TestCtrlAsyncClient.h"

namespace facebook::fboss::utility {

std::unique_ptr<apache::thrift::Client<facebook::fboss::TestCtrl>>
getSwAgentThriftClient(const std::string& switchName);
std::unique_ptr<apache::thrift::Client<FbossHwCtrl>> getHwAgentThriftClient(
    const std::string& switchName,
    int port);

MultiSwitchRunState getMultiSwitchRunState(const std::string& switchName);
int getNumHwSwitches(const std::string& switchName);

std::map<std::string, FabricEndpoint> getFabricPortToFabricEndpoint(
    const std::string& switchName);
std::map<std::string, std::vector<std::string>> getRemoteSwitchToReachablePorts(
    const std::string& switchName,
    const std::vector<std::string>& remoteSwitches);

std::map<int32_t, PortInfoThrift> getPortIdToPortInfo(
    const std::string& switchName);
std::map<int64_t, facebook::fboss::SystemPortThrift>
getSystemPortdIdToSystemPort(const std::string& switchName);

std::map<int32_t, facebook::fboss::InterfaceDetail> getIntfIdToIntf(
    const std::string& switchName);
std::vector<facebook::fboss::NdpEntryThrift> getNdpEntries(
    const std::string& switchName);

std::vector<facebook::fboss::DsfSessionThrift> getDsfSessions(
    const std::string& switchName);

void triggerGracefulAgentRestart(const std::string& switchName);

} // namespace facebook::fboss::utility
