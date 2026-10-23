// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/PbrAclManager.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclTable.h"
#include "fboss/agent/state/AclTableGroup.h"
#include "fboss/agent/state/AclTableGroupMap.h"
#include "fboss/agent/state/AclTableMap.h"
#include "fboss/agent/state/ClassBasedPolicyMap.h"
#include "fboss/agent/state/ClassBasedPolicyNode.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/PbrUtils.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace facebook::fboss {

namespace {

[[noreturn]] void throwPbrTableMissing(
    const std::string& pbrTable,
    size_t numPolicies) {
  throw FbossError(
      "PBR table '",
      pbrTable,
      "' missing from SwitchState but ",
      numPolicies,
      " PBR policies are configured");
}

void removePolicyEntries(
    AclMap& aclMap,
    const std::shared_ptr<ClassBasedPolicyNode>& policy) {
  for (const auto& [tc, _] : policy->getClass2NextHopGroup()) {
    bool removed = false;
    for (bool temporary : {false, true}) {
      removed |= aclMap.removeNodeIf(makePbrAclEntryName(
                     policy->getID(), tc, temporary)) != nullptr;
    }
    if (!removed) {
      throw FbossError(
          "removed PBR policy '",
          policy->getID(),
          "' had no programmed ACL entry for traffic class ",
          static_cast<int>(tc));
    }
  }
}

bool addPolicyEntries(
    AclMap& aclMap,
    const std::shared_ptr<ClassBasedPolicyNode>& policy) {
  bool changed = false;
  for (const auto& entry : createAclEntriesFromPolicy(policy)) {
    // reconstructFromSwitchState replays every policy as added against a state
    // whose ACL map is already populated, so an add can meet its own entry.
    if (auto cur = aclMap.getEntryIf(entry->getID()); cur && *cur == *entry) {
      continue;
    }
    aclMap.removeNodeIf(entry->getID());
    aclMap.addNode(entry);
    changed = true;
  }
  return changed;
}

bool applyChangedPolicy(
    AclMap& aclMap,
    const std::shared_ptr<ClassBasedPolicyNode>& old,
    const std::shared_ptr<ClassBasedPolicyNode>& newPolicy) {
  const bool defaultChanged = *old->getDefaultNextHopGroup().id() !=
      *newPolicy->getDefaultNextHopGroup().id();
  auto oldTc2Nhg = old->getClass2NextHopGroup();
  auto newTc2Nhg = newPolicy->getClass2NextHopGroup();
  std::map<std::string, std::shared_ptr<AclEntry>> newEntries;
  for (const auto& entry : createAclEntriesFromPolicy(newPolicy)) {
    newEntries.emplace(entry->getID(), entry);
  }
  bool changed = false;
  for (const auto& [tc, newNhg] : newTc2Nhg) {
    auto name =
        makePbrAclEntryName(newPolicy->getID(), tc, false /*temporary*/);
    auto oldIt = oldTc2Nhg.find(tc);
    if (oldIt == oldTc2Nhg.end()) {
      aclMap.addNode(newEntries.at(name));
      changed = true;
    } else if (defaultChanged || oldIt->second != newNhg) {
      if (!aclMap.getEntryIf(name)) {
        throw FbossError(
            "changed PBR policy '",
            newPolicy->getID(),
            "' has no programmed ACL entry '",
            name,
            "' to update");
      }
      // TODO(zecheng): implement hitless (make-before-break) update here.
      aclMap.updateNode(newEntries.at(name));
      changed = true;
    }
  }
  for (const auto& [tc, _] : oldTc2Nhg) {
    if (!newTc2Nhg.count(tc)) {
      if (aclMap.removeNodeIf(makePbrAclEntryName(
              newPolicy->getID(), tc, false /*temporary*/))) {
        changed = true;
      }
    }
  }
  return changed;
}

bool applyPolicyDelta(AclMap& newAclMap, const StateDelta& delta) {
  bool changed = false;
  DeltaFunctions::forEachChanged(
      delta.getClassBasedPoliciesDelta(),
      [&](const std::shared_ptr<ClassBasedPolicyNode>& old,
          const std::shared_ptr<ClassBasedPolicyNode>& newPolicy) {
        if (!old->isReferenced()) {
          if (newPolicy->isReferenced()) {
            changed |= addPolicyEntries(newAclMap, newPolicy);
          }
          return;
        }
        if (!newPolicy->isReferenced()) {
          removePolicyEntries(newAclMap, old);
          changed = true;
          return;
        }
        changed |= applyChangedPolicy(newAclMap, old, newPolicy);
      },
      [&](const std::shared_ptr<ClassBasedPolicyNode>& newPolicy) {
        if (!newPolicy->isReferenced()) {
          return;
        }
        changed |= addPolicyEntries(newAclMap, newPolicy);
      },
      [&](const std::shared_ptr<ClassBasedPolicyNode>& old) {
        if (!old->isReferenced()) {
          return;
        }
        removePolicyEntries(newAclMap, old);
        changed = true;
      });
  return changed;
}

} // namespace

PbrAclManager::InputOutputState::InputOutputState(const StateDelta& in) {
  in.oldState()->publish();
  in.newState()->publish();
  out_.emplace_back(in.oldState(), in.newState());
}

void PbrAclManager::InputOutputState::publishLastDelta() {
  if (!out_.empty()) {
    DCHECK(out_.back().oldState()->isPublished());
    out_.back().newState()->publish();
  }
}

void PbrAclManager::InputOutputState::appendDelta(const StateDelta& delta) {
  publishLastDelta();
  DCHECK(out_.empty() || out_.back().newState() == delta.oldState());
  out_.emplace_back(delta.oldState(), delta.newState());
}

