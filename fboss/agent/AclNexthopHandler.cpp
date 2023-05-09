// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/AclNexthopHandler.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclTable.h"
#include "fboss/agent/state/AclTableGroup.h"
#include "fboss/agent/state/AclTableMap.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

using DeltaFunctions::isEmpty;

AclNexthopHandler::AclNexthopHandler(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "AclNexthopHandler");
}
AclNexthopHandler::~AclNexthopHandler() {
  sw_->unregisterStateObserver(this);
}
bool AclNexthopHandler::hasAclChanges(const StateDelta& delta) {
  bool aclsChanged = (sw_->getState()->getMultiSwitchAcls()->numNodes() > 0) &&
      (!isEmpty(delta.getAclsDelta()));
  aclsChanged =
      (aclsChanged || !isEmpty(delta.getFibsDelta()) ||
       !isEmpty(delta.getLabelForwardingInformationBaseDelta()));
  XLOG(DBG2) << "aclsChanged: " << aclsChanged;
  return aclsChanged;
}

void AclNexthopHandler::stateUpdated(const StateDelta& delta) {
  if (!hasAclChanges(delta)) {
    return;
  }

  auto updateAclsFn = [this](const std::shared_ptr<SwitchState>& state) {
    return handleUpdate(state);
  };
  sw_->updateState("Updating ACLs", updateAclsFn);
}

std::shared_ptr<SwitchState> AclNexthopHandler::handleUpdate(
    const std::shared_ptr<SwitchState>& state) {
  auto newState = state->clone();
  if (!updateAcls(newState)) {
    return std::shared_ptr<SwitchState>(nullptr);
  }
  return newState;
}

void AclNexthopHandler::resolveActionNexthops(MatchAction& action) {
  RouteNextHopSet nexthops;
  const auto& redirect = action.getRedirectToNextHop();
  auto addFilteredNexthops = [&nexthops](auto& fibNextHops, auto& intfID) {
    if (intfID.has_value()) {
      for (const auto& nhop : fibNextHops) {
        if (nhop.intfID().has_value() &&
            InterfaceID(intfID.value()) == nhop.intfID().value()) {
          nexthops.insert(nhop);
        }
      }
    } else {
      nexthops.merge(std::move(fibNextHops));
    }
  };
  for (auto& nhIpStruct : *redirect.value().first.redirectNextHops()) {
    auto nhIp = folly::IPAddress(*nhIpStruct.ip());
    auto intfID = nhIpStruct.intfID();
    if (nhIp.isV4()) {
      const auto route = sw_->longestMatch<folly::IPAddressV4>(
          sw_->getState(), nhIp.asV4(), RouterID(0));
      if (!route || !route->isResolved()) {
        continue;
      }
      RouteNextHopSet routeNextHops =
          route->getForwardInfo().normalizedNextHops();
      addFilteredNexthops(routeNextHops, intfID);
    } else {
      const auto route = sw_->longestMatch<folly::IPAddressV6>(
          sw_->getState(), nhIp.asV6(), RouterID(0));
      if (!route || !route->isResolved()) {
        continue;
      }
      RouteNextHopSet routeNextHops =
          route->getForwardInfo().normalizedNextHops();
      addFilteredNexthops(routeNextHops, intfID);
    }
  }
  action.setRedirectToNextHop(std::make_pair(redirect.value().first, nexthops));
}

std::shared_ptr<MultiSwitchAclMap> AclNexthopHandler::updateAcls(
    std::shared_ptr<SwitchState>& newState) {
  bool changed = false;
  auto origmultiSwitchAcls = newState->getMultiSwitchAcls();
  for (const auto& mIter : std::as_const(*origmultiSwitchAcls)) {
    auto origAcls = mIter.second;
    for (const auto& iter : std::as_const(*origAcls)) {
      if (updateAcl(iter.second, newState)) {
        changed = true;
      }
    }
  }
  if (!changed) {
    return nullptr;
  }
  return newState->getMultiSwitchAcls();
}

AclEntry* FOLLY_NULLABLE AclNexthopHandler::updateAcl(
    const std::shared_ptr<AclEntry>& origAclEntry,
    std::shared_ptr<SwitchState>& newState) {
  const auto& origAclAction = origAclEntry->getAclAction();
  if (origAclAction &&
      origAclAction->cref<switch_state_tags::redirectToNextHop>()) {
    auto newAclEntry = origAclEntry->modify(
        &newState, sw_->getScopeResolver()->scope(origAclEntry));
    newAclEntry->setEnabled(true);

    // THRIFT_COPY
    MatchAction action =
        MatchAction::fromThrift(newAclEntry->getAclAction()->toThrift());
    resolveActionNexthops(action);
    newAclEntry->setAclAction(action);
    // Disable acl if there are no nexthops available
    if (action.getRedirectToNextHop().has_value() &&
        !action.getRedirectToNextHop().value().second.size()) {
      XLOG(DBG2) << "Disabling acl as no resolved nexthops are available";
      newAclEntry->setEnabled(false);
    }
    const auto& newAclAction = newAclEntry->getAclAction();
    return ((newAclAction->cref<switch_state_tags::redirectToNextHop>()
                 ->toThrift() !=
             origAclAction->cref<switch_state_tags::redirectToNextHop>()
                 ->toThrift()) ||
            (newAclEntry->isEnabled() != origAclEntry->isEnabled()))
        ? newAclEntry
        : nullptr;
  }
  return nullptr;
}

} // namespace facebook::fboss
