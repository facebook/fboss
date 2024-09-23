namespace cpp2 facebook.fboss
namespace py neteng.fboss.hardware_stats
namespace go neteng.fboss.hardware_stats
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.hardware_stats
namespace php fboss_hw

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
  27: i64 fecCorrectableErrors = STAT_UNINITIALIZED;
  28: i64 fecUncorrectableErrors = STAT_UNINITIALIZED;
  29: i64 inPfcCtrl_ = STAT_UNINITIALIZED;
  30: i64 outPfcCtrl_ = STAT_UNINITIALIZED;
  31: map<i16, i64> inPfc_ = {};
  32: map<i16, i64> inPfcXon_ = {};
  33: map<i16, i64> outPfc_ = {};
  34: map<i16, i64> queueWredDroppedPackets_ = {};
  35: map<i16, i64> queueEcnMarkedPackets_ = {};
  36: i64 fecCorrectedBits_ = STAT_UNINITIALIZED;
  /* Map of codewords received (value) with different counts of symbol errors (key).
   * fecCodewords_[0] = number of codewords with 0 symbol errors
   * fecCodewords_[1] = number of codewords with 1 symbol errors etc..
   */
  37: map<i16, i64> fecCodewords_ = {};
  38: optional i64 pqpErrorEgressDroppedPackets_;
  39: optional i64 fabricLinkDownDroppedCells_;

  // seconds from epoch
  50: i64 timestamp_ = STAT_UNINITIALIZED;
  51: string portName_ = "";
  52: optional MacsecStats macsecStats;
  53: i64 inLabelMissDiscards_ = STAT_UNINITIALIZED;
  54: map<i16, i64> queueWatermarkLevel_ = {};
  55: i64 inCongestionDiscards_ = STAT_UNINITIALIZED;
  56: optional i64 inAclDiscards_;
  57: optional i64 inTrapDiscards_;
  58: optional i64 outForwardingDiscards_;
  // This mismatch is communicated directly via callback
  59: optional i64 fabricConnectivityMismatch_DEPRECATED;
  60: optional i32 logicalPortId;
  61: optional i64 leakyBucketFlapCount_;
  62: optional i64 cableLengthMeters;
  63: optional bool dataCellsFilterOn;
  64: map<i16, i64> egressGvoqWatermarkBytes_ = {};
}

struct HwSysPortStats {
  // These map keys are the queue and the value is the counter value
  1: map<i16, i64> queueOutDiscardBytes_ = {};
  2: map<i16, i64> queueOutBytes_ = {};
  3: map<i16, i64> queueWatermarkBytes_ = {};
  4: map<i16, i64> queueWredDroppedPackets_ = {};
  5: map<i16, i64> queueCreditWatchdogDeletedPackets_ = {};
  6: map<i16, i64> queueLatencyWatermarkNsec_ = {};

  // seconds from epoch
  // Field index at a distance to allow for other stat additions
  100: i64 timestamp_ = STAT_UNINITIALIZED;
  101: string portName_ = "";
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

  // VOQ system resources
  52: i32 system_ports_free = STAT_UNINITIALIZED;
  53: i32 voqs_free = STAT_UNINITIALIZED;
}

struct HwAsicErrors {
  1: i64 parityErrors;
  2: i64 correctedParityErrors;
  3: i64 uncorrectedParityErrors;
  4: i64 asicErrors;
  // DNX specific errors
  5: optional i64 ingressReceiveEditorErrors;
  6: optional i64 ingressTransmitPipelineErrors;
  7: optional i64 egressPacketNetworkInterfaceErrors;
  8: optional i64 alignerErrors;
  9: optional i64 forwardingQueueProcessorErrors;
  10: optional i64 allReassemblyContextsTaken;
}

struct HwTeFlowStats {
  1: i64 bytes = STAT_UNINITIALIZED;
}

struct TeFlowStats {
  1: map<string, HwTeFlowStats> hwTeFlowStats;
  2: i64 timestamp;
}

struct FabricReachabilityStats {
  1: i64 mismatchCount;
  2: i64 missingCount;
  3: i64 virtualDevicesWithAsymmetricConnectivity;
  4: i64 switchReachabilityChangeCount;
}

struct HwRxReasonStats {
  1: map<i64, i64> rxReasonStats;
}