void PbrAclManager::InputOutputState::replaceLastDelta(
    const StateDelta& delta) {
  CHECK(!out_.empty());
  DCHECK(out_.back().oldState()->isPublished());
  DCHECK_EQ(out_.back().oldState(), delta.oldState());
  out_.pop_back();
  out_.emplace_back(delta.oldState(), delta.newState());
}

template <typename AddrT>
void PbrAclManager::InputOutputState::addOrUpdateRoute(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& route,
    bool withRouteDelta) {
  DCHECK(getCurrentStateDelta().oldState()->isPublished());
  if (withRouteDelta) {
    publishLastDelta();
  }
  auto oldState = getCurrentStateDelta().newState();
  auto newState = oldState->clone();
  auto fib =
      newState->getFibsInfoMap()->getFibContainer(rid)->getFib<AddrT>()->modify(
          rid, &newState);
  if (fib->getRouteIf(route->prefix())) {
    fib->updateNode(route);
  } else {
    fib->addNode(route);
  }
  if (withRouteDelta) {
    appendDelta(StateDelta(oldState, newState));
  } else {
    replaceLastDelta(StateDelta(getCurrentStateDelta().oldState(), newState));
  }
}

template void PbrAclManager::InputOutputState::addOrUpdateRoute(
    RouterID rid,
    const std::shared_ptr<Route<folly::IPAddressV4>>& route,
    bool withRouteDelta);
template void PbrAclManager::InputOutputState::addOrUpdateRoute(
    RouterID rid,
    const std::shared_ptr<Route<folly::IPAddressV6>>& route,
    bool withRouteDelta);

StateDelta PbrAclManager::InputOutputState::getCurrentStateDelta() const {
  CHECK(!out_.empty());
  return StateDelta(out_.back().oldState(), out_.back().newState());
}

std::vector<StateDelta> PbrAclManager::modifyState(
    const std::vector<StateDelta>& deltas) {
  return modifyStateImpl(deltas);
}

std::vector<StateDelta> PbrAclManager::reconstructFromSwitchState(
    const std::shared_ptr<SwitchState>& curState) {
  XLOG(DBG2) << "PbrAclManager reconstructing from switch state";
  std::vector<StateDelta> deltas;
  deltas.emplace_back(std::make_shared<SwitchState>(), curState);
  deltas = modifyStateImpl(deltas);
  CHECK(!deltas.empty());
  StateDelta combined(deltas.front().oldState(), deltas.back().newState());
  deltas.clear();
  deltas.emplace_back(std::move(combined));
  return deltas;
}

void PbrAclManager::updateDone() {
  XLOG(DBG2) << "PbrAclManager update done";
}

void PbrAclManager::updateFailed(const std::shared_ptr<SwitchState>& curState) {
  XLOG(DBG2) << "PbrAclManager update failed";
  reconstructFromSwitchState(curState);
}

void PbrAclManager::processDelta(
    const StateDelta& delta,
    InputOutputState* inOutState) {
  auto newState = delta.newState();

  // TODO(zecheng): also react to namedNextHopGroup member changes (FibInfo
  // normalized-id deltas), which shift a policy's match/redirect ids without a
  // policy delta.
  if (DeltaFunctions::isEmpty(delta.getClassBasedPoliciesDelta())) {
    return;
  }
  auto policies = newState->getClassBasedPolicies();
  size_t numPolicies = policies ? policies->numNodes() : 0;

  const auto& kPbrTableId =
      cfg::switch_config_constants::DEFAULT_PBR_ACL_TABLE();
  auto aclTableGroups = newState->getAclTableGroups();
  if (!aclTableGroups) {
    if (numPolicies > 0) {
      throwPbrTableMissing(kPbrTableId, numPolicies);
    }
    return;
  }

  bool foundTable = false;
  bool changed = false;
  for (const auto& [matcherStr, groupMap] : std::as_const(*aclTableGroups)) {
    auto group = groupMap->getAclTableGroupIf(cfg::AclStage::INGRESS);
    auto tableMap = group ? group->getAclTableMap() : nullptr;
    auto table = tableMap ? tableMap->getTableIf(kPbrTableId) : nullptr;
    if (!table) {
      continue;
    }
    foundTable = true;
    auto oldAclMap = table->getAclMap();
    auto newAclMap =
        oldAclMap ? oldAclMap->clone() : std::make_shared<AclMap>();
    if (!applyPolicyDelta(*newAclMap, delta)) {
      continue;
    }
    table
        ->modify(&newState, HwSwitchMatcher(matcherStr), cfg::AclStage::INGRESS)
        ->setAclMap(std::move(newAclMap));
    changed = true;
  }

  if (!foundTable) {
    if (numPolicies > 0) {
      throwPbrTableMissing(kPbrTableId, numPolicies);
    }
    return;
  }
  if (!changed) {
    return;
  }
  newState->publish();
  inOutState->replaceLastDelta(StateDelta(delta.oldState(), newState));
  inOutState->updated = true;
}

std::vector<StateDelta> PbrAclManager::modifyStateImpl(
    const std::vector<StateDelta>& deltas) {
  if (deltas.empty()) {
    return {};
  }
  std::vector<StateDelta> retDeltas;
  retDeltas.reserve(deltas.size());
  auto oldState = deltas.begin()->oldState();
  for (const auto& delta : deltas) {
    StateDelta curDelta(oldState, delta.newState());
    InputOutputState inOutState{curDelta};
    processDelta(curDelta, &inOutState);
    inOutState.publishLastDelta();
    for (auto& out : inOutState.moveDeltas()) {
      oldState = out.newState();
      retDeltas.emplace_back(out.oldState(), out.newState());
    }
  }
  return retDeltas;
}

} // namespace facebook::fboss
