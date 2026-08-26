include "neteng/fboss/bgp/if/bgp_thrift.thrift"

package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowBgpTableSummaryModel {
  // One RIB summary per address family (IPv4, IPv6).
  1: list<bgp_thrift.TRibSummary> summaries;
}
