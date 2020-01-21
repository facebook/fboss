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

#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/hw/sai/hw_test/SaiLinkStateToggler.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

#include "fboss/agent/hw/sai/switch/SaiHandler.h"
#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/SwitchStats.h"

#include <folly/gen/Base.h>

#include <boost/container/flat_set.hpp>

#include <memory>
#include <thread>

DECLARE_int32(thrift_port);
DECLARE_bool(setup_thrift);

namespace facebook::fboss {

SaiSwitchEnsemble::SaiSwitchEnsemble(uint32_t featuresDesired)
    : HwSwitchEnsemble(featuresDesired) {
  // TODO pass in agent config
  auto platform = initSaiPlatform(nullptr, featuresDesired);
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
  std::unique_ptr<std::thread> thriftThread;
  if (FLAGS_setup_thrift) {
    thriftThread = createThriftThread(hwSwitch.get());
  }
  setupEnsemble(
      std::move(platform),
      std::move(hwSwitch),
      std::move(linkToggler),
      std::move(thriftThread));
}

std::unique_ptr<std::thread> SaiSwitchEnsemble::createThriftThread(
    const SaiSwitch* hwSwitch) {
  return std::make_unique<std::thread>([hwSwitch] {
    auto handler = std::make_shared<SaiHandler>(nullptr, hwSwitch);
    folly::EventBase eventBase;
    auto server = setupThriftServer(
        eventBase,
        handler,
        FLAGS_thrift_port,
        false /* isDuplex */,
        false /* setupSSL*/,
        true /* isStreaming */);
    // Run the EventBase main loop
    eventBase.loopForever();
  });
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
  SwitchStats dummy;
  getHwSwitch()->updateStats(&dummy);
  auto allPortStats =
      getHwSwitch()->managerTable()->portManager().getPortStats();
  boost::container::flat_set<PortID> portIds(ports.begin(), ports.end());
  return folly::gen::from(allPortStats) |
      folly::gen::filter([&portIds](const auto& portIdAndStat) {
           return portIds.find(portIdAndStat.first) != portIds.end();
         }) |
      folly::gen::as<std::map<PortID, HwPortStats>>();
}

void SaiSwitchEnsemble::stopHwCallLogging() const {
  // TODO - if we support cint style h/w call logging
}

} // namespace facebook::fboss
