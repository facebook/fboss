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
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_clients.h"

#include <folly/IPAddress.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <memory>
#include <optional>

DECLARE_string(wedge_agent_host);
DECLARE_int32(wedge_agent_port);
DECLARE_string(qsfp_service_host);
DECLARE_int32(qsfp_service_port);
DECLARE_int32(bgp_service_port);
namespace facebook::fboss::utils {

template <typename ServiceT>
std::unique_ptr<apache::thrift::Client<ServiceT>> createPlaintextClient(
    ConnectionOptions options,
    folly::EventBase* eb = nullptr) {
  folly::EventBase* socketEb =
      eb ? eb : folly::EventBaseManager::get()->getEventBase();

  auto socket = folly::AsyncSocket::UniquePtr(new folly::AsyncSocket(socketEb));
  socket->setSendTimeout(options.getSendTimeoutMs());
  socket->connect(
      nullptr,
      options.getDstAddr(),
      options.getConnTimeoutMs(),
      options.getSocketOptionMap(),
      options.getSrcAddr());
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  channel->setTimeout(options.getRecvTimeoutMs());
  return std::make_unique<apache::thrift::Client<ServiceT>>(std::move(channel));
}

// client with default dst and options
template <typename ServiceT>
std::unique_ptr<apache::thrift::Client<ServiceT>> createPlaintextClient(
    folly::EventBase* eb = nullptr) {
  return createPlaintextClient<ServiceT>(
      ConnectionOptions::defaultOptions<ServiceT>(), eb);
}

// Prefers encrypted client, creates plaintext client if certs are unavailble
template <typename ServiceT>
std::unique_ptr<apache::thrift::Client<ServiceT>> tryCreateEncryptedClient(
    const ConnectionOptions& options,
    folly::EventBase* eb = nullptr);

template <typename ServiceT>
std::unique_ptr<apache::thrift::Client<ServiceT>> tryCreateEncryptedClient(
    folly::EventBase* eb = nullptr) {
  return tryCreateEncryptedClient<ServiceT>(
      ConnectionOptions::defaultOptions<ServiceT>(), eb);
}

std::unique_ptr<apache::thrift::Client<facebook::fboss::FbossCtrl>>
createWedgeAgentClient(
    const std::optional<folly::SocketAddress>& dstAddr = std::nullopt,
    folly::EventBase* eb = nullptr);

std::unique_ptr<apache::thrift::Client<facebook::fboss::QsfpService>>
createQsfpServiceClient(
    const std::optional<folly::SocketAddress>& dstAddr = std::nullopt,
    folly::EventBase* eb = nullptr);

std::unique_ptr<apache::thrift::Client<facebook::fboss::fsdb::FsdbService>>
createFsdbClient(
    ConnectionOptions options =
        ConnectionOptions::defaultOptions<facebook::fboss::fsdb::FsdbService>(),
    folly::EventBase* eb = nullptr);
} // namespace facebook::fboss::utils
