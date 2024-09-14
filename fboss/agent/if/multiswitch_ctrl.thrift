namespace cpp2 facebook.fboss.multiswitch
namespace go neteng.fboss.multiswitch
namespace php fboss.multiswitch
namespace py neteng.fboss.multiswitch_ctrl
namespace py3 neteng.fboss.multiswitch
namespace py.asyncio neteng.fboss.asyncio.multiswitch_ctrl

include "fboss/fsdb/if/fsdb_oper.thrift"
include "fboss/agent/if/common.thrift"
include "fboss/agent/if/ctrl.thrift"
include "thrift/annotation/cpp.thrift"
include "fboss/lib/phy/phy.thrift"
include "fboss/agent/hw/hardware_stats.thrift"
include "thrift/annotation/thrift.thrift"

@cpp.Type{name = "std::unique_ptr<folly::IOBuf>"}
typedef binary fbbinary

struct LinkEvent {
  1: i32 port;
  2: bool up;
  3: optional phy.LinkFaultStatus iPhyLinkFaultStatus;
}

struct LinkActiveEvent {
  1: map<i32, bool> port2IsActive;
}

struct FabricConnectivityDelta {
  1: optional ctrl.FabricEndpoint oldConnectivity;
  2: optional ctrl.FabricEndpoint newConnectivity;
}

struct LinkConnectivityEvent {
  1: map<i32, FabricConnectivityDelta> port2ConnectivityDelta;
}

struct LinkChangeEvent {
  1: optional LinkEvent linkStateEvent;
  2: LinkActiveEvent linkActiveEvents;
  3: LinkConnectivityEvent linkConnectivityEvents;
}

struct SwitchReachabilityChangeEvent {
  1: map<i64, set<i32>> switchId2FabricPorts;
}

struct FdbEvent {
  1: ctrl.L2EntryThrift entry;
  2: ctrl.L2EntryUpdateType updateType;
}

struct TxPacket {
  1: fbbinary data;
  2: optional i32 port;
  3: optional i32 queue;
  4: i32 length;
}

struct RxPacket {
  1: fbbinary data;
  2: i32 port;
  3: optional i32 aggPort;
  4: optional i16 vlan;
  5: i32 length;
  6: optional ctrl.CpuCosQueueId cosQueue;
}

struct StateOperDelta {
  1: fsdb_oper.OperDelta operDelta;
  2: bool transaction;
  3: i64 seqNum;
  # OperDelta can be applied to empty state to create full switchstate
  4: bool isFullState;
  5: common.HwWriteBehavior hwWriteBehavior = common.HwWriteBehavior.WRITE;
}

struct HwSwitchStats {
  1: i64 timestamp;
  @cpp.Type{template = "folly::F14FastMap"}
  2: map<string, hardware_stats.HwPortStats> hwPortStats;
  3: map<string, hardware_stats.HwTrunkStats> hwTrunkStats;
  4: hardware_stats.HwResourceStats hwResourceStats;
  5: hardware_stats.HwAsicErrors hwAsicErrors;
  6: map<string, hardware_stats.HwSysPortStats> sysPortStats;
  7: hardware_stats.TeFlowStats teFlowStats;
  8: hardware_stats.HwBufferPoolStats bufferPoolStats_DEPRECATED;
  9: hardware_stats.FabricReachabilityStats fabricReachabilityStats;
  10: hardware_stats.HwSwitchFb303GlobalStats fb303GlobalStats;
  11: hardware_stats.CpuPortStats cpuPortStats;
  12: hardware_stats.HwSwitchDropStats switchDropStats;
  13: hardware_stats.HwFlowletStats flowletStats;
  14: map<i32, phy.PhyInfo> phyInfo;
  15: hardware_stats.AclStats aclStats;
  16: hardware_stats.HwSwitchWatermarkStats switchWatermarkStats;
}

service MultiSwitchCtrl {
  /* notify fdb event through sink */
  sink<FdbEvent, bool> notifyFdbEvent(1: i64 switchId);

  /* notify rx packet through sink */
  sink<RxPacket, bool> notifyRxPacket(1: i64 switchId);

  /* keep getting tx packet from SwSwitch, through stream */
  stream<TxPacket> getTxPackets(1: i64 switchId);

  /* notify link change event*/
  @thrift.Priority{level = thrift.RpcPriority.IMPORTANT}
  sink<LinkChangeEvent, bool> notifyLinkChangeEvent(1: i64 switchId);

  /* switch reachability change event*/
  sink<SwitchReachabilityChangeEvent, bool> notifySwitchReachabilityChangeEvent(
    1: i64 switchId,
  );

  /* get next oper delta from SwSwitch */
  @thrift.Priority{level = thrift.RpcPriority.HIGH}
  StateOperDelta getNextStateOperDelta(
    1: i64 switchId,
    2: StateOperDelta prevOperResult,
    /* sequence number of last oper delta received. 0 indicates initial sync */
    3: i64 lastUpdateSeqNum,
  );

  /* HwAgent graceful shutdown notification */
  void gracefulExit(1: i64 switchId);

  /* send hardware stats through sink */
  @thrift.Priority{level = thrift.RpcPriority.BEST_EFFORT}
  sink<HwSwitchStats, bool> syncHwStats(1: i16 switchIndex);
}
