// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
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
 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::TeFlowEntryFields, TeFlowEntry, TeFlowEntryFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
