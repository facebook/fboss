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
      fabricReachabilityMissingCount_(
          map,
          getCounterPrefix() + "fabric_reachability_missing"),
      fabricReachabilityMismatchCount_(
          map,
          getCounterPrefix() + "fabric_reachability_mismatch"),
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
      leabaSdkVer_(map, getCounterPrefix() + "leaba_sai_sdk_version") {}

void HwSwitchFb303Stats::update(const HwSwitchDropStats& dropStats) {
  if (dropStats.globalDrops().has_value()) {
    globalDrops_.addValue(*dropStats.globalDrops());
  }
  if (dropStats.globalReachabilityDrops().has_value()) {
    globalReachDrops_.addValue(*dropStats.globalReachabilityDrops());
  }
  if (dropStats.packetIntegrityDrops().has_value()) {
    packetIntegrityDrops_.addValue(*dropStats.packetIntegrityDrops());
  }
  if (dropStats.fdrCellDrops().has_value()) {
    fdrCellDrops_.addValue(*dropStats.fdrCellDrops());
  }
}

void HwSwitchFb303Stats::update(const HwSwitchDramStats& dramStats) {
  if (dramStats.dramEnqueuedBytes().has_value()) {
    dramEnqueuedBytes_.addValue(*dramStats.dramEnqueuedBytes());
  }
  if (dramStats.dramDequeuedBytes().has_value()) {
    dramDequeuedBytes_.addValue(*dramStats.dramDequeuedBytes());
  }
}

int64_t HwSwitchFb303Stats::getDramEnqueuedBytes() const {
  return getCumulativeValue(dramEnqueuedBytes_);
}

int64_t HwSwitchFb303Stats::getDramDequeuedBytes() const {
  return getCumulativeValue(dramDequeuedBytes_);
}

int64_t HwSwitchFb303Stats::getPacketIntegrityDrops() const {
  return getCumulativeValue(packetIntegrityDrops_);
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

int64_t HwSwitchFb303Stats::getFdrCellDrops() const {
  return getCumulativeValue(fdrCellDrops_);
}

HwAsicErrors HwSwitchFb303Stats::getHwAsicErrors() const {
  HwAsicErrors asicErrors;
  asicErrors.parityErrors() = getCumulativeValue(parityErrors_);
  asicErrors.correctedParityErrors() = getCumulativeValue(corrParityErrors_);
  asicErrors.uncorrectedParityErrors() =
      getCumulativeValue(uncorrParityErrors_);
  asicErrors.asicErrors() = getCumulativeValue(asicErrors_);
  asicErrors.ingressReceiveEditorErrors() = getCumulativeValue(ireErrors_);
  asicErrors.ingressTransmitPipelineErrors() = getCumulativeValue(itppErrors_);
  asicErrors.egressPacketNetworkInterfaceErrors() =
      getCumulativeValue(epniErrors_);
  asicErrors.alignerErrors() = getCumulativeValue(alignerErrors_);
  asicErrors.forwardingQueueProcessorErrors() =
      getCumulativeValue(forwardingQueueProcessorErrors_);
  return asicErrors;
}

FabricReachabilityStats HwSwitchFb303Stats::getFabricReachabilityStats() {
  FabricReachabilityStats stats;
  stats.mismatchCount() = getFabricReachabilityMismatchCount();
  stats.missingCount() = getFabricReachabilityMissingCount();
  return stats;
}

void HwSwitchFb303Stats::fabricReachabilityMissingCount(int64_t value) {
  fb303::fbData->setCounter(fabricReachabilityMissingCount_.name(), value);
}

void HwSwitchFb303Stats::fabricReachabilityMismatchCount(int64_t value) {
  fb303::fbData->setCounter(fabricReachabilityMismatchCount_.name(), value);
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
  hwFb303Stats.global_drops() = getCumulativeValue(globalDrops_);
  hwFb303Stats.global_reachability_drops() =
      getCumulativeValue(globalReachDrops_);
  hwFb303Stats.packet_integrity_drops() =
      getCumulativeValue(packetIntegrityDrops_);
  hwFb303Stats.dram_enqueued_bytes() = getCumulativeValue(dramEnqueuedBytes_);
  hwFb303Stats.dram_dequeued_bytes() = getCumulativeValue(dramDequeuedBytes_);
  hwFb303Stats.fabric_reachability_missing() =
      getFabricReachabilityMismatchCount();
  hwFb303Stats.fabric_reachability_mismatch() =
      getFabricReachabilityMissingCount();
  hwFb303Stats.fdr_cell_drops() = getFdrCellDrops();
  hwFb303Stats.ingress_receive_editor_errors() = getIreErrors();
  hwFb303Stats.ingress_transmit_pipeline_errors() = getItppErrors();
  hwFb303Stats.egress_packet_network_interface_errors() = getEpniErrors();
  hwFb303Stats.aligner_errors() = getAlignerErrors();
  hwFb303Stats.forwarding_queue_processor_errors() =
      getForwardingQueueProcessorErrors();
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
  fb303::fbData->setCounter(
      fabricReachabilityMissingCount_.name(),
      *globalStats.fabric_reachability_missing());
  fb303::fbData->setCounter(
      fabricReachabilityMismatchCount_.name(),
      *globalStats.fabric_reachability_mismatch());
}

} // namespace facebook::fboss
