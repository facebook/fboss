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

#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <mutex>
#include <condition_variable>

namespace facebook { namespace fboss {

class BcmLinkStateDependentTests : public BcmTest {
 protected:
  void SetUp() override;
  void applyInitialConfig();
  void bringUpPort(PortID port) {
    bringUpPorts({port});
  }
  void bringDownPort(PortID port) {
    bringDownPorts({port});
  }
  void bringUpPorts(const std::vector<PortID>& ports) {
    portStateChangeImpl(
        ports, cfg::PortState::ENABLED, cfg::PortLoopbackMode::MAC);
  }
  void bringDownPorts(const std::vector<PortID>& ports) {
    // To trigger a link down event on loopback ports, I had to also set
    // loopback mode to NONE when port was disabled.
    portStateChangeImpl(
        ports, cfg::PortState::DISABLED, cfg::PortLoopbackMode::NONE);
  }

 private:
  void portStateChangeImpl(
      const std::vector<PortID>& ports,
      cfg::PortState desiredState,
      cfg::PortLoopbackMode desiredLoopBackMode);
  void setPortIDAndStateToWaitFor(PortID port, bool waitForUp);
  uint32_t featuresDesired() const override {
    return (BcmSwitch::LINKSCAN_DESIRED & ~BcmSwitch::PACKET_RX_DESIRED);
  }
  void linkStateChanged(PortID port, bool up) noexcept override;

  static auto constexpr kPortIdNone{0};
  mutable std::mutex linkEventMutex_;
  // We never use logical port ID 0, so use that to represent uninit port val
  PortID portIdToWaitFor_{kPortIdNone};
  bool waitForPortUp_{false};
  bool desiredPortEventOccurred_{false};
  std::condition_variable linkEventCV_;
};
}}
