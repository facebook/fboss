/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/NodeBase-defs.h"

#include <algorithm>
#include <utility>

namespace {
constexpr auto kId = "id";
constexpr auto kName = "name";
constexpr auto kDescription = "description";
constexpr auto kSubports = "subports";
}

namespace facebook { namespace fboss {

AggregatePortFields::AggregatePortFields(
    AggregatePortID id,
    const std::string& name,
    const std::string& description,
    Subports&& ports,
    AggregatePortFields::Forwarding fwd)
    : id_(id),
      name_(name),
      description_(description),
      ports_(std::move(ports)),
      portToFwdState_() {
  for (const auto& port : ports_) {
    auto hint = portToFwdState_.end();
    portToFwdState_.emplace_hint(hint, port, fwd);
  }
}

folly::dynamic AggregatePortFields::toFollyDynamic() const {
  folly::dynamic aggPortFields = folly::dynamic::object;
  aggPortFields[kId] = static_cast<uint16_t>(id_);
  aggPortFields[kName] = name_;
  aggPortFields[kDescription] = description_;
  folly::dynamic subports = folly::dynamic::array;
  for (const auto& port : ports_) {
    subports.push_back(static_cast<uint16_t>(port));
  }
  aggPortFields[kSubports] = std::move(subports);
  return aggPortFields;
}

AggregatePortFields AggregatePortFields::fromFollyDynamic(
    const folly::dynamic& json) {
  Subports ports;

  ports.reserve(json[kSubports].size());

  for (auto const& port : json[kSubports]) {
    ports.emplace_hint(ports.cend(), PortID(port.getInt()));
  }

  return AggregatePortFields(
      AggregatePortID(json[kId].getInt()),
      json[kName].getString(),
      json[kDescription].getString(),
      std::move(ports));
}

AggregatePort::SubportsDifferenceType AggregatePort::subportsCount() const {
  auto subportsRange = sortedSubports();
  return std::distance(subportsRange.begin(), subportsRange.end());
}

template class NodeBaseT<AggregatePort, AggregatePortFields>;

}} // facebook::fboss
