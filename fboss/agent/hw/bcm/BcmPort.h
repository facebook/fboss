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

extern "C" {
#include <bcm/cosq.h>
#include <bcm/port.h>
#include <bcm/stat.h>
#include <bcm/types.h>
}

#include <fb303/ThreadCachedServiceData.h>
#include "common/stats/MonotonicCounter.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManager.h"
#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/types.h"

#include <folly/Range.h>
#include <folly/Synchronized.h>
#include <mutex>
#include <utility>

namespace facebook::fboss {

class BcmSwitch;
class BcmPortGroup;
class SwitchState;
enum class MirrorDirection;
enum class MirrorAction;
class BcmPortIngressBufferManager;

/**
 * BcmPort is the class to abstract the physical port in BcmSwitch.
 */
class BcmPort {
 public:
  /*
   * Construct the BcmPort object.
   *
   * This method shouldn't make any calls to the Broadcom SDK to query/modify
   * the port yet.  init() will be called soon after construction, and any
   * actual initialization logic should be performed there.
   */
  BcmPort(BcmSwitch* hw, bcm_port_t port, BcmPlatformPort* platformPort);
  ~BcmPort();

  void init(bool warmBoot);

  void enable(const std::shared_ptr<Port>& swPort);
  void enableLinkscan();
  void disable(const std::shared_ptr<Port>& swPort);
  void disableLinkscan();
  void program(const std::shared_ptr<Port>& swPort);
  void enableStatCollection(const std::shared_ptr<Port>& swPort);
  void disableStatCollection();
  void setupQueue(const PortQueue& queue);

  void attachIngressQosPolicy(const std::string& name);
  void detachIngressQosPolicy();
  void destroy();

  /*
   * Getters.
   */
  BcmPlatformPort* getPlatformPort() const {
    return platformPort_;
  }
  BcmSwitch* getHW() const {
    return hw_;
  }
  bcm_port_t getBcmPortId() const {
    return port_;
  }
  bcm_gport_t getBcmGport() const {
    return gport_;
  }
  BcmPortGroup* getPortGroup() const {
    return portGroup_;
  }
  uint8_t getPipe() const {
    return pipe_;
  }
  cfg::SampleDestination getSampleDestination() const {
    return sampleDest_;
  }
  QueueConfig getCurrentQueueSettings();

  /*
   * Helpers for retreiving the SwitchState node for a given
   * port. There is no great place for this so I am putting it in here
   * for now.
   */
  std::shared_ptr<Port> getSwitchStatePort(
      const std::shared_ptr<SwitchState>& state) const;
  std::shared_ptr<Port> getSwitchStatePortIf(
      const std::shared_ptr<SwitchState>& state) const;

  PortID getPortID() const;
  std::string getPortName() const;
  LaneSpeeds supportedLaneSpeeds() const;

  bool supportsSpeed(cfg::PortSpeed speed);

  // Return whether we have enabled the port from cached programmedSetting_
  // isEnabled() should be used whenever possible to improve performance
  bool isEnabled() const;
  // Return whether the link status of the port is actually up.
  // Note: if it is not enabled, return false
  bool isUp() const;
  bool isProgrammed();

  cfg::PortSpeed getSpeed() const;
  cfg::PortSpeed getMaxSpeed() const;

  // Reads Tx Settings from chip given a relative lane number
  phy::TxSettings getTxSettingsForLane(int lane) const;

  /*
   * Setters.
   */
  void registerInPortGroup(BcmPortGroup* portGroup);
  void setIngressVlan(const std::shared_ptr<Port>& swPort);
  void setSpeed(const std::shared_ptr<Port>& swPort);
  void setSflowRates(const std::shared_ptr<Port>& swPort);
  void disableSflow();
  int sampleDestinationToBcmDestFlag(cfg::SampleDestination dest);
  void configureSampleDestination(cfg::SampleDestination sampleDest);
  /**
   * Set port resource to desired settings
   * @return Whether we actually changed anything
   */
  bool setPortResource(const std::shared_ptr<Port>& swPort);
  void setupStatsIfNeeded(const std::shared_ptr<Port>& swPort);
  void setupPrbs(const std::shared_ptr<Port>& swPort);
  std::vector<prbs::PrbsPolynomial> getPortPrbsPolynomials();

