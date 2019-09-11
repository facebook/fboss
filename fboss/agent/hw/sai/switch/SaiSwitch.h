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
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiRxPacket.h"

#include <memory>
#include <mutex>

namespace facebook {
namespace fboss {
class SaiPlatform;

class SaiSwitch : public HwSwitch {
 public:
  explicit SaiSwitch(SaiPlatform* platform) : platform_(platform) {}
  HwInitResult init(Callback* callback) noexcept override;

  void unregisterCallbacks() noexcept override;

  std::shared_ptr<SwitchState> stateChanged(const StateDelta& delta) override;

  bool isValidStateUpdate(const StateDelta& delta) const override;

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) const override;

  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept override;

  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      folly::Optional<uint8_t> queue) noexcept override;

  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override;

  bool sendPacketOutOfPortSync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID) noexcept override;

  void updateStats(SwitchStats* switchStats) override;

  void fetchL2Table(std::vector<L2EntryThrift>* l2Table) const override;

  void gracefulExit(folly::dynamic& switchState) override;

  folly::dynamic toFollyDynamic() const override;

  void initialConfigApplied() override;

  void switchRunStateChanged(SwitchRunState newState) override;

  void exitFatal() const override;

  bool isPortUp(PortID port) const override;

  bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) override;

  void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& ports) override;

  cfg::PortSpeed getPortMaxSpeed(PortID port) const override;

  /*
   * This signature matches the SAI callback signature and will be invoked
   * immediately by the non-method SAI callback function.
   */
  void packetRxCallback(
      sai_object_id_t switch_id,
      sai_size_t buffer_size,
      const void* buffer,
      uint32_t attr_count,
      const sai_attribute_t* attr_list);

  void linkStateChangedCallback(
      uint32_t count,
      const sai_port_oper_status_notification_t* data);

  BootType getBootType() const override;

  const SaiManagerTable* managerTable() const;

  SaiManagerTable* managerTable();

  const SaiApiTable* apiTable() const;

  SaiApiTable* apiTable();

 private:
  /*
   * To make SaiSwitch thread-safe, we mirror the public interface with
   * private functions with the same name followed by Locked.
   *
   * The private methods take an additional argument -- a const ref to
   * lock_guard -- which ensures that a lock is held during the call.
   * While it could be any lock, there is only saiSwitchMutex_ in this context.
   *
   * The public methods take saiSwitchMutex_ with an
   * std::lock_guard and then call the Locked version passing the const
   * lock_guard ref. The Locked versions don't take any additional locks.
   *
   * Within SaiSwitch, no method should call methods from the public interface
   * to avoid a deadlock trying to lock saiSwitchMutex_ twice.
   */
  HwInitResult initLocked(
      const std::lock_guard<std::mutex>& lock,
      Callback* callback) noexcept;

  void unregisterCallbacksLocked(
      const std::lock_guard<std::mutex>& lock) noexcept;

  std::shared_ptr<SwitchState> stateChangedLocked(
      const std::lock_guard<std::mutex>& lock,
      const StateDelta& delta);

  bool isValidStateUpdateLocked(
      const std::lock_guard<std::mutex>& lock,
      const StateDelta& delta) const;

  std::unique_ptr<TxPacket> allocatePacketLocked(
      const std::lock_guard<std::mutex>& lock,
      uint32_t size) const;

  bool sendPacketSwitchedAsyncLocked(
      const std::lock_guard<std::mutex>& lock,
      std::unique_ptr<TxPacket> pkt) noexcept;

  bool sendPacketOutOfPortAsyncLocked(
      const std::lock_guard<std::mutex>& lock,
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      folly::Optional<uint8_t> queue) noexcept;

  bool sendPacketSwitchedSyncLocked(
      const std::lock_guard<std::mutex>& lock,
      std::unique_ptr<TxPacket> pkt) noexcept;

  bool sendPacketOutOfPortSyncLocked(
      const std::lock_guard<std::mutex>& lock,
      std::unique_ptr<TxPacket> pkt,
      PortID portID) noexcept;

  void updateStatsLocked(
      const std::lock_guard<std::mutex>& lock,
      SwitchStats* switchStats);

  void fetchL2TableLocked(
      const std::lock_guard<std::mutex>& lock,
      std::vector<L2EntryThrift>* l2Table) const;

  void gracefulExitLocked(
      const std::lock_guard<std::mutex>& lock,
      folly::dynamic& switchState);

  folly::dynamic toFollyDynamicLocked(
      const std::lock_guard<std::mutex>& lock) const;

  void initialConfigAppliedLocked(const std::lock_guard<std::mutex>& lock);

  void switchRunStateChangedLocked(
      const std::lock_guard<std::mutex>& lock,
      SwitchRunState newState);

  void exitFatalLocked(const std::lock_guard<std::mutex>& lock) const;

  bool isPortUpLocked(const std::lock_guard<std::mutex>& lock, PortID port)
      const;

  bool getAndClearNeighborHitLocked(
      const std::lock_guard<std::mutex>& lock,
      RouterID vrf,
      folly::IPAddress& ip);

  void clearPortStatsLocked(
      const std::lock_guard<std::mutex>& lock,
      const std::unique_ptr<std::vector<int32_t>>& ports);

  cfg::PortSpeed getPortMaxSpeedLocked(
      const std::lock_guard<std::mutex>& lock,
      PortID port) const;

  void packetRxCallbackLocked(
      const std::lock_guard<std::mutex>& lock,
      sai_object_id_t switch_id,
      sai_size_t buffer_size,
      const void* buffer,
      uint32_t attr_count,
      const sai_attribute_t* attr_list);

  BootType getBootTypeLocked(const std::lock_guard<std::mutex>& lock) const;

  const SaiManagerTable* managerTableLocked(
      const std::lock_guard<std::mutex>& lock) const;
  SaiManagerTable* managerTableLocked(const std::lock_guard<std::mutex>& lock);

  const SaiApiTable* apiTableLocked(
      const std::lock_guard<std::mutex>& lock) const;
  SaiApiTable* apiTableLocked(const std::lock_guard<std::mutex>& lock);

  std::shared_ptr<SaiApiTable> saiApiTable_;
  std::unique_ptr<SaiManagerTable> managerTable_;
  BootType bootType_{BootType::UNINITIALIZED};
  SaiPlatform* platform_;
  Callback* callback_{nullptr};
  /*
   * This mutex acts as a global lock for using SaiSwitch, since updates
   * are serialized by SwSwitch naturally, this prevents races between:
   * 1. updates
   * 2. queries (counters, thrift calls)
   * 3. sai callbacks (port status, packet received)
   */
  mutable std::mutex saiSwitchMutex_;

  SwitchSaiId switchId_;
};

} // namespace fboss
} // namespace facebook
