/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

DEFINE_string(wedge_agent_host, "::1", "Host running wedge_agent");
DEFINE_int32(wedge_agent_port, 5909, "Port running wedge_agent");
DEFINE_string(qsfp_service_host, "::1", "Host running qsfp_service");
DEFINE_int32(qsfp_service_port, 5910, "Port running qsfp_service");

namespace facebook::fboss::utils {

template <typename Client>
std::unique_ptr<Client> createPlaintextClient(
    const folly::IPAddress& ip,
    const int port,
    folly::EventBase* eb) {
  folly::EventBase* socketEb =
      eb ? eb : folly::EventBaseManager::get()->getEventBase();
  auto addr = folly::SocketAddress(ip, port);
  auto socket = folly::AsyncSocket::newSocket(socketEb, addr, kConnTimeout);
  socket->setSendTimeout(kSendTimeout);
  auto channel =
      apache::thrift::HeaderClientChannel::newChannel(std::move(socket));
  channel->setTimeout(kRecvTimeout);
  return std::make_unique<Client>(std::move(channel));
}

template std::unique_ptr<apache::thrift::Client<facebook::fboss::FbossCtrl>>
createPlaintextClient(
    const folly::IPAddress& ip,
    const int port,
    folly::EventBase* eb);

template std::unique_ptr<apache::thrift::Client<facebook::fboss::QsfpService>>
createPlaintextClient(
    const folly::IPAddress& ip,
    const int port,
    folly::EventBase* eb);
} // namespace facebook::fboss::utils
