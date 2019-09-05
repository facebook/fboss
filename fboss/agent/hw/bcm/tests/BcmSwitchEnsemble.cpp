/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"

#include <memory>
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmConfig.h"
#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/platforms/test_platforms/CreateTestPlatform.h"

namespace {
void addPort(
    facebook::fboss::BcmConfig::ConfigMap& cfg,
    int port,
    int speed,
    bool enabled = true) {
  auto key = folly::to<std::string>("portmap_", port);
  auto value = folly::to<std::string>(port, ":", speed);
  if (!enabled) {
    value = folly::to<std::string>(value, ":i");
  }
  cfg[key] = value;
}

void addFlexPort(
    facebook::fboss::BcmConfig::ConfigMap& cfg,
    int start,
    int speed) {
  addPort(cfg, start, speed);
  addPort(cfg, start + 1, speed / 4, false);
  addPort(cfg, start + 2, speed / 2, false);
  addPort(cfg, start + 3, speed / 4, false);
}
} // namespace

namespace facebook {
namespace fboss {

BcmSwitchEnsemble::BcmSwitchEnsemble(uint32_t featuresDesired)
    : HwSwitchEnsemble(featuresDesired) {
  BcmConfig::ConfigMap cfg;
  if (getPlatformMode() == PlatformMode::FAKE_WEDGE) {
    FLAGS_flexports = true;
    for (int n = 1; n <= 125; n += 4) {
      addFlexPort(cfg, n, 40);
    }
    cfg["pbmp_xport_xe"] = "0x1fffffffffffffffffffffffe";
  } else {
    // Load from a local file
    cfg = BcmConfig::loadFromFile(FLAGS_bcm_config);
  }
  BcmAPI::init(cfg);
  auto platform = createTestPlatform();
  auto hwSwitch = std::make_unique<BcmSwitch>(
      static_cast<BcmPlatform*>(platform.get()), featuresDesired);
  auto bcmTestPlatform = static_cast<BcmTestPlatform*>(platform.get());

  std::unique_ptr<HwLinkStateToggler> linkToggler;
  if (featuresDesired & HwSwitch::LINKSCAN_DESIRED) {
    linkToggler = createLinkToggler(
        hwSwitch.get(), bcmTestPlatform->desiredLoopbackMode());
  }
  setupEnsemble(
      std::move(platform), std::move(hwSwitch), std::move(linkToggler));
}

const std::vector<PortID>& BcmSwitchEnsemble::logicalPortIds() const {
  return getPlatform()->logicalPortIds();
}

const std::vector<PortID>& BcmSwitchEnsemble::masterLogicalPortIds() const {
  return getPlatform()->masterLogicalPortIds();
}

std::vector<PortID> BcmSwitchEnsemble::getAllPortsinGroup(PortID portID) const {
  return getPlatform()->getAllPortsinGroup(portID);
}

std::vector<FlexPortMode> BcmSwitchEnsemble::getSupportedFlexPortModes() const {
  return getPlatform()->getSupportedFlexPortModes();
}

} // namespace fboss
} // namespace facebook
