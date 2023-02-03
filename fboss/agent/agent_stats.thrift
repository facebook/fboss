namespace cpp2 facebook.fboss
namespace py neteng.fboss.agent_stats
namespace go neteng.fboss.agent_stats
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.agent_stats

cpp_include "folly/container/F14Map.h"
cpp_include "folly/FBString.h"

include "fboss/agent/hw/hardware_stats.thrift"

struct AgentStats {
  1: map<string, hardware_stats.HwPortStats> (
    cpp.template = 'folly::F14FastMap',
  ) hwPortStats;
  2: map<string, hardware_stats.HwTrunkStats> hwTrunkStats;
  3: hardware_stats.HwResourceStats hwResourceStats;
  4: hardware_stats.HwAsicErrors hwAsicErrors;
  5: i64 linkFlaps;
  7: map<string, hardware_stats.HwSysPortStats> sysPortStats;
  8: hardware_stats.TeFlowStats teFlowStats;
  9: hardware_stats.HwBufferPoolStats bufferPoolStats;
}
