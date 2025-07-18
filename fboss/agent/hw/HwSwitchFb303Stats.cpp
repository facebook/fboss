/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwSwitchFb303Stats.h"

#include "fboss/agent/SwitchStats.h"
#include "fboss/lib/CommonUtils.h"

using facebook::fb303::RATE;
using facebook::fb303::SUM;
using TLTimeseries = facebook::fb303::ThreadCachedServiceData::TLTimeseries;

namespace {
void updateValue(TLTimeseries& counter, int64_t value) {
  counter.addValue(value - facebook::fboss::getCumulativeValue(counter));
}
const std::string kAsicRevision = "asic_revision";
} // namespace

namespace facebook::fboss {

std::string HwSwitchFb303Stats::getCounterPrefix() const {
  return statsPrefix_
      ? folly::to<std::string>(*statsPrefix_, SwitchStats::kCounterPrefix)
      : SwitchStats::kCounterPrefix;
}

HwSwitchFb303Stats::HwSwitchFb303Stats(
    ThreadLocalStatsMap* map,
    const std::string& vendor,
    std::optional<std::string> statsPrefix)
    : statsPrefix_(statsPrefix),
      txPktAlloc_(
          map,
          getCounterPrefix() + vendor + ".tx.pkt.allocated",
          SUM,
          RATE),
      txPktFree_(map, getCounterPrefix() + vendor + ".tx.pkt.freed", SUM, RATE),
      txSent_(map, getCounterPrefix() + vendor + ".tx.pkt.sent", SUM, RATE),
      txSentDone_(
          map,
          getCounterPrefix() + vendor + ".tx.pkt.sent.done",
          SUM,
          RATE),
      txErrors_(map, getCounterPrefix() + vendor + ".tx.errors", SUM, RATE),
      txPktAllocErrors_(
          map,
          getCounterPrefix() + vendor + ".tx.pkt.allocation.errors",
          SUM,
          RATE),
      txQueued_(
          map,
          getCounterPrefix() + vendor + ".tx.pkt.queued_us",
          100,
          0,
          1000),
      parityErrors_(
          map,
          getCounterPrefix() + vendor + ".parity.errors",
          SUM,
          RATE),
      corrParityErrors_(
          map,
          getCounterPrefix() + vendor + ".parity.corr",
          SUM,
          RATE),
      uncorrParityErrors_(
          map,
          getCounterPrefix() + vendor + ".parity.uncorr",
          SUM,
          RATE),
      asicErrors_(map, getCounterPrefix() + vendor + ".asic.error", SUM, RATE),
      globalDrops_(map, getCounterPrefix() + "global_drops", SUM, RATE),
      globalReachDrops_(
          map,
          getCounterPrefix() + "global_reachability_drops",
          SUM,
          RATE),
      packetIntegrityDrops_(
          map,
          getCounterPrefix() + "packet_integrity_drops",
          SUM,
          RATE),
      fdrCellDrops_(map, getCounterPrefix() + "fdr_cell_drops", SUM, RATE),
      voqResourceExhaustionDrops_(
          map,
          getCounterPrefix() + "voq_resource_exhaustion_drops",
          SUM,
          RATE),
      globalResourceExhaustionDrops_(
          map,
          getCounterPrefix() + "global_resource_exhaustion_drops",
          SUM,
          RATE),
      sramResourceExhaustionDrops_(
          map,
          getCounterPrefix() + "sram_resource_exhaustion_drops",
          SUM,
          RATE),
      vsqResourceExhaustionDrops_(
          map,
          getCounterPrefix() + "vsq_resource_exhaustion_drops",
          SUM,
          RATE),
      dropPrecedenceDrops_(
          map,
          getCounterPrefix() + "drop_precedence_drops",
          SUM,
          RATE),
      queueResolutionDrops_(
          map,
          getCounterPrefix() + "queue_resolution_drops",
          SUM,
          RATE),
      ingressPacketPipelineRejectDrops_(
          map,
          getCounterPrefix() + "ingress_packet_pipeline_reject_drops",
          SUM,
          RATE),
      corruptedCellPacketIntegrityDrops_(
          map,
          getCounterPrefix() + "corrupted_cell_packet_integrity_drops",
          SUM,
          RATE),
      missingCellPacketIntegrityDrops_(
          map,
          getCounterPrefix() + "missing_cell_packet_integrity_drops",
          SUM,
          RATE),
      dramEnqueuedBytes_(
          map,
          getCounterPrefix() + "dram_enqueued_bytes",
          SUM,
          RATE),
      dramDequeuedBytes_(
          map,
          getCounterPrefix() + "dram_dequeued_bytes",
          SUM,
          RATE),
      dramBlockedTimeNsec_(
          map,
          getCounterPrefix() + "dram_blocked_time_ns",
          SUM,
          RATE),
      deletedCreditBytes_(
          map,
          getCounterPrefix() + "deleted_credit_bytes",
          SUM,
          RATE),
      rqpFabricCellCorruptionDrops_(
          map,
          getCounterPrefix() + "rqp_fabric_cell_corruption_drops",
          SUM,
          RATE),
      rqpNonFabricCellCorruptionDrops_(
          map,
          getCounterPrefix() + "rqp_non_fabric_cell_corruption_drops",
          SUM,
          RATE),
      rqpNonFabricCellMissingDrops_(
          map,
          getCounterPrefix() + "rqp_non_fabric_cell_missing_drops",
          SUM,
          RATE),
      rqpParityErrorDrops_(
          map,
          getCounterPrefix() + "rqp_parity_error_drops",
          SUM,
          RATE),
      tc0RateLimitDrops_(
          map,
          getCounterPrefix() + "tc0_rate_limit_drops",
          SUM,
          RATE),
      dramDataPathPacketError_(
          map,
          getCounterPrefix() + "dram_data_path_packet_error",
          SUM,
          RATE),
      sramLowBufferLimitHitCount_(
          map,
          getCounterPrefix() + "sram_low_buffer_limit_hit_count",
          SUM,
          RATE),
      fabricConnectivityMissingCount_(
          map,
          getCounterPrefix() + "fabric_connectivity_missing"),
      fabricConnectivityMismatchCount_(
          map,
          getCounterPrefix() + "fabric_connectivity_mismatch"),
      fabricConnectivityBogusCount_(
          map,
          getCounterPrefix() + "fabric_connectivity_bogus"),
      virtualDevicesWithAsymmetricConnectivity_(
          map,
          getCounterPrefix() + "virtual_devices_with_asymmetric_connectivity"),
      portGroupSkew_(map, getCounterPrefix() + "port_group_skew"),
      switchReachabilityChangeCount_(
          map,
          getCounterPrefix() + "switch_reachability_change",
          SUM,
          RATE),
      ireErrors_(map, getCounterPrefix() + vendor + ".ire.errors", SUM, RATE),
      itppErrors_(map, getCounterPrefix() + vendor + ".itpp.errors", SUM, RATE),
      epniErrors_(map, getCounterPrefix() + vendor + ".epni.errors", SUM, RATE),
      alignerErrors_(
          map,
          getCounterPrefix() + vendor + ".aligner.errors",
          SUM,
          RATE),
      reassemblyErrors_(
          map,
          getCounterPrefix() + vendor + ".reassembly.errors",
          SUM,
          RATE),
      fdrFifoOverflowErrors_(
          map,
          getCounterPrefix() + vendor + ".fdr.fifo_overflow.errors",
          SUM,
          RATE),
      fdaFifoOverflowErrors_(
          map,
          getCounterPrefix() + vendor + ".fda.fifo_overflow.errors",
          SUM,
          RATE),
      forwardingQueueProcessorErrors_(
          map,
          getCounterPrefix() + vendor + ".forwardingQueueProcessor.errors",
          SUM,
          RATE),
      allReassemblyContextsTaken_(
          map,
          getCounterPrefix() + vendor + ".allReassemblyContextsTaken.errors",
          SUM,
          RATE),
      isolationFirmwareCrashes_(
          map,
          getCounterPrefix() + vendor + ".isolationFirmwareCrash",
          SUM,
          RATE),
      rxFifoStuckDetected_(
          map,
          getCounterPrefix() + vendor + ".rxFifoStuckDetected.errors",
          SUM,
          RATE),
      congestionManagementErrors_(
          map,
          getCounterPrefix() + vendor + ".congestion_management.errors",
          SUM,
          RATE),
      dramDataPathErrors_(
          map,
          getCounterPrefix() + vendor + ".dram_data_path.errors",
          SUM,
          RATE),
      dramQueueManagementErrors_(
          map,
          getCounterPrefix() + vendor + ".dram_queue_management.errors",
          SUM,
          RATE),
      egressCongestionManagementErrors_(
          map,
          getCounterPrefix() + vendor + ".egress_congestion_management.errors",
          SUM,
          RATE),
      egressDataBufferErrors_(
          map,
          getCounterPrefix() + vendor + ".egress_data_buffer.errors",
          SUM,
          RATE),
      fabricControlReceiveErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_control_receive.errors",
          SUM,
          RATE),
      fabricControlTransmitErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_control_transmit.errors",
          SUM,
          RATE),
      fabricDataAggregateErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_data_aggregate.errors",
          SUM,
          RATE),
      fabricDataReceiveErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_data_receive.errors",
          SUM,
          RATE),
      fabricDataTransmitErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_data_transmit.errors",
          SUM,
          RATE),
      fabricMacErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_mac.errors",
          SUM,
          RATE),
      ingressPacketSchedulerErrors_(
          map,
          getCounterPrefix() + vendor + ".ingress_packet_scheduler.errors",
          SUM,
          RATE),
      ingressPacketTransmitErrors_(
          map,
          getCounterPrefix() + vendor + ".ingress_packet_transmit.errors",
          SUM,
          RATE),
      managementUnitErrors_(
          map,
          getCounterPrefix() + vendor + ".management_unit.errors",
          SUM,
          RATE),
      nifBufferUnitErrors_(
          map,
          getCounterPrefix() + vendor + ".nif_buffer.errors",
          SUM,
          RATE),
      nifManagementErrors_(
          map,
          getCounterPrefix() + vendor + ".nif_management.errors",
          SUM,
          RATE),
      onChipBufferMemoryErrors_(
          map,
          getCounterPrefix() + vendor + ".on_chip_buffer_memory.errors",
          SUM,
          RATE),
      packetDescriptorMemoryErrors_(
          map,
          getCounterPrefix() + vendor + ".packet_descriptor_memory.errors",
          SUM,
          RATE),
      packetQueueProcessorErrors_(
          map,
          getCounterPrefix() + vendor + ".packet_queue_processor.errors",
          SUM,
          RATE),
      receiveQueueProcessorErrors_(
          map,
          getCounterPrefix() + vendor + ".receive_queue_processor.errors",
          SUM,
          RATE),
      schedulerErrors_(
          map,
          getCounterPrefix() + vendor + ".scheduler.errors",
          SUM,
          RATE),
      sramPacketBufferErrors_(
          map,
          getCounterPrefix() + vendor + ".sram_packet_buffer.errors",
          SUM,
          RATE),
      sramQueueManagementErrors_(
          map,
          getCounterPrefix() + vendor + ".sram_queue_management.errors",
          SUM,
          RATE),
      tmActionResolutionErrors_(
          map,
          getCounterPrefix() + vendor + ".tm_action_resolution.errors",
          SUM,
          RATE),
      ingressTmErrors_(
          map,
          getCounterPrefix() + vendor + ".ingress_tm.errors",
          SUM,
          RATE),
      egressTmErrors_(
          map,
          getCounterPrefix() + vendor + ".egress_tm.errors",
          SUM,
          RATE),
      ingressPpErrors_(
          map,
          getCounterPrefix() + vendor + ".ingress_pp.errors",
          SUM,
          RATE),
      egressPpErrors_(
          map,
          getCounterPrefix() + vendor + ".egress_pp.errors",
          SUM,
          RATE),
      dramErrors_(map, getCounterPrefix() + vendor + ".dram.errors", SUM, RATE),
      counterAndMeterErrors_(
          map,
          getCounterPrefix() + vendor + ".counter_and_meter.errors",
          SUM,
          RATE),
      fabricRxErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_rx.errors",
          SUM,
          RATE),
      fabricTxErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_tx.errors",
          SUM,
          RATE),
      fabricLinkErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_link.errors",
          SUM,
          RATE),
      fabricTopologyErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_topology.errors",
          SUM,
          RATE),
      networkInterfaceErrors_(
          map,
          getCounterPrefix() + vendor + ".network_interface.errors",
          SUM,
          RATE),
      fabricControlPathErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_control_path.errors",
          SUM,
          RATE),
      fabricDataPathErrors_(
          map,
          getCounterPrefix() + vendor + ".fabric_data_path.errors",
          SUM,
          RATE),
      cpuErrors_(map, getCounterPrefix() + vendor + ".cpu.errors", SUM, RATE),
      asicSoftResetErrors_(
          map,
          getCounterPrefix() + vendor + ".asicSoftReset.errors",
          SUM,
          RATE),
      ingressTmWarnings_(
          map,
          getCounterPrefix() + vendor + ".ingress_tm.warnings",
          SUM,
          RATE),
      egressTmWarnings_(
          map,
          getCounterPrefix() + vendor + ".egress_tm.warnings",
          SUM,
          RATE),
      dramWarnings_(
          map,
          getCounterPrefix() + vendor + ".dram.warnings",
          SUM,
          RATE),
      fabricRxWarnings_(
          map,
          getCounterPrefix() + vendor + ".fabric_rx.warnings",
          SUM,
          RATE),
      fabricTxWarnings_(
          map,
          getCounterPrefix() + vendor + ".fabric_tx.warnings",
          SUM,
          RATE),
      fabricLinkWarnings_(
          map,
          getCounterPrefix() + vendor + ".fabric_link.warnings",
          SUM,
          RATE),
      networkInterfaceWarnings_(
          map,
          getCounterPrefix() + vendor + ".network_interface.warnings",
          SUM,
          RATE),
      interruptMaskedEvents_(
          map,
          getCounterPrefix() + vendor + ".interrupt_masked_events",
          SUM,
          RATE),
      hwInitializedTimeMs_(
          map,
          getCounterPrefix() + vendor + ".hw_initialized_time_ms",
          SUM,
          RATE),
      bootTimeMs_(
          map,
          getCounterPrefix() + vendor + ".hw_boot_time_ms",
          SUM,
          RATE),
      coldBoot_(map, getCounterPrefix() + vendor + ".hw_cold_boot", SUM, RATE),
      warmBoot_(map, getCounterPrefix() + vendor + ".hw_warm_boot", SUM, RATE),
      /*
       * Omit vendor prefix for SDK/SAI version in name since
       * the backward compatible name already has the vendor name
       */
      bcmSdkVer_(map, getCounterPrefix() + "bcm_sdk_version"),
      bcmSaiSdkVer_(map, getCounterPrefix() + "bcm_sai_sdk_version"),
      leabaSdkVer_(map, getCounterPrefix() + "leaba_sai_sdk_version"),
      hwStatsCollectionFailed_(
          map,
          getCounterPrefix() + "hw_stats_collection_failed",
          SUM,
          RATE),
      phyInfoCollectionFailed_(
          map,
          getCounterPrefix() + "phy_info_collection_failed",
          SUM,
          RATE),
      invalidQueueRxPackets_(
          map,
          getCounterPrefix() + "invalid_queue_rx_packets",
          SUM,
          RATE),
      arsResourceExhausted_(map, getCounterPrefix() + "ars_resource_exhausted"),
      isolationFirmwareVersion_(
          map,
          getCounterPrefix() + "isolation_firmware_version"),
      isolationFirmwareOpStatus_(
          map,
          getCounterPrefix() + "isolation_firmware_op_status"),
      isolationFirmwareFuncStatus_(
          map,
          getCounterPrefix() + "isolation_firmware_functional_status"),
      pfcDeadlockDetectionCount_(
          map,
          getCounterPrefix() + "pfc_deadlock_detection_count",
          SUM),
      pfcDeadlockRecoveryCount_(
          map,
          getCounterPrefix() + "pfc_deadlock_recovery_count",
          SUM) {}

