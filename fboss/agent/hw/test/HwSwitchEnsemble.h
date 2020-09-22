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

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/platforms/tests/utils/TestPlatformTypes.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/types.h"

#include <folly/Synchronized.h>

#include <memory>
#include <set>
#include <thread>

namespace folly {
class FunctionScheduler;
}

namespace facebook::fboss {

class Platform;
class SwitchState;
class HwLinkStateToggler;

class HwSwitchEnsemble : public HwSwitch::Callback {
 public:
  class HwSwitchEventObserverIf {
   public:
    virtual ~HwSwitchEventObserverIf() = default;
    virtual void packetReceived(RxPacket* pkt) noexcept = 0;
    virtual void linkStateChanged(PortID port, bool up) = 0;
    virtual void l2LearningUpdateReceived(
        L2Entry l2Entry,
        L2EntryUpdateType l2EntryUpdateType) = 0;
  };
  enum Feature : uint32_t {
    PACKET_RX,
    LINKSCAN,
    STATS_COLLECTION,
  };
  using Features = std::set<Feature>;

  explicit HwSwitchEnsemble(const Features& featuresDesired);
  ~HwSwitchEnsemble() override;
  void setAllowPartialStateApplication(bool allow) {
    allowPartialStateApplication_ = allow;
  }
  std::shared_ptr<SwitchState> applyNewState(
      std::shared_ptr<SwitchState> newState);
  void applyInitialConfig(const cfg::SwitchConfig& cfg);

  std::shared_ptr<SwitchState> getProgrammedState() const;
  HwLinkStateToggler* getLinkToggler() {
    return linkToggler_.get();
  }
  const rib::RoutingInformationBase* getRib() const {
    return routingInformationBase_.get();
  }
  rib::RoutingInformationBase* getRib() {
    return routingInformationBase_.get();
  }
  virtual Platform* getPlatform() {
    return platform_.get();
  }
  virtual const Platform* getPlatform() const {
    return platform_.get();
  }
  virtual HwSwitch* getHwSwitch();
  virtual const HwSwitch* getHwSwitch() const {
    return const_cast<HwSwitchEnsemble*>(this)->getHwSwitch();
  }
  void packetReceived(std::unique_ptr<RxPacket> pkt) noexcept override;
  void linkStateChanged(PortID port, bool up) override;
  void l2LearningUpdateReceived(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType) override;
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

  virtual std::vector<PortID> logicalPortIds() const = 0;
  virtual std::vector<PortID> masterLogicalPortIds() const = 0;
  virtual std::vector<PortID> getAllPortsInGroup(PortID portID) const = 0;
  virtual std::vector<FlexPortMode> getSupportedFlexPortModes() const = 0;
  virtual bool isRouteScaleEnabled() const = 0;
  virtual uint64_t getSwitchId() const = 0;

  virtual void dumpHwCounters() const = 0;
  /*
   * Get latest port stats for given ports
   */
  virtual std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports) = 0;
  HwPortStats getLatestPortStats(PortID port);
  /*
   * Initiate graceful exit
   */
  void gracefulExit();
  void waitForLineRateOnPort(PortID port);
  void ensureThrift();

 protected:
  /*
   * Setup ensemble
   */
  void setupEnsemble(
      std::unique_ptr<Platform> platform,
      std::unique_ptr<HwLinkStateToggler> linkToggler,
      std::unique_ptr<std::thread> thriftThread);
  uint32_t getHwSwitchFeatures() const;
  bool haveFeature(Feature feature) {
    return featuresDesired_.find(feature) != featuresDesired_.end();
  }

 private:
  virtual std::unique_ptr<std::thread> setupThrift() = 0;

  bool waitForAnyPorAndQueutOutBytesIncrement(
      const std::map<PortID, HwPortStats>& originalPortStats);

  std::shared_ptr<SwitchState> programmedState_{nullptr};
  std::unique_ptr<rib::RoutingInformationBase> routingInformationBase_;
  std::unique_ptr<HwLinkStateToggler> linkToggler_;
  std::unique_ptr<Platform> platform_;
  const Features featuresDesired_;
  folly::Synchronized<std::set<HwSwitchEventObserverIf*>> hwEventObservers_;
  std::unique_ptr<std::thread> thriftThread_;
  bool allowPartialStateApplication_{false};
  std::unique_ptr<folly::FunctionScheduler> fs_;
  // Test and observer threads can both apply state
  // updadtes. So protect with a mutex
  mutable std::mutex updateStateMutex_;
};

} // namespace facebook::fboss
