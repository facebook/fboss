/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <fb303/ThreadCachedServiceData.h>
#include <folly/ThreadLocal.h>
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

namespace facebook::fboss {

class HwSwitchFb303Stats {
 public:
  using ThreadLocalStatsMap =
      fb303::ThreadCachedServiceData::ThreadLocalStatsMap;

  HwSwitchFb303Stats(
      ThreadLocalStatsMap* map,
      const std::string& vendor,
      std::optional<std::string> statsPrefix = std::nullopt);

  std::string getCounterPrefix() const;

  void txPktAlloc() {
    txPktAlloc_.addValue(1);
  }
  void txPktFree() {
    txPktFree_.addValue(1);
  }
  void txSent() {
    txSent_.addValue(1);
  }
  void txSentDone(uint64_t q) {
    txSentDone_.addValue(1);
    txQueued_.addValue(q);
  }
  void txError() {
    txErrors_.addValue(1);
  }
  void txPktAllocErrors() {
    txErrors_.addValue(1);
    txPktAllocErrors_.addValue(1);
  }

  void corrParityError() {
    parityErrors_.addValue(1);
    corrParityErrors_.addValue(1);
  }

  void uncorrParityError() {
    parityErrors_.addValue(1);
    uncorrParityErrors_.addValue(1);
  }

  void asicError() {
    asicErrors_.addValue(1);
  }
  void ireError() {
    ireErrors_.addValue(1);
  }
  void itppError() {
    itppErrors_.addValue(1);
  }
  void epniError() {
    epniErrors_.addValue(1);
  }
  void alignerError() {
    alignerErrors_.addValue(1);
  }
  void forwardingQueueProcessorError() {
    forwardingQueueProcessorErrors_.addValue(1);
  }
  void allReassemblyContextsTaken() {
    allReassemblyContextsTaken_.addValue(1);
  }
  void reassemblyError() {
    reassemblyErrors_.addValue(1);
  }
  void fdrFifoOverflowError() {
    fdrFifoOverflowErrors_.addValue(1);
  }
  void fdaFifoOverflowError() {
    fdaFifoOverflowErrors_.addValue(1);
  }
  void hwInitializedTime(uint64_t ms) {
    hwInitializedTimeMs_.addValue(ms);
  }
  void bootTime(uint64_t ms) {
    bootTimeMs_.addValue(ms);
  }
  void coldBoot() {
    coldBoot_.addValue(1);
  }
  void warmBoot() {
    warmBoot_.addValue(1);
  }
  void switchReachabilityChangeCount() {
    switchReachabilityChangeCount_.addValue(1);
  }
  void statsCollectionFailed() {
    hwStatsCollectionFailed_.addValue(1);
  }
  void phyInfoCollectionFailed() {
    phyInfoCollectionFailed_.addValue(1);
  }
  void invalidQueueRxPackets() {
    invalidQueueRxPackets_.addValue(1);
  }
  void isolationFirmwareCrash() {
    isolationFirmwareCrashes_.addValue(1);
  }
  void rxFifoStuckDetected() {
    rxFifoStuckDetected_.addValue(1);
  }
  void fabricConnectivityMissingCount(int64_t value);
  void fabricConnectivityMismatchCount(int64_t value);
  void fabricConnectivityBogusCount(int64_t value);
  void virtualDevicesWithAsymmetricConnectivity(int64_t value);
  void portGroupSkew(int64_t value);

  void bcmSdkVer(int64_t ver);
  void bcmSaiSdkVer(int64_t ver);
  void leabaSdkVer(int64_t ver);

  void update(const HwSwitchDramStats& dramStats);
  void update(const HwSwitchDropStats& dropStats);
  void update(const HwSwitchCreditStats& creditStats);

  int64_t getTxPktAllocCount() const {
    return txPktAlloc_.count();
  }
  int64_t getTxPktFreeCount() const {
    return txPktFree_.count();
  }
  int64_t getTxSentCount() const {
    return txSent_.count();
  }
  int64_t getTxSentDoneCount() const {
    return txSentDone_.count();
  }
  int64_t getTxErrorCount() const {
    return txErrors_.count();
  }
  int64_t getTxPktAllocErrorsCount() const {
    return txPktAllocErrors_.count();
  }
  int64_t getCorrParityErrorCount() const {
    return corrParityErrors_.count();
  }
  int64_t getUncorrParityErrorCount() const {
    return uncorrParityErrors_.count();
  }
  int64_t getAsicErrorCount() const {
    return asicErrors_.count();
  }
  int64_t getFabricConnectivityMismatchCount() const;
  int64_t getFabricConnectivityMissingCount() const;
  int64_t getFabricConnectivityBogusCount() const;
  int64_t getVirtualDevicesWithAsymmetricConnectivityCount() const;
  int64_t getPortGroupSkewCount() const;
  int64_t getSwitchReachabilityChangeCount() const;
  int64_t getPacketIntegrityDropsCount() const {
    return packetIntegrityDrops_.count();
  }
  // Dram bytes
  int64_t getDramEnqueuedBytes() const;
  int64_t getDramDequeuedBytes() const;
  int64_t getDramBlockedTimeNsec() const;
  // Credit stats
  int64_t getDeletedCreditBytes() const;
  // Asic errors
  int64_t getIreErrors() const;
  int64_t getItppErrors() const;
  int64_t getEpniErrors() const;
  int64_t getAlignerErrors() const;
  int64_t getReassemblyErrors() const;
  int64_t getFdrFifoOverflowErrors() const;
  int64_t getFdaFifoOverflowErrors() const;
  int64_t getForwardingQueueProcessorErrors() const;
  int64_t getAllReassemblyContextsTakenError() const;
  // FW Errors
  int64_t getIsolationFirmwareCrashes() const;
  int64_t getRxFifoStuckDetected() const;

