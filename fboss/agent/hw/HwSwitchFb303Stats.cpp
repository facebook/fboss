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
      fabricReachabilityMissingCount_(
          map,
          getCounterPrefix() + "fabric_reachability_missing"),
      fabricReachabilityMismatchCount_(
          map,
          getCounterPrefix() + "fabric_reachability_mismatch"),
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
          RATE) {}

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

int64_t HwSwitchFb303Stats::getForwardingQueueProcessorErrors() const {
  return getCumulativeValue(forwardingQueueProcessorErrors_);
}

int64_t HwSwitchFb303Stats::getAllReassemblyContextsTakenError() const {
  return getCumulativeValue(allReassemblyContextsTaken_);
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
  asicErrors.allReassemblyContextsTaken() =
      getAllReassemblyContextsTakenError();
  return asicErrors;
}

FabricReachabilityStats HwSwitchFb303Stats::getFabricReachabilityStats() {
  FabricReachabilityStats stats;
  stats.mismatchCount() = getFabricReachabilityMismatchCount();
  stats.missingCount() = getFabricReachabilityMissingCount();
  stats.virtualDevicesWithAsymmetricConnectivity() =
      getVirtualDevicesWithAsymmetricConnectivityCount();
  stats.switchReachabilityChangeCount() = getSwitchReachabilityChangeCount();
  return stats;
}

void HwSwitchFb303Stats::fabricReachabilityMissingCount(int64_t value) {
  fb303::fbData->setCounter(fabricReachabilityMissingCount_.name(), value);
}

void HwSwitchFb303Stats::fabricReachabilityMismatchCount(int64_t value) {
  fb303::fbData->setCounter(fabricReachabilityMismatchCount_.name(), value);
}

void HwSwitchFb303Stats::virtualDevicesWithAsymmetricConnectivity(
    int64_t value) {
  fb303::fbData->setCounter(
      virtualDevicesWithAsymmetricConnectivity_.name(), value);
}

void HwSwitchFb303Stats::portGroupSkew(int64_t value) {
  fb303::fbData->setCounter(portGroupSkew_.name(), value);
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

int64_t HwSwitchFb303Stats::getFabricReachabilityMismatchCount() const {
  auto counterVal = fb303::fbData->getCounterIfExists(
      fabricReachabilityMismatchCount_.name());
  return counterVal ? *counterVal : 0;
}

int64_t HwSwitchFb303Stats::getFabricReachabilityMissingCount() const {
  auto counterVal =
      fb303::fbData->getCounterIfExists(fabricReachabilityMissingCount_.name());
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
      getFabricReachabilityMismatchCount();
  hwFb303Stats.fabric_reachability_mismatch() =
      getFabricReachabilityMissingCount();
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
      fabricReachabilityMissingCount_.name(),
      *globalStats.fabric_reachability_missing());
  fb303::fbData->setCounter(
      fabricReachabilityMismatchCount_.name(),
      *globalStats.fabric_reachability_mismatch());
}

} // namespace facebook::fboss
