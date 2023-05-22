// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/MirrorManager.h"

#include <boost/container/flat_set.hpp>
#include <tuple>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/Route.h"

#include "fboss/agent/state/SwitchState.h"

using boost::container::flat_set;
using facebook::fboss::DeltaFunctions::isEmpty;
using folly::IPAddress;
using std::optional;

namespace facebook::fboss {

MirrorManager::MirrorManager(SwSwitch* sw)
    : sw_(sw),
      v4Manager_(std::make_unique<MirrorManagerV4>(sw)),
      v6Manager_(std::make_unique<MirrorManagerV6>(sw)) {
  sw_->registerStateObserver(this, "MirrorManager");
}
MirrorManager::~MirrorManager() {
  sw_->unregisterStateObserver(this);
}
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
  auto updatedState = state->clone();
  auto mnpuMirrors = state->getMirrors()->modify(&updatedState);
  bool mirrorsUpdated = false;

  for (auto mniter = mnpuMirrors->cbegin(); mniter != mnpuMirrors->cend();
       ++mniter) {
    HwSwitchMatcher matcher(mniter->first);
    auto& mirrors = std::as_const(mniter->second);
    for (auto iter = mirrors->cbegin(); iter != mirrors->cend(); ++iter) {
      auto mirror = iter->second;
      if (!mirror->getDestinationIp()) {
        /* SPAN mirror does not require resolving */
        continue;
      }
      const auto destinationIp = mirror->getDestinationIp().value();
      std::shared_ptr<Mirror> updatedMirror = destinationIp.isV4()
          ? v4Manager_->updateMirror(mirror)
          : v6Manager_->updateMirror(mirror);
      if (updatedMirror) {
        XLOG(DBG2) << "Mirror: " << updatedMirror->getID() << " updated.";
        mirrors->updateNode(updatedMirror);
        mirrorsUpdated = true;
      }
    }
  }
  if (!mirrorsUpdated) {
    return std::shared_ptr<SwitchState>(nullptr);
  }
  return updatedState;
}

bool MirrorManager::hasMirrorChanges(const StateDelta& delta) {
  if (!sw_->getState()->getMirrors()->numNodes()) {
    return false;
  }
  if (!isEmpty(delta.getMirrorsDelta()) || !isEmpty(delta.getFibsDelta())) {
    return true;
  }
  for (const auto& entry : delta.getVlansDelta()) {
    if (!isEmpty(entry.getArpDelta()) || !isEmpty(entry.getNdpDelta())) {
      return true;
    }
  }
  return false;
}

} // namespace facebook::fboss
