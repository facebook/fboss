package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct FabricMonitoringCounter {
  1: string portName;
  2: i32 portId;
  3: i64 txCount;
  4: i64 rxCount;
  5: i64 droppedCount;
  6: i64 invalidPayloadCount;
  7: i64 noPendingSeqNumCount;
}

struct FabricMonitoringCountersModel {
  1: list<FabricMonitoringCounter> counters;
}

struct FabricMonitoringDetail {
  1: string portName;
  2: i32 portId;
  3: string neighborSwitch;
  4: string neighborPortName;
  5: i32 virtualDevice;
  6: i32 linkSwitchId;
  7: string linkSystemPort;
}

struct FabricMonitoringDetailsModel {
  1: list<FabricMonitoringDetail> details;
}
