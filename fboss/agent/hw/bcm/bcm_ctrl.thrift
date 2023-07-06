namespace cpp facebook.fboss
namespace cpp2 facebook.fboss
namespace py neteng.fboss.bcm_ctrl
namespace py3 neteng.fboss

include "fboss/agent/if/fboss.thrift"
include "fboss/agent/if/common.thrift"
include "fboss/agent/if/ctrl.thrift"
include "fboss/agent/hw/bcm/packettrace.thrift"

service BcmCtrl extends ctrl.FbossCtrl {
  list<common.fbstring> listCmds() throws (1: fboss.FbossBaseError error);

  /*
   * Get Packet Trace.
   *
   * This is modelled after Broadcom BroadView Packet Trace
   * A trace packet is sent to the ASIC and we gather various information
   * regarding how it is processed.
   */
  packettrace.PacketTraceInfo getPacketTrace(
    1: i32 ingressPort,
    2: common.fbbinary payload,
  ) throws (1: fboss.FbossBaseError error);

  void startFineGrainedBufferStatLogging() throws (
    1: fboss.FbossBaseError error,
  );
  void stopFineGrainedBufferStatLogging() throws (
    1: fboss.FbossBaseError error,
  );
  bool isFineGrainedBufferStatLoggingEnabled() throws (
    1: fboss.FbossBaseError error,
  );
}
