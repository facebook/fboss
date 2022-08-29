// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {
TeFlowEntryFields TeFlowEntryFields::fromThrift(
    const state::TeFlowEntryFields& teFlowEntryThrift) {
  TeFlowEntryFields teFlowEntry(*teFlowEntryThrift.flow());
  teFlowEntry.writableData() = teFlowEntryThrift;
  return teFlowEntry;
}

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

std::string TeFlowEntry::str() const {
  std::string flowString;
  if (!getFlow().dstPrefix().has_value() || !getFlow().srcPort().has_value()) {
    return "invalid";
  }
  auto prefix = getFlow().dstPrefix().value();
  folly::IPAddress ipaddr = network::toIPAddress(*prefix.ip());
  flowString.append(fmt::format(
      "dstPrefix:{}/{},srcPort:{}",
      ipaddr.str(),
      *prefix.prefixLength(),
      *getFlow().srcPort()));
  flowString.append(fmt::format(
      ",counterID:{}", getCounterID().has_value() ? *getCounterID() : "null"));
  flowString.append(fmt::format(",Nexthop:"));
  for (const auto& nhop : util::toRouteNextHopSet(getNextHops())) {
    flowString.append(fmt::format("{},", nhop.str()));
  }
  flowString.append(fmt::format("ResolvedNexthop:"));
  for (const auto& nhop : util::toRouteNextHopSet(getResolvedNextHops())) {
    flowString.append(fmt::format("{},", nhop.str()));
  }
  flowString.append(fmt::format("Enabled:{}", getEnabled()));
  return flowString;
}

} // namespace facebook::fboss