void HwSwitchFb303Stats::update(const HwSwitchDropStats& dropStats) {
  if (dropStats.globalDrops().has_value()) {
    globalDrops_.addValue(
        *dropStats.globalDrops() - currentDropStats_.globalDrops().value_or(0));
  }
  if (dropStats.globalReachabilityDrops().has_value()) {
    globalReachDrops_.addValue(
        *dropStats.globalReachabilityDrops() -
        currentDropStats_.globalReachabilityDrops().value_or(0));
  }
  if (dropStats.packetIntegrityDrops().has_value()) {
    packetIntegrityDrops_.addValue(
        *dropStats.packetIntegrityDrops() -
        currentDropStats_.packetIntegrityDrops().value_or(0));
  }
  if (dropStats.fdrCellDrops().has_value()) {
    fdrCellDrops_.addValue(
        *dropStats.fdrCellDrops() -
        currentDropStats_.fdrCellDrops().value_or(0));
  }
  if (dropStats.voqResourceExhaustionDrops().has_value()) {
    voqResourceExhaustionDrops_.addValue(
        *dropStats.voqResourceExhaustionDrops() -
        currentDropStats_.voqResourceExhaustionDrops().value_or(0));
  }
  if (dropStats.globalResourceExhaustionDrops().has_value()) {
    globalResourceExhaustionDrops_.addValue(
        *dropStats.globalResourceExhaustionDrops() -
        currentDropStats_.globalResourceExhaustionDrops().value_or(0));
  }
  if (dropStats.sramResourceExhaustionDrops().has_value()) {
    sramResourceExhaustionDrops_.addValue(
        *dropStats.sramResourceExhaustionDrops() -
        currentDropStats_.sramResourceExhaustionDrops().value_or(0));
  }
  if (dropStats.vsqResourceExhaustionDrops().has_value()) {
    vsqResourceExhaustionDrops_.addValue(
        *dropStats.vsqResourceExhaustionDrops() -
        currentDropStats_.vsqResourceExhaustionDrops().value_or(0));
  }
  if (dropStats.dropPrecedenceDrops().has_value()) {
    dropPrecedenceDrops_.addValue(
        *dropStats.dropPrecedenceDrops() -
        currentDropStats_.dropPrecedenceDrops().value_or(0));
  }
  if (dropStats.queueResolutionDrops().has_value()) {
    queueResolutionDrops_.addValue(
        *dropStats.queueResolutionDrops() -
        currentDropStats_.queueResolutionDrops().value_or(0));
  }
  if (dropStats.ingressPacketPipelineRejectDrops().has_value()) {
    ingressPacketPipelineRejectDrops_.addValue(
        *dropStats.ingressPacketPipelineRejectDrops() -
        currentDropStats_.ingressPacketPipelineRejectDrops().value_or(0));
  }
  if (dropStats.corruptedCellPacketIntegrityDrops().has_value()) {
    corruptedCellPacketIntegrityDrops_.addValue(
        *dropStats.corruptedCellPacketIntegrityDrops() -
        currentDropStats_.corruptedCellPacketIntegrityDrops().value_or(0));
  }
  if (dropStats.missingCellPacketIntegrityDrops().has_value()) {
    missingCellPacketIntegrityDrops_.addValue(
        *dropStats.missingCellPacketIntegrityDrops() -
        currentDropStats_.missingCellPacketIntegrityDrops().value_or(0));
  }
  if (dropStats.rqpFabricCellCorruptionDrops().has_value()) {
    rqpFabricCellCorruptionDrops_.addValue(
        *dropStats.rqpFabricCellCorruptionDrops() -
        currentDropStats_.rqpFabricCellCorruptionDrops().value_or(0));
  }
  if (dropStats.rqpNonFabricCellCorruptionDrops().has_value()) {
    rqpNonFabricCellCorruptionDrops_.addValue(
        *dropStats.rqpNonFabricCellCorruptionDrops() -
        currentDropStats_.rqpNonFabricCellCorruptionDrops().value_or(0));
  }
  if (dropStats.rqpNonFabricCellMissingDrops().has_value()) {
    rqpNonFabricCellMissingDrops_.addValue(
        *dropStats.rqpNonFabricCellMissingDrops() -
        currentDropStats_.rqpNonFabricCellMissingDrops().value_or(0));
  }
  if (dropStats.rqpParityErrorDrops().has_value()) {
    rqpParityErrorDrops_.addValue(
        *dropStats.rqpParityErrorDrops() -
        currentDropStats_.rqpParityErrorDrops().value_or(0));
  }
  if (dropStats.tc0RateLimitDrops().has_value()) {
    tc0RateLimitDrops_.addValue(
        *dropStats.tc0RateLimitDrops() -
        currentDropStats_.tc0RateLimitDrops().value_or(0));
  }
  if (dropStats.dramDataPathPacketError().has_value()) {
    dramDataPathPacketError_.addValue(
        *dropStats.dramDataPathPacketError() -
        currentDropStats_.dramDataPathPacketError().value_or(0));
  }

  currentDropStats_ = dropStats;
}

