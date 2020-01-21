/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/test_platforms/BcmTestPlatform.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"

#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/Memory.h>
#include <folly/logging/xlog.h>

using folly::MacAddress;
using std::string;

namespace facebook::fboss {

BcmTestPlatform::BcmTestPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::vector<PortID> masterLogicalPorts,
    int numPortsPerTranceiver)
    : BcmPlatform(std::move(productInfo)),
      masterLogicalPortIds_(std::move(masterLogicalPorts)) {
  numPortsPerTranceiver_ = numPortsPerTranceiver;
  for (auto masterPort : masterLogicalPortIds_) {
    for (int i = 0; i < numPortsPerTranceiver; ++i) {
      logicalPortIds_.push_back(PortID(masterPort + i));
    }
  }
}

BcmTestPlatform::~BcmTestPlatform() {}

HwSwitch* BcmTestPlatform::getHwSwitch() const {
  return bcmSwitch_.get();
}

void BcmTestPlatform::onUnitCreate(int unit) {
  warmBootHelper_ =
      std::make_unique<DiscBackedBcmWarmBootHelper>(unit, getWarmBootDir());
  dumpHwConfig();
}

void BcmTestPlatform::onHwInitialized(SwSwitch* /*sw*/) {}

void BcmTestPlatform::onInitialConfigApplied(SwSwitch* /*sw*/) {}

void BcmTestPlatform::stop() {}

void BcmTestPlatform::initImpl(uint32_t hwFeaturesDesired) {
  initPorts();
  bcmSwitch_ = std::make_unique<BcmSwitch>(this, hwFeaturesDesired);
}

std::unique_ptr<ThriftHandler> BcmTestPlatform::createHandler(
    SwSwitch* /*sw*/) {
  XLOG(FATAL) << "unexpected call to BcmTestPlatform::createHandler()";
  return nullptr;
}

MacAddress BcmTestPlatform::getLocalMac() const {
  return utility::kLocalCpuMac();
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

std::vector<PortID> BcmTestPlatform::getAllPortsinGroup(PortID portID) const {
  std::vector<PortID> allPortsinGroup;
  auto portItr = std::find(
      masterLogicalPortIds_.begin(), masterLogicalPortIds_.end(), portID);
  CHECK(portItr != masterLogicalPortIds_.end());
  for (int i = 0; i < numPortsPerTranceiver_; ++i) {
    allPortsinGroup.push_back(PortID(portID + i));
  }
  return allPortsinGroup;
}

PlatformPort* BcmTestPlatform::getPlatformPort(PortID portID) const {
  auto it = ports_.find(portID);
  CHECK(it != ports_.end());
  return it->second.get();
}

} // namespace facebook::fboss
