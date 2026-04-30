include "fboss/bgp/if/bgp_thrift.thrift"

package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowBgpSummaryModel {
  1: list<bgp_thrift.TBgpSession> sessions;
  2: bgp_thrift.TBgpLocalConfig config;
  3: bgp_thrift.TBgpDrainState drain_state;
  4: list<bgp_thrift.TPeerEgressStats> peer_egress_stats;
}
