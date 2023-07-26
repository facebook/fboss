/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/SplitAgentThriftSyncer.h"

#include <folly/IPAddress.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

static constexpr folly::StringPiece kClientName = "mnpu-syncer-client";

namespace facebook::fboss {

SplitAgentThriftSyncer::SplitAgentThriftSyncer(
    HwSwitch* hw,
    uint16_t serverPort)
    : hw_(hw) {
  evbThread_ = std::make_shared<folly::ScopedEventBaseThread>(kClientName);
  auto channel = apache::thrift::PooledRequestChannel::newChannel(
      evbThread_->getEventBase(),
      evbThread_,
      [serverPort](folly::EventBase& evb) mutable {
        return apache::thrift::RocketClientChannel::newChannel(
            folly::AsyncSocket::UniquePtr(
                new folly::AsyncSocket(&evb, "::1", serverPort)));
      });
  multiSwitchClient_ =
      std::make_unique<apache::thrift::Client<multiswitch::MultiSwitchCtrl>>(
          std::move(channel));
}

} // namespace facebook::fboss
