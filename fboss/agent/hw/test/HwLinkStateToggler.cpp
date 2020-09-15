/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwLinkStateToggler.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Port.h"

#include <boost/container/flat_map.hpp>
#include <folly/gen/Base.h>

namespace facebook::fboss {

void HwLinkStateToggler::linkStateChanged(PortID port, bool up) noexcept {
  {
    std::lock_guard<std::mutex> lk(linkEventMutex_);
    if (!portIdToWaitFor_ || port != portIdToWaitFor_ || up != waitForPortUp_) {
      return;
    }
    desiredPortEventOccurred_ = true;
    portIdToWaitFor_ = std::nullopt;
  }
  linkEventCV_.notify_one();
}

void HwLinkStateToggler::setPortIDAndStateToWaitFor(
    PortID port,
    bool waitForPortUp) {
  std::lock_guard<std::mutex> lk(linkEventMutex_);
  portIdToWaitFor_ = port;
  waitForPortUp_ = waitForPortUp;
  desiredPortEventOccurred_ = false;
}

void HwLinkStateToggler::portStateChangeImpl(
    std::shared_ptr<SwitchState> switchState,
    const std::vector<PortID>& ports,
    bool up) {
  auto newState = switchState;
  auto desiredLoopbackMode =
      up ? desiredLoopbackMode_ : cfg::PortLoopbackMode::NONE;
  for (auto port : ports) {
    if (newState->getPorts()->getPort(port)->getLoopbackMode() ==
        desiredLoopbackMode) {
      continue;
    }
    newState = newState->clone();
    auto newPort = newState->getPorts()->getPort(port)->modify(&newState);
    setPortIDAndStateToWaitFor(port, up);
    newPort->setLoopbackMode(desiredLoopbackMode);
    stateUpdateFn_(newState);
    invokeLinkScanIfNeeded(port, up);
    std::unique_lock<std::mutex> lock{linkEventMutex_};
    linkEventCV_.wait(lock, [this] { return desiredPortEventOccurred_; });

    /* toggle the oper state */
    newState = newState->clone();
    newPort = newState->getPorts()->getPort(port)->modify(&newState);
    newPort->setOperState(up);
    stateUpdateFn_(newState);
  }
}

void HwLinkStateToggler::applyInitialConfig(
    const std::shared_ptr<SwitchState>& curState,
    const Platform* platform,
    const cfg::SwitchConfig& initCfg) {
  auto newState = applyInitialConfigWithPortsDown(curState, platform, initCfg);
  bringUpPorts(newState, initCfg);
}

std::shared_ptr<SwitchState>
HwLinkStateToggler::applyInitialConfigWithPortsDown(
    const std::shared_ptr<SwitchState>& curState,
    const Platform* platform,
    const cfg::SwitchConfig& initCfg) {
  // Goal of this function is twofold
  // - Apply initial config.
  // - Set preemphasis on all ports to 0
  // We do this in 2 steps
  // i)  Apply initial config, but with ports disabled. This is done to cater
  // for platforms where ports only show up after first config application
  // ii) Set preempahsis for all ports to 0
  // iii) Apply initial config with the correct port state.
  // Coupled with preempahsis set to 0 and lbmode=NONE, this will keep ports
  // down. We will then set the enabled ports to desired loopback mode in
  // bringUpPorts API
  auto cfg = initCfg;
  boost::container::flat_map<int, cfg::PortState> portId2DesiredState;
  for (auto& port : *cfg.ports_ref()) {
    portId2DesiredState[*port.logicalID_ref()] = *port.state_ref();
    // Keep ports down by disabling them and setting loopback mode to NONE
    *port.state_ref() = cfg::PortState::DISABLED;
    *port.loopbackMode_ref() = cfg::PortLoopbackMode::NONE;
  }
  // i) Set preempahsis to 0, so ports state can be manipulated by just setting
  // loopback mode (lopbackMode::NONE == down), loopbackMode::{MAC, PHY} == up)
  // ii) Apply first config with all ports set to loopback mode as NONE
  // iii) Synchronously bring ports up. By doing this we are guaranteed to have
  // tided over, over the first set of linkscan events that come as a result of
  // init (since there are no portup events in init + initial config
  // application). iii) Start tests.
  auto newState = applyThriftConfig(curState, &cfg, platform);
  stateUpdateFn_(newState);
  for (auto& port : *cfg.ports_ref()) {
    // Set all port preemphasis values to 0 so that we can bring ports up and
    // down by setting their loopback mode to PHY and NONE respectively.
    setPortPreemphasis(
        newState->getPorts()->getPort(PortID(*port.logicalID_ref())), 0);
    *port.state_ref() = portId2DesiredState[*port.logicalID_ref()];
  }
  newState = applyThriftConfig(newState, &cfg, platform);
  stateUpdateFn_(newState);
  platform->getHwSwitch()->switchRunStateChanged(SwitchRunState::CONFIGURED);
  return newState;
}

void HwLinkStateToggler::bringUpPorts(
    const std::shared_ptr<SwitchState>& newState,
    const cfg::SwitchConfig& initCfg) {
  std::vector<PortID> portsToBringUp;
  folly::gen::from(*initCfg.ports_ref()) |
      folly::gen::filter([](const auto& port) {
        return *port.state_ref() == cfg::PortState::ENABLED;
      }) |
      folly::gen::map(
          [](const auto& port) { return PortID(*port.logicalID_ref()); }) |
      folly::gen::appendTo(portsToBringUp);
  bringUpPorts(newState, portsToBringUp);
}

} // namespace facebook::fboss
