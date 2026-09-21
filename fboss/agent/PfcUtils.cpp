// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/PfcUtils.h"

#include <algorithm>
#include <set>

#include "fboss/agent/BufferUtils.h" // isLosslessPg

namespace facebook::fboss::utility {

std::vector<int16_t> findPfcEnabledPgIds(const PortPgConfigs& portPgCfgs) {
  std::vector<int16_t> pgIds;
  for (const auto& portPgCfg : portPgCfgs) {
    // PortPgConfigs entries come from SwitchState and are never null; assert to
    // document the invariant rather than crash on a raw deref.
    DCHECK(portPgCfg) << "null PortPgConfig in PortPgConfigs";
    if (isLosslessPg(*portPgCfg)) {
      pgIds.push_back(static_cast<int16_t>(portPgCfg->getID()));
    }
  }
  std::sort(pgIds.begin(), pgIds.end());
  return pgIds;
}

std::vector<PfcPriority> findPfcEnabledPriorities(
    const PortPgConfigs& portPgCfgs,
    const std::map<int16_t, int16_t>& pfcPriorityToPgId) {
  auto losslessPgIdsVec = findPfcEnabledPgIds(portPgCfgs);

  std::vector<PfcPriority> priorities;
  if (pfcPriorityToPgId.empty()) {
    // Identity fallback: the lossless PG ids are the enabled PFC priorities.
    for (auto pgId : losslessPgIdsVec) {
      priorities.emplace_back(static_cast<uint8_t>(pgId));
    }
  } else {
    std::set<int16_t> losslessPgIds(
        losslessPgIdsVec.begin(), losslessPgIdsVec.end());
    for (const auto& [pri, pgId] : pfcPriorityToPgId) {
      if (losslessPgIds.find(pgId) != losslessPgIds.end()) {
        priorities.emplace_back(static_cast<uint8_t>(pri));
      }
    }
  }
  // Return priorities in ascending order, independent of the iteration order
  // of the inputs.
  std::sort(priorities.begin(), priorities.end());
  return priorities;
}

std::vector<uint8_t> findPfcEnabledQueues(
    const std::vector<PfcPriority>& enabledPriorities,
    const std::map<int16_t, int16_t>& pfcPriorityToQueueId) {
  std::set<uint8_t> queues;
  const bool identity = pfcPriorityToQueueId.empty();
  for (auto pri : enabledPriorities) {
    if (identity) {
      // Identity fallback: queue == priority.
      queues.insert(static_cast<uint8_t>(pri));
      continue;
    }
    // Absent from a non-empty map => no queue.
    auto it = pfcPriorityToQueueId.find(static_cast<int16_t>(pri));
    if (it != pfcPriorityToQueueId.end()) {
      queues.insert(static_cast<uint8_t>(it->second));
    }
  }
  return std::vector<uint8_t>(queues.begin(), queues.end());
}

} // namespace facebook::fboss::utility