  /*
   * Update this port's statistics.
   */
  void updateStats();
  std::optional<HwPortStats> getPortStats() const;
  std::chrono::seconds getTimeRetrieved() const;

  /**
   * Take actions on this port (especially if it is up), so that it will not
   * flap on warm boot.
   */
  void prepareForGracefulExit();

  /**
   * return the FecMode (if any) that is currently enabled
   */
  phy::FecMode getFECMode() const;

  /**
   * The maximum errors the current FEC mode can correct
   *
   * RS544 can correct 16. Everyone else is 8.
   */
  int getFecMaxErrors() {
    auto fecMode = getFECMode();
    if (fecMode == phy::FecMode::RS544 || fecMode == phy::FecMode::RS544_2N) {
      return 16;
    }

    return 8;
  }

  bool getFdrEnabled() const;

  /*
   * Take the appropriate actions for reacting to the port's state changing.
   * e.g. optionally remediate, turn on/off LEDs
   */
  void linkStatusChanged(const std::shared_ptr<Port>& port);

  void cacheFaultStatus(phy::LinkFaultStatus faultStatus);

  static bcm_gport_t asGPort(bcm_port_t port);
  static bool isValidLocalPort(bcm_gport_t gport);
  BcmCosQueueManager* getQueueManager() const {
    return queueManager_.get();
  }

  BcmPortIngressBufferManager* getIngressBufferManager() const {
    return ingressBufferManager_.get();
  }

  std::optional<std::string> getIngressPortMirror() const {
    return ingressMirror_;
  }
  std::optional<std::string> getEgressPortMirror() const {
    return egressMirror_;
  }

  void setIngressPortMirror(const std::string& mirrorName);
  void setEgressPortMirror(const std::string& mirrorName);
  void setQueueWaterMarks(std::map<int16_t, int64_t> queueId2WatermarkBytes);

  /**
   * Enable V4 and V6 L3
   */
  void enableL3(bool enableV4, bool enableV6);

  cfg::PortProfileID getCurrentProfile() const;

  void setLoopbackMode(cfg::PortLoopbackMode mode);

  bool isPortPgConfigured() const;
  PortPgConfigs getCurrentProgrammedPgSettings() const;
  PortPgConfigs getCurrentPgSettings() const;
  BufferPoolCfgPtr getCurrentIngressPoolSettings() const;
  int getProgrammedPgLosslessMode(const int pgId) const;
  int getProgrammedPfcStatusInPg(const int pgId) const;
  void getProgrammedPfcWatchdogParams(
      const int pri,
      std::map<bcm_cosq_pfc_deadlock_control_t, int>& pfcWatchdogControls);
  void getProgrammedPfcState(int* pfcRx, int* pfcTx);

  const PortPgConfig& getDefaultPgSettings() const;
  const BufferPoolCfg& getDefaultIngressPoolSettings() const;
  uint8_t determinePipe() const;
  int getPgMinLimitBytes(const int pgId) const;
  int getIngressSharedBytes(const int pgId) const;
  phy::PhyInfo updateIPhyInfo();

  uint32_t getInterPacketGapBits() const;

  cfg::PortFlowletConfig getPortFlowletConfig() const;

 private:
  class BcmPortStats {
    // All actions or instantiations of this class need to be done in a
    // thread-safe way (for example, the way that locking is done on
    // lastPortStats_) - the class itself does not guarantee this on it's own
   public:
    BcmPortStats() {
      *portStats_.inDiscards_() = 0;
    }
    explicit BcmPortStats(int numUnicastQueues);
    BcmPortStats(HwPortStats portStats, std::chrono::seconds seconds);
    HwPortStats portStats() const;
    void setQueueWaterMarks(std::map<int16_t, int64_t> queueId2WatermarkBytes);
    std::chrono::seconds timeRetrieved() const;

