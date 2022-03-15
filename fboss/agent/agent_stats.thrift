namespace cpp2 facebook.fboss
namespace py neteng.fboss.agent_stats
namespace go neteng.fboss.agent_stats
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.agent_stats

include "fboss/agent/hw/hardware_stats.thrift"

struct AgentStats {
  1: map<string, hardware_stats.HwPortStats> hwPortStats;
  2: map<string, hardware_stats.HwTrunkStats> hwTrunkStats;
  3: hardware_stats.HwResourceStats hwResourceStats;
}
