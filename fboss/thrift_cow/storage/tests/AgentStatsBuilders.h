// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/agent_stats_types.h"
#include "fboss/thrift_cow/storage/tests/TestDataFactory.h"

namespace facebook::fboss::fsdb::test {

using facebook::fboss::AgentStats;
using facebook::fboss::test_data::AgentStatsScale;

void populateHwResourceStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateHwAsicErrorsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateCpuPortStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateSwitchDropStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateSwitchWatermarkStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateFabricReachabilityStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateSwitchPipelineStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateSysPortShelStateMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateAsicTemp(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateFlowletStats(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateSimpleCounters(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateHwAgentStatus(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);
void populateFabricOverdrainPct(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version = 0);

// Zero out all scale fields except those for the given target field.
// If targetField is empty, returns scale unchanged.
// Returns false if targetField is not a recognized AgentStats field.
bool filterAgentStatsScaleForPath(
    AgentStatsScale& scale,
    const std::string& targetField);

} // namespace facebook::fboss::fsdb::test
