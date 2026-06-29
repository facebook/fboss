include "neteng/fboss/bgp/if/bgp_thrift.thrift"

package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowBgpUpdateGroupModel {
  1: list<bgp_thrift.TUpdateGroupInfo> update_groups;

  2: bool enable_update_group;

  3: bool detail_mode;
}
