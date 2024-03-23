namespace cpp2 facebook.fboss
namespace go neteng.fboss.hw_ctrl
namespace php fboss
namespace py neteng.fboss.hw_ctrl
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.hw_ctrl

include "fboss/agent/if/common.thrift"
include "fboss/agent/if/fboss.thrift"
include "fboss/agent/if/ctrl.thrift"

struct RemoteEndpoint {
  1: i64 switchId;
  2: string switchName;
  3: list<string> connectingPorts;
}

service FbossHwCtrl {
  /*
   * Enables submitting diag cmds to the switch
   */
  common.fbstring diagCmd(
    1: common.fbstring cmd,
    2: common.ClientInformation client,
    3: i16 serverTimeoutMsecs = 0,
    4: bool bypassFilter = false,
  );

  /*
   * Get formatted string for diag cmd filters configuration
   */
  common.fbstring cmdFiltersAsString() throws (1: fboss.FbossBaseError error);

  // an api  to test hw switch handler in hardware agnostic way
  common.SwitchRunState getHwSwitchRunState();

  map<i64, ctrl.FabricEndpoint> getHwFabricReachability();
  map<string, ctrl.FabricEndpoint> getHwFabricConnectivity();
  map<string, list<string>> getHwSwitchReachability(
    1: list<string> switchNames,
  ) throws (1: fboss.FbossBaseError error);

  /* clear stats for specified port(s) */
  void clearHwPortStats(1: list<i32> ports);

  /* clears stats for all ports */
  void clearAllHwPortStats();

  list<ctrl.L2EntryThrift> getHwL2Table() throws (
    1: fboss.FbossBaseError error,
  );
}
