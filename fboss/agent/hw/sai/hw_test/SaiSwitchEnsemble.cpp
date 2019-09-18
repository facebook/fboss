/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/hw_test/SaiSwitchEnsemble.h"

#include "fboss/agent/hw/sai/hw_test/SaiLinkStateToggler.h"

#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"

#include "fboss/agent/HwSwitch.h"

#include <memory>

namespace facebook {
namespace fboss {

SaiSwitchEnsemble::SaiSwitchEnsemble(uint32_t featuresDesired)
    : HwSwitchEnsemble(featuresDesired) {
  // TODO pass in agent config
  auto platform = initSaiPlatform();
  auto hwSwitch = std::make_unique<SaiSwitch>(
      static_cast<SaiPlatform*>(platform.get()), featuresDesired);
  std::unique_ptr<HwLinkStateToggler> linkToggler;
  if (featuresDesired & HwSwitch::LINKSCAN_DESIRED) {
    linkToggler = std::make_unique<SaiLinkStateToggler>(
        hwSwitch.get(),
        [this](const std::shared_ptr<SwitchState>& toApply) {
          applyNewState(toApply);
        },
        cfg::PortLoopbackMode::MAC);
  }
  setupEnsemble(
      std::move(platform),
      std::move(hwSwitch),
      std::move(linkToggler),
      nullptr // Add support for accessing HW shell once we have a h/w shell for
              // SAI
  );
}

std::vector<PortID> SaiSwitchEnsemble::logicalPortIds() const {
  // TODO
  return {};
}

std::vector<PortID> SaiSwitchEnsemble::masterLogicalPortIds() const {
  return getPlatform()->masterLogicalPortIds();
}

std::vector<PortID> SaiSwitchEnsemble::getAllPortsinGroup(PortID portID) const {
  return {};
}

std::vector<FlexPortMode> SaiSwitchEnsemble::getSupportedFlexPortModes() const {
  // TODO
  return {};
}

void SaiSwitchEnsemble::dumpHwCounters() const {
  // TODO once hw shell access is supported
}

std::map<PortID, HwPortStats> SaiSwitchEnsemble::getLatestPortStats(
    const std::vector<PortID>& ports) {
  // TODO
  return {};
}

void SaiSwitchEnsemble::recreateHwSwitchFromWBState() {
  // Maybe TODO - will implement this if we decide to support in process
  // recosntruction of warm boot state from serialized switch state.
}

void SaiSwitchEnsemble::stopHwCallLogging() const {
  // TODO - if we support cint style h/w call logging
}

} // namespace fboss
} // namespace facebook