void HwSwitchFb303Stats::update(const HwSwitchDramStats& dramStats) {
  if (dramStats.dramEnqueuedBytes().has_value()) {
    dramEnqueuedBytes_.addValue(*dramStats.dramEnqueuedBytes());
  }
  if (dramStats.dramDequeuedBytes().has_value()) {
    dramDequeuedBytes_.addValue(*dramStats.dramDequeuedBytes());
  }
  if (dramStats.dramBlockedTimeNsec().has_value()) {
    dramBlockedTimeNsec_.addValue(*dramStats.dramBlockedTimeNsec());
  }
}

void HwSwitchFb303Stats::update(const HwSwitchCreditStats& creditStats) {
  if (creditStats.deletedCreditBytes().has_value()) {
    deletedCreditBytes_.addValue(*creditStats.deletedCreditBytes());
  }
}

int64_t HwSwitchFb303Stats::getDramEnqueuedBytes() const {
  return getCumulativeValue(dramEnqueuedBytes_);
}

int64_t HwSwitchFb303Stats::getDramDequeuedBytes() const {
  return getCumulativeValue(dramDequeuedBytes_);
}

int64_t HwSwitchFb303Stats::getDramBlockedTimeNsec() const {
  return getCumulativeValue(dramBlockedTimeNsec_);
}

