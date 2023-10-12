/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/HwSwitchThriftClientTable.h"

#include <folly/IPAddress.h>
#include <netinet/in.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/ReconnectingRequestChannel.h>
#include <thrift/lib/cpp2/async/RetryingRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

#include "fboss/agent/FbossError.h"

#include <memory>

namespace facebook::fboss {
HwSwitchThriftClientTable::HwSwitchThriftClientTable(
    int16_t basePort,
    const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo) {
  evbThread_ =
      std::make_shared<folly::ScopedEventBaseThread>("HwSwitchCtrlClient");
  for (const auto& [switchId, switchInfo] : switchIdToSwitchInfo) {
    auto port = basePort + *switchInfo.switchIndex();
    clients_.emplace(
        SwitchID(switchId),
        std::make_unique<apache::thrift::Client<FbossHwCtrl>>(
            createClient(port)));
  }
}

/*
 * Creates a reconnecting thrift client to query HwAgent. This does not
 * connect to server yet. The connection happens when the first thrift api
 * call is made
 */
apache::thrift::Client<FbossHwCtrl> HwSwitchThriftClientTable::createClient(
    int16_t port) {
  auto reconnectingChannel =
      apache::thrift::PooledRequestChannel::newSyncChannel(
          evbThread_, [this, port](folly::EventBase& evb) {
            return apache::thrift::RetryingRequestChannel::newChannel(
                evb,
                2, /*retries before error*/
                apache::thrift::ReconnectingRequestChannel::newChannel(
                    *evbThread_->getEventBase(),
                    [port](
                        folly::EventBase& evb,
                        folly::AsyncSocket::ConnectCallback& /*cb*/) {
                      auto socket = folly::AsyncSocket::UniquePtr(
                          new folly::AsyncSocket(&evb));
                      socket->connect(
                          nullptr, folly::SocketAddress("::1", port));
                      auto channel =
                          apache::thrift::RocketClientChannel::newChannel(
                              std::move(socket));
                      return channel;
                    }));
          });
  return apache::thrift::Client<FbossHwCtrl>(std::move(reconnectingChannel));
}

apache::thrift::Client<FbossHwCtrl>* HwSwitchThriftClientTable::getClient(
    SwitchID switchId) {
  if (clients_.find(switchId) == clients_.end()) {
    throw FbossError("No client found for switch ", switchId);
  }
  return clients_.at(switchId).get();
}

} // namespace facebook::fboss
