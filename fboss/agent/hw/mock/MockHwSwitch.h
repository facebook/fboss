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
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/gen-cpp/switch_config_types.h"

#include <gmock/gmock.h>

namespace facebook { namespace fboss {

class MockPlatform;

class MockHwSwitch : public HwSwitch {
 public:
  explicit MockHwSwitch(MockPlatform* platform);
  typedef std::pair<std::shared_ptr<SwitchState>, BootType> StateAndBootType;
  MOCK_METHOD1(init, HwInitResult(Callback*));
  MOCK_METHOD1(stateChanged, void(const StateDelta&));

  // gmock currently doesn't support move-only types, so we have to
  // use some janky work-arounds.
  std::unique_ptr<TxPacket> allocatePacket(uint32_t) override;

  MOCK_METHOD1(sendPacketSwitched_, void(std::shared_ptr<TxPacket>));
  bool sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept override;

  MOCK_METHOD1(sendPacketOutOfPort_, void(std::shared_ptr<TxPacket>));
  bool sendPacketOutOfPort(std::unique_ptr<TxPacket> pkt,
                          facebook::fboss::PortID portID) noexcept override;

  // TODO
  void updateStats(SwitchStats *switchStats) override {}

  int getHighresSamplers(
      HighresSamplerList* samplers,
      const folly::StringPiece namespaceString,
      const std::set<folly::StringPiece>& counterSet) override {
    return 0;
  }

  void fetchL2Table(std::vector<L2EntryThrift> *l2Table) override {
    return;
  }

  cfg::PortSpeed getPortSpeed(PortID port) const override {
    return cfg::PortSpeed::GIGE;
  }

  cfg::PortSpeed getMaxPortSpeed(PortID port) const override {
    return cfg::PortSpeed::GIGE;
  }

  void gracefulExit(folly::dynamic& switchState) override {
    // TODO
  }

  folly::dynamic toFollyDynamic() const override;

  void initialConfigApplied() override {
    // TODO
  }

  void clearWarmBootCache() override {
    // TODO
  }

  void exitFatal() const override {
    //TODO
  }

  void unregisterCallbacks() override {
    // TODO
  }

  void remedyPorts() override {
    // TODO
  }

  bool isValidStateUpdate(const StateDelta& delta) const override {
    return true;
  }

  MOCK_METHOD2(getAndClearNeighborHit, bool(RouterID, folly::IPAddress&));

  bool isPortUp(PortID port) const override {
    // Should be called only from SwSwitch which knows whether
    // the port is enabled or not
    return true;
  }

 private:
  // Forbidden copy constructor and assignment operator
  MockHwSwitch(MockHwSwitch const &) = delete;
  MockHwSwitch& operator=(MockHwSwitch const &) = delete;
};
}} // facebook::fboss
