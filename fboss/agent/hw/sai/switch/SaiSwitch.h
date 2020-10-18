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
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/hw/HwSwitchStats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiRxPacket.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/io/async/EventBase.h>

#include <memory>
#include <mutex>
#include <thread>

namespace facebook::fboss {

class ConcurrentIndices;

class SaiSwitch : public HwSwitch {
 public:
  explicit SaiSwitch(
      SaiPlatform* platform,
      uint32_t featuresDesired =
          (FeaturesDesired::PACKET_RX_DESIRED |
           FeaturesDesired::LINKSCAN_DESIRED));

  ~SaiSwitch() override;

  static auto constexpr kAclTable1 = "AclTable1";

  HwInitResult init(Callback* callback) noexcept override;

  void unregisterCallbacks() noexcept override;
  /*
   * SaiSwitch requires L2/FDB entries to be created for resolved
   * neighbors to be able to compute egress ports for nhops. A join
   * b/w the FDB (mac, port), neighbor (ip, mac) is done to figure
   * out the nhop ip-> mac, egress port information
   */
  bool needL2EntryForNeighbor() const override {
    return true;
  }

  std::shared_ptr<SwitchState> stateChanged(const StateDelta& delta) override;

  bool isValidStateUpdate(const StateDelta& delta) const override;

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) const override;

  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept override;

  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queueId) noexcept override;

  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override;

  bool sendPacketOutOfPortSync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queueId) noexcept override;

  folly::F14FastMap<std::string, HwPortStats> getPortStats() const override;

  uint64_t getDeviceWatermarkBytes() const override;

  void fetchL2Table(std::vector<L2EntryThrift>* l2Table) const override;

  void gracefulExit(folly::dynamic& switchState) override;

  folly::dynamic toFollyDynamic() const override;

  void exitFatal() const override;

  bool isPortUp(PortID port) const override;

  bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) override;

  void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& ports) override;

  cfg::PortSpeed getPortMaxSpeed(PortID port) const override;

  void linkStateChangedCallbackTopHalf(
      uint32_t count,
      const sai_port_oper_status_notification_t* data);
  void fdbEventCallback(
      uint32_t count,
      const sai_fdb_event_notification_data_t* data);

  BootType getBootType() const override;

  const SaiManagerTable* managerTable() const;

  SaiManagerTable* managerTable();

  /*
   * This method is not thread safe, it should only be used
   * from the SAI adapter's rx callback caller thread.
   */
  void packetRxCallback(
      SwitchSaiId switch_id,
      sai_size_t buffer_size,
      const void* buffer,
      uint32_t attr_count,
      const sai_attribute_t* attr_list);

  SwitchSaiId getSwitchId() const {
    return switchId_;
  }
  SaiPlatform* getPlatform() const override {
    return platform_;
  }

  const ConcurrentIndices& concurrentIndices() const {
    return *concurrentIndices_;
  }
  SwitchRunState getSwitchRunState() const;
  bool isFullyInitialized() const;

  std::string listObjects(
      const std::vector<HwObjectType>& types) const override;
  void dumpDebugState(const std::string& /*path*/) const override;
  std::string listObjects(const std::vector<sai_object_type_t>& objects) const;

 private:
  void switchRunStateChangedImpl(SwitchRunState newState) override;

  void updateStatsImpl(SwitchStats* switchStats) override;
  void updateResourceUsageLocked(const std::lock_guard<std::mutex>& lock);
  /*
   * To make SaiSwitch thread-safe, we mirror the public interface with
   * private functions with the same name followed by Locked.
   *
   * The private methods take an additional argument -- a const ref to
   * lock_guard -- which ensures that a lock is held during the call.
   * While it could be any lock, there is only saiSwitchMutex_ in this context.
   *
   * The public methods take saiSwitchMutex_ with an std::lock_guard and then
   * call the Locked version passing the const lock_guard ref. The Locked
   * versions don't take any additional locks.
   *
   * Within SaiSwitch, no method should call methods from the public interface
   * to avoid a deadlock trying to lock saiSwitchMutex_ twice.
   *
   * N.B., packet rx is handled slightly differently, which is documented
   * along with its methods.
   */
  HwInitResult initLocked(
      const std::lock_guard<std::mutex>& lock,
      Callback* callback) noexcept;

  void unregisterCallbacksLocked(
      const std::lock_guard<std::mutex>& lock) noexcept;

  bool isValidStateUpdateLocked(
      const std::lock_guard<std::mutex>& lock,
      const StateDelta& delta) const;

  void fetchL2TableLocked(
      const std::lock_guard<std::mutex>& lock,
      std::vector<L2EntryThrift>* l2Table) const;

  void gracefulExitLocked(
      const std::lock_guard<std::mutex>& lock,
      folly::dynamic& switchState);

  folly::dynamic toFollyDynamicLocked(
      const std::lock_guard<std::mutex>& lock) const;

  void switchRunStateChangedImplLocked(
      const std::lock_guard<std::mutex>& lock,
      SwitchRunState newState);

  void exitFatalLocked(const std::lock_guard<std::mutex>& lock) const;

  bool isPortUpLocked(const std::lock_guard<std::mutex>& lock, PortID port)
      const;

  cfg::PortSpeed getPortMaxSpeedLocked(
      const std::lock_guard<std::mutex>& lock,
      PortID port) const;

  void fdbEventCallbackLockedBottomHalf(
      const std::lock_guard<std::mutex>& lock,
      std::vector<sai_fdb_event_notification_data_t> data);

  BootType getBootTypeLocked(const std::lock_guard<std::mutex>& lock) const;

  const SaiManagerTable* managerTableLocked(
      const std::lock_guard<std::mutex>& lock) const;
  SaiManagerTable* managerTableLocked(const std::lock_guard<std::mutex>& lock);

  void gracefulExitLocked(
      folly::dynamic& switchState,
      const std::lock_guard<std::mutex>& lock);
  void initLinkScanLocked(const std::lock_guard<std::mutex>& lock);
  void initRxLocked(const std::lock_guard<std::mutex>& lock);

  folly::F14FastMap<std::string, HwPortStats> getPortStatsLocked(
      const std::lock_guard<std::mutex>& lock) const;

  void linkStateChangedCallbackBottomHalf(
      std::vector<sai_port_oper_status_notification_t> data);

  uint64_t getDeviceWatermarkBytesLocked(
      const std::lock_guard<std::mutex>& lock) const;

  template <typename ManagerT>
  void processDefaultDataPlanePolicyDelta(
      const StateDelta& delta,
      ManagerT& mgr);

  void processLinkStateChangeDelta(const StateDelta& delta);

  template <
      typename Delta,
      typename Manager,
      typename... Args,
      typename ChangeFunc = void (Manager::*)(
          const std::shared_ptr<typename Delta::Node>&,
          const std::shared_ptr<typename Delta::Node>&,
          Args...),
      typename AddedFunc = void (
          Manager::*)(const std::shared_ptr<typename Delta::Node>&, Args...),
      typename RemovedFunc = void (
          Manager::*)(const std::shared_ptr<typename Delta::Node>&, Args...)>
  void processDelta(
      Delta delta,
      Manager& manager,
      ChangeFunc changedFunc,
      AddedFunc addedFunc,
      RemovedFunc removedFunc,
      Args... args);

  template <
      typename Delta,
      typename Manager,
      typename... Args,
      typename ChangeFunc = void (Manager::*)(
          const std::shared_ptr<typename Delta::Node>&,
          const std::shared_ptr<typename Delta::Node>&,
          Args...)>
  void processChangedDelta(
      Delta delta,
      Manager& manager,
      ChangeFunc changedFunc,
      Args... args);

  template <
      typename Delta,
      typename Manager,
      typename... Args,
      typename AddedFunc = void (
          Manager::*)(const std::shared_ptr<typename Delta::Node>&, Args...)>
  void processAddedDelta(
      Delta delta,
      Manager& manager,
      AddedFunc addedFunc,
      Args... args);

  template <
      typename Delta,
      typename Manager,
      typename... Args,
      typename RemovedFunc = void (
          Manager::*)(const std::shared_ptr<typename Delta::Node>&, Args...)>
  void processRemovedDelta(
      Delta delta,
      Manager& manager,
      RemovedFunc removedFunc,
      Args... args);

  void processSwitchSettingsChanged(const StateDelta& delta);

  static PortSaiId getCPUPortSaiId(SwitchSaiId switchId);

  /*
   * SaiSwitch must support a few varieties of concurrent access:
   * 1. state updates on the SwSwitch update thread calling stateChanged
   * 2. packet rx callback
   * 3. async tx thread
   * 4. port state event callback (i.e., linkscan)
   * 5. stats collection
   * 6. getters exposed to thrift or other threads
   *
   * It is critical that 2, 3, and 4 are not blocked by other, possibly slower
   * operations. Ideally, 1 and 5 are able to make progress relatively freely
   * as well. To that end, we synchronize most access (1, 6) with a global
   * lock, but give a fast-path for 2, 3, 4, 5 in the form of possibly out
   * of date indices stored in folly::ConcurrentHashMaps in ConcurrentIndices
   * e.g., rx can look up the PortID from the sai_object_id_t on the
   * packet without blocking normal hardware programming.
   *
   * By using the concurent hashmap, Rx and Tx path is lock free. Running Tx/Rx
   * in a separate eventbase thread severely affects the slow path performance.
   * Handling Rx in single thread improved the performance to be on-par with
   * native bcm. Handling Tx without eventbase thread improved the
   * performance by 2000 pps.
   */
  mutable std::mutex saiSwitchMutex_;
  std::unique_ptr<ConcurrentIndices> concurrentIndices_;

  std::shared_ptr<SwitchState> getColdBootSwitchState();

  std::optional<L2Entry> getL2Entry(
      const sai_fdb_event_notification_data_t& fdbEvent) const;

  std::unique_ptr<SaiManagerTable> managerTable_;
  BootType bootType_{BootType::UNINITIALIZED};
  SaiPlatform* platform_;
  Callback* callback_{nullptr};

  SwitchSaiId switchId_;

  std::unique_ptr<std::thread> linkStateBottomHalfThread_;
  folly::EventBase linkStateBottomHalfEventBase_;
  std::unique_ptr<std::thread> fdbEventBottomHalfThread_;
  folly::EventBase fdbEventBottomHalfEventBase_;

  HwResourceStats hwResourceStats_;
  std::atomic<SwitchRunState> runState_{SwitchRunState::UNINITIALIZED};
};

} // namespace facebook::fboss
