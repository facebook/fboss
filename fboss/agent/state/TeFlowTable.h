// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;

using TeFlowTableTraits = NodeMapTraits<TeFlow, TeFlowEntry>;

/*
 * A container for TE flow entries
 */
class TeFlowTable
    : public ThriftyNodeMapT<
          TeFlowTable,
          TeFlowTableTraits,
          ThriftyNodeMapTraits<TeFlow, state::TeFlowEntryFields>> {
 public:
  TeFlowTable();
  ~TeFlowTable() override;

  const std::shared_ptr<TeFlowEntry>& getTeFlow(TeFlow id) const {
    return getNode(id);
  }
  std::shared_ptr<TeFlowEntry> getTeFlowIf(TeFlow id) const {
    return getNodeIf(id);
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyNodeMapT::ThriftyNodeMapT;
  friend class CloneAllocator;
};
void toAppend(const TeFlow& flow, std::string* result);
} // namespace facebook::fboss
