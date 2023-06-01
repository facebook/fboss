// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/EncapIndexAllocator.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateDelta.h"
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
  for (const auto& vlanTable : std::as_const(*state->getVlans())) {
    std::for_each(
        vlanTable.second->cbegin(),
        vlanTable.second->cend(),
        [&](const auto& idAndVlan) {
          extractIndices(idAndVlan.second->getArpTable());
          extractIndices(idAndVlan.second->getNdpTable());
        });
  }
  for (const auto& [_, intfMap] : std::as_const(*state->getInterfaces())) {
    std::for_each(
        intfMap->cbegin(), intfMap->cend(), [&](const auto& idAndIntf) {
          extractIndices(idAndIntf.second->getArpTable());
          extractIndices(idAndIntf.second->getNdpTable());
        });
  }
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

std::shared_ptr<SwitchState> EncapIndexAllocator::updateEncapIndices(
    const StateDelta& delta,
    const HwAsic& asic) {
  auto newState = delta.newState()->clone();
  // This function's charter is
  // - Assign encap index to reachable nbr entries (if one is not already
  //  assigned)
  // - Clear encap index from unreachable/pending entries
  if (asic.isSupported(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
    auto handleNewNbr = [&newState, &asic](auto newNbr) {
      if (!newNbr->isPending() && !newNbr->getEncapIndex()) {
        auto nbr = newNbr->clone();
        nbr->setEncapIndex(
            EncapIndexAllocator::getNextAvailableEncapIdx(newState, asic));
        return nbr;
      }
      if (newNbr->isPending() && newNbr->getEncapIndex()) {
        auto nbr = newNbr->clone();
        nbr->setEncapIndex(std::nullopt);
        return nbr;
      }
      return std::shared_ptr<std::remove_reference_t<decltype(*newNbr)>>();
    };
    for (const auto& intfDelta : delta.getIntfsDelta()) {
      auto updateIntf =
          [&](auto newNbr, const auto& nbrTable, InterfaceID intfId) {
            auto intf = newState->getInterfaces()->getNode(intfId);
            if (intf->getType() != cfg::InterfaceType::SYSTEM_PORT) {
              return;
            }
            auto updatedNbr = handleNewNbr(newNbr);
            if (updatedNbr) {
              auto intfNew = intf->modify(&newState);
              auto updatedNbrTable = nbrTable->toThrift();
              if (!nbrTable->getEntry(newNbr->getIP())) {
                updatedNbrTable.insert(
                    {newNbr->getIP().str(), updatedNbr->toThrift()});
              } else {
                updatedNbrTable.erase(newNbr->getIP().str());
                updatedNbrTable.insert(
                    {newNbr->getIP().str(), updatedNbr->toThrift()});
              }
              if (newNbr->getIP().version() == 6) {
                intfNew->setNdpTable(updatedNbrTable);
              } else {
                intfNew->setArpTable(updatedNbrTable);
              }
            }
          };
      DeltaFunctions::forEachChanged(
          intfDelta.getArpEntriesDelta(),
          [&](auto /*oldNbr*/, auto newNbr) {
            updateIntf(
                newNbr,
                intfDelta.getNew()->getArpTable(),
                intfDelta.getNew()->getID());
          },
          [&](auto newNbr) {
            updateIntf(
                newNbr,
                intfDelta.getNew()->getArpTable(),
                intfDelta.getNew()->getID());
          },
          [&](auto /*rmNbr*/) {});

      DeltaFunctions::forEachChanged(
          intfDelta.getNdpEntriesDelta(),
          [&](auto /*oldNbr*/, auto newNbr) {
            updateIntf(
                newNbr,
                intfDelta.getNew()->getNdpTable(),
                intfDelta.getNew()->getID());
          },
          [&](auto newNbr) {
            updateIntf(
                newNbr,
                intfDelta.getNew()->getNdpTable(),
                intfDelta.getNew()->getID());
          },
          [&](auto /*rmNbr*/) {});
    }
  }
  return newState;
}
} // namespace facebook::fboss
