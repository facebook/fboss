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
#include "fboss/agent/state/PbrUtils.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

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
    if (!aclMap.removeNodeIf(makePbrAclEntryName(policy->getID(), tc))) {
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

bool applyPolicyDelta(AclMap& newAclMap, const StateDelta& delta) {
  bool changed = false;
  DeltaFunctions::forEachChanged(
      delta.getClassBasedPoliciesDelta(),
      [&](const std::shared_ptr<ClassBasedPolicyNode>& /*old*/,
          const std::shared_ptr<ClassBasedPolicyNode>& /*newPolicy*/) {},
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

std::vector<StateDelta> PbrAclManager::modifyState(
    const std::vector<StateDelta>& deltas) {
  return modifyStateImpl(deltas);
}

std::vector<StateDelta> PbrAclManager::reconstructFromSwitchState(
    const std::shared_ptr<SwitchState>& curState) {
  XLOG(DBG2) << "PbrAclManager reconstructing from switch state";
  std::vector<StateDelta> deltas;
  deltas.emplace_back(std::make_shared<SwitchState>(), curState);
  return modifyStateImpl(deltas);
}

void PbrAclManager::updateDone() {
  XLOG(DBG2) << "PbrAclManager update done";
}

void PbrAclManager::updateFailed(const std::shared_ptr<SwitchState>& curState) {
  XLOG(DBG2) << "PbrAclManager update failed";
  reconstructFromSwitchState(curState);
}

std::shared_ptr<SwitchState> PbrAclManager::processDelta(
    const StateDelta& delta) {
  auto newState = delta.newState();

  // TODO(zecheng): also react to namedNextHopGroup member changes (FibInfo
  // normalized-id deltas), which shift a policy's match/redirect ids without a
  // policy delta.
  if (DeltaFunctions::isEmpty(delta.getClassBasedPoliciesDelta())) {
    return newState;
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
    return newState;
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
    return newState;
  }
  if (!changed) {
    return newState;
  }
  newState->publish();
  return newState;
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
    auto modifiedState = processDelta(StateDelta(oldState, delta.newState()));
    retDeltas.emplace_back(oldState, modifiedState);
    oldState = modifiedState;
  }
  return retDeltas;
}

} // namespace facebook::fboss
