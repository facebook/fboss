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

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AlpmUtils.h"
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

DEFINE_bool(
    setup_thrift,
    false,
    "Setup a thrift handler. Primarily useful for inspecting state setup "
    "by a test, say for debugging things via a shell");

DEFINE_int32(
    thrift_port,
    5908,
    "Port for thrift server to use (use with --setup_thrift");

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

void HwTest::SetUp() {
  // Reset any global state being tracked in singletons
  // Each test then sets up its own state as needed.
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();
  hwSwitchEnsemble_ = createHw();
  hwSwitchEnsemble_->addHwEventObserver(this);
  if (FLAGS_setup_thrift) {
    thriftThread_ = createThriftThread();
  }
}

void HwTest::TearDown() {
  if (thriftThread_) {
    thriftThread_->join();
  }
  hwSwitchEnsemble_->removeHwEventObserver(this);
  if (FLAGS_setup_for_warmboot) {
    // Leak HwSwitchEnsemble for warmboot, so that
    // we don't run destructors and unprogram h/w. We are
    // going to exit the process anyways.
    // TODO - use SdkBlackholer once we have these
    // SDK exit calls covered.
    __attribute__((unused)) auto leakedHwEnsemble = hwSwitchEnsemble_.release();
  }
  if (getHwSwitchEnsemble() != nullptr) {
    // ALPM requires that the default routes (always required to be
    // present for ALPM) be deleted last. When we destroy the HwSwitch
    // and the contained routeTable, there is no notion of a *order* of
    // destruction. So blow away all routes except the min required for ALPM
    // We are going to reset HwSwith anyways, so deleting routes does not
    // matter here.
    auto rmRoutesState{getProgrammedState()->clone()};
    auto routeTables = rmRoutesState->getRouteTables()->modify(&rmRoutesState);
    routeTables->removeRouteTable(routeTables->getRouteTable(RouterID(0)));
    applyNewState(setupAlpmState(rmRoutesState));
    hwSwitchEnsemble_.reset();
  }
}

std::shared_ptr<SwitchState> HwTest::applyNewConfig(
    const cfg::SwitchConfig& config) {
  auto newState =
      applyThriftConfig(getProgrammedState(), &config, getPlatform());
  return newState ? applyNewState(newState) : getProgrammedState();
}

std::shared_ptr<SwitchState> HwTest::applyNewState(
    std::shared_ptr<SwitchState> newState) {
  return hwSwitchEnsemble_->applyNewState(newState);
}

HwPortStats HwTest::getLatestPortStats(PortID port) {
  return getLatestPortStats(std::vector<PortID>{port})[port];
}
} // namespace fboss
} // namespace facebook