// The deviceWatermarkBytes has been moved from HwBufferPoolStats to
// HwSwitchWatermarkStats, but retaining the HwBufferPoolStats but
// will be marked as deprecated everywhere until it has some other
// stats in it.
struct HwBufferPoolStats {
  // Deprecate deviceWatermarkBytes once HwSwitchWatermarkStats is
  // available in prod!
  1: i64 deviceWatermarkBytes;
}

struct HwSwitchWatermarkStats {
  1: optional i64 fdrRciWatermarkBytes;
  2: optional i64 coreRciWatermarkBytes;
  3: optional i64 dtlQueueWatermarkBytes;
  4: i64 deviceWatermarkBytes;
  5: map<string, i64> globalHeadroomWatermarkBytes;
  6: map<string, i64> globalSharedWatermarkBytes;
  7: optional i64 egressCoreBufferWatermarkBytes;
}

struct CpuPortStats {
  1: map<i32, i64> queueInPackets_; // TODO: Deprecate this
  2: map<i32, i64> queueDiscardPackets_; // TODO: Deprecate this
  3: map<i32, string> queueToName_; // TODO: Deprecate this
  4: HwPortStats portStats_;
}

struct HwSwitchDropStats {
  1: optional i64 globalDrops;
  2: optional i64 globalReachabilityDrops;
  3: optional i64 packetIntegrityDrops;
  // DNX Specific drop counters
  4: optional i64 fdrCellDrops;
  5: optional i64 voqResourceExhaustionDrops;
  6: optional i64 globalResourceExhaustionDrops;
  7: optional i64 sramResourceExhaustionDrops;
  8: optional i64 vsqResourceExhaustionDrops;
  9: optional i64 dropPrecedenceDrops;
  10: optional i64 queueResolutionDrops;
  11: optional i64 ingressPacketPipelineRejectDrops;
  12: optional i64 corruptedCellPacketIntegrityDrops;
  13: optional i64 missingCellPacketIntegrityDrops;
}

struct HwSwitchDramStats {
  1: optional i64 dramEnqueuedBytes;
  2: optional i64 dramDequeuedBytes;
  3: optional i64 dramBlockedTimeNsec;
}

struct HwSwitchCreditStats {
  1: optional i64 deletedCreditBytes;
}

struct HwSwitchFb303GlobalStats {
  1: i64 tx_pkt_allocated;
  2: i64 tx_pkt_freed;
  3: i64 tx_pkt_sent;
  4: i64 tx_pkt_sent_done;
  5: i64 tx_errors;
  6: i64 tx_pkt_allocation_errors;
  7: i64 parity_errors;
  8: i64 parity_corr;
  9: i64 parity_uncorr;
  10: i64 asic_error;
  11: i64 global_drops;
  12: i64 global_reachability_drops;
  13: i64 packet_integrity_drops;
  14: i64 dram_enqueued_bytes;
  15: i64 dram_dequeued_bytes;
  16: i64 fabric_reachability_missing;
  17: i64 fabric_reachability_mismatch;
  // DNX Specific counters
  18: optional i64 fdr_cell_drops;
  19: optional i64 ingress_receive_editor_errors;
  20: optional i64 ingress_transmit_pipeline_errors;
  21: optional i64 egress_packet_network_interface_errors;
  22: optional i64 aligner_errors;
  23: optional i64 forwarding_queue_processor_errors;
  24: i64 virtual_devices_with_asymmetric_connectivity;
  25: i64 switch_reachability_change;
  /*
   * Until later versions of the chip (B0), cable lengths seen by
   * each port group has a bearing on how efficiently FDR-in buffers on J3 are
   * utilized.
   * If all port groups see roughly the same cable length then they all dequeue
   * cells for a pkt to FDR-out buffers around the same time. If OTOH a port
   * groups sees substantially lower cable lengths, FDR-in corresponding
   * to that port group dequeues to FDR-out faster. This causes cells to
   * sit for longer in FDR-out buffers, leading to more stress on the
   * latter. Our cabling plans try to minimize this. Report the skew seen
   * here to corroborate that cabling was as desired
  */
  26: optional i64 inter_port_group_cable_skew_meters;
  27: optional i64 dram_blocked_time_ns;
  28: optional i64 deleted_credit_bytes;
  29: optional i64 vsq_resource_exhaustion_drops;
}

struct HwFlowletStats {
  1: i64 l3EcmpDlbFailPackets;
  2: i64 l3EcmpDlbPortReassignmentCount;
}

struct AclStats {
  1: map<string, i64> statNameToCounterMap;
}