   private:
    HwPortStats portStats_;
    std::chrono::seconds timeRetrieved_{0};
  };
  using PortStatsWLockedPtr =
      folly::Synchronized<std::optional<BcmPortStats>>::WLockedPtr;

  void updateWredStats(std::chrono::seconds now, int64_t* portStatVal);
  void updateInCongestionDiscardStats(
      std::chrono::seconds now,
      uint64_t* portStatVal);
  uint32_t getCL91FECStatus() const;
  bool isCL91FECApplicable() const;
  // no copy or assignment
  BcmPort(BcmPort const&) = delete;
  BcmPort& operator=(BcmPort const&) = delete;

  stats::MonotonicCounter* getPortCounterIf(folly::StringPiece statName);
  void reinitPortStatsLocked(
      const BcmPort::PortStatsWLockedPtr& lockedPortStatsPtr,
      const std::shared_ptr<Port>& swPort);
  void reinitPortStat(folly::StringPiece newName, folly::StringPiece portName);
  void destroyAllPortStatsLocked(
      const BcmPort::PortStatsWLockedPtr& lockedPortStatsPtr);
  void updateStat(
      std::chrono::seconds now,
      folly::StringPiece statName,
      bcm_stat_val_t type,
      int64_t* portStatVal);
  void updateMultiStat(
      std::chrono::seconds now,
      std::vector<folly::StringPiece> statKeys,
      bcm_stat_val_t* types,
      std::vector<int64_t*> portStatVals,
      uint8_t statsCount);
  void updateFecStats(std::chrono::seconds now, HwPortStats& curPortStats);
  void removePortStat(folly::StringPiece statKey);
  void removePortPfcStatsLocked(
      const BcmPort::PortStatsWLockedPtr& lockedPortStatsPtr,
      const std::shared_ptr<Port>& swPort,
      const std::vector<PfcPriority>& priorities);
  void reinitPortPfcStats(const std::shared_ptr<Port>& swPort);
  void updatePortPfcStats(
      std::chrono::seconds now,
      HwPortStats& curPortStats,
      const std::vector<PfcPriority>& pfcPriorities);
  std::string getPfcPriorityStatsKey(
      folly::StringPiece statKey,
      PfcPriority priority);
  void updatePktLenHist(
      std::chrono::seconds now,
      fb303::ExportedHistogramMapImpl::LockableHistogram* hist,
      const std::vector<bcm_stat_val_t>& stats);
  void reinitPortFdrStats(const std::shared_ptr<Port>& swPort);
  void updateFdrStats(std::chrono::seconds now);
  void initCustomStats() const;
  std::string statName(folly::StringPiece statName, folly::StringPiece portName)
      const;

  cfg::PortSpeed getDesiredPortSpeed(const std::shared_ptr<Port>& swPort);
  bcm_port_if_t getDesiredInterfaceMode(
      cfg::PortSpeed speed,
      const std::shared_ptr<Port>& swPort);
  void updateMirror(
      const std::optional<std::string>& swMirrorName,
      MirrorDirection direction,
      cfg::SampleDestination newDestination);

  bcm_pbmp_t getPbmp();

  void setInterfaceMode(const std::shared_ptr<Port>& swPort);
  /**
   * Sets FEC to the desired mode, if port isn't up
   * @return Whether we actually changed anything
   */
  bool setFEC(const std::shared_ptr<Port>& swPort);
  void setPause(const std::shared_ptr<Port>& swPort);
  void setCosqProfile(const int profileId);
  void setPfc(const std::shared_ptr<Port>& swPort);
  void programPfc(const int enableTxPfc, const int enableRxPfc);
  bool pfcWatchdogNeedsReprogramming(const std::shared_ptr<Port>& port);
  void programPfcWatchdog(const std::shared_ptr<Port>& swPort);
  void getPfcCosqDeadlockControl(
      const int pri,
      const bcm_cosq_pfc_deadlock_control_t control,
      int* value,
      const std::string& controlStr);
  void setPfcCosqDeadlockControl(
      const int pri,
      const bcm_cosq_pfc_deadlock_control_t control,
      const int value,
      const std::string& controlStr);
  std::vector<PfcPriority> getLastConfiguredPfcPriorities();
  void programFlowletPortQuality(std::optional<PortFlowletCfgPtr> portFlowlet);

