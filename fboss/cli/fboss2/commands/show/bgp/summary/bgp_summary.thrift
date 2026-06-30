include "neteng/fboss/bgp/if/bgp_thrift.thrift"

package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowBgpSummaryModel {
  1: list<bgp_thrift.TBgpSession> sessions;
  2: bgp_thrift.TBgpLocalConfig config;
  3: bgp_thrift.TBgpDrainState drain_state;
  4: list<bgp_thrift.TPeerEgressStats> peer_egress_stats;
  // BGP++ process uptime in seconds (time since the daemon started).
  5: i64 process_uptime_seconds;
  // Total number of prefixes in the loc-RIB (RibBase::ribEntries_ count).
  6: i64 total_prefix_count;
  // Global monotonic RIB table version.
  7: i64 rib_version;
}
