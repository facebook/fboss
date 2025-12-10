package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/agent/if/common.thrift"

struct ShowRouteModel {
  1: list<RouteEntry> routeEntries;
}

struct ShowRouteDetailsModel {
  1: list<RouteDetailEntry> routeEntries;
}

struct ShowRouteSummaryModel {
  1: i32 numV4Routes;
  2: i32 numV6Small;
  3: i32 numV6Big;
  4: i32 numV6;
  5: i32 hwEntriesUsed;
}

struct MplsActionInfo {
  1: string action;
  2: optional i32 swapLabel;
  3: optional list<i32> pushLabels;
}

struct NextHopInfo {
  1: string addr;
  2: i32 weight = 0;
  3: optional MplsActionInfo mplsAction;
  4: optional string ifName;
  5: optional i32 interfaceID;
  6: optional common.NetworkTopologyInformation topologyInfo;
}

struct ClientAndNextHops {
  1: i32 clientId;
  2: list<NextHopInfo> nextHops;
}

struct RouteEntry {
  1: string networkAddress;
  2: list<NextHopInfo> nextHops;
  3: string overridenEcmpMode;
  4: optional list<NextHopInfo> overridenNextHops;
}

struct RouteDetailEntry {
  1: string ip;
  2: i32 prefixLength;
  3: string action;
  4: list<ClientAndNextHops> nextHopMulti;
  5: bool isConnected;
  6: string adminDistance;
  7: list<NextHopInfo> nextHops;
  8: string counterID;
  9: string classID;
  10: string overridenEcmpMode;
  11: optional list<NextHopInfo> overridenNextHops;
  12: i32 nhopsLostDueToOverride = 0;
  13: map<string, common.NetworkTopologyInformation> nhAddressToTopologyInfo;
}
