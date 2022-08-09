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

class TeFlowEntry : public ThriftyBaseT<
                        state::TeFlowEntryFields,
                        TeFlowEntry,
                        TeFlowEntryFields> {
 public:
  TeFlow getID() const {
    return *getFields()->data().flow();
  }
  const TeFlow& getFlow() const {
    return *getFields()->data().flow();
  }
  TeCounterID getCounterID() const {
    return *getFields()->data().counterID();
  }
  void setCounterID(std::optional<TeCounterID> counter) {
    if (counter.has_value()) {
      writableFields()->writableData().counterID() = *counter;
    } else {
      writableFields()->writableData().counterID().reset();
    }
  }
  const std::vector<NextHopThrift>& getNextHops() const {
    return *getFields()->data().nexthops();
  }
  void setNextHops(const std::vector<NextHopThrift>& nexthops) {
    writableFields()->writableData().nexthops() = nexthops;
  }
  const std::vector<NextHopThrift>& getResolvedNextHops() const {
    return *getFields()->data().resolvedNexthops();
  }
  void setResolvedNextHops(std::vector<NextHopThrift> nexthops) {
    writableFields()->writableData().resolvedNexthops() = nexthops;
  }
  bool getEnabled() const {
    return *getFields()->data().enabled();
  }
  void setEnabled(bool enable) {
    writableFields()->writableData().enabled() = enable;
  }

  bool operator==(const TeFlowEntry& entry) {
    return (getID() == entry.getID()) &&
        (getNextHops() == entry.getNextHops()) &&
        (getResolvedNextHops() == entry.getResolvedNextHops()) &&
        (getCounterID() == entry.getCounterID()) &&
        (getEnabled() == entry.getEnabled());
  }
  bool operator!=(const TeFlowEntry& entry) {
    return !(*this == entry);
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::TeFlowEntryFields, TeFlowEntry, TeFlowEntryFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
