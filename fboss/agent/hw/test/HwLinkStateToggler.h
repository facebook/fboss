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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

namespace facebook::fboss {

class Platform;
class SwitchState;

class HwLinkStateToggler {
 public:
  using StateUpdateFn = std::function<void(std::shared_ptr<SwitchState>)>;

  explicit HwLinkStateToggler(
      StateUpdateFn stateUpdateFn,
      cfg::PortLoopbackMode desiredLoopbackMode = cfg::PortLoopbackMode::MAC)
      : stateUpdateFn_(stateUpdateFn),
        desiredLoopbackMode_(desiredLoopbackMode) {}
  virtual ~HwLinkStateToggler() {}

  void applyInitialConfig(
      const std::shared_ptr<SwitchState>& curState,
      const Platform* platform,
      const cfg::SwitchConfig& initCfg);
  void linkStateChanged(PortID port, bool up) noexcept;
  void bringUpPorts(
      std::shared_ptr<SwitchState> switchState,
      const std::vector<PortID>& ports) {
    portStateChangeImpl(switchState, ports, true);
  }
  void bringDownPorts(
      std::shared_ptr<SwitchState> switchState,
      const std::vector<PortID>& ports) {
    portStateChangeImpl(switchState, ports, false);
  }

 private:
  std::shared_ptr<SwitchState> applyInitialConfigWithPortsDown(
      const std::shared_ptr<SwitchState>& curState,
      const Platform* platform,
      const cfg::SwitchConfig& initCfg);
  void bringUpPorts(
      const std::shared_ptr<SwitchState>& curState,
      const cfg::SwitchConfig& initCfg);
  void portStateChangeImpl(
      std::shared_ptr<SwitchState> switchState,
      const std::vector<PortID>& ports,
      bool up);
  virtual void invokeLinkScanIfNeeded(PortID port, bool isUp) = 0;
  virtual void setPortPreemphasis(
      const std::shared_ptr<Port>& port,
      int preemphasis) = 0;

  void setPortIDAndStateToWaitFor(PortID port, bool waitForUp);
  mutable std::mutex linkEventMutex_;
  std::optional<PortID> portIdToWaitFor_;
  bool waitForPortUp_{false};
  bool desiredPortEventOccurred_{false};
  std::condition_variable linkEventCV_;

  StateUpdateFn stateUpdateFn_;
  const cfg::PortLoopbackMode desiredLoopbackMode_;
};

} // namespace facebook::fboss
