namespace cpp2 facebook.fboss.multiswitch
namespace go neteng.fboss.multiswitch
namespace php fboss.multiswitch
namespace py neteng.fboss.multiswitch_ctrl
namespace py3 neteng.fboss.multiswitch
namespace py.asyncio neteng.fboss.asyncio.multiswitch_ctrl

include "fboss/fsdb/if/fsdb_oper.thrift"
include "fboss/agent/if/ctrl.thrift"
include "thrift/annotation/cpp.thrift"
include "fboss/lib/phy/phy.thrift"
include "fboss/agent/hw/hardware_stats.thrift"

@cpp.Type{name = "std::unique_ptr<folly::IOBuf>"}
typedef binary fbbinary

struct LinkEvent {
  1: i32 port;
  2: bool up;
  3: optional phy.LinkFaultStatus iPhyLinkFaultStatus;
}

struct FdbEvent {
  1: ctrl.L2EntryThrift entry;
  2: ctrl.L2EntryUpdateType updateType;
}

struct TxPacket {
  1: fbbinary data;
  2: optional i32 port;
  3: optional i32 queue;
}

struct RxPacket {
  1: fbbinary data;
  2: i32 port;
  3: optional i32 aggPort;
  4: optional i16 vlan;
}

struct StateOperDelta {
  1: fsdb_oper.OperDelta operDelta;
  2: bool transaction;
}

struct SystemPortStats {
  @cpp.Ref{type = cpp.RefType.Unique}
  1: hardware_stats.HwSysPortStats systemPortStats;
}

struct HwSwitchStats {
  1: i64 timestamp;
  @cpp.Type{template = "folly::F14FastMap"}
  2: map<string, hardware_stats.HwPortStats> hwPortStats;
  3: map<string, hardware_stats.HwTrunkStats> hwTrunkStats;
  4: hardware_stats.HwResourceStats hwResourceStats;
  5: hardware_stats.HwAsicErrors hwAsicErrors;
  @cpp.Ref{type = cpp.RefType.Shared}
  6: map<string, SystemPortStats> sysPortStats;
  7: hardware_stats.TeFlowStats teFlowStats;
  8: hardware_stats.HwBufferPoolStats bufferPoolStats;
  9: hardware_stats.FabricReachabilityStats fabricReachabilityStats;
}

service MultiSwitchCtrl {
  /* notify link event through sink */
  sink<LinkEvent, bool> notifyLinkEvent(1: i64 switchId);

  /* notify fdb event through sink */
  sink<FdbEvent, bool> notifyFdbEvent(1: i64 switchId);

  /* notify rx packet through sink */
  sink<RxPacket, bool> notifyRxPacket(1: i64 switchId);

  /* keep getting tx packet from SwSwitch, through stream */
  stream<TxPacket> getTxPackets(1: i64 switchId);

  /* get next oper delta from SwSwitch */
  StateOperDelta getNextStateOperDelta(
    1: i64 switchId,
    2: StateOperDelta prevOperResult,
    /* indicates whether HwSwitch is syncing for first time */
    3: bool initialSync,
  );

  /* HwAgent graceful shutdown notification */
  void gracefulExit(1: i64 switchId);

  /* send hardware stats through sink */
  sink<HwSwitchStats, bool> syncHwStats(1: i16 switchIndex) (
    priority = 'BEST_EFFORT',
  );
}
