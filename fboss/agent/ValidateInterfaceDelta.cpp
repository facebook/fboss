// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ValidateInterfaceDelta.h"

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

bool IntfDeltaValidator::processNeighborDelete(
    const InterfaceDelta& intfDelta) {
  // SaiSwitch processes all neighbor removals before changes/adds.
  // Removals cannot violate the single-MAC invariant (they only reduce
  // the number of neighbors), but we must update ref counts so that
  // subsequent change/add checks see the correct state.
  auto intfId = intfDelta.getNew()->getID();

  auto removeNbr = [&](const auto& oldNbr) {
    if (!oldNbr->isReachable()) {
      return LoopAction::CONTINUE;
    }
    intfToSingleNbrMac_.unref(intfId);
    return LoopAction::CONTINUE;
  };
  DeltaFunctions::forEachRemoved(intfDelta.getArpDelta(), removeNbr);
  DeltaFunctions::forEachRemoved(intfDelta.getNdpDelta(), removeNbr);
  return true;
}

bool IntfDeltaValidator::processNeighborChange(
    const InterfaceDelta& intfDelta) {
  // SaiSwitch processes neighbor changes one at a time (ARP then NDP).
  // A change with a different MAC is effectively a remove + add.
  // If refCount > 1 when the old MAC is "removed", other neighbors still
  // reference the old MAC, so adding the new MAC would create a transient
  // multi-MAC state on the interface.
  // Example: N1(MAC_A->MAC_B) and N2(MAC_A->MAC_B) - after changeNeighbor(N1)
  // but before changeNeighbor(N2), hardware sees N1=MAC_B and N2=MAC_A.
  auto intfId = intfDelta.getNew()->getID();

  bool isValid = true;
  auto changeNbr = [&](const auto& oldNbr, const auto& newNbr) {
    bool oldReachable = oldNbr->isReachable();
    bool newReachable = newNbr->isReachable();

    if (!oldReachable && !newReachable) {
      return LoopAction::CONTINUE; // Neither state is reachable, skip
    }

    // REACHABLE -> PENDING: treat as removal, decrement ref count
    if (oldReachable && !newReachable) {
      intfToSingleNbrMac_.unref(intfId);
      return LoopAction::CONTINUE;
    }

    // PENDING -> REACHABLE: treat as addition, validate MAC
    if (!oldReachable && newReachable) {
      if (!intfToSingleNbrMac_.ref(intfId, newNbr->getMac())) {
        XLOG(ERR) << "Interface " << intfId << " neighbor MAC "
                  << newNbr->getMac().toString() << " differs from expected "
                  << intfToSingleNbrMac_.get(intfId)->toString()
                  << " during neighbor change processing";
        isValid = false;
        return LoopAction::BREAK;
      }
      return LoopAction::CONTINUE;
    }

    // REACHABLE -> REACHABLE: MAC change
    if (oldNbr->getMac() == newNbr->getMac()) {
      return LoopAction::CONTINUE; // MAC unchanged, no effect on invariant
    }

    // Remove old MAC reference
    intfToSingleNbrMac_.unref(intfId);

    // Add new MAC reference
    if (!intfToSingleNbrMac_.ref(intfId, newNbr->getMac())) {
      XLOG(ERR) << "Interface " << intfId << " neighbor MAC "
                << newNbr->getMac().toString() << " differs from expected "
                << intfToSingleNbrMac_.get(intfId)->toString()
                << " during neighbor change processing";
      isValid = false;
      return LoopAction::BREAK;
    }
    return LoopAction::CONTINUE;
  };

  // SaiSwitch order: ARP changes then NDP changes
  DeltaFunctions::forEachChanged(intfDelta.getArpDelta(), changeNbr);
  if (isValid) {
    DeltaFunctions::forEachChanged(intfDelta.getNdpDelta(), changeNbr);
  }
  return isValid;
}

bool IntfDeltaValidator::processNeighborAdd(const InterfaceDelta& intfDelta) {
  // SaiSwitch processes neighbor adds one at a time (ARP then NDP).
  // Each added neighbor's MAC must match the existing MAC on the interface.
  // If no MAC is stored (new interface or all neighbors were removed),
  // the first added neighbor's MAC becomes the expected MAC.
  auto intfId = intfDelta.getNew()->getID();

  bool isValid = true;
  auto addNbr = [&](const auto& newNbr) {
    if (!newNbr->isReachable()) {
      return LoopAction::CONTINUE;
    }
    if (!intfToSingleNbrMac_.ref(intfId, newNbr->getMac())) {
      XLOG(ERR) << "Interface " << intfId << " neighbor MAC "
                << newNbr->getMac().toString() << " differs from expected "
                << intfToSingleNbrMac_.get(intfId)->toString()
                << " during neighbor add processing";
      isValid = false;
      return LoopAction::BREAK;
    }
    return LoopAction::CONTINUE;
  };

  // SaiSwitch order: ARP adds then NDP adds
  DeltaFunctions::forEachAdded(intfDelta.getArpDelta(), addNbr);
  if (isValid) {
    DeltaFunctions::forEachAdded(intfDelta.getNdpDelta(), addNbr);
  }
  return isValid;
}

bool IntfDeltaValidator::isValidDelta(const StateDelta& delta) {
  if (!FLAGS_enforce_single_nbr_mac_per_intf) {
    return true;
  }
  // Single loop over interface deltas. For each interface, mimic SaiSwitch
  // processing order: removals first, then changes, then adds.
  for (const auto& intfDelta : delta.getIntfsDelta()) {
    const auto& oldIntf = intfDelta.getOld();
    const auto& newIntf = intfDelta.getNew();

    // Interface removed
    if (oldIntf && !newIntf) {
      intfToSingleNbrMac_.erase(oldIntf->getID());
      continue;
    }
    // No new interface or VLAN type - skip
    if (!newIntf || newIntf->getType() == cfg::InterfaceType::VLAN) {
      continue;
    }

    // Interface added or changed - delegate to per-interface methods
    if (!isValidIntfDelta(intfDelta)) {
      return false;
    }
  }
  return true;
}

void IntfDeltaValidator::updateRejected(const StateDelta& delta) {
  if (!FLAGS_enforce_single_nbr_mac_per_intf) {
    return;
  }
  // reconstruct intfToSingleNbrMac_ from current state
  intfToSingleNbrMac_.clear();
  for (const auto& intfDelta : delta.getIntfsDelta()) {
    const auto& oldIntf = intfDelta.getOld();
    const auto& newIntf = intfDelta.getNew();
    // Interface removed
    if (oldIntf && !newIntf) {
      continue;
    }
    // old state is empty, so only process add
    processNeighborAdd(intfDelta);
  }
}

// NOTE: Processing order (delete → change → add) must match SaiSwitch's
// neighbor processing in SaiSwitch::stateChangedImpl. See the comments at
// processRemovedNeighborDeltaForIntfs and
// processNeighborChangedAndAddedDeltaForIntfs in SaiSwitch.cpp.
bool IntfDeltaValidator::isValidIntfDelta(const InterfaceDelta& intfDelta) {
  if (!FLAGS_enforce_single_nbr_mac_per_intf) {
    return true;
  }
  return processNeighborDelete(intfDelta) && processNeighborChange(intfDelta) &&
      processNeighborAdd(intfDelta);
}

} // namespace facebook::fboss