int64_t HwSwitchFb303Stats::getDeletedCreditBytes() const {
  return getCumulativeValue(deletedCreditBytes_);
}

int64_t HwSwitchFb303Stats::getIreErrors() const {
  return getCumulativeValue(ireErrors_);
}

int64_t HwSwitchFb303Stats::getItppErrors() const {
  return getCumulativeValue(itppErrors_);
}

int64_t HwSwitchFb303Stats::getEpniErrors() const {
  return getCumulativeValue(epniErrors_);
}

int64_t HwSwitchFb303Stats::getAlignerErrors() const {
  return getCumulativeValue(alignerErrors_);
}

int64_t HwSwitchFb303Stats::getReassemblyErrors() const {
  return getCumulativeValue(reassemblyErrors_);
}

int64_t HwSwitchFb303Stats::getFdrFifoOverflowErrors() const {
  return getCumulativeValue(fdrFifoOverflowErrors_);
}

int64_t HwSwitchFb303Stats::getFdaFifoOverflowErrors() const {
  return getCumulativeValue(fdaFifoOverflowErrors_);
}

int64_t HwSwitchFb303Stats::getForwardingQueueProcessorErrors() const {
  return getCumulativeValue(forwardingQueueProcessorErrors_);
}

int64_t HwSwitchFb303Stats::getAllReassemblyContextsTakenError() const {
  return getCumulativeValue(allReassemblyContextsTaken_);
}

int64_t HwSwitchFb303Stats::getCongestionManagementErrors() const {
  return getCumulativeValue(congestionManagementErrors_);
}

