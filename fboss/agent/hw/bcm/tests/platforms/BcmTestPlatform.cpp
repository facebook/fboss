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

#include <folly/Memory.h>
#include <folly/logging/xlog.h>

using folly::MacAddress;
using std::make_unique;
using std::string;

namespace facebook {
namespace fboss {

MacAddress BcmTestPlatform::kLocalCpuMac() {
  static const MacAddress kLocalMac("02:00:00:00:00:01");
  return kLocalMac;
}

BcmTestPlatform::BcmTestPlatform(
    std::vector<int> masterLogicalPorts,
    int numPortsPerTranceiver)
    : masterLogicalPortIds_(std::move(masterLogicalPorts)) {
  numPortsPerTranceiver_ = numPortsPerTranceiver;
  for (int masterPort : masterLogicalPortIds_) {
    for (int i = 0; i < numPortsPerTranceiver; ++i) {
      logicalPortIds_.push_back(masterPort + i);
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
}

void BcmTestPlatform::onHwInitialized(SwSwitch* /*sw*/) {}

void BcmTestPlatform::onInitialConfigApplied(SwSwitch* /*sw*/) {}

std::unique_ptr<ThriftHandler> BcmTestPlatform::createHandler(
    SwSwitch* /*sw*/) {
  XLOG(FATAL) << "unexpected call to BcmTestPlatform::createHandler()";
  return nullptr;
}

MacAddress BcmTestPlatform::getLocalMac() const {
  return kLocalCpuMac();
}

void BcmTestPlatform::onUnitAttach(int /*unit*/) {
  // Nothing to do
}

BcmTestPlatform::InitPortMap BcmTestPlatform::initPorts() {
  InitPortMap ports;

  for (int logicalPortId : logicalPortIds_) {
    ports[logicalPortId] = createPort(logicalPortId);
  }
  return ports;
}

BcmTestPort* BcmTestPlatform::createPort(int number) {
  PortID id(number);
  ports_[id] = getPlatformPort(id);
  return ports_[id].get();
}

std::vector<int> BcmTestPlatform::getAllPortsinGroup(int portID) {
  std::vector<int> allPortsinGroup;
  auto portItr = std::find(
      masterLogicalPortIds_.begin(), masterLogicalPortIds_.end(), portID);
  CHECK(portItr != masterLogicalPortIds_.end());
  for (int i = 0; i < numPortsPerTranceiver_; ++i) {
    allPortsinGroup.push_back(portID + i);
  }
  return allPortsinGroup;
}

} // namespace fboss
} // namespace facebook
