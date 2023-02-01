// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <optional>

namespace facebook::fboss {

struct TeFlowEntryFields
    : public ThriftyFields<TeFlowEntryFields, state::TeFlowEntryFields> {
  explicit TeFlowEntryFields(TeFlow flowId) {
    auto& data = writableData();
    *data.flow() = flowId;
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  state::TeFlowEntryFields toThrift() const override {
    return data();
  }
  static TeFlowEntryFields fromThrift(
      const state::TeFlowEntryFields& teFlowEntryFields);
};

USE_THRIFT_COW(TeFlowEntry);

class TeFlowEntry
    : public ThriftStructNode<TeFlowEntry, state::TeFlowEntryFields> {
 public:
  using Base = ThriftStructNode<TeFlowEntry, state::TeFlowEntryFields>;
  using LegacyFields = TeFlowEntryFields;
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

  TeFlowEntry* modify(std::shared_ptr<SwitchState>* state);
  std::string str() const;

  folly::dynamic toFollyDynamic() const override {
    return toFollyDynamicLegacy();
  }

  folly::dynamic toFollyDynamicLegacy() const {
    auto fields = TeFlowEntryFields::fromThrift(toThrift());
    return fields.toFollyDynamic();
  }

  static std::shared_ptr<TeFlowEntry> fromFollyDynamic(
      const folly::dynamic& dyn) {
    return fromFollyDynamicLegacy(dyn);
  }

  static std::shared_ptr<TeFlowEntry> fromFollyDynamicLegacy(
      const folly::dynamic& dyn) {
    auto fields = TeFlowEntryFields::fromFollyDynamic(dyn);
    return std::make_shared<TeFlowEntry>(fields.toThrift());
  }

  TeFlowDetails toDetails() const;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
