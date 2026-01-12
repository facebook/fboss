namespace cpp2 facebook.fboss
namespace go neteng.fboss.highfreq
namespace php fboss
namespace py neteng.fboss.highfreq
namespace py.asyncio neteng.fboss.asyncio.hw_ctrl
namespace py3 neteng.fboss

include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

// Configure the scheduler settings for high frequency stats collection.
// A pair of stats will be collected after waiting for statsWaitDurationInMicroseconds.
// The stats collection will be run for statsCollectionDurationInMicroseconds.
struct HfSchedulerConfig {
  1: i32 statsWaitDurationInMicroseconds = 20000; // 20ms
  2: i32 statsCollectionDurationInMicroseconds = 10000000; // 10s
}

// Config for high frequency stats collection for port stats.
struct HfPortStatsCollectionConfig {
  1: bool includePfcRx = true;
  2: bool includePfcTx = true;
  3: bool includeQueueWatermark = false;
  4: bool includePgWatermark = false;
}

// Config for high frequency stats collection for port stats on a switch.
union HfPortStatsConfig {
  1: HfPortStatsCollectionConfig allPortsConfig;
  // filterConfig returns the stats only for the set of ports that are keys
  // specified by the filterConfig map. The stats defined in the
  // HfPortStatsCollectionConfig value will be collected for that particular
  // port.
  @cpp.Type{template = "folly::F14FastMap"}
  2: map<string, HfPortStatsCollectionConfig> filterConfig;
}

// Config for high frequency stats collection on a switch.
struct HfStatsConfig {
  1: HfPortStatsConfig portStatsConfig = {allPortsConfig: {}};
  2: bool includeDeviceWatermark = false;
}

// Config for high frequency stats collection on a switch.
struct HighFrequencyStatsCollectionConfig {
  1: HfSchedulerConfig schedulerConfig;
  2: HfStatsConfig statsConfig;
}

// Options to get high frequency stats with limit and timestamp range filtering.
struct GetHighFrequencyStatsOptions {
  1: i64 limit = 1024;
  2: optional i64 beginTimestampUs;
  3: optional i64 endTimestampUs;
}
