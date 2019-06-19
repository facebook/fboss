/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/platforms/BcmTestPlatform.h"

#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/ThriftHandler.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/Memory.h>
#include <folly/logging/xlog.h>

using folly::MacAddress;
using std::string;

namespace facebook {
namespace fboss {

BcmTestPlatform::BcmTestPlatform(
    std::vector<PortID> masterLogicalPorts,
    int numPortsPerTranceiver)
    : masterLogicalPortIds_(std::move(masterLogicalPorts)) {
  numPortsPerTranceiver_ = numPortsPerTranceiver;
  for (auto masterPort : masterLogicalPortIds_) {
    for (int i = 0; i < numPortsPerTranceiver; ++i) {
      logicalPortIds_.push_back(PortID(masterPort + i));
    }
  }
}

BcmTestPlatform::~BcmTestPlatform() {}

HwSwitch* BcmTestPlatform::getHwSwitch() const {
  // We don't create a BcmSwitch, since the various tests
  // will create their own BcmSwitch objects to use for testing.
  return nullptr;
}

void BcmTestPlatform::onUnitCreate(int unit) {
  warmBootHelper_ =
      std::make_unique<DiscBackedBcmWarmBootHelper>(unit, getWarmBootDir());
  dumpHwConfig();
}

void BcmTestPlatform::onHwInitialized(SwSwitch* /*sw*/) {}

void BcmTestPlatform::onInitialConfigApplied(SwSwitch* /*sw*/) {}

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

BcmTestPlatform::InitPortMap BcmTestPlatform::initPorts() {
  InitPortMap ports;

  for (int logicalPortId : logicalPortIds_) {
    PortID id(logicalPortId);

    auto rtn = ports_.emplace(id, createTestPort(id));

    ports.emplace(logicalPortId, rtn.first->second.get());
  }
  return ports;
}

std::vector<PortID> BcmTestPlatform::getAllPortsinGroup(PortID portID) {
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
  return it == ports_.end() ? nullptr : it->second.get();
}

} // namespace fboss
} // namespace facebook
