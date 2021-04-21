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

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

#include <string>

namespace facebook::fboss::utils {

static auto constexpr kConnTimeout = 1000;
static auto constexpr kRecvTimeout = 45000;
static auto constexpr kSendTimeout = 5000;

static auto constexpr kFbossAgentPort = 5909;

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createAgentClient(
    const std::string& ip);

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient>
createPlaintextAgentClient(const std::string& ip);

const folly::IPAddress getIPFromHost(const std::string& hostname);
std::vector<std::string> getHostsInSmcTier(const std::string& parentTierName);
std::vector<std::string> getHostsFromFile(const std::string& filename);

void setLogLevel(std::string logLevelStr);

void logUsage(const std::string& cmdName);

} // namespace facebook::fboss::utils
