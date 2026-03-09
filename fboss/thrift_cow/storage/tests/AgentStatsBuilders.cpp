// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/AgentStatsBuilders.h"

#include <fmt/format.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/if/gen-cpp2/asic_temp_types.h"

namespace facebook::fboss::fsdb::test {

void populateHwResourceStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (!scale.hasHwResourceStats) {
    return;
  }

  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    HwResourceStats hwResStats;
    hwResStats.hw_table_stats_stale() = false;
    hwResStats.l3_host_max() = 131072;
    hwResStats.l3_host_used() = 100 + asicIdx + version;
    hwResStats.l3_host_free() = 130972 - asicIdx;
    hwResStats.l3_ipv4_host_used() = 10 + asicIdx + version;
    hwResStats.l3_ipv4_host_free() = 131062 - asicIdx;
    hwResStats.l3_ipv6_host_used() = 90 + asicIdx + version;
    hwResStats.l3_ipv6_host_free() = 130982 - asicIdx;
    hwResStats.l3_nexthops_max() = 65536;
    hwResStats.l3_nexthops_used() = 200 + asicIdx + version;
    hwResStats.l3_nexthops_free() = 65336 - asicIdx;
    hwResStats.l3_ipv4_nexthops_free() = 32000 - asicIdx;
    hwResStats.l3_ipv6_nexthops_free() = 32000 - asicIdx;
    hwResStats.l3_ecmp_groups_max() = 4096;
    hwResStats.l3_ecmp_groups_used() = 50 + asicIdx + version;
    hwResStats.l3_ecmp_groups_free() = 4046 - asicIdx;
    hwResStats.l3_ecmp_group_members_free() = 16000 - asicIdx;
    hwResStats.lpm_ipv4_max() = 65536;
    hwResStats.lpm_ipv4_used() = 1000 + asicIdx + version;
    hwResStats.lpm_ipv4_free() = 64536 - asicIdx;
    hwResStats.lpm_ipv6_mask_0_64_max() = 32768;
    hwResStats.lpm_ipv6_mask_0_64_used() = 500 + asicIdx + version;
    hwResStats.lpm_ipv6_mask_0_64_free() = 32268 - asicIdx;
    hwResStats.lpm_ipv6_mask_65_127_max() = 32768;
    hwResStats.lpm_ipv6_mask_65_127_used() = 300 + asicIdx + version;
    hwResStats.lpm_ipv6_mask_65_127_free() = 32468 - asicIdx;
    hwResStats.lpm_ipv6_free() = 64736 - asicIdx;
    hwResStats.acl_entries_max() = 8192;
    hwResStats.acl_entries_used() = 100 + asicIdx + version;
    hwResStats.acl_entries_free() = 8092 - asicIdx;
    hwResStats.acl_counters_max() = 4096;
    hwResStats.acl_counters_used() = 50 + asicIdx + version;
    hwResStats.acl_counters_free() = 4046 - asicIdx;
    hwResStats.mirrors_max() = 8;
    hwResStats.mirrors_used() = 2;
    hwResStats.mirrors_free() = 6;
    hwResStats.mirrors_span() = 0;
    hwResStats.mirrors_erspan() = 0;
    hwResStats.mirrors_sflow() = 2;
    hwResStats.system_ports_free() = 1000 - asicIdx;
    hwResStats.voqs_free() = 8000 - asicIdx;
    hwResStats.lpm_slots_max() = 262144;
    hwResStats.lpm_slots_used() = 5000 + asicIdx + version;
    hwResStats.lpm_slots_free() = 257144 - asicIdx;
    hwResStats.em_entries_max() = 524288;
    hwResStats.em_entries_used() = 1000 + asicIdx + version;
    hwResStats.em_entries_free() = 523288 - asicIdx;
    hwResStats.em_counters_max() = 8192;
    hwResStats.em_counters_used() = 100 + asicIdx + version;
    hwResStats.em_counters_free() = 8092 - asicIdx;

    stats.hwResourceStatsMap()[asicIdx] = std::move(hwResStats);
  }
}

void populateHwAsicErrorsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (!scale.hasHwAsicErrors) {
    return;
  }

  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    HwAsicErrors asicErrors;
    asicErrors.parityErrors() = 0;
    asicErrors.correctedParityErrors() = asicIdx * 10 + version;
    asicErrors.uncorrectedParityErrors() = 0;
    asicErrors.asicErrors() = 0;

    stats.hwAsicErrorsMap()[asicIdx] = std::move(asicErrors);
  }
}

void populateCpuPortStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (!scale.hasCpuPortStats) {
    return;
  }

  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    CpuPortStats cpuPortStats;

    for (int queueId = 0; queueId < 10; ++queueId) {
      cpuPortStats.queueToName_()[queueId] = fmt::format("cpuQueue{}", queueId);
      cpuPortStats.queueInPackets_()[queueId] =
          1000 + queueId * 100 + asicIdx * 10 + version;
      cpuPortStats.queueDiscardPackets_()[queueId] = queueId % 3 == 0 ? 1 : 0;
    }

    stats.cpuPortStatsMap()[asicIdx] = std::move(cpuPortStats);
  }
}

void populateSwitchDropStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (!scale.hasSwitchDropStats) {
    return;
  }

  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    HwSwitchDropStats dropStats;
    dropStats.globalDrops() = asicIdx * 100 + version;
    dropStats.globalReachabilityDrops() = asicIdx * 50 + version;
    dropStats.packetIntegrityDrops() = 0;
    dropStats.fdrCellDrops() = 0;
    dropStats.voqResourceExhaustionDrops() = 0;
    dropStats.globalResourceExhaustionDrops() = 0;
    dropStats.sramResourceExhaustionDrops() = 0;
    dropStats.vsqResourceExhaustionDrops() = 0;
    dropStats.dropPrecedenceDrops() = 0;
    dropStats.queueResolutionDrops() = 0;
    dropStats.ingressPacketPipelineRejectDrops() = 0;
    dropStats.corruptedCellPacketIntegrityDrops() = 0;

    stats.switchDropStatsMap()[asicIdx] = std::move(dropStats);
  }
}

void populateSwitchWatermarkStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (!scale.hasSwitchWatermarkStats) {
    return;
  }

  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    HwSwitchWatermarkStats watermarkStats;
    watermarkStats.deviceWatermarkBytes() = 524288 + asicIdx * 10000 + version;

    stats.switchWatermarkStatsMap()[asicIdx] = std::move(watermarkStats);
  }
}

void populateFabricReachabilityStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (!scale.hasFabricReachabilityStats) {
    return;
  }

  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    FabricReachabilityStats reachStats;
    reachStats.mismatchCount() = 0;
    reachStats.missingCount() = 0;

    stats.fabricReachabilityStatsMap()[asicIdx] = std::move(reachStats);
  }
}

void populateSwitchPipelineStatsMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (!scale.hasSwitchPipelineStats) {
    return;
  }

  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    HwSwitchPipelineStats pipelineStats;
    for (int16_t pipe = 0; pipe < 4; ++pipe) {
      pipelineStats.rxCells()[pipe] = 1000 + pipe * 100 + version;
      pipelineStats.txCells()[pipe] = 900 + pipe * 100 + version;
      pipelineStats.rxBytes()[pipe] = 100000 + pipe * 10000 + version;
      pipelineStats.txBytes()[pipe] = 90000 + pipe * 10000 + version;
      pipelineStats.rxWatermarkLevels()[pipe] = 50 + pipe;
      pipelineStats.txWatermarkLevels()[pipe] = 40 + pipe;
      pipelineStats.curOccupancyBytes()[pipe] = 8192 + pipe * 1024;
      pipelineStats.globalDrops()[pipe] = 0;
    }

    stats.switchPipelineStatsMap()[asicIdx] = std::move(pipelineStats);
  }
}

void populateSysPortShelStateMap(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (scale.sysPortShelStateCount == 0) {
    return;
  }

  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    std::map<int32_t, cfg::PortState> sysPortStates;
    for (int i = 0; i < scale.sysPortShelStateCount; ++i) {
      sysPortStates[i] = cfg::PortState::ENABLED;
    }
    stats.sysPortShelStateMap()[asicIdx] = std::move(sysPortStates);
  }
}

void populateAsicTemp(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (scale.asicTempCount == 0) {
    return;
  }

  for (int i = 0; i < scale.asicTempCount; ++i) {
    asic_temp::AsicTempData tempData;
    std::string sensorName = fmt::format("sensor{}", i);
    tempData.name() = sensorName;
    tempData.value() = 45.0f + i + version;
    tempData.timeStamp() = 1700000000 + i + version;
    stats.asicTemp()[sensorName] = std::move(tempData);
  }
}

void populateFlowletStats(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (!scale.hasFlowletStats) {
    return;
  }

  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    HwFlowletStats flowletStats;
    flowletStats.l3EcmpDlbFailPackets() = 0;

    stats.flowletStatsMap()[asicIdx] = std::move(flowletStats);
  }
}

void populateSimpleCounters(
    AgentStats& stats,
    const AgentStatsScale& /*scale*/,
    int64_t version) {
  stats.linkFlaps() = version;
  stats.trappedPktsDropped() = version;
  stats.threadHeartBeatMiss() = version;
  stats.ecmpOverShelDisabledPort() = version;
}

void populateHwAgentStatus(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    stats.hwagentConnectionStatus()[asicIdx] = 1;
    stats.hwagentOperSyncTimeoutCount()[asicIdx] = version;

    HwAgentEventSyncStatus syncStatus;
    syncStatus.statsEventSyncActive() = 1;
    syncStatus.fdbEventSyncActive() = 1;
    syncStatus.linkEventSyncActive() = 1;
    syncStatus.rxPktEventSyncActive() = 1;
    syncStatus.txPktEventSyncActive() = 1;
    syncStatus.statsEventSyncDisconnects() = 0;
    syncStatus.fdbEventSyncDisconnects() = 0;
    syncStatus.linkEventSyncDisconnects() = 0;
    syncStatus.rxPktEventSyncDisconnects() = 0;
    syncStatus.txPktEventSyncDisconnects() = 0;
    syncStatus.switchReachabilityChangeEventSyncActive() = 1;
    syncStatus.switchReachabilityChangeEventSyncDisconnects() = 0;
    stats.hwAgentEventSyncStatusMap()[asicIdx] = std::move(syncStatus);
  }
}

void populateFabricOverdrainPct(
    AgentStats& stats,
    const AgentStatsScale& scale,
    int64_t version) {
  if (!scale.hasFabricOverdrainPct) {
    return;
  }

  for (int asicIdx = 0; asicIdx < scale.asicCount; ++asicIdx) {
    stats.fabricOverdrainPct()[asicIdx] = static_cast<double>(version);
  }
}

} // namespace facebook::fboss::fsdb::test
