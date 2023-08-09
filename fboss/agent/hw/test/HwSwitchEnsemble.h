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

#include "fboss/agent/HwAgent.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/mnpu/SplitAgentThriftSyncer.h"
#include "fboss/agent/platforms/tests/utils/TestPlatformTypes.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/test/MultiSwitchTestServer.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/agent/types.h"

#include <folly/Synchronized.h>

#include <memory>
#include <set>
#include <thread>

DECLARE_bool(mmu_lossless_mode);
DECLARE_bool(qgroup_guarantee_enable);
DECLARE_bool(enable_exact_match);
DECLARE_bool(flowletSwitchingEnable);
DECLARE_bool(skip_buffer_reservation);

namespace folly {
class FunctionScheduler;
}

namespace facebook::fboss {

class Platform;
class SwitchState;
class HwLinkStateToggler;
class SwitchIdScopeResolver;
class StateObserver;

class HwSwitchEnsemble : public TestEnsembleIf {
 public:
  class HwSwitchEventObserverIf {
   public:
    virtual ~HwSwitchEventObserverIf() = default;

    void receivePacket(RxPacket* pkt) noexcept {
      if (!stopped_.load()) {
        packetReceived(pkt);
      }
    }
    void changeLinkState(PortID port, bool up) {
      if (!stopped_.load()) {
        linkStateChanged(port, up);
      }
    }
    void updateL2EntryState(
        L2Entry l2Entry,
        L2EntryUpdateType l2EntryUpdateType) {
      if (!stopped_.load()) {
        l2LearningUpdateReceived(l2Entry, l2EntryUpdateType);
      }
    }
    virtual void stopObserving() {
      stopped_.store(true);
    }

   private:
    virtual void packetReceived(RxPacket* pkt) noexcept = 0;
    virtual void linkStateChanged(PortID port, bool up) = 0;
    virtual void l2LearningUpdateReceived(
        L2Entry l2Entry,
        L2EntryUpdateType l2EntryUpdateType) = 0;
    std::atomic<bool> stopped_{false};
  };
  void stopObservers();
  struct HwSwitchEnsembleInitInfo {
    std::optional<TransceiverInfo> overrideTransceiverInfo;
    std::optional<std::map<int64_t, cfg::DsfNode>> overrideDsfNodes;
  };
  enum Feature : uint32_t {
    PACKET_RX,
    LINKSCAN,
    STATS_COLLECTION,
    TAM_NOTIFY,
    MULTISWITCH_THRIFT_SERVER, /* Start multiswitch service on test server */
  };
  using Features = std::set<Feature>;

  explicit HwSwitchEnsemble(const Features& featuresDesired);
  ~HwSwitchEnsemble() override;
  std::shared_ptr<SwitchState> applyNewState(
      std::shared_ptr<SwitchState> newState,
      bool rollbackOnHwOverflow = false) override {
    return applyNewStateImpl(newState, false, rollbackOnHwOverflow);
  }
  std::shared_ptr<SwitchState> applyNewStateTransaction(
      std::shared_ptr<SwitchState> newState) {
    return applyNewStateImpl(newState, true);
  }
  void applyInitialConfig(const cfg::SwitchConfig& cfg) override;
  std::shared_ptr<SwitchState> applyNewConfig(
      const cfg::SwitchConfig& config) override;

  std::shared_ptr<SwitchState> getProgrammedState() const override;
  HwLinkStateToggler* getLinkToggler() {
    return linkToggler_.get();
  }
  RoutingInformationBase* getRib() {
    return routingInformationBase_.get();
  }
  virtual Platform* getPlatform() {
    return hwAgent_->getPlatform();
  }
  virtual const Platform* getPlatform() const {
    return hwAgent_->getPlatform();
  }
  HwSwitch* getHwSwitch() override;
  const HwSwitch* getHwSwitch() const override {
    return const_cast<HwSwitchEnsemble*>(this)->getHwSwitch();
  }
  const HwAsic* getAsic() const {
    return getPlatform()->getAsic();
  }
  // Used for testing only
  HwAsicTable* getHwAsicTable() {
    return hwAsicTable_.get();
  }
  virtual std::map<int64_t, cfg::DsfNode> dsfNodesFromInputConfig() const {
    return {};
  }
  virtual void init(
      const HwSwitchEnsemble::HwSwitchEnsembleInitInfo& /*info*/) = 0;

  void packetReceived(std::unique_ptr<RxPacket> pkt) noexcept override;
  void linkStateChanged(
      PortID port,
      bool up,
      std::optional<phy::LinkFaultStatus> iPhyFaultStatus =
          std::nullopt) override;
  void l2LearningUpdateReceived(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType) override;
  void pfcWatchdogStateChanged(const PortID& port, const bool deadlock)
      override;
  void exitFatal() const noexcept override {}
  void addHwEventObserver(HwSwitchEventObserverIf* observer);
  void removeHwEventObserver(HwSwitchEventObserverIf* observer);
  void switchRunStateChanged(SwitchRunState switchState);

  static Features getAllFeatures();

  /*
   * Depending on the implementation of the underlying forwarding plane, it is
   * possible that HwSwitch::sendPacketSwitchedSync and
   * HwSwitch::sendPacketOutOfPortSync are not able to block until we are sure
   * the packet has been sent.
   *
   * These methods wrap a packet send with a check of the port counters to
   * ensure that the packet has really been sent before returning. Since
   * checking counters per packet can have an adverse effect on performance,
   * these methods should only be used in tests which send a small number of
   * packets but want to benefit from the simpler synchronous semantices. Tests
   * which send packets in bulk should use a different strategy, such as
   * implementing retries on the expected post-send conditions.
   */
  bool ensureSendPacketSwitched(std::unique_ptr<TxPacket> pkt);
  bool ensureSendPacketOutOfPort(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt);
  bool waitPortStatsCondition(
      std::function<bool(const std::map<PortID, HwPortStats>&)> conditionFn,
      uint32_t retries = 20,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(20));
  bool waitStatsCondition(
      const std::function<bool()>& conditionFn,
      const std::function<void()>& updateStatsFn,
      uint32_t retries = 20,
      const std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(20));

