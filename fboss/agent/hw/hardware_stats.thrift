namespace cpp2 facebook.fboss
namespace py neteng.fboss.hardware_stats
namespace go neteng.fboss.hardware_stats
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.hardware_stats

include "fboss/mka_service/if/mka_structs.thrift"

const i64 STAT_UNINITIALIZED = -1;

struct MacsecSciFlowStats {
  1: mka_structs.MKASci sci;
  2: mka_structs.MacsecFlowStats flowStats;
}

struct MacsecSaIdSaStats {
  1: mka_structs.MKASecureAssociationId saId;
  2: mka_structs.MacsecSaStats saStats;
}

struct MacsecStats {
  1: mka_structs.MacsecPortStats ingressPortStats;
  2: mka_structs.MacsecPortStats egressPortStats;
  3: list<MacsecSciFlowStats> ingressFlowStats;
  4: list<MacsecSciFlowStats> egressFlowStats;
  // Secure association stats
  5: list<MacsecSaIdSaStats> rxSecureAssociationStats;
  6: list<MacsecSaIdSaStats> txSecureAssociationStats;
  7: mka_structs.MacsecAclStats ingressAclStats;
  8: mka_structs.MacsecAclStats egressAclStats;
}

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
  10: i64 inDstNullDiscards_ = STAT_UNINITIALIZED;
  11: i64 inDiscardsRaw_ = STAT_UNINITIALIZED;

  12: i64 outBytes_ = STAT_UNINITIALIZED;
  13: i64 outUnicastPkts_ = STAT_UNINITIALIZED;
  14: i64 outMulticastPkts_ = STAT_UNINITIALIZED;
  15: i64 outBroadcastPkts_ = STAT_UNINITIALIZED;
  16: i64 outDiscards_ = STAT_UNINITIALIZED;
  17: i64 outErrors_ = STAT_UNINITIALIZED;
  18: i64 outPause_ = STAT_UNINITIALIZED;
  19: i64 outCongestionDiscardPkts_ = STAT_UNINITIALIZED;
  20: i64 wredDroppedPackets_ = STAT_UNINITIALIZED;
  21: map<i16, i64> queueOutDiscardBytes_ = {};
  22: map<i16, i64> queueOutBytes_ = {};
  23: i64 outEcnCounter_ = STAT_UNINITIALIZED;
  24: map<i16, i64> queueOutPackets_ = {};
  25: map<i16, i64> queueOutDiscardPackets_ = {};
  26: map<i16, i64> queueWatermarkBytes_ = {};
  27: i64 fecCorrectableErrors = STAT_UNINITIALIZED (
    cpp2.type = "std::uint64_t",
  );
  28: i64 fecUncorrectableErrors = STAT_UNINITIALIZED (
    cpp2.type = "std::uint64_t",
  );
  29: i64 inPfcCtrl_ = STAT_UNINITIALIZED;
  30: i64 outPfcCtrl_ = STAT_UNINITIALIZED;
  31: map<i16, i64> inPfc_ = {};
  32: map<i16, i64> inPfcXon_ = {};
  33: map<i16, i64> outPfc_ = {};
  34: map<i16, i64> queueWredDroppedPackets_ = {};
  35: map<i16, i64> queueEcnMarkedPackets_ = {};

  // seconds from epoch
  50: i64 timestamp_ = STAT_UNINITIALIZED;
  51: string portName_ = "";
  52: optional MacsecStats macsecStats;
  53: i64 inLabelMissDiscards_ = STAT_UNINITIALIZED;
}

struct HwSysPortStats {
  1: map<i16, i64> queueOutDiscardBytes_ = {};
  2: map<i16, i64> queueOutBytes_ = {};
  3: map<i16, i64> queueWatermarkBytes_ = {};
}

struct HwTrunkStats {
  1: i64 capacity_ = STAT_UNINITIALIZED;

  2: i64 inBytes_ = STAT_UNINITIALIZED;
  3: i64 inUnicastPkts_ = STAT_UNINITIALIZED;
  4: i64 inMulticastPkts_ = STAT_UNINITIALIZED;
  5: i64 inBroadcastPkts_ = STAT_UNINITIALIZED;
  6: i64 inDiscards_ = STAT_UNINITIALIZED;
  7: i64 inErrors_ = STAT_UNINITIALIZED;
  8: i64 inPause_ = STAT_UNINITIALIZED;
  9: i64 inIpv4HdrErrors_ = STAT_UNINITIALIZED;
  10: i64 inIpv6HdrErrors_ = STAT_UNINITIALIZED;
  11: i64 inDstNullDiscards_ = STAT_UNINITIALIZED;
  12: i64 inDiscardsRaw_ = STAT_UNINITIALIZED;

  13: i64 outBytes_ = STAT_UNINITIALIZED;
  14: i64 outUnicastPkts_ = STAT_UNINITIALIZED;
  15: i64 outMulticastPkts_ = STAT_UNINITIALIZED;
  16: i64 outBroadcastPkts_ = STAT_UNINITIALIZED;
  17: i64 outDiscards_ = STAT_UNINITIALIZED;
  18: i64 outErrors_ = STAT_UNINITIALIZED;
  19: i64 outPause_ = STAT_UNINITIALIZED;
  20: i64 outCongestionDiscardPkts_ = STAT_UNINITIALIZED;
  21: i64 outEcnCounter_ = STAT_UNINITIALIZED;
  22: i64 wredDroppedPackets_ = STAT_UNINITIALIZED;
  23: optional MacsecStats macsecStats;
}

