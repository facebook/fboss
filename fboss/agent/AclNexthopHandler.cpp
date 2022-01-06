// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/AclNexthopHandler.h"
#include <folly/logging/xlog.h>
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

bool AclNexthopHandler::hasAclChanges(const StateDelta& delta) {
  bool aclsChanged = (sw_->getState()->getAcls()->size() > 0) &&
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
  for (auto& nhIpStr : *redirect.value().first.nexthops_ref()) {
    auto nhIp = folly::IPAddress(nhIpStr);
    if (nhIp.isV4()) {
      const auto route = sw_->longestMatch<folly::IPAddressV4>(
          sw_->getState(), nhIp.asV4(), RouterID(0));
      if (!route || !route->isResolved()) {
        continue;
      }
      RouteNextHopSet routeNextHops = route->getForwardInfo().getNextHopSet();
      nexthops.merge(std::move(routeNextHops));
    } else {
      const auto route = sw_->longestMatch<folly::IPAddressV6>(
          sw_->getState(), nhIp.asV6(), RouterID(0));
      if (!route || !route->isResolved()) {
        continue;
      }
      RouteNextHopSet routeNextHops = route->getForwardInfo().getNextHopSet();
      nexthops.merge(std::move(routeNextHops));
    }
  }
  action.setRedirectToNextHop(std::make_pair(redirect.value().first, nexthops));
}

std::shared_ptr<AclMap> AclNexthopHandler::updateAcls(
    std::shared_ptr<SwitchState>& newState) {
  bool changed = false;
  auto origAcls = newState->getAcls();
  for (auto origAclEntry : *origAcls) {
    if (updateAcl(origAclEntry, newState)) {
      changed = true;
    }
  }
  if (!changed) {
    return nullptr;
  }
  return newState->getAcls();
}

AclEntry* FOLLY_NULLABLE AclNexthopHandler::updateAcl(
    const std::shared_ptr<AclEntry>& origAclEntry,
    std::shared_ptr<SwitchState>& newState) {
  if (origAclEntry->getAclAction().has_value() &&
      origAclEntry->getAclAction().value().getRedirectToNextHop().has_value()) {
    auto newAclEntry = origAclEntry->modify(&newState);
    MatchAction action = newAclEntry->getAclAction().value();
    resolveActionNexthops(action);
    newAclEntry->setAclAction(action);
    return (newAclEntry->getAclAction().value().getRedirectToNextHop() !=
            origAclEntry->getAclAction().value().getRedirectToNextHop())
        ? newAclEntry
        : nullptr;
  }
  return nullptr;
}

} // namespace facebook::fboss
