// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <optional>

namespace facebook::fboss {

USE_THRIFT_COW(TeFlowEntry);

class TeFlowEntry
    : public ThriftStructNode<TeFlowEntry, state::TeFlowEntryFields> {
 public:
  using Base = ThriftStructNode<TeFlowEntry, state::TeFlowEntryFields>;
  using Base::modify;
  explicit TeFlowEntry(TeFlow flowId) {
    set<switch_state_tags::flow>(flowId);
  }
  std::string getID() const;

  const auto& getFlow() const {
    return cref<switch_state_tags::flow>();
  }
  auto& getCounterID() const {
    return cref<switch_state_tags::counterID>();
  }
  void setCounterID(std::optional<TeCounterID> counter) {
    if (counter.has_value()) {
      set<switch_state_tags::counterID>(counter.value());
    } else {
      ref<switch_state_tags::counterID>().reset();
    }
  }
  const auto& getNextHops() const {
    return cref<switch_state_tags::nexthops>();
  }
  void setNextHops(const std::vector<NextHopThrift>& nexthops) {
    set<switch_state_tags::nexthops>(nexthops);
  }
  const auto& getResolvedNextHops() const {
    return cref<switch_state_tags::resolvedNexthops>();
  }
  void setResolvedNextHops(std::vector<NextHopThrift> nexthops) {
    set<switch_state_tags::resolvedNexthops>(std::move(nexthops));
  }
  bool getEnabled() const {
    return get<switch_state_tags::enabled>()->toThrift();
  }
  void setEnabled(bool enable) {
    set<switch_state_tags::enabled>(enable);
  }

  std::string str() const;

  TeFlowDetails toDetails() const;

  static std::shared_ptr<TeFlowEntry> createTeFlowEntry(const FlowEntry& entry);
  void resolve(const std::shared_ptr<SwitchState>& state);
  static bool isNexthopResolved(
      NextHopThrift nexthop,
      const std::shared_ptr<SwitchState>& state);

  template <typename AddrT>
  static std::optional<folly::MacAddress> getNeighborMac(
      const std::shared_ptr<SwitchState>& state,
      const std::shared_ptr<Interface>& interface,
      AddrT ip);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