int64_t HwSwitchFb303Stats::getDramDataPathErrors() const {
  return getCumulativeValue(dramDataPathErrors_);
}

int64_t HwSwitchFb303Stats::getDramQueueManagementErrors() const {
  return getCumulativeValue(dramQueueManagementErrors_);
}

int64_t HwSwitchFb303Stats::getEgressCongestionManagementErrors() const {
  return getCumulativeValue(egressCongestionManagementErrors_);
}

int64_t HwSwitchFb303Stats::getEgressDataBufferErrors() const {
  return getCumulativeValue(egressDataBufferErrors_);
}

int64_t HwSwitchFb303Stats::getFabricControlReceiveErrors() const {
  return getCumulativeValue(fabricControlReceiveErrors_);
}

int64_t HwSwitchFb303Stats::getFabricControlTransmitErrors() const {
  return getCumulativeValue(fabricControlTransmitErrors_);
}

int64_t HwSwitchFb303Stats::getFabricDataAggregateErrors() const {
  return getCumulativeValue(fabricDataAggregateErrors_);
}

int64_t HwSwitchFb303Stats::getFabricDataReceiveErrors() const {
  return getCumulativeValue(fabricDataReceiveErrors_);
}

int64_t HwSwitchFb303Stats::getFabricDataTransmitErrors() const {
  return getCumulativeValue(fabricDataTransmitErrors_);
}

int64_t HwSwitchFb303Stats::getFabricMacErrors() const {
  return getCumulativeValue(fabricMacErrors_);
}

int64_t HwSwitchFb303Stats::getIngressPacketSchedulerErrors() const {
  return getCumulativeValue(ingressPacketSchedulerErrors_);
}

int64_t HwSwitchFb303Stats::getIngressPacketTransmitErrors() const {
  return getCumulativeValue(ingressPacketTransmitErrors_);
}

int64_t HwSwitchFb303Stats::getManagementUnitErrors() const {
  return getCumulativeValue(managementUnitErrors_);
}

int64_t HwSwitchFb303Stats::getNifBufferUnitErrors() const {
  return getCumulativeValue(nifBufferUnitErrors_);
}

int64_t HwSwitchFb303Stats::getNifManagementErrors() const {
  return getCumulativeValue(nifManagementErrors_);
}

int64_t HwSwitchFb303Stats::getOnChipBufferMemoryErrors() const {
  return getCumulativeValue(onChipBufferMemoryErrors_);
}

int64_t HwSwitchFb303Stats::getPacketDescriptorMemoryErrors() const {
  return getCumulativeValue(packetDescriptorMemoryErrors_);
}

int64_t HwSwitchFb303Stats::getPacketQueueProcessorErrors() const {
  return getCumulativeValue(packetQueueProcessorErrors_);
}

int64_t HwSwitchFb303Stats::getReceiveQueueProcessorErrors() const {
  return getCumulativeValue(receiveQueueProcessorErrors_);
}

int64_t HwSwitchFb303Stats::getSchedulerErrors() const {
  return getCumulativeValue(schedulerErrors_);
}

int64_t HwSwitchFb303Stats::getSramPacketBufferErrors() const {
  return getCumulativeValue(sramPacketBufferErrors_);
}

int64_t HwSwitchFb303Stats::getSramQueueManagementErrors() const {
  return getCumulativeValue(sramQueueManagementErrors_);
}

int64_t HwSwitchFb303Stats::getTmActionResolutionErrors() const {
  return getCumulativeValue(tmActionResolutionErrors_);
}

int64_t HwSwitchFb303Stats::getIngressTmErrors() const {
  return getCumulativeValue(ingressTmErrors_);
}

int64_t HwSwitchFb303Stats::getEgressTmErrors() const {
  return getCumulativeValue(egressTmErrors_);
}

int64_t HwSwitchFb303Stats::getIngressPpErrors() const {
  return getCumulativeValue(ingressPpErrors_);
}

int64_t HwSwitchFb303Stats::getEgressPpErrors() const {
  return getCumulativeValue(egressPpErrors_);
}

int64_t HwSwitchFb303Stats::getDramErrors() const {
  return getCumulativeValue(dramErrors_);
}

int64_t HwSwitchFb303Stats::getCounterAndMeterErrors() const {
  return getCumulativeValue(counterAndMeterErrors_);
}

int64_t HwSwitchFb303Stats::getFabricRxErrors() const {
  return getCumulativeValue(fabricRxErrors_);
}

int64_t HwSwitchFb303Stats::getFabricTxErrors() const {
  return getCumulativeValue(fabricTxErrors_);
}

int64_t HwSwitchFb303Stats::getFabricLinkErrors() const {
  return getCumulativeValue(fabricLinkErrors_);
}

int64_t HwSwitchFb303Stats::getFabricTopologyErrors() const {
  return getCumulativeValue(fabricTopologyErrors_);
}

int64_t HwSwitchFb303Stats::getNetworkInterfaceErrors() const {
  return getCumulativeValue(networkInterfaceErrors_);
}

int64_t HwSwitchFb303Stats::getFabricControlPathErrors() const {
  return getCumulativeValue(fabricControlPathErrors_);
}

int64_t HwSwitchFb303Stats::getFabricDataPathErrors() const {
  return getCumulativeValue(fabricDataPathErrors_);
}

int64_t HwSwitchFb303Stats::getCpuErrors() const {
  return getCumulativeValue(cpuErrors_);
}

int64_t HwSwitchFb303Stats::getAsicSoftResetErrors() const {
  return getCumulativeValue(asicSoftResetErrors_);
}

int64_t HwSwitchFb303Stats::getIngressTmWarnings() const {
  return getCumulativeValue(ingressTmWarnings_);
}

int64_t HwSwitchFb303Stats::getEgressTmWarnings() const {
  return getCumulativeValue(egressTmWarnings_);
}

int64_t HwSwitchFb303Stats::getDramWarnings() const {
  return getCumulativeValue(dramWarnings_);
}

