/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwTest.h"

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

using namespace facebook::fboss;

DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for SDK warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");


namespace {

auto kStageLogPrefix = "RUNNING STAGE: ";

}

namespace facebook {
namespace fboss {

HwSwitch* HwTest::getHwSwitch() {
  return hwSwitchEnsemble_->getHwSwitch();
}

Platform* HwTest::getPlatform() {
  return hwSwitchEnsemble_->getPlatform();
}

std::shared_ptr<SwitchState> HwTest::getProgrammedState() const {
  return hwSwitchEnsemble_->getProgrammedState();
}

std::vector<PortID> HwTest::logicalPortIds() const {
  return hwSwitchEnsemble_->logicalPortIds();
}

std::vector<PortID> HwTest::masterLogicalPortIds() const {
  return hwSwitchEnsemble_->masterLogicalPortIds();
}

std::vector<PortID> HwTest::getAllPortsinGroup(PortID portID) const {
  return hwSwitchEnsemble_->getAllPortsinGroup(portID);
}

void HwTest::SetUp() {
  // Reset any global state being tracked in singletons
  // Each test then sets up its own state as needed.
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();
  hwSwitchEnsemble_ = createHwEnsemble(featuresDesired());
  hwSwitchEnsemble_->addHwEventObserver(this);
}

void HwTest::TearDown() {
  tearDownSwitchEnsemble();
}

void HwTest::logStage(folly::StringPiece msg) {
  XLOG(INFO);
  XLOG(INFO) << kStageLogPrefix << msg;
  XLOG(INFO);
}

void HwTest::tearDownSwitchEnsemble(bool doWarmboot) {
  if (!hwSwitchEnsemble_) {
    // hwSwitchEnsemble already torn down, nothing to do
    return;
  }
  if (::testing::Test::HasFailure()) {
    collectTestFailureInfo();
  }
  hwSwitchEnsemble_->removeHwEventObserver(this);
  if (doWarmboot) {
    hwSwitchEnsemble_->gracefulExit();
    // Leak HwSwitchEnsemble for warmboot, so that
    // we don't run destructors and unprogram h/w. We are
    // going to exit the process anyways.
    // TODO - use SdkBlackholer once we have these
    // SDK exit calls covered.
    __attribute__((unused)) auto leakedHwEnsemble = hwSwitchEnsemble_.release();
  }
  hwSwitchEnsemble_.reset();
}

std::shared_ptr<SwitchState> HwTest::applyNewConfig(
    const cfg::SwitchConfig& config) {
  auto newState =
      applyThriftConfig(getProgrammedState(), &config, getPlatform());
  return newState ? applyNewState(newState) : getProgrammedState();
}

std::shared_ptr<SwitchState> HwTest::applyNewState(
    std::shared_ptr<SwitchState> newState) {
  if (!newState) {
    return hwSwitchEnsemble_->getProgrammedState();
  }
  auto programmedState = hwSwitchEnsemble_->applyNewState(newState);
  // Assert that our desired state was applied exactly
  EXPECT_EQ(newState, programmedState);
  return programmedState;
}

HwPortStats HwTest::getLatestPortStats(PortID port) {
  return getLatestPortStats(std::vector<PortID>{port})[port];
}
std::map<PortID, HwPortStats> HwTest::getLatestPortStats(
    const std::vector<PortID>& ports) {
  return hwSwitchEnsemble_->getLatestPortStats(ports);
}
} // namespace fboss
} // namespace facebook