struct HwResourceStats {
  /**
   * Stale flag is set when for whatever reason
   * counter collection fails we log
   * errors and set stale counter to 1 in such scenarios.
   * This should never really happen in practice.
   **/
  1: bool hw_table_stats_stale = true;
  /*
   * Not all platforms will have all of these stats
   * available. Post a platform stat populate,
   * STAT_UNINITIALIZED can be read as the stat being
   * unavailable at the particular hw/platform
   */
  2: i32 l3_host_max = STAT_UNINITIALIZED;
  3: i32 l3_host_used = STAT_UNINITIALIZED;
  4: i32 l3_host_free = STAT_UNINITIALIZED;
  5: i32 l3_ipv4_host_used = STAT_UNINITIALIZED;
  6: i32 l3_ipv4_host_free = STAT_UNINITIALIZED;
  7: i32 l3_ipv6_host_used = STAT_UNINITIALIZED;
  8: i32 l3_ipv6_host_free = STAT_UNINITIALIZED;
  9: i32 l3_nexthops_max = STAT_UNINITIALIZED;
  10: i32 l3_nexthops_used = STAT_UNINITIALIZED;
  11: i32 l3_nexthops_free = STAT_UNINITIALIZED;
  12: i32 l3_ipv4_nexthops_free = STAT_UNINITIALIZED;
  13: i32 l3_ipv6_nexthops_free = STAT_UNINITIALIZED;
  14: i32 l3_ecmp_groups_max = STAT_UNINITIALIZED;
  15: i32 l3_ecmp_groups_used = STAT_UNINITIALIZED;
  16: i32 l3_ecmp_groups_free = STAT_UNINITIALIZED;
  17: i32 l3_ecmp_group_members_free = STAT_UNINITIALIZED;

  // LPM
  18: i32 lpm_ipv4_max = STAT_UNINITIALIZED;
  19: i32 lpm_ipv4_used = STAT_UNINITIALIZED;
  20: i32 lpm_ipv4_free = STAT_UNINITIALIZED;
  21: i32 lpm_ipv6_free = STAT_UNINITIALIZED;
  22: i32 lpm_ipv6_mask_0_64_max = STAT_UNINITIALIZED;
  23: i32 lpm_ipv6_mask_0_64_used = STAT_UNINITIALIZED;
  24: i32 lpm_ipv6_mask_0_64_free = STAT_UNINITIALIZED;
  25: i32 lpm_ipv6_mask_65_127_max = STAT_UNINITIALIZED;
  26: i32 lpm_ipv6_mask_65_127_used = STAT_UNINITIALIZED;
  27: i32 lpm_ipv6_mask_65_127_free = STAT_UNINITIALIZED;
  28: i32 lpm_slots_max = STAT_UNINITIALIZED;
  29: i32 lpm_slots_used = STAT_UNINITIALIZED;
  30: i32 lpm_slots_free = STAT_UNINITIALIZED;

  // ACLs
  31: i32 acl_entries_used = STAT_UNINITIALIZED;
  32: i32 acl_entries_free = STAT_UNINITIALIZED;
  33: i32 acl_entries_max = STAT_UNINITIALIZED;
  34: i32 acl_counters_used = STAT_UNINITIALIZED;
  35: i32 acl_counters_free = STAT_UNINITIALIZED;
  36: i32 acl_counters_max = STAT_UNINITIALIZED;
  37: i32 acl_meters_used = STAT_UNINITIALIZED;
  38: i32 acl_meters_free = STAT_UNINITIALIZED;
  39: i32 acl_meters_max = STAT_UNINITIALIZED;

  // Mirrors
  40: i32 mirrors_used = STAT_UNINITIALIZED;
  41: i32 mirrors_free = STAT_UNINITIALIZED;
  42: i32 mirrors_max = STAT_UNINITIALIZED;
  43: i32 mirrors_span = STAT_UNINITIALIZED;
  44: i32 mirrors_erspan = STAT_UNINITIALIZED;
  45: i32 mirrors_sflow = STAT_UNINITIALIZED;

  // EM
  46: i32 em_entries_used = STAT_UNINITIALIZED;
  47: i32 em_entries_free = STAT_UNINITIALIZED;
  48: i32 em_entries_max = STAT_UNINITIALIZED;
  49: i32 em_counters_used = STAT_UNINITIALIZED;
  50: i32 em_counters_free = STAT_UNINITIALIZED;
  51: i32 em_counters_max = STAT_UNINITIALIZED;
}

struct HwAsicErrors {
  1: i64 parityErrors;
  2: i64 correctedParityErrors;
  3: i64 uncorrectedParityErrors;
  4: i64 asicErrors;
}

struct HwTeFlowStats {
  1: i64 bytes;
}

struct HwRxReasonStats {
  1: map<i64, i64> rxReasonStats;
}