int64_t HwSwitchFb303Stats::getFabricRxWarnings() const {
  return getCumulativeValue(fabricRxWarnings_);
}

int64_t HwSwitchFb303Stats::getFabricTxWarnings() const {
  return getCumulativeValue(fabricTxWarnings_);
}

int64_t HwSwitchFb303Stats::getFabricLinkWarnings() const {
  return getCumulativeValue(fabricLinkWarnings_);
}

int64_t HwSwitchFb303Stats::getNetworkInterfaceWarnings() const {
  return getCumulativeValue(networkInterfaceWarnings_);
}

int64_t HwSwitchFb303Stats::getIsolationFirmwareCrashes() const {
  return getCumulativeValue(isolationFirmwareCrashes_);
}

int64_t HwSwitchFb303Stats::getRxFifoStuckDetected() const {
  return getCumulativeValue(rxFifoStuckDetected_);
}

int64_t HwSwitchFb303Stats::getInterruptMaskedEvents() const {
  return getCumulativeValue(interruptMaskedEvents_);
}

int64_t HwSwitchFb303Stats::getPacketIntegrityDrops() const {
  return currentDropStats_.packetIntegrityDrops().value_or(0);
}

int64_t HwSwitchFb303Stats::getFdrCellDrops() const {
  return currentDropStats_.fdrCellDrops().value_or(0);
}

int64_t HwSwitchFb303Stats::getVoqResourcesExhautionDrops() const {
  return currentDropStats_.voqResourceExhaustionDrops().value_or(0);
}
int64_t HwSwitchFb303Stats::getGlobalResourcesExhautionDrops() const {
  return currentDropStats_.globalResourceExhaustionDrops().value_or(0);
}
int64_t HwSwitchFb303Stats::getSramResourcesExhautionDrops() const {
  return currentDropStats_.sramResourceExhaustionDrops().value_or(0);
}
int64_t HwSwitchFb303Stats::getVsqResourcesExhautionDrops() const {
  return currentDropStats_.vsqResourceExhaustionDrops().value_or(0);
}
int64_t HwSwitchFb303Stats::getDropPrecedenceDrops() const {
  return currentDropStats_.dropPrecedenceDrops().value_or(0);
}
int64_t HwSwitchFb303Stats::getQueueResolutionDrops() const {
  return currentDropStats_.queueResolutionDrops().value_or(0);
}
int64_t HwSwitchFb303Stats::getIngresPacketPipelineRejectDrops() const {
  return currentDropStats_.ingressPacketPipelineRejectDrops().value_or(0);
}

int64_t HwSwitchFb303Stats::getCorruptedCellPacketIntegrityDrops() const {
  return currentDropStats_.corruptedCellPacketIntegrityDrops().value_or(0);
}

int64_t HwSwitchFb303Stats::getMissingCellPacketIntegrityDrops() const {
  return currentDropStats_.missingCellPacketIntegrityDrops().value_or(0);
}

int64_t HwSwitchFb303Stats::getSwitchReachabilityChangeCount() const {
  return getCumulativeValue(switchReachabilityChangeCount_);
}

HwAsicErrors HwSwitchFb303Stats::getHwAsicErrors() const {
  HwAsicErrors asicErrors;
  asicErrors.parityErrors() = getCumulativeValue(parityErrors_);
  asicErrors.correctedParityErrors() = getCumulativeValue(corrParityErrors_);
  asicErrors.uncorrectedParityErrors() =
      getCumulativeValue(uncorrParityErrors_);
  asicErrors.asicErrors() = getCumulativeValue(asicErrors_);
  asicErrors.ingressReceiveEditorErrors() = getIreErrors();
  asicErrors.ingressTransmitPipelineErrors() = getItppErrors();
  asicErrors.egressPacketNetworkInterfaceErrors() = getEpniErrors();
  asicErrors.alignerErrors() = getAlignerErrors();
  asicErrors.forwardingQueueProcessorErrors() =
      getForwardingQueueProcessorErrors();
  asicErrors.reassemblyErrors() = getReassemblyErrors();
  asicErrors.fdrFifoOverflowErrors() = getFdrFifoOverflowErrors();
  asicErrors.fdaFifoOverflowErrors() = getFdaFifoOverflowErrors();
  asicErrors.allReassemblyContextsTaken() =
      getAllReassemblyContextsTakenError();
  asicErrors.isolationFirmwareCrashes() = getIsolationFirmwareCrashes();
  asicErrors.rxFifoStuckDetected() = getRxFifoStuckDetected();
  asicErrors.congestionManagementErrors() = getCongestionManagementErrors();
  asicErrors.dramDataPathErrors() = getDramDataPathErrors();
  asicErrors.dramQueueManagementErrors() = getDramQueueManagementErrors();
  asicErrors.egressCongestionManagementErrors() =
      getEgressCongestionManagementErrors();
  asicErrors.egressDataBufferErrors() = getEgressDataBufferErrors();
  asicErrors.fabricControlReceiveErrors() = getFabricControlReceiveErrors();
  asicErrors.fabricControlTransmitErrors() = getFabricControlTransmitErrors();
  asicErrors.fabricDataAggregateErrors() = getFabricDataAggregateErrors();
  asicErrors.fabricDataReceiveErrors() = getFabricDataReceiveErrors();
  asicErrors.fabricDataTransmitErrors() = getFabricDataTransmitErrors();
  asicErrors.fabricMacErrors() = getFabricMacErrors();
  asicErrors.ingressPacketSchedulerErrors() = getIngressPacketSchedulerErrors();
  asicErrors.ingressPacketTransmitErrors() = getIngressPacketTransmitErrors();
  asicErrors.managementUnitErrors() = getManagementUnitErrors();
  asicErrors.nifBufferUnitErrors() = getNifBufferUnitErrors();
  asicErrors.nifManagementErrors() = getNifManagementErrors();
  asicErrors.onChipBufferMemoryErrors() = getOnChipBufferMemoryErrors();
  asicErrors.packetDescriptorMemoryErrors() = getPacketDescriptorMemoryErrors();
  asicErrors.packetQueueProcessorErrors() = getPacketQueueProcessorErrors();
  asicErrors.receiveQueueProcessorErrors() = getReceiveQueueProcessorErrors();
  asicErrors.schedulerErrors() = getSchedulerErrors();
  asicErrors.sramPacketBufferErrors() = getSramPacketBufferErrors();
  asicErrors.sramQueueManagementErrors() = getSramQueueManagementErrors();
  asicErrors.tmActionResolutionErrors() = getTmActionResolutionErrors();
  asicErrors.ingressTmErrors() = getIngressTmErrors();
  asicErrors.egressTmErrors() = getEgressTmErrors();
  asicErrors.ingressPpErrors() = getIngressPpErrors();
  asicErrors.egressPpErrors() = getEgressPpErrors();
  asicErrors.dramErrors() = getDramErrors();
  asicErrors.counterAndMeterErrors() = getCounterAndMeterErrors();
  asicErrors.fabricRxErrors() = getFabricRxErrors();
  asicErrors.fabricTxErrors() = getFabricTxErrors();
  asicErrors.fabricLinkErrors() = getFabricLinkErrors();
  asicErrors.fabricTopologyErrors() = getFabricTopologyErrors();
  asicErrors.networkInterfaceErrors() = getNetworkInterfaceErrors();
  asicErrors.fabricControlPathErrors() = getFabricControlPathErrors();
  asicErrors.fabricDataPathErrors() = getFabricDataPathErrors();
  asicErrors.cpuErrors() = getCpuErrors();
  asicErrors.asicSoftResetErrors() = getAsicSoftResetErrors();
  asicErrors.ingressTmWarnings() = getIngressTmWarnings();
  asicErrors.egressTmWarnings() = getEgressTmWarnings();
  asicErrors.dramWarnings() = getDramWarnings();
  asicErrors.fabricRxWarnings() = getFabricRxWarnings();
  asicErrors.fabricTxWarnings() = getFabricTxWarnings();
  asicErrors.fabricLinkWarnings() = getFabricLinkWarnings();
  asicErrors.networkInterfaceWarnings() = getNetworkInterfaceWarnings();
  return asicErrors;
}

