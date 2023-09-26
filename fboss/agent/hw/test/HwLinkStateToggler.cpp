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
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/lib/CommonUtils.h"

#include <boost/container/flat_map.hpp>
#include <folly/gen/Base.h>
#include <gtest/gtest.h>

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
  for (auto port : ports) {
    auto currPort = newState->getPorts()->getNodeIf(port);
    auto switchId = hwEnsemble_->scopeResolver().scope(currPort).switchId();
    auto asic = hwEnsemble_->getHwAsicTable()->getHwAsic(switchId);
    auto desiredLoopbackMode = up
        ? asic->getDesiredLoopbackMode(currPort->getPortType())
        : cfg::PortLoopbackMode::NONE;
    if (currPort->getLoopbackMode() == desiredLoopbackMode) {
      continue;
    }
    newState = newState->clone();
    auto newPort = currPort->modify(&newState);
    setPortIDAndStateToWaitFor(port, up);
    newPort->setLoopbackMode(desiredLoopbackMode);
    hwEnsemble_->applyNewState(newState);
    invokeLinkScanIfNeeded(port, up);
    XLOG(DBG2) << " Wait for port " << (up ? "up" : "down")
               << " event on : " << port;
    waitForPortEvent();
    XLOG(DBG2) << " Got port " << (up ? "up" : "down")
               << " event on : " << port;

    /* toggle the oper state */
    newState = hwEnsemble_->getProgrammedState();
    newPort = newState->getPorts()->getNodeIf(port)->modify(&newState);
    newPort->setOperState(up);
    hwEnsemble_->applyNewState(newState);
  }
}

bool HwLinkStateToggler::waitForPortEvent() {
  std::unique_lock<std::mutex> lock{linkEventMutex_};
  linkEventCV_.wait(lock, [this] { return desiredPortEventOccurred_; });
  return desiredPortEventOccurred_;
}

void HwLinkStateToggler::waitForPortDown(PortID port) {
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        hwEnsemble_->getProgrammedState()
            ->getPorts()
            ->getNodeIf(port)
            ->getOperState(),
        PortFields::OperState::DOWN);
  });
}

void HwLinkStateToggler::applyInitialConfig(const cfg::SwitchConfig& initCfg) {
  auto newState = applyInitialConfigWithPortsDown(initCfg);
  bringUpPorts(newState, initCfg);
}

std::shared_ptr<SwitchState>
HwLinkStateToggler::applyInitialConfigWithPortsDown(
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
  for (auto& port : *cfg.ports()) {
    if (port.portType() == cfg::PortType::RECYCLE_PORT) {
      continue;
    }
    portId2DesiredState[*port.logicalID()] = *port.state();
    // Keep ports down by disabling them and setting loopback mode to NONE
    *port.state() = cfg::PortState::DISABLED;
    *port.loopbackMode() = cfg::PortLoopbackMode::NONE;
  }
  // i) Set preempahsis to 0, so ports state can be manipulated by just setting
  // loopback mode (lopbackMode::NONE == down), loopbackMode::{MAC, PHY} == up)
  // ii) Apply first config with all ports set to loopback mode as NONE
  // iii) Synchronously bring ports up. By doing this we are guaranteed to have
  // tided over, over the first set of linkscan events that come as a result of
  // init (since there are no portup events in init + initial config
  // application). iii) Start tests.
  hwEnsemble_->applyNewConfig(cfg);

  // Wait for oper state to be down in switch state.
  // For tests without SwSwitch (e.g. all HwTests), this should not be an
  // concern as initial config is not yet applied. However, for
  // SwSwitch/AgentTests, initial config has already applied, meaning swSwitch
  // could be receiving linkState callbacks and update oper state to be UP.
  // Therefore, wait for all port's oper state to be DOWN before reading the
  // programmed state.
  for (auto& port : *cfg.ports()) {
    if (port.portType() == cfg::PortType::RECYCLE_PORT) {
      continue;
    }
    waitForPortDown(PortID(*port.logicalID()));
  }

  auto switchState = hwEnsemble_->getProgrammedState();
  for (auto& cfgPort : *cfg.ports()) {
    if (portId2DesiredState.find(*cfgPort.logicalID()) ==
        portId2DesiredState.end()) {
      continue;
    }
    // Set all port preemphasis values to 0 so that we can bring ports up and
    // down by setting their loopback mode to PHY and NONE respectively.
    // TODO: use sw port's pinConfigs to set this
    auto port = switchState->getPorts()
                    ->getNodeIf(PortID(*cfgPort.logicalID()))
                    ->modify(&switchState);
    port->setZeroPreemphasis(true);
    *cfgPort.state() = portId2DesiredState[*cfgPort.logicalID()];
  }
  // Update txSetting first and then enable admin state
  hwEnsemble_->applyNewState(switchState);
  hwEnsemble_->applyNewConfig(cfg);

  // Some platforms silently undo squelch setting on admin enable. Prevent it
  // by setting squelch after admin enable.
  if (hwEnsemble_->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::RX_LANE_SQUELCH_ENABLE)) {
    switchState = hwEnsemble_->getProgrammedState();
    for (auto& cfgPort : *cfg.ports()) {
      auto port = switchState->getPorts()
                      ->getNodeIf(PortID(*cfgPort.logicalID()))
                      ->modify(&switchState);
      port->setRxLaneSquelch(true);
    }
    hwEnsemble_->applyNewState(switchState);
  }

  hwEnsemble_->getHwSwitch()->switchRunStateChanged(SwitchRunState::CONFIGURED);
  return hwEnsemble_->getProgrammedState();
}

void HwLinkStateToggler::bringUpPorts(
    const std::shared_ptr<SwitchState>& newState,
    const cfg::SwitchConfig& initCfg) {
  std::vector<PortID> portsToBringUp;
  folly::gen::from(*initCfg.ports()) | folly::gen::filter([](const auto& port) {
    return *port.state() == cfg::PortState::ENABLED &&
        *port.portType() != cfg::PortType::RECYCLE_PORT;
  }) | folly::gen::map([](const auto& port) {
    return PortID(*port.logicalID());
  }) | folly::gen::appendTo(portsToBringUp);
  bringUpPorts(newState, portsToBringUp);
}

} // namespace facebook::fboss
