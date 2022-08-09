// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/TeFlowEntry.h"

namespace facebook::fboss {
TeFlowEntryFields TeFlowEntryFields::fromThrift(
    const state::TeFlowEntryFields& teFlowEntryThrift) {
  TeFlowEntryFields teFlowEntry(*teFlowEntryThrift.flow());
  teFlowEntry.writableData() = teFlowEntryThrift;
  return teFlowEntry;
}

} // namespace facebook::fboss
