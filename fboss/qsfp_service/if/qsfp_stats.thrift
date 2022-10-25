namespace cpp2 facebook.fboss.stats
namespace go neteng.fboss.qsfp_stats
namespace py neteng.fboss.qsfp_stats
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.qsfp_stats

include "fboss/agent/hw/hardware_stats.thrift"
include "fboss/lib/phy/phy.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"

struct QsfpStats {
  1: map<string, phy.PhyStats> phyStats;
  2: map<i32, transceiver.TcvrStats> tcvrStats;
  3: map<string, hardware_stats.HwPortStats> portStats;
}