  std::vector<SystemPortID> masterLogicalSysPortIds() const;
  virtual std::vector<PortID> getAllPortsInGroup(PortID portID) const = 0;
  virtual std::vector<FlexPortMode> getSupportedFlexPortModes() const = 0;
  virtual bool isRouteScaleEnabled() const = 0;
  virtual uint64_t getSdkSwitchId() const = 0;

  virtual void dumpHwCounters() const = 0;
  /*
   * Get latest port stats for given ports
   */
  virtual std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports);
  HwPortStats getLatestPortStats(PortID port);
  /*
   * Get latest sys port stats for given sys ports
   */
  virtual std::map<SystemPortID, HwSysPortStats> getLatestSysPortStats(
      const std::vector<SystemPortID>& ports);
  HwSysPortStats getLatestSysPortStats(SystemPortID port);
  /*
   * Get latest stats for given aggregate ports
   */
  virtual std::map<AggregatePortID, HwTrunkStats> getLatestAggregatePortStats(
      const std::vector<AggregatePortID>& aggregatePorts) = 0;
  HwTrunkStats getLatestAggregatePortStats(AggregatePortID port);

  state::WarmbootState gracefulExitState() const;
  /*
   * Initiate graceful exit
   */
  virtual void gracefulExit();
  void waitForLineRateOnPort(PortID port);
  void waitForSpecificRateOnPort(
      PortID port,
      const uint64_t desiredBps,
      int secondsToWaitPerIteration = 1);
  void ensureThrift();

  virtual void runDiagCommand(
      const std::string& input,
      std::string& output) = 0;
  HwSwitchEnsembleRouteUpdateWrapper getRouteUpdater() {
    return HwSwitchEnsembleRouteUpdateWrapper(
        this, routingInformationBase_.get());
  }
  size_t getMinPktsForLineRate(const PortID& portId);
  int readPfcDeadlockDetectionCounter(const PortID& port);
  int readPfcDeadlockRecoveryCounter(const PortID& port);
  void clearPfcDeadlockRecoveryCounter(const PortID& port);
  void clearPfcDeadlockDetectionCounter(const PortID& port);

  virtual void createEqualDistributedUplinkDownlinks(
      const std::vector<PortID>& ports,
      std::vector<PortID>& uplinks,
      std::vector<PortID>& downlinks,
      std::vector<PortID>& disabled,
      const int totalLinkCount);

  virtual bool isSai() const = 0;

  const SwitchIdScopeResolver& scopeResolver() const override;

  void registerStateObserver(
      StateObserver* /*observer*/,
      const std::string& /*name*/) override {}
  void unregisterStateObserver(StateObserver* /*observer*/) override {}

 protected:
  /*
   * Setup ensemble
   */
  void setupEnsemble(
      std::unique_ptr<HwAgent> hwAgent,
      std::unique_ptr<HwLinkStateToggler> linkToggler,
      std::unique_ptr<std::thread> thriftThread,
      const HwSwitchEnsembleInitInfo& info);
  uint32_t getHwSwitchFeatures() const;
  bool haveFeature(Feature feature) {
    return featuresDesired_.find(feature) != featuresDesired_.end();
  }

 private:
  void storeWarmBootState(const state::WarmbootState& state);
  std::shared_ptr<SwitchState> updateEncapIndices(
      const std::shared_ptr<SwitchState>& in) const;
  // To update programmed state after rollback
  friend class SaiRollbackTest;
  std::shared_ptr<SwitchState> applyNewStateImpl(
      const std::shared_ptr<SwitchState>& newState,
      bool transaction,
      bool disableAppliedStateVerification = false);
  fsdb::OperDelta applyUpdate(
      const fsdb::OperDelta& operDelta,
      const std::lock_guard<std::mutex>& lock,
      bool transaction);
  virtual std::unique_ptr<std::thread> setupThrift() = 0;

  void addOrUpdateCounter(const PortID& port, const bool deadlock);
  void clearPfcWatchdogCounter(const PortID& port, const bool deadlock);
  int readPfcWatchdogCounter(const PortID& port, const bool deadlock);
  bool waitForRateOnPort(
      PortID port,
      uint64_t desiredBps,
      int secondsToWaitPerIteration = 1);

  std::shared_ptr<SwitchState> programmedState_{nullptr};
  std::unique_ptr<RoutingInformationBase> routingInformationBase_;
  std::unique_ptr<HwLinkStateToggler> linkToggler_;
  std::unique_ptr<SplitAgentThriftSyncer> thriftSyncer_{nullptr};
  std::unique_ptr<HwAgent> hwAgent_;
  const Features featuresDesired_;
  folly::Synchronized<std::set<HwSwitchEventObserverIf*>> hwEventObservers_;
  std::unique_ptr<std::thread> thriftThread_;
  std::unique_ptr<folly::FunctionScheduler> fs_;

  std::map<PortID, int> watchdogDeadlockCounter_;
  std::map<PortID, int> watchdogRecoveryCounter_;

  // Test and observer threads can both apply state
  // updadtes. So protect with a mutex
  mutable std::mutex updateStateMutex_;

  std::unique_ptr<HwAsicTable> hwAsicTable_;
  std::unique_ptr<SwitchIdScopeResolver> scopeResolver_;
  std::unique_ptr<MultiSwitchTestServer> swSwitchTestServer_;
};

} // namespace facebook::fboss
