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

#include <folly/Optional.h>

namespace facebook { namespace fboss {

class SimPlatform;
class SwitchState;

class SimSwitch : public HwSwitch {
 public:
  SimSwitch(SimPlatform* platform, uint32_t numPorts);

  HwInitResult init(Callback* callback) override;
  std::shared_ptr<SwitchState> stateChanged(const StateDelta& delta) override;
  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) override;
  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept override;
  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      folly::Optional<uint8_t> cos = folly::none) noexcept override;
  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override;
  bool sendPacketOutOfPortSync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID) noexcept override;
  void gracefulExit(folly::dynamic& /*switchState*/) override {}

  folly::dynamic toFollyDynamic() const override;

  void clearWarmBootCache() override {}
  void injectPacket(std::unique_ptr<RxPacket> pkt);
  void initialConfigApplied() override {}

  // TODO
  void updateStats(SwitchStats* /*switchStats*/) override {}

  void fetchL2Table(std::vector<L2EntryThrift>* /*l2Table*/) override {
    return;
  }

  void resetTxCount() { txCount_ = 0; }
  uint64_t getTxCount() const { return txCount_; }
  void exitFatal() const override {
    // TODO
  }

  void unregisterCallbacks() override {
    // TODO
  }

  bool getAndClearNeighborHit(RouterID /*vrf*/, folly::IPAddress& /*ip*/)
      override {
    //TODO
    return false;
  }

  bool isPortUp(PortID /*port*/) const override {
    // Should be called only from SwSwitch which knows whether
    // the port is enabled or not
    return true;
  }

  bool isValidStateUpdate(const StateDelta& /*delta*/) const override {
    return true;
  }

  void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& /*ports*/) override {}

 private:
  // Forbidden copy constructor and assignment operator
  SimSwitch(SimSwitch const &) = delete;
  SimSwitch& operator=(SimSwitch const &) = delete;

  HwSwitch::Callback* callback_{nullptr};
  uint32_t numPorts_{0};
  uint64_t txCount_{0};
};

}} // facebook::fboss
