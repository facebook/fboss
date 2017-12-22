namespace cpp2 facebook.fboss
namespace d neteng.fboss.hardware_stats
namespace py neteng.fboss.hardware_stats


const i64 STAT_UNINITIALIZED = -1


struct HwPortStats {
  1: i64 inBytes_ = STAT_UNINITIALIZED;
  2: i64 inUnicastPkts_ = STAT_UNINITIALIZED;
  3: i64 inMulticastPkts_ = STAT_UNINITIALIZED;
  4: i64 inBroadcastPkts_ = STAT_UNINITIALIZED;
  5: i64 inDiscards_ = STAT_UNINITIALIZED;
  6: i64 inErrors_ = STAT_UNINITIALIZED;
  7: i64 inPause_ = STAT_UNINITIALIZED;
  8: i64 inIpv4HdrErrors_ = STAT_UNINITIALIZED;
  9: i64 inIpv6HdrErrors_ = STAT_UNINITIALIZED;
  10: i64 inNonPauseDiscards_ = STAT_UNINITIALIZED;

  11:  i64 outBytes_ = STAT_UNINITIALIZED;
  12: i64 outUnicastPkts_ = STAT_UNINITIALIZED;
  13: i64 outMulticastPkts_ = STAT_UNINITIALIZED;
  14: i64 outBroadcastPkts_ = STAT_UNINITIALIZED;
  15: i64 outDiscards_ = STAT_UNINITIALIZED;
  16: i64 outErrors_ = STAT_UNINITIALIZED;
  17: i64 outPause_ = STAT_UNINITIALIZED;
  18: i64 outCongestionDiscardPkts_ = STAT_UNINITIALIZED;
  19: list<i64> queueOutDiscardBytes_ = []
  20: list<i64> queueOutBytes_ = []
}
