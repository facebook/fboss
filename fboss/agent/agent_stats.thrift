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

struct AgentStats {
  @cpp.Type{template = "folly::F14FastMap"}
  1: map<string, hardware_stats.HwPortStats> hwPortStats;
  2: map<string, hardware_stats.HwTrunkStats> hwTrunkStats;
  3: hardware_stats.HwResourceStats hwResourceStats;
  4: hardware_stats.HwAsicErrors hwAsicErrors;
  5: i64 linkFlaps;
  7: map<string, hardware_stats.HwSysPortStats> sysPortStats;
  8: hardware_stats.TeFlowStats teFlowStats;
  9: hardware_stats.HwBufferPoolStats bufferPoolStats;
  10: map<i16, hardware_stats.HwResourceStats> hwResourceStatsMap;
  11: map<i16, hardware_stats.HwAsicErrors> hwAsicErrorsMap;
  12: map<i16, hardware_stats.TeFlowStats> teFlowStatsMap;
  13: map<i16, hardware_stats.HwBufferPoolStats> bufferPoolStatsMap;
  14: map<i16, map<string, hardware_stats.HwSysPortStats>> sysPortStatsMap;
  15: map<i16, hardware_stats.HwSwitchDropStats> switchDropStatsMap;
  16: hardware_stats.HwFlowletStats flowletStats;
  17: map<string, phy.PhyStats> phyStats;
  18: map<i16, hardware_stats.HwFlowletStats> flowletStatsMap;
  19: i64 trappedPktsDropped;
  20: i64 threadHeartBeatMiss;
  21: map<i16, hardware_stats.CpuPortStats> cpuPortStatsMap;
}
