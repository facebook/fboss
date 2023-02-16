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
class RoutingInformationBase;
class Platform;
class SwitchState;
class HwSwitchEnsemble;
class TestEnsembleIf;

class HwLinkStateToggler {
 public:
  explicit HwLinkStateToggler(
      TestEnsembleIf* ensemble,
      cfg::PortLoopbackMode desiredLoopbackMode = cfg::PortLoopbackMode::MAC)
      : hwEnsemble_(ensemble), desiredLoopbackMode_(desiredLoopbackMode) {}
  virtual ~HwLinkStateToggler() {}

  void applyInitialConfig(const cfg::SwitchConfig& initCfg);
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

 protected:
  TestEnsembleIf* getHwSwitchEnsemble() const {
    return hwEnsemble_;
  }

 private:
  std::shared_ptr<SwitchState> applyInitialConfigWithPortsDown(
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
  virtual void setLinkTraining(
      const std::shared_ptr<Port>& port,
      bool enable) = 0;

  void setPortIDAndStateToWaitFor(PortID port, bool waitForUp);
  mutable std::mutex linkEventMutex_;
  std::optional<PortID> portIdToWaitFor_;
  bool waitForPortUp_{false};
  bool desiredPortEventOccurred_{false};
  std::condition_variable linkEventCV_;

  TestEnsembleIf* hwEnsemble_;
  const cfg::PortLoopbackMode desiredLoopbackMode_;
};

std::unique_ptr<HwLinkStateToggler> createHwLinkStateToggler(
    TestEnsembleIf* ensemble,
    cfg::PortLoopbackMode desiredLoopbackMode);

} // namespace facebook::fboss
