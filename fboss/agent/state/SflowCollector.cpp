/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/NodeBase-defs.h"

#include <folly/Conv.h>

namespace {
constexpr auto kIp = "ip";
constexpr auto kPort = "port";
constexpr auto kId = "id";
constexpr auto kAddress = "address";
constexpr auto kHost = "host";
} // namespace

namespace facebook::fboss {

SflowCollector::SflowCollector(std::string ip, uint16_t port) {
  folly::SocketAddress socketAddr(ip, port);
  ref<switch_state_tags::id>() = folly::to<std::string>(
      socketAddr.getFullyQualified(), ':', socketAddr.getPort());
  ref<switch_state_tags::address>()->ref<switch_state_tags::host>() =
      socketAddr.getFullyQualified();
  ref<switch_state_tags::address>()->ref<switch_state_tags::port>() = port;
}

template class ThriftStructNode<SflowCollector, state::SflowCollectorFields>;
} // namespace facebook::fboss
