namespace cpp2 facebook.fboss
namespace go neteng.fboss.hw_ctrl
namespace php fboss
namespace py neteng.fboss.hw_ctrl
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.hw_ctrl

include "fboss/agent/if/common.thrift"
include "fboss/agent/if/fboss.thrift"
include "fboss/agent/if/ctrl.thrift"
include "fboss/lib/phy/phy.thrift"
include "fboss/lib/phy/prbs.thrift"
include "fboss/agent/switch_state.thrift"

const i32 DEFAULT_HW_CTRL_BASE_PORT = 5931;

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
  map<
    i64,
    map<i64, list<RemoteEndpoint>>
  > getVirtualDeviceToConnectionGroups() throws (1: fboss.FbossBaseError error);

  /*
   * String formatted information of givens Hw Objects.
   */
  string listHwObjects(
    1: list<ctrl.HwObjectType> objects,
    2: bool cached,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Type of boot performed by the hw agent
   */
  ctrl.BootType getBootType();

  /*
   * Get the PRBS state on an interface.
   */
  prbs.InterfacePrbsState getInterfacePrbsState(
    1: string interface,
    2: phy.PortComponent component,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Get the PRBS settings on all interfaces.
   */
  map<string, prbs.InterfacePrbsState> getAllInterfacePrbsStates(
    1: phy.PortComponent component,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Get the PRBS stats on an interface.
   */
  phy.PrbsStats getInterfacePrbsStats(
    1: string interface,
    2: phy.PortComponent component,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Get the PRBS stats on all interfaces.
   */
  map<string, phy.PrbsStats> getAllInterfacePrbsStats(
    1: phy.PortComponent component,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Clear the PRBS stats counters on an interface.
   */
  void clearInterfacePrbsStats(
    1: string interface,
    2: phy.PortComponent component,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Bulk clear the PRBS stats counters on interfaces.
   */
  void bulkClearInterfacePrbsStats(
    1: list<string> interfaces,
    2: phy.PortComponent component,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Reconstruct switch state.
   */
  switch_state.SwitchState reconstructSwitchState() throws (
    1: fboss.FbossBaseError error,
  );
}
