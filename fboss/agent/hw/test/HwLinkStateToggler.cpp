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
    if (newState->getPorts()->getNodeIf(port)->getLoopbackMode() ==
        desiredLoopbackMode) {
      continue;
    }
    newState = newState->clone();
    auto newPort = newState->getPorts()->getNodeIf(port)->modify(&newState);
    setPortIDAndStateToWaitFor(port, up);
    newPort->setLoopbackMode(desiredLoopbackMode);
    hwEnsemble_->applyNewState(newState);
    invokeLinkScanIfNeeded(port, up);
    XLOG(DBG2) << " Wait for port " << (up ? "up" : "down")
               << " event on : " << port;
    std::unique_lock<std::mutex> lock{linkEventMutex_};
    linkEventCV_.wait(lock, [this] { return desiredPortEventOccurred_; });
    XLOG(DBG2) << " Got port " << (up ? "up" : "down")
               << " event on : " << port;

    /* toggle the oper state */
    newState = hwEnsemble_->getProgrammedState();
    newPort = newState->getPorts()->getNodeIf(port)->modify(&newState);
    newPort->setOperState(up);
    hwEnsemble_->applyNewState(newState);
  }
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
  for (auto& port : *cfg.ports()) {
    if (portId2DesiredState.find(*port.logicalID()) ==
        portId2DesiredState.end()) {
      continue;
    }
    // Set all port preemphasis values to 0 so that we can bring ports up and
    // down by setting their loopback mode to PHY and NONE respectively.
    // TODO: use sw port's pinConfigs to set this

    // Some ASICs require disabling Link Training before we can disable
    // preemphasis.
    if (hwEnsemble_->getHwSwitch()->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::LINK_TRAINING)) {
      setLinkTraining(
          hwEnsemble_->getProgrammedState()->getPorts()->getNodeIf(
              PortID(*port.logicalID())),
          false /* disable link training */);
    }

    setPortPreemphasis(
        hwEnsemble_->getProgrammedState()->getPorts()->getNodeIf(
            PortID(*port.logicalID())),
        0);
    *port.state() = portId2DesiredState[*port.logicalID()];
  }
  hwEnsemble_->applyNewConfig(cfg);
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
