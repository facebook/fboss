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

#include <gmock/gmock.h>

#include "fboss/agent/TunManager.h"

namespace facebook::fboss {

class RxPacket;
class SwitchState;

class MockTunManager : public TunManager {
 public:
  MockTunManager(SwSwitch* sw, folly::EventBase* evb);
  ~MockTunManager() override {}

  MOCK_METHOD0(startObservingUpdates, void());
  MOCK_METHOD1(stateUpdated, void(const StateDelta& delta));
  MOCK_METHOD1(sync, void(std::shared_ptr<SwitchState>));

  MOCK_METHOD1(
      sendPacketToHost_,
      bool(std::tuple<InterfaceID, std::shared_ptr<RxPacket>>));
  bool sendPacketToHost(InterfaceID, std::unique_ptr<RxPacket> pkt) override;
  MOCK_METHOD1(doProbe, void(std::lock_guard<std::mutex>&));
};

} // namespace facebook::fboss
