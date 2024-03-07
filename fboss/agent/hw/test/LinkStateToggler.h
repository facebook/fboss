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

DECLARE_bool(multi_switch);

namespace facebook::fboss {
class RoutingInformationBase;
class Platform;
class SwitchState;
class HwSwitchEnsemble;
class TestEnsembleIf;

class LinkStateToggler {
 public:
  explicit LinkStateToggler(TestEnsembleIf* ensemble) : ensemble_(ensemble) {}
  virtual ~LinkStateToggler() {}

  void applyInitialConfig(const cfg::SwitchConfig& initCfg);
  void linkStateChanged(PortID port, bool up) noexcept;
  void bringUpPorts(const std::vector<PortID>& ports) {
    portStateChangeImpl(ports, true);
  }
  void bringDownPorts(const std::vector<PortID>& ports) {
    portStateChangeImpl(ports, false);
  }

 protected:
  TestEnsembleIf* getHwSwitchEnsemble() const {
    return ensemble_;
  }

 private:
  std::shared_ptr<SwitchState> applyInitialConfigWithPortsDown(
      const cfg::SwitchConfig& initCfg);
  void bringUpPorts(const cfg::SwitchConfig& initCfg);
  void portStateChangeImpl(const std::vector<PortID>& ports, bool up);
  virtual void invokeLinkScanIfNeeded(PortID port, bool isUp);
  void waitForPortDown(PortID port);
  void setPortIDsAndStateToWaitFor(
      const std::set<PortID>& ports,
      bool waitForUp);
  bool waitForPortEvents(const std::set<PortID>& ports, bool up);
  cfg::PortLoopbackMode findDesiredLoopbackMode(
      const std::shared_ptr<SwitchState>& newState,
      PortID port,
      bool up) const;

  mutable std::mutex linkEventMutex_;
  std::set<PortID> portIdToWaitFor_;
  bool waitForPortUp_{false};
  bool desiredPortEventsOccurred_{true};
  std::condition_variable linkEventCV_;

  TestEnsembleIf* ensemble_;
};

} // namespace facebook::fboss
