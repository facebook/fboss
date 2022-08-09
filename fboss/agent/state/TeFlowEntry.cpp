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

} // namespace facebook::fboss
