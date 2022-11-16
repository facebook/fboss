// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/EncapIndexAllocator.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"

namespace facebook::fboss {
class HwAsic;
class SwitchState;

int64_t EncapIndexAllocator::getNextAvailableEncapIdx(
    const std::shared_ptr<SwitchState>& state,
    const HwAsic& asic) {
  if (!asic.isSupported(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
    throw FbossError(
        "Encap index allocation not supported on: ", asic.getAsicTypeStr());
  }
  std::unordered_set<int64_t> allocatedIndices;

  auto extractIndices = [&allocatedIndices](const auto& nbrTable) {
    for (auto itr = nbrTable->cbegin(); itr != nbrTable->cend(); ++itr) {
      if (itr->second->getEncapIndex()) {
        allocatedIndices.insert(*itr->second->getEncapIndex());
      }
    }
  };
  std::for_each(
      state->getVlans()->cbegin(),
      state->getVlans()->cend(),
      [&](const auto& idAndVlan) {
        extractIndices(idAndVlan.second->getArpTable());
        extractIndices(idAndVlan.second->getNdpTable());
      });
  std::for_each(
      state->getInterfaces()->cbegin(),
      state->getInterfaces()->cend(),
      [&](const auto& idAndIntf) {
        extractIndices(idAndIntf.second->getArpTable());
        extractIndices(idAndIntf.second->getNdpTable());
      });
  auto start = *asic.getReservedEncapIndexRange().minimum() +
      kEncapIdxReservedForLoopbacks;
  while (start <= *asic.getReservedEncapIndexRange().maximum()) {
    if (allocatedIndices.find(start) == allocatedIndices.end()) {
      return start;
    }
    start++;
  }
  throw FbossError("No more unallocated indices for: ", asic.getAsicTypeStr());
}
} // namespace facebook::fboss
