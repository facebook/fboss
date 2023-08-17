/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/Memory.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/platforms/wedge/utils/BcmLedUtils.h"

using folly::MacAddress;
using std::string;

namespace facebook::fboss {

BcmTestPlatform::BcmTestPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping)
    : BcmPlatform(
          std::move(productInfo),
          std::move(platformMapping),
          utility::kLocalCpuMac()) {
  const auto& portsByMasterPort =
      utility::getSubsidiaryPortIDs(getPlatformPorts());
  CHECK(portsByMasterPort.size() > 1);
  for (auto itPort : portsByMasterPort) {
    masterLogicalPortIds_.push_back(itPort.first);
    logicalPortIds_.insert(
        logicalPortIds_.end(), itPort.second.begin(), itPort.second.end());
  }
}

BcmTestPlatform::~BcmTestPlatform() {}

HwSwitch* BcmTestPlatform::getHwSwitch() const {
  return bcmSwitch_.get();
}

void BcmTestPlatform::onUnitCreate(int unit) {
  warmBootHelper_ = std::make_unique<BcmWarmBootHelper>(
      unit, getDirectoryUtil()->getWarmBootDir());
  dumpHwConfig();
}

void BcmTestPlatform::onHwInitialized(HwSwitchCallback* /*sw*/) {}

void BcmTestPlatform::onInitialConfigApplied(HwSwitchCallback* /*sw*/) {}

void BcmTestPlatform::stop() {}

void BcmTestPlatform::initImpl(uint32_t hwFeaturesDesired) {
  initPorts();
  bcmSwitch_ = std::make_unique<BcmSwitch>(this, hwFeaturesDesired);
}

void BcmTestPlatform::onUnitAttach(int /*unit*/) {
  // Nothing to do
}

void BcmTestPlatform::initPorts() {
  for (int logicalPortId : logicalPortIds_) {
    PortID id(logicalPortId);
    ports_[PortID(logicalPortId)] = createTestPort(id);
  }
}

BcmTestPlatform::BcmPlatformPortMap BcmTestPlatform::getPlatformPortMap() {
  BcmPlatformPortMap ports;
  for (const auto& port : ports_) {
    ports[static_cast<bcm_port_t>(port.first)] = port.second.get();
  }
  return ports;
}

std::vector<PortID> BcmTestPlatform::getAllPortsInGroup(PortID portID) const {
  std::vector<PortID> allPortsinGroup;
  if (const auto& platformPorts = getPlatformPorts(); !platformPorts.empty()) {
    const auto& portList =
        utility::getPlatformPortsByControllingPort(platformPorts, portID);
    for (const auto& port : portList) {
      allPortsinGroup.push_back(PortID(*port.mapping()->id()));
    }
  } else {
    auto portItr = std::find(
        masterLogicalPortIds_.begin(), masterLogicalPortIds_.end(), portID);
    CHECK(portItr != masterLogicalPortIds_.end());
    for (int i = 0; i < numPortsPerTranceiver_; ++i) {
      allPortsinGroup.push_back(PortID(portID + i));
    }
  }
  return allPortsinGroup;
}

PlatformPort* BcmTestPlatform::getPlatformPort(PortID portID) const {
  auto it = ports_.find(portID);
  CHECK(it != ports_.end());
  return it->second.get();
}

void BcmTestPlatform::initLEDs(
    int unit,
    folly::ByteRange led0,
    folly::ByteRange led1) {
  BcmLedUtils::initLED0(unit, led0);
  BcmLedUtils::initLED1(unit, led1);
}

const std::optional<phy::PortProfileConfig>
BcmTestPlatform::getPortProfileConfig(
    PlatformPortProfileConfigMatcher profileMatcher) const {
  return Platform::getPortProfileConfig(profileMatcher);
}

} // namespace facebook::fboss
