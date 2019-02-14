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

#include <memory>

namespace facebook { namespace fboss {

class SaiApiTable;
class SaiManagerTable;

class SaiSwitch : public HwSwitch {
 public:
  HwInitResult init(Callback* callback) noexcept override;
  void unregisterCallbacks() noexcept override;
  std::shared_ptr<SwitchState> stateChanged(const StateDelta& delta) override;
  bool isValidStateUpdate(const StateDelta& delta) const override;
  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) override;
  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept override;
  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      folly::Optional<uint8_t> cos) noexcept override;
  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override;
  bool sendPacketOutOfPortSync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID) noexcept override;
  void updateStats(SwitchStats* switchStats) override;
  void fetchL2Table(std::vector<L2EntryThrift>* l2Table) override;
  void gracefulExit(folly::dynamic& switchState) override;
  folly::dynamic toFollyDynamic() const override;
  void initialConfigApplied() override;
  void clearWarmBootCache() override;
  void exitFatal() const override;
  bool isPortUp(PortID port) const override;
  bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) override;
  void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& ports) override;
 private:
  std::unique_ptr<SaiApiTable> saiApiTable_;
  std::unique_ptr<SaiManagerTable> managerTable_;
};

} // namespace fboss
} // namespace facebook