FabricReachabilityStats HwSwitchFb303Stats::getFabricReachabilityStats() {
  FabricReachabilityStats stats;
  stats.mismatchCount() = getFabricConnectivityMismatchCount();
  stats.missingCount() = getFabricConnectivityMissingCount();
  stats.virtualDevicesWithAsymmetricConnectivity() =
      getVirtualDevicesWithAsymmetricConnectivityCount();
  stats.switchReachabilityChangeCount() = getSwitchReachabilityChangeCount();
  stats.bogusCount() = getFabricConnectivityBogusCount();
  return stats;
}

void HwSwitchFb303Stats::fabricConnectivityMissingCount(int64_t value) {
  fb303::fbData->setCounter(fabricConnectivityMissingCount_.name(), value);
}

void HwSwitchFb303Stats::fabricConnectivityMismatchCount(int64_t value) {
  fb303::fbData->setCounter(fabricConnectivityMismatchCount_.name(), value);
}

void HwSwitchFb303Stats::fabricConnectivityBogusCount(int64_t value) {
  fb303::fbData->setCounter(fabricConnectivityBogusCount_.name(), value);
}

void HwSwitchFb303Stats::virtualDevicesWithAsymmetricConnectivity(
    int64_t value) {
  fb303::fbData->setCounter(
      virtualDevicesWithAsymmetricConnectivity_.name(), value);
}

void HwSwitchFb303Stats::portGroupSkew(int64_t value) {
  fb303::fbData->setCounter(portGroupSkew_.name(), value);
}

void HwSwitchFb303Stats::asicRevision(int64_t value) {
  fb303::fbData->setCounter(getCounterPrefix() + kAsicRevision, value);
}

void HwSwitchFb303Stats::bcmSdkVer(int64_t ver) {
  fb303::fbData->setCounter(bcmSdkVer_.name(), ver);
}
void HwSwitchFb303Stats::bcmSaiSdkVer(int64_t ver) {
  fb303::fbData->setCounter(bcmSaiSdkVer_.name(), ver);
}

void HwSwitchFb303Stats::leabaSdkVer(int64_t ver) {
  fb303::fbData->setCounter(leabaSdkVer_.name(), ver);
}

void HwSwitchFb303Stats::arsResourceExhausted(bool exhausted) {
  fb303::fbData->setCounter(arsResourceExhausted_.name(), exhausted ? 1 : 0);
}

void HwSwitchFb303Stats::isolationFirmwareVersion(int64_t ver) {
  fb303::fbData->setCounter(isolationFirmwareVersion_.name(), ver);
}

void HwSwitchFb303Stats::isolationFirmwareOpStatus(int64_t opStatus) {
  fb303::fbData->setCounter(isolationFirmwareOpStatus_.name(), opStatus);
}

void HwSwitchFb303Stats::isolationFirmwareFuncStatus(int64_t funcStatus) {
  fb303::fbData->setCounter(isolationFirmwareFuncStatus_.name(), funcStatus);
}

int64_t HwSwitchFb303Stats::getFabricConnectivityMismatchCount() const {
  auto counterVal = fb303::fbData->getCounterIfExists(
      fabricConnectivityMismatchCount_.name());
  return counterVal ? *counterVal : 0;
}

int64_t HwSwitchFb303Stats::getFabricConnectivityMissingCount() const {
  auto counterVal =
      fb303::fbData->getCounterIfExists(fabricConnectivityMissingCount_.name());
  return counterVal ? *counterVal : 0;
}

int64_t HwSwitchFb303Stats::getFabricConnectivityBogusCount() const {
  auto counterVal =
      fb303::fbData->getCounterIfExists(fabricConnectivityBogusCount_.name());
  return counterVal ? *counterVal : 0;
}

int64_t HwSwitchFb303Stats::getVirtualDevicesWithAsymmetricConnectivityCount()
    const {
  auto counterVal = fb303::fbData->getCounterIfExists(
      virtualDevicesWithAsymmetricConnectivity_.name());
  return counterVal ? *counterVal : 0;
}

