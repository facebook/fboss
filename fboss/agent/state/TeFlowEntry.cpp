// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

TeFlowEntry* TeFlowEntry::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  TeFlowTable* table = (*state)->getTeFlowTable()->modify(state);
  auto newEntry = clone();
  auto* ptr = newEntry.get();
  table->updateNode(std::move(newEntry));
  return ptr;
}

std::string TeFlowEntry::getID() const {
  return getTeFlowStr(getFlow()->toThrift());
}

std::string TeFlowEntry::str() const {
  std::string flowString{};
  auto flow = getFlow()->toThrift();
  if (!flow.dstPrefix().has_value() || !flow.srcPort().has_value()) {
    return "invalid";
  }
  auto prefix = flow.dstPrefix().value();
  folly::IPAddress ipaddr = network::toIPAddress(*prefix.ip());
  flowString.append(fmt::format(
      "dstPrefix:{}/{},srcPort:{}",
      ipaddr.str(),
      *prefix.prefixLength(),
      *flow.srcPort()));
  auto counter = getCounterID();
  flowString.append(
      fmt::format(",counterID:{}", counter ? counter->toThrift() : "null"));
  flowString.append(fmt::format(",Nexthop:"));
  const auto& nhops = getNextHops();
  for (const auto& nhop : util::toRouteNextHopSet(nhops->toThrift())) {
    flowString.append(fmt::format("{},", nhop.str()));
  }
  flowString.append(fmt::format("ResolvedNexthop:"));
  const auto& resolvedNhops = getResolvedNextHops();
  for (const auto& nhop : util::toRouteNextHopSet(resolvedNhops->toThrift())) {
    flowString.append(fmt::format("{},", nhop.str()));
  }
  flowString.append(fmt::format("Enabled:{}", getEnabled()));
  return flowString;
}

TeFlowDetails TeFlowEntry::toDetails() const {
  TeFlowDetails details{};
  details.flow() = getFlow()->toThrift();
  details.enabled() = getEnabled();
  details.nexthops() = getNextHops()->toThrift();
  details.resolvedNexthops() = getResolvedNextHops()->toThrift();
  details.counterID() = "null";
  if (const auto& counter = getCounterID()) {
    details.counterID() = counter->toThrift();
  }
  return details;
}
} // namespace facebook::fboss