  void setTxSetting(const std::shared_ptr<Port>& swPort);
  void setTxSettingViaPhyControl(
      const std::shared_ptr<Port>& swPort,
      const std::vector<phy::PinConfig>& iphyPinConfigs);
  void setTxSettingViaPhyTx(
      const std::shared_ptr<Port>& swPort,
      const std::vector<phy::PinConfig>& iphyPinConfigs);

  bool isMmuLossy() const;
  bool isMmuLossless() const;

  void applyMirrorAction(
      MirrorAction action,
      MirrorDirection direction,
      cfg::SampleDestination newDestination);

  // Return whether we have enabled the port from SDK
  bool isEnabledInSDK() const;

  // Return programmed port config
  std::shared_ptr<Port> getProgrammedSettings() const {
    auto savedSettings = programmedSettings_.rlock();
    return *savedSettings;
  }

  void setInterPacketGapBits(uint32_t ipgBits);

  BcmSwitch* const hw_{nullptr};
  const bcm_port_t port_; // Broadcom physical port number
  // The gport_ is logically a const, but needs to be initialized as a parameter
  // to SDK call.
  bcm_gport_t gport_; // Broadcom global port number
  uint8_t pipe_;
  BcmPlatformPort* const platformPort_{nullptr};
  int unit_{-1};
  std::optional<std::string> ingressMirror_;
  std::optional<std::string> egressMirror_;
  cfg::SampleDestination sampleDest_;
  TransmitterTechnology transmitterTechnology_{TransmitterTechnology::UNKNOWN};

  // The port group this port is a part of
  BcmPortGroup* portGroup_{nullptr};

  std::map<std::string, stats::MonotonicCounter> portCounters_;
  std::unique_ptr<BcmCosQueueManager> queueManager_;
  std::unique_ptr<BcmPortIngressBufferManager> ingressBufferManager_;

  fb303::ExportedStatMapImpl::LockableStat outQueueLen_;
  std::vector<utility::ExportedCounter> fdrStats_;
  fb303::ExportedHistogramMapImpl::LockableHistogram inPktLengths_;
  fb303::ExportedHistogramMapImpl::LockableHistogram outPktLengths_;

  int codewordErrorsPage_{0};

  folly::Synchronized<std::shared_ptr<Port>> programmedSettings_;

  // Due to enable/disable statCollection and get statCollection are currently
  // called by different threads, and enable/disable statCollection might need
  // to attach/detach FlexCounter, which means if we're trying to collect stats
  // while it's in the process of detaching FlexCounter, the updateStats will
  // fail. Therefore, we need to make sure these calls from different threads
  // can maintain exclusive.
  // Besides since we always reset `portStats_` to nullopt when we disable
  // the statsCollection, we don't actually need an extra bool to represent
  // whether the statCollection is enabled or not. What's more, since
  // `portStats_` is already Synchornized, we can just utilize it to make sure
  // enable/disable and get stats are thread safe.
  folly::Synchronized<std::optional<BcmPortStats>> portStats_;

  std::atomic<bool> destroyed_{false};

  // Need to synchronize cachedFaultStatus since it can be accessed from
  // different threads. The updateStats thread will read the cachedFaultStatus
  // and update in the periodic PhyInfo snapshot, and then clear it to nullopt.
  // A port state update also updates this cachedFaultStatus with the status
  // provided in linkscan callback
  folly::Synchronized<std::optional<phy::LinkFaultStatus>> cachedFaultStatus{
      std::nullopt};

  std::atomic<int> numLanes_{0};

  // TODO: remove this when removing the deprecated fields
  phy::PhyInfo defaultPhyInfo() {
    phy::PhyInfo phyInfo;
    phyInfo.phyChip().ensure();
    phyInfo.line().ensure();
    return phyInfo;
  }
  phy::PhyInfo lastPhyInfo_ = defaultPhyInfo();
};

} // namespace facebook::fboss