  // Switch drops
  int64_t getPacketIntegrityDrops() const;
  int64_t getFdrCellDrops() const;
  int64_t getVoqResourcesExhautionDrops() const;
  int64_t getGlobalResourcesExhautionDrops() const;
  int64_t getSramResourcesExhautionDrops() const;
  int64_t getVsqResourcesExhautionDrops() const;
  int64_t getDropPrecedenceDrops() const;
  int64_t getQueueResolutionDrops() const;
  int64_t getIngresPacketPipelineRejectDrops() const;
  int64_t getCorruptedCellPacketIntegrityDrops() const;
  int64_t getMissingCellPacketIntegrityDrops() const;

  HwAsicErrors getHwAsicErrors() const;
  FabricReachabilityStats getFabricReachabilityStats();
  HwSwitchFb303GlobalStats getAllFb303Stats() const;
  // Used in SwAgent to update stats based on HwSwitch synced counters
  void updateStats(HwSwitchFb303GlobalStats& globalStats);

 private:
  // Forbidden copy constructor and assignment operator
  HwSwitchFb303Stats(HwSwitchFb303Stats const&) = delete;
  HwSwitchFb303Stats& operator=(HwSwitchFb303Stats const&) = delete;

  using TLTimeseries = fb303::ThreadCachedServiceData::TLTimeseries;
  using TLHistogram = fb303::ThreadCachedServiceData::TLHistogram;
  using TLCounter = fb303::ThreadCachedServiceData::TLCounter;

  std::optional<std::string> statsPrefix_;

  // Total number of Tx packet allocated right now
  TLTimeseries txPktAlloc_;
  TLTimeseries txPktFree_;
  TLTimeseries txSent_;
  TLTimeseries txSentDone_;

  // Errors in sending packets
  TLTimeseries txErrors_;
  TLTimeseries txPktAllocErrors_;

  // Time spent for each Tx packet queued in HW
  TLHistogram txQueued_;

  // parity errors
  TLTimeseries parityErrors_;
  TLTimeseries corrParityErrors_;
  TLTimeseries uncorrParityErrors_;

  // Other ASIC errors
  TLTimeseries asicErrors_;
  // Drops
  TLTimeseries globalDrops_;
  TLTimeseries globalReachDrops_;
  TLTimeseries packetIntegrityDrops_;
  TLTimeseries fdrCellDrops_;
  TLTimeseries voqResourceExhaustionDrops_;
  TLTimeseries globalResourceExhaustionDrops_;
  TLTimeseries sramResourceExhaustionDrops_;
  TLTimeseries vsqResourceExhaustionDrops_;
  TLTimeseries dropPrecedenceDrops_;
  TLTimeseries queueResolutionDrops_;
  TLTimeseries ingressPacketPipelineRejectDrops_;
  TLTimeseries corruptedCellPacketIntegrityDrops_;
  TLTimeseries missingCellPacketIntegrityDrops_;
  HwSwitchDropStats currentDropStats_;
  // Dram enqueue, dequeue bytes
  TLTimeseries dramEnqueuedBytes_;
  TLTimeseries dramDequeuedBytes_;
  TLTimeseries dramBlockedTimeNsec_;
  // Credit stats
  TLTimeseries deletedCreditBytes_;
  // RQP errors
  TLTimeseries rqpFabricCellCorruptionDrops_;
  TLTimeseries rqpNonFabricCellCorruptionDrops_;
  TLTimeseries rqpNonFabricCellMissingDrops_;
  TLTimeseries rqpParityErrorDrops_;
  // fabric connectivity errors
  TLCounter fabricConnectivityMissingCount_;
  TLCounter fabricConnectivityMismatchCount_;
  TLCounter fabricConnectivityBogusCount_;
  TLCounter virtualDevicesWithAsymmetricConnectivity_;
  TLCounter portGroupSkew_;
  TLTimeseries switchReachabilityChangeCount_;
  TLTimeseries ireErrors_;
  TLTimeseries itppErrors_;
  TLTimeseries epniErrors_;
  TLTimeseries alignerErrors_;
  TLTimeseries reassemblyErrors_;
  TLTimeseries fdrFifoOverflowErrors_;
  TLTimeseries fdaFifoOverflowErrors_;
  TLTimeseries forwardingQueueProcessorErrors_;
  TLTimeseries allReassemblyContextsTaken_;
  TLTimeseries isolationFirmwareCrashes_;
  TLTimeseries rxFifoStuckDetected_;
  TLTimeseries hwInitializedTimeMs_;
  TLTimeseries bootTimeMs_;
  TLTimeseries coldBoot_;
  TLTimeseries warmBoot_;
  TLCounter bcmSdkVer_;
  TLCounter bcmSaiSdkVer_;
  TLCounter leabaSdkVer_;
  // Meta stats to capture stat and phy
  // info collection failures
  TLTimeseries hwStatsCollectionFailed_;
  TLTimeseries phyInfoCollectionFailed_;
  TLTimeseries invalidQueueRxPackets_;
};

} // namespace facebook::fboss
