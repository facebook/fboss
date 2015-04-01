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

namespace facebook { namespace fboss {

class SimPlatform;

class SimSwitch : public HwSwitch {
 public:
  SimSwitch(SimPlatform* platform, uint32_t numPorts);

  std::pair<std::shared_ptr<SwitchState>, BootType>
    init(Callback* callback) override;
  void stateChanged(const StateDelta& delta) override;
  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) override;
  bool sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept override;
  bool sendPacketOutOfPort(
      std::unique_ptr<TxPacket> pkt,
      PortID portID) noexcept override;

  void gracefulExit() override {}
  void clearWarmBootCache() override {}
  void injectPacket(std::unique_ptr<RxPacket> pkt);
  void initialConfigApplied() override {}

  // TODO
  void updateStats(SwitchStats *switchStats) override {}

  int getHighresSamplers(
      HighresSamplerList* samplers,
      const folly::StringPiece namespaceString,
      const std::set<folly::StringPiece>& counterSet) override {
    return 0;
  }

  void resetTxCount() { txCount_ = 0; }
  uint64_t getTxCount() const { return txCount_; }
  void exitFatal() const override {
    // TODO
  }

  bool neighborEntryHit(RouterID vrf, folly::IPAddress& ip) const override {
    //TODO
    return false;
  }

  bool isPortUp(PortID port) const override {
    // Should be called only from SwSwitch which knows whether
    // the port is enabled or not
    return true;
  }

 private:
  // Forbidden copy constructor and assignment operator
  SimSwitch(SimSwitch const &) = delete;
  SimSwitch& operator=(SimSwitch const &) = delete;

  HwSwitch::Callback* callback_{nullptr};
  uint32_t numPorts_{0};
  uint64_t txCount_{0};
};

}} // facebook::fboss
