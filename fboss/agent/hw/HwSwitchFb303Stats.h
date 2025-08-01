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
  void congestionManagementError() {
    congestionManagementErrors_.addValue(1);
  }
  void dramDataPathError() {
    dramDataPathErrors_.addValue(1);
  }
  void dramQueueManagementError() {
    dramQueueManagementErrors_.addValue(1);
  }
  void egressCongestionManagementError() {
    egressCongestionManagementErrors_.addValue(1);
  }
  void egressDataBufferError() {
    egressDataBufferErrors_.addValue(1);
  }
  void fabricControlReceiveError() {
    fabricControlReceiveErrors_.addValue(1);
  }
  void fabricControlTransmitError() {
    fabricControlTransmitErrors_.addValue(1);
  }
  void fabricDataAggregateError() {
    fabricDataAggregateErrors_.addValue(1);
  }
  void fabricDataReceiveError() {
    fabricDataReceiveErrors_.addValue(1);
  }
  void fabricDataTransmitError() {
    fabricDataTransmitErrors_.addValue(1);
  }
  void fabricMacError() {
    fabricMacErrors_.addValue(1);
  }
  void ingressPacketSchedulerError() {
    ingressPacketSchedulerErrors_.addValue(1);
  }
  void ingressPacketTransmitError() {
    ingressPacketTransmitErrors_.addValue(1);
  }
  void managementUnitError() {
    managementUnitErrors_.addValue(1);
  }
  void nifBufferUnitError() {
    nifBufferUnitErrors_.addValue(1);
  }
  void nifManagementError() {
    nifManagementErrors_.addValue(1);
  }
  void onChipBufferMemoryError() {
    onChipBufferMemoryErrors_.addValue(1);
  }
  void packetDescriptorMemoryError() {
    packetDescriptorMemoryErrors_.addValue(1);
  }
  void packetQueueProcessorError() {
    packetQueueProcessorErrors_.addValue(1);
  }
  void receiveQueueProcessorError() {
    receiveQueueProcessorErrors_.addValue(1);
  }
  void schedulerError() {
    schedulerErrors_.addValue(1);
  }
  void sramPacketBufferError() {
    sramPacketBufferErrors_.addValue(1);
  }
  void sramQueueManagementError() {
    sramQueueManagementErrors_.addValue(1);
  }
  void tmActionResolutionError() {
    tmActionResolutionErrors_.addValue(1);
  }
  void ingressTmError() {
    ingressTmErrors_.addValue(1);
  }
  void egressTmError() {
    egressTmErrors_.addValue(1);
  }
  void ingressPpError() {
    ingressPpErrors_.addValue(1);
  }
  void egressPpError() {
    egressPpErrors_.addValue(1);
  }
  void dramError() {
    dramErrors_.addValue(1);
  }
  void counterAndMeterError() {
    counterAndMeterErrors_.addValue(1);
  }
  void fabricRxError() {
    fabricRxErrors_.addValue(1);
  }
  void fabricTxError() {
    fabricTxErrors_.addValue(1);
  }
  void fabricLinkError() {
    fabricLinkErrors_.addValue(1);
  }
  void fabricTopologyError() {
    fabricTopologyErrors_.addValue(1);
  }
  void networkInterfaceError() {
    networkInterfaceErrors_.addValue(1);
  }
  void fabricControlPathError() {
    fabricControlPathErrors_.addValue(1);
  }
  void fabricDataPathError() {
    fabricDataPathErrors_.addValue(1);
  }
  void cpuError() {
    cpuErrors_.addValue(1);
  }
  void asicSoftResetError() {
    asicSoftResetErrors_.addValue(1);
  }
  void ingressTmWarning() {
    ingressTmWarnings_.addValue(1);
  }
  void egressTmWarning() {
    egressTmWarnings_.addValue(1);
  }
  void dramWarning() {
    dramWarnings_.addValue(1);
  }
  void fabricRxWarning() {
    fabricRxWarnings_.addValue(1);
  }
  void fabricTxWarning() {
    fabricTxWarnings_.addValue(1);
  }
  void fabricLinkWarning() {
    fabricLinkWarnings_.addValue(1);
  }
  void networkInterfaceWarning() {
    networkInterfaceWarnings_.addValue(1);
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
  void interruptMaskedEvent() {
    interruptMaskedEvents_.addValue(1);
  }
  void pfcDeadlockDetectionCount() {
    pfcDeadlockDetectionCount_.addValue(1);
  }
  void pfcDeadlockRecoveryCount() {
    pfcDeadlockRecoveryCount_.addValue(1);
  }
  void sramLowBufferLimitHitCount() {
    sramLowBufferLimitHitCount_.addValue(1);
  }
  void fabricConnectivityMissingCount(int64_t value);
  void fabricConnectivityMismatchCount(int64_t value);
  void fabricConnectivityBogusCount(int64_t value);
  void virtualDevicesWithAsymmetricConnectivity(int64_t value);
  void portGroupSkew(int64_t value);
  void asicRevision(int64_t value);

  void bcmSdkVer(int64_t ver);
  void bcmSaiSdkVer(int64_t ver);
  void leabaSdkVer(int64_t ver);

  void isolationFirmwareVersion(int64_t ver);
  void isolationFirmwareOpStatus(int64_t opStatus);
  void isolationFirmwareFuncStatus(int64_t funcStatus);

  void update(const HwSwitchDramStats& dramStats);
  void update(const HwSwitchDropStats& dropStats);
  void update(const HwSwitchCreditStats& creditStats);

  void arsResourceExhausted(bool exhausted);

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
  int64_t getCongestionManagementErrors() const;
  int64_t getDramDataPathErrors() const;
  int64_t getDramQueueManagementErrors() const;
  int64_t getEgressCongestionManagementErrors() const;
  int64_t getEgressDataBufferErrors() const;
  int64_t getFabricControlReceiveErrors() const;
  int64_t getFabricControlTransmitErrors() const;
  int64_t getFabricDataAggregateErrors() const;
  int64_t getFabricDataReceiveErrors() const;
  int64_t getFabricDataTransmitErrors() const;
  int64_t getFabricMacErrors() const;
  int64_t getIngressPacketSchedulerErrors() const;
  int64_t getIngressPacketTransmitErrors() const;
  int64_t getManagementUnitErrors() const;
  int64_t getNifBufferUnitErrors() const;
  int64_t getNifManagementErrors() const;
  int64_t getOnChipBufferMemoryErrors() const;
  int64_t getPacketDescriptorMemoryErrors() const;
  int64_t getPacketQueueProcessorErrors() const;
  int64_t getReceiveQueueProcessorErrors() const;
  int64_t getSchedulerErrors() const;
  int64_t getSramPacketBufferErrors() const;
  int64_t getSramQueueManagementErrors() const;
  int64_t getTmActionResolutionErrors() const;
  int64_t getIngressTmErrors() const;
  int64_t getEgressTmErrors() const;
  int64_t getIngressPpErrors() const;
  int64_t getEgressPpErrors() const;
  int64_t getDramErrors() const;
  int64_t getCounterAndMeterErrors() const;
  int64_t getFabricRxErrors() const;
  int64_t getFabricTxErrors() const;
  int64_t getFabricLinkErrors() const;
  int64_t getFabricTopologyErrors() const;
  int64_t getNetworkInterfaceErrors() const;
  int64_t getFabricControlPathErrors() const;
  int64_t getFabricDataPathErrors() const;
  int64_t getCpuErrors() const;
  int64_t getIngressTmWarnings() const;
  int64_t getEgressTmWarnings() const;
  int64_t getDramWarnings() const;
  int64_t getFabricRxWarnings() const;
  int64_t getFabricTxWarnings() const;
  int64_t getFabricLinkWarnings() const;
  int64_t getNetworkInterfaceWarnings() const;
  int64_t getInterruptMaskedEvents() const;

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
  std::optional<int64_t> getAsicRevision() const;
  int64_t getAsicSoftResetErrors() const;

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
  TLTimeseries tc0RateLimitDrops_;
  // DDP errors
  TLTimeseries dramDataPathPacketError_;
  TLTimeseries sramLowBufferLimitHitCount_;
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
  TLTimeseries congestionManagementErrors_;
  TLTimeseries dramDataPathErrors_;
  TLTimeseries dramQueueManagementErrors_;
  TLTimeseries egressCongestionManagementErrors_;
  TLTimeseries egressDataBufferErrors_;
  TLTimeseries fabricControlReceiveErrors_;
  TLTimeseries fabricControlTransmitErrors_;
  TLTimeseries fabricDataAggregateErrors_;
  TLTimeseries fabricDataReceiveErrors_;
  TLTimeseries fabricDataTransmitErrors_;
  TLTimeseries fabricMacErrors_;
  TLTimeseries ingressPacketSchedulerErrors_;
  TLTimeseries ingressPacketTransmitErrors_;
  TLTimeseries managementUnitErrors_;
  TLTimeseries nifBufferUnitErrors_;
  TLTimeseries nifManagementErrors_;
  TLTimeseries onChipBufferMemoryErrors_;
  TLTimeseries packetDescriptorMemoryErrors_;
  TLTimeseries packetQueueProcessorErrors_;
  TLTimeseries receiveQueueProcessorErrors_;
  TLTimeseries schedulerErrors_;
  TLTimeseries sramPacketBufferErrors_;
  TLTimeseries sramQueueManagementErrors_;
  TLTimeseries tmActionResolutionErrors_;
  TLTimeseries ingressTmErrors_;
  TLTimeseries egressTmErrors_;
  TLTimeseries ingressPpErrors_;
  TLTimeseries egressPpErrors_;
  TLTimeseries dramErrors_;
  TLTimeseries counterAndMeterErrors_;
  TLTimeseries fabricRxErrors_;
  TLTimeseries fabricTxErrors_;
  TLTimeseries fabricLinkErrors_;
  TLTimeseries fabricTopologyErrors_;
  TLTimeseries networkInterfaceErrors_;
  TLTimeseries fabricControlPathErrors_;
  TLTimeseries fabricDataPathErrors_;
  TLTimeseries cpuErrors_;
  TLTimeseries asicSoftResetErrors_;
  TLTimeseries ingressTmWarnings_;
  TLTimeseries egressTmWarnings_;
  TLTimeseries dramWarnings_;
  TLTimeseries fabricRxWarnings_;
  TLTimeseries fabricTxWarnings_;
  TLTimeseries fabricLinkWarnings_;
  TLTimeseries networkInterfaceWarnings_;
  TLTimeseries interruptMaskedEvents_;
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
  TLCounter arsResourceExhausted_;
  TLCounter isolationFirmwareVersion_;
  TLCounter isolationFirmwareOpStatus_;
  TLCounter isolationFirmwareFuncStatus_;
  TLTimeseries pfcDeadlockDetectionCount_;
  TLTimeseries pfcDeadlockRecoveryCount_;
};

} // namespace facebook::fboss
