// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/MirrorManager.h"

#include <boost/container/flat_set.hpp>
#include <tuple>
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/SwitchState.h"

using boost::container::flat_set;
using facebook::fboss::DeltaFunctions::isEmpty;
using folly::IPAddress;
using std::optional;

namespace facebook::fboss {

void MirrorManager::stateUpdated(const StateDelta& delta) {
  if (!hasMirrorChanges(delta)) {
    return;
  }

  auto updateMirrorsFn = [this](const std::shared_ptr<SwitchState>& state) {
    return resolveMirrors(state);
  };
  sw_->updateState("Updating mirrors", updateMirrorsFn);
}

std::shared_ptr<SwitchState> MirrorManager::resolveMirrors(
    const std::shared_ptr<SwitchState>& state) {
  auto mirrors = state->getMirrors()->clone();
  bool mirrorsUpdated = false;

  for (const auto& mirror : *state->getMirrors()) {
    if (!mirror->getDestinationIp()) {
      /* SPAN mirror does not require resolving */
      continue;
    }
    const auto destinationIp = mirror->getDestinationIp().value();
    std::shared_ptr<Mirror> updatedMirror = destinationIp.isV4()
        ? v4Manager_->updateMirror(mirror)
        : v6Manager_->updateMirror(mirror);
    if (updatedMirror) {
      XLOG(INFO) << "Mirror: " << updatedMirror->getID() << " updated.";
      mirrors->updateNode(updatedMirror);
      mirrorsUpdated = true;
    }
  }
  if (!mirrorsUpdated) {
    return std::shared_ptr<SwitchState>(nullptr);
  }
  auto updatedState = state->clone();
  updatedState->resetMirrors(mirrors);
  return updatedState;
}

bool MirrorManager::hasMirrorChanges(const StateDelta& delta) {
  return (sw_->getState()->getMirrors()->size() > 0) &&
      (!isEmpty(delta.getMirrorsDelta()) ||
       !isEmpty(delta.getRouteTablesDelta()) ||
       std::any_of(
           std::begin(delta.getVlansDelta()),
           std::end(delta.getVlansDelta()),
           [](const VlanDelta& vlanDelta) {
             return !isEmpty(vlanDelta.getArpDelta()) ||
                 !isEmpty(vlanDelta.getNdpDelta());
           }));
}

} // namespace facebook::fboss
