include "neteng/fboss/bgp/if/bgp_thrift.thrift"

package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowBgpUpdateGroupModel {
  // Populated only for the single-group detail view.
  1: list<bgp_thrift.TUpdateGroupInfo> update_groups;

  2: bool enable_update_group;

  3: bool detail_mode;

  // Populated only for the all-groups summary view.
  4: list<bgp_thrift.TUpdateGroupSummary> update_group_summaries;
}
