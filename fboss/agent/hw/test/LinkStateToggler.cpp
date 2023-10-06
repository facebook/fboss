/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/LinkStateToggler.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/lib/CommonUtils.h"

#include <boost/container/flat_map.hpp>
#include <folly/gen/Base.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

void LinkStateToggler::linkStateChanged(PortID port, bool up) noexcept {
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

void LinkStateToggler::setPortIDAndStateToWaitFor(
    PortID port,
    bool waitForPortUp) {
  std::lock_guard<std::mutex> lk(linkEventMutex_);
  portIdToWaitFor_ = port;
  waitForPortUp_ = waitForPortUp;
  desiredPortEventOccurred_ = false;
}

void LinkStateToggler::portStateChangeImpl(
    const std::vector<PortID>& ports,
    bool up) {
  for (auto port : ports) {
    auto newState = ensemble_->getProgrammedState();
    auto currPort = newState->getPorts()->getNodeIf(port);
    auto switchId = ensemble_->scopeResolver().scope(currPort).switchId();
    auto asic = ensemble_->getHwAsicTable()->getHwAsic(switchId);
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
    ensemble_->applyNewState(newState);
    invokeLinkScanIfNeeded(port, up);
    XLOG(DBG2) << " Wait for port " << (up ? "up" : "down")
               << " event on : " << port;
    waitForPortEvent(port);
    XLOG(DBG2) << " Got port " << (up ? "up" : "down")
               << " event on : " << port;
  }
}

bool LinkStateToggler::waitForPortEvent(PortID port) {
  if (!FLAGS_multi_switch) {
    std::unique_lock<std::mutex> lock{linkEventMutex_};
    linkEventCV_.wait(lock, [this] { return desiredPortEventOccurred_; });
    /* toggle the oper state */
    auto newState = ensemble_->getProgrammedState();
    auto newPort = newState->getPorts()->getNodeIf(port)->modify(&newState);
    newPort->setOperState(waitForPortUp_);
    ensemble_->applyNewState(newState);
  }
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        ensemble_->getProgrammedState()
            ->getPorts()
            ->getNodeIf(port)
            ->getOperState(),
        (waitForPortUp_ ? PortFields::OperState::UP
                        : PortFields::OperState::DOWN));
  });
  return true;
}

void LinkStateToggler::waitForPortDown(PortID port) {
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        ensemble_->getProgrammedState()
            ->getPorts()
            ->getNodeIf(port)
            ->getOperState(),
        PortFields::OperState::DOWN);
  });
}

void LinkStateToggler::applyInitialConfig(const cfg::SwitchConfig& initCfg) {
  applyInitialConfigWithPortsDown(initCfg);
  bringUpPorts(initCfg);
}

std::shared_ptr<SwitchState> LinkStateToggler::applyInitialConfigWithPortsDown(
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
  ensemble_->applyNewConfig(cfg);

  // Wait for port state to be disabled in switch state
  for (auto& port : *cfg.ports()) {
    if (port.portType() == cfg::PortType::RECYCLE_PORT) {
      continue;
    }
    waitForPortDown(PortID(*port.logicalID()));
  }

  auto switchState = ensemble_->getProgrammedState();
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
  ensemble_->applyNewState(switchState);
  ensemble_->applyNewConfig(cfg);

  // Some platforms silently undo squelch setting on admin enable. Prevent it
  // by setting squelch after admin enable.
  if (ensemble_->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::RX_LANE_SQUELCH_ENABLE)) {
    switchState = ensemble_->getProgrammedState();
    for (auto& cfgPort : *cfg.ports()) {
      auto port = switchState->getPorts()
                      ->getNodeIf(PortID(*cfgPort.logicalID()))
                      ->modify(&switchState);
      port->setRxLaneSquelch(true);
    }
    ensemble_->applyNewState(switchState);
  }
  ensemble_->switchRunStateChanged(SwitchRunState::CONFIGURED);
  return ensemble_->getProgrammedState();
}

void LinkStateToggler::bringUpPorts(const cfg::SwitchConfig& initCfg) {
  std::vector<PortID> portsToBringUp;
  folly::gen::from(*initCfg.ports()) | folly::gen::filter([](const auto& port) {
    return *port.state() == cfg::PortState::ENABLED &&
        *port.portType() != cfg::PortType::RECYCLE_PORT;
  }) | folly::gen::map([](const auto& port) {
    return PortID(*port.logicalID());
  }) | folly::gen::appendTo(portsToBringUp);
  bringUpPorts(portsToBringUp);
}

void LinkStateToggler::invokeLinkScanIfNeeded(
    PortID /* port */,
    bool /* isUp */) {
  // For fake bcm tests, there's no link event callbacks from SDK.
  // Hence link scan is needed to mimic SDK behavior for port UP/DOWn
  // processing. For all other platforms that support linkscan, skip this step.
}

} // namespace facebook::fboss
