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
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

#include <string>

namespace facebook::fboss::utils {

enum class ObjectArgTypeId : uint8_t {
  OBJECT_ARG_TYPE_ID_NONE = 0,
  OBJECT_ARG_TYPE_ID_IPV6_LIST,
  OBJECT_ARG_TYPE_ID_PORT_LIST,
};

static auto constexpr kConnTimeout = 1000;
static auto constexpr kRecvTimeout = 45000;
static auto constexpr kSendTimeout = 5000;

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createAgentClient(
    const std::string& ip);

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient>
createPlaintextAgentClient(const std::string& ip);

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> createQsfpClient(
    const std::string& ip);

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient>
createPlaintextQsfpClient(const std::string& ip);

const folly::IPAddress getIPFromHost(const std::string& hostname);
std::vector<std::string> getHostsInSmcTier(const std::string& parentTierName);
std::vector<std::string> getHostsFromFile(const std::string& filename);

void setLogLevel(std::string logLevelStr);

void logUsage(const std::string& cmdName);

} // namespace facebook::fboss::utils
