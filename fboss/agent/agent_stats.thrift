namespace cpp2 facebook.fboss
namespace py neteng.fboss.agent_stats
namespace go neteng.fboss.agent_stats
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.agent_stats

cpp_include "folly/container/F14Map.h"
cpp_include "folly/FBString.h"

include "fboss/agent/hw/hardware_stats.thrift"
include "thrift/annotation/cpp.thrift"
include "fboss/lib/phy/phy.thrift"

struct HwAgentEventSyncStatus {
  1: i32 statsEventSyncActive;
  2: i32 fdbEventSyncActive;
  3: i32 linkEventSyncActive;
  4: optional i32 linkActiveEventSyncActive_DEPRECATED;
  5: i32 rxPktEventSyncActive;
  6: i32 txPktEventSyncActive;
  7: i64 statsEventSyncDisconnects;
  8: i64 fdbEventSyncDisconnects;
  9: i64 linkEventSyncDisconnects;
  10: i64 linkActiveEventSyncDisconnects_DEPRECATED;
  11: i64 rxPktEventSyncDisconnects;
  12: i64 txPktEventSyncDisconnects;
  13: i32 switchReachabilityChangeEventSyncActive;
  14: i64 switchReachabilityChangeEventSyncDisconnects;
}

struct AgentStats {
  @cpp.Type{template = "folly::F14FastMap"}
  1: map<string, hardware_stats.HwPortStats> hwPortStats;
  2: map<string, hardware_stats.HwTrunkStats> hwTrunkStats;
  3: hardware_stats.HwResourceStats hwResourceStats;
  4: hardware_stats.HwAsicErrors hwAsicErrors;
  5: i64 linkFlaps;
  7: map<string, hardware_stats.HwSysPortStats> sysPortStats;
  8: hardware_stats.TeFlowStats teFlowStats;
  // Deprecate this once newly added switchWatermarkStatsMap is
  // available in prod!
  9: hardware_stats.HwBufferPoolStats bufferPoolStats;
  10: map<i16, hardware_stats.HwResourceStats> hwResourceStatsMap;
  11: map<i16, hardware_stats.HwAsicErrors> hwAsicErrorsMap;
  12: map<i16, hardware_stats.TeFlowStats> teFlowStatsMap;
  13: map<i16, hardware_stats.HwBufferPoolStats> bufferPoolStatsMap_DEPRECATED;
  14: map<i16, map<string, hardware_stats.HwSysPortStats>> sysPortStatsMap;
  15: map<i16, hardware_stats.HwSwitchDropStats> switchDropStatsMap;
  16: hardware_stats.HwFlowletStats flowletStats;
  17: map<string, phy.PhyStats> phyStats;
  18: map<i16, hardware_stats.HwFlowletStats> flowletStatsMap;
  19: i64 trappedPktsDropped;
  20: i64 threadHeartBeatMiss;
  21: map<i16, hardware_stats.CpuPortStats> cpuPortStatsMap;
  22: map<i16, i64> hwagentConnectionStatus;
  23: map<i16, i64> hwagentOperSyncTimeoutCount;
  24: map<i16, HwAgentEventSyncStatus> hwAgentEventSyncStatusMap;
  25: map<i16, i16> fabricOverdrainPct;
  26: map<i16, hardware_stats.HwSwitchWatermarkStats> switchWatermarkStatsMap;
  27: map<
    i16,
    hardware_stats.FabricReachabilityStats
  > fabricReachabilityStatsMap;
}
