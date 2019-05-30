/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include "fboss/agent/state/Port.h"

extern "C" {
#include <bcm/port.h>
}

namespace facebook {
namespace fboss {

void BcmLinkStateDependentTests::linkStateChanged(
    PortID port,
    bool up) noexcept {
  {
    std::lock_guard<std::mutex> lk(linkEventMutex_);
    if (portIdToWaitFor_ == PortID(kPortIdNone) || port != portIdToWaitFor_ ||
        up != waitForPortUp_) {
      return;
    }
    desiredPortEventOccurred_ = true;
    portIdToWaitFor_ = kPortIdNone;
  }
  linkEventCV_.notify_one();
}

void BcmLinkStateDependentTests::setPortIDAndStateToWaitFor(
    PortID port,
    bool waitForPortUp) {
  std::lock_guard<std::mutex> lk(linkEventMutex_);
  portIdToWaitFor_ = port;
  waitForPortUp_ = waitForPortUp;
  desiredPortEventOccurred_ = false;
}

void BcmLinkStateDependentTests::portStateChangeImpl(
    const std::vector<PortID>& ports,
    cfg::PortState desiredAdminState,
    cfg::PortLoopbackMode desiredLoopBackMode) {
  for (auto port : ports) {
    auto newState{getProgrammedState()->clone()};
    auto newPort = newState->getPorts()->getPort(port)->modify(&newState);
    setPortIDAndStateToWaitFor(
        port, desiredAdminState == cfg::PortState::ENABLED);
    newPort->setAdminState(desiredAdminState);
    newPort->setLoopbackMode(desiredLoopBackMode);
    applyNewState(newState);
    if (!static_cast<BcmTestPlatform*>(getHwSwitch()->getPlatform())
             ->hasLinkScanCapability()) {
      opennsl_port_info_t portInfo;
      portInfo.linkstatus = desiredAdminState == cfg::PortState::ENABLED
          ? BCM_PORT_LINK_STATUS_UP
          : BCM_PORT_LINK_STATUS_DOWN;
      getHwSwitch()->linkscanCallback(getUnit(), port, &portInfo);
    }
    std::unique_lock<std::mutex> lock{linkEventMutex_};
    linkEventCV_.wait(lock, [this] { return desiredPortEventOccurred_; });
  }
}

void BcmLinkStateDependentTests::SetUp() {
  BcmTest::SetUp();
  if (getHwSwitch()->getBootType() != BootType::WARM_BOOT) {
    // No setup beyond init required for warm boot. We should
    // recover back to the state the switch went down with prior
    // to warm boot
    applyInitialConfig();
  }
}

void BcmLinkStateDependentTests::applyInitialConfig() {
  auto initCfg = initialConfig();
  auto cfg = initCfg;
  for (auto& port : cfg.ports) {
    port.state = cfg::PortState::DISABLED;
  }
  // The init sequence here is
  // i) Apply first config with ports disabled
  // ii) Synchronously bring ports up. By doing this we are guaranteed to have
  // tided over, over the first set of linkscan events that come as a result of
  // init (since there are no portup events in init + initial config
  // application). iii) Start tests.
  applyNewConfig(cfg);
  for (auto& port: initCfg.ports) {
    if (port.state == cfg::PortState::ENABLED) {
      EXPECT_NE(cfg::PortLoopbackMode::NONE, port.loopbackMode);
      bringUpPort(PortID(port.logicalID));
    }
  }
}
} // namespace fboss
} // namespace facebook
