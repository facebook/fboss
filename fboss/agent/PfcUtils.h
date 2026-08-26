// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "fboss/agent/state/PortPgConfig.h" // PortPgConfigs
#include "fboss/agent/types.h" // PfcPriority

namespace facebook::fboss::utility {

// Lossless PG ids (PGs with non-zero headroom), ascending.
std::vector<int16_t> findPfcEnabledPgIds(const PortPgConfigs& portPgCfgs);

// PFC priorities whose mapped PG (via pfcPriorityToPgId) is lossless; identity
// (priority == PG id) when pfcPriorityToPgId is empty. Ascending; empty if
// none.
std::vector<PfcPriority> findPfcEnabledPriorities(
    const PortPgConfigs& portPgCfgs,
    const std::map<int16_t, int16_t>& pfcPriorityToPgId);

// Queues serving the given priorities (via pfcPriorityToQueueId); identity
// (queue == priority) when pfcPriorityToQueueId is empty. Ascending, de-duped.
std::vector<uint8_t> findPfcEnabledQueues(
    const std::vector<PfcPriority>& enabledPriorities,
    const std::map<int16_t, int16_t>& pfcPriorityToQueueId);

} // namespace facebook::fboss::utility
