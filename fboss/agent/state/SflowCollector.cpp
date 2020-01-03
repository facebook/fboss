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
} // namespace

namespace facebook::fboss {

folly::dynamic SflowCollectorFields::toFollyDynamic() const {
  folly::dynamic collector = folly::dynamic::object;
  collector[kIp] = address.getFullyQualified();
  collector[kPort] = address.getPort();

  return collector;
}

SflowCollectorFields SflowCollectorFields::fromFollyDynamic(
    const folly::dynamic& collectorJson) {
  std::string ip = collectorJson[kIp].asString();
  uint16_t port = collectorJson[kPort].asInt();

  return SflowCollectorFields(ip, port);
}

SflowCollector::SflowCollector(const std::string& ip, const uint16_t port)
    : NodeBaseT(ip, port) {}

template class NodeBaseT<SflowCollector, SflowCollectorFields>;

} // namespace facebook::fboss