int64_t HwSwitchFb303Stats::getPortGroupSkewCount() const {
  auto counterVal = fb303::fbData->getCounterIfExists(portGroupSkew_.name());
  return counterVal ? *counterVal : 0;
}

std::optional<int64_t> HwSwitchFb303Stats::getAsicRevision() const {
  auto counterVal =
      fb303::fbData->getCounterIfExists(getCounterPrefix() + kAsicRevision);
  return counterVal.toStdOptional();
}

HwSwitchFb303GlobalStats HwSwitchFb303Stats::getAllFb303Stats() const {
  HwSwitchFb303GlobalStats hwFb303Stats;
  hwFb303Stats.tx_pkt_allocated() = getCumulativeValue(txPktAlloc_);
  hwFb303Stats.tx_pkt_freed() = getCumulativeValue(txPktFree_);
  hwFb303Stats.tx_pkt_sent() = getCumulativeValue(txSent_);
  hwFb303Stats.tx_pkt_sent_done() = getCumulativeValue(txSentDone_);
  hwFb303Stats.tx_errors() = getCumulativeValue(txErrors_);
  hwFb303Stats.tx_pkt_allocation_errors() =
      getCumulativeValue(txPktAllocErrors_);
  hwFb303Stats.parity_errors() = getCumulativeValue(parityErrors_);
  hwFb303Stats.parity_corr() = getCumulativeValue(corrParityErrors_);
  hwFb303Stats.parity_uncorr() = getCumulativeValue(uncorrParityErrors_);
  hwFb303Stats.asic_error() = getCumulativeValue(asicErrors_);
  hwFb303Stats.dram_enqueued_bytes() = getCumulativeValue(dramEnqueuedBytes_);
  hwFb303Stats.dram_dequeued_bytes() = getCumulativeValue(dramDequeuedBytes_);
  hwFb303Stats.dram_blocked_time_ns() =
      getCumulativeValue(dramBlockedTimeNsec_);
  hwFb303Stats.fabric_reachability_missing() =
      getFabricConnectivityMismatchCount();
  hwFb303Stats.fabric_reachability_mismatch() =
      getFabricConnectivityMissingCount();
  hwFb303Stats.fabric_connectivity_bogus() = getFabricConnectivityBogusCount();
  hwFb303Stats.virtual_devices_with_asymmetric_connectivity() =
      getVirtualDevicesWithAsymmetricConnectivityCount();
  hwFb303Stats.switch_reachability_change() =
      getSwitchReachabilityChangeCount();
  hwFb303Stats.ingress_receive_editor_errors() = getIreErrors();
  hwFb303Stats.ingress_transmit_pipeline_errors() = getItppErrors();
  hwFb303Stats.egress_packet_network_interface_errors() = getEpniErrors();
  hwFb303Stats.aligner_errors() = getAlignerErrors();
  hwFb303Stats.forwarding_queue_processor_errors() =
      getForwardingQueueProcessorErrors();
  hwFb303Stats.global_drops() = currentDropStats_.globalDrops().value_or(0);
  hwFb303Stats.global_reachability_drops() =
      currentDropStats_.globalReachabilityDrops().value_or(0);
  hwFb303Stats.packet_integrity_drops() = getPacketIntegrityDrops();
  if (currentDropStats_.fdrCellDrops().has_value()) {
    hwFb303Stats.fdr_cell_drops() = *currentDropStats_.fdrCellDrops();
  }
  hwFb303Stats.deleted_credit_bytes() = getDeletedCreditBytes();
  hwFb303Stats.vsq_resource_exhaustion_drops() =
      getVsqResourcesExhautionDrops();
  hwFb303Stats.interrupt_masked_events() = getInterruptMaskedEvents();
  if (auto asicRevision = getAsicRevision()) {
    hwFb303Stats.asic_revision() = *asicRevision;
  }
  return hwFb303Stats;
}

void HwSwitchFb303Stats::updateStats(HwSwitchFb303GlobalStats& globalStats) {
  updateValue(txPktAlloc_, *globalStats.tx_pkt_allocated());
  updateValue(txPktFree_, *globalStats.tx_pkt_freed());
  updateValue(txSent_, *globalStats.tx_pkt_sent());
  updateValue(txSentDone_, *globalStats.tx_pkt_sent_done());
  updateValue(txErrors_, *globalStats.tx_errors());
  updateValue(txPktAllocErrors_, *globalStats.tx_pkt_allocation_errors());
  updateValue(parityErrors_, *globalStats.parity_errors());
  updateValue(corrParityErrors_, *globalStats.parity_corr());
  updateValue(uncorrParityErrors_, *globalStats.parity_uncorr());
  updateValue(asicErrors_, *globalStats.asic_error());
  updateValue(globalDrops_, *globalStats.global_drops());
  updateValue(globalReachDrops_, *globalStats.global_reachability_drops());
  updateValue(packetIntegrityDrops_, *globalStats.packet_integrity_drops());
  updateValue(dramEnqueuedBytes_, *globalStats.dram_enqueued_bytes());
  updateValue(dramDequeuedBytes_, *globalStats.dram_dequeued_bytes());
  if (globalStats.dram_blocked_time_ns().has_value()) {
    updateValue(dramBlockedTimeNsec_, *globalStats.dram_blocked_time_ns());
  }
  if (globalStats.vsq_resource_exhaustion_drops().has_value()) {
    updateValue(
        vsqResourceExhaustionDrops_,
        *globalStats.vsq_resource_exhaustion_drops());
  }
  updateValue(
      switchReachabilityChangeCount_,
      *globalStats.switch_reachability_change());
  fb303::fbData->setCounter(
      fabricConnectivityMissingCount_.name(),
      *globalStats.fabric_reachability_missing());
  fb303::fbData->setCounter(
      fabricConnectivityMismatchCount_.name(),
      *globalStats.fabric_reachability_mismatch());
  fb303::fbData->setCounter(
      fabricConnectivityBogusCount_.name(),
      *globalStats.fabric_connectivity_bogus());
}

} // namespace facebook::fboss
