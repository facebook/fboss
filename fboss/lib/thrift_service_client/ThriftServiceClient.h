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

#include "fboss/agent/if/gen-cpp2/ctrl_clients.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_clients.h"

#include <folly/IPAddress.h>
#include <folly/io/async/EventBase.h>
#include <memory>
#include <optional>

DECLARE_string(wedge_agent_host);
DECLARE_int32(wedge_agent_port);
DECLARE_string(qsfp_service_host);
DECLARE_int32(qsfp_service_port);

namespace facebook::fboss::utils {

auto constexpr kConnTimeout = 1000;
auto constexpr kRecvTimeout = 45000;
auto constexpr kSendTimeout = 5000;

template <typename Client>
std::unique_ptr<Client> createPlaintextClient(
    const folly::IPAddress& ip,
    const int port,
    folly::EventBase* eb = nullptr);

std::unique_ptr<apache::thrift::Client<facebook::fboss::FbossCtrl>>
createWedgeAgentClient(
    std::optional<folly::IPAddress> ip = std::nullopt,
    std::optional<int> port = std::nullopt,
    folly::EventBase* eb = nullptr);

std::unique_ptr<apache::thrift::Client<facebook::fboss::QsfpService>>
createQsfpServiceClient(
    std::optional<folly::IPAddress> ip = std::nullopt,
    std::optional<int> port = std::nullopt,
    folly::EventBase* eb = nullptr);
} // namespace facebook::fboss::utils
