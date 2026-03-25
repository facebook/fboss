package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/cli/fboss2/commands/show/route/model.thrift"

struct ShowMplsRouteModel {
  1: list<MplsEntry> mplsEntries;
}

struct MplsEntry {
  1: i32 topLabel;
  2: string action;
  3: list<ClientAndNextHops> nextHopMulti;
  4: i32 adminDistance;
  5: list<NextHopInfo> nextHops;
}
