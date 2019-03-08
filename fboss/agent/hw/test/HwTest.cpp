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

#include <folly/logging/xlog.h>
#include <folly/Singleton.h>


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

namespace facebook { namespace fboss {

void HwTest::SetUp() {
  // Reset any global state being tracked in singletons
  // Each test then sets up its own state as needed.
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();
  std::tie(platform_, hwSwitch_) = createHw();
  initHwSwitch();
  if (FLAGS_setup_thrift) {
    thriftThread_ = createThriftThread();
  }
}

void HwTest::TearDown() {
  if (thriftThread_) {
    thriftThread_->join();
  }
  if (FLAGS_setup_for_warmboot) {
    // Leak platform, unit, HwSwitch for warmboot, so that
    // we don't run destructors and unprogram h/w. We are
    // going to exit the process anyways.
    // TODO - use SdkBlackholer once we have these
    // SDK exit calls covered.
    __attribute__((unused)) auto leakedHw = hwSwitch_.release();
    __attribute__((unused)) auto leakedPlatform = platform_.release();
  }
  if (hwSwitch_ != nullptr) {
    // ALPM requires that the default routes (always required to be
    // present for ALPM) be deleted last. When we destroy the HwSwitch
    // and the contained routeTable, there is no notion of a *order* of
    // destruction. So blow away all routes except the min required for ALPM
    // We are going to reset HwSwith anyways, so deleting routes does not
    // matter here.
    auto rmRoutesState{programmedState_->clone()};
    auto routeTables = rmRoutesState->getRouteTables()->modify(&rmRoutesState);
    routeTables->removeRouteTable(routeTables->getRouteTable(RouterID(0)));
    applyNewState(setupAlpmState(rmRoutesState));
    hwSwitch_.reset();
    platform_.reset();
  }
}

std::shared_ptr<SwitchState> HwTest::initHwSwitch() {
  programmedState_ = hwSwitch_->init(this).switchState;
  // HwSwitch::init() returns an unpublished programmedState_.  SwSwitch is
  // normally responsible for publishing it.  Go ahead and call publish now.
  // This will catch errors if test cases accidentally try to modify this
  // programmedState_ without first cloning it.
  programmedState_->publish();
  // Handle ALPM state. ALPM requires that default routes be programmed
  // before any other routes. We handle that setup here. Similarly ALPM
  // requires that default routes be deleted last. That aspect is handled
  // in TearDown
  auto alpmState = setupAlpmState(programmedState_);
  if (alpmState) {
    applyNewState(alpmState);
  }
  hwSwitch_->initialConfigApplied();
  return programmedState_;
}


void HwTest::packetReceived(std::unique_ptr<RxPacket> /*pkt*/) noexcept {
  // We simply ignore trapped packets for now
}

void HwTest::linkStateChanged(PortID /*port*/, bool /*up*/) noexcept {
  // We ignore link state changes for now
}

void HwTest::exitFatal() const noexcept {
  // No special behavior on a fatal from HwTest
}


std::shared_ptr<SwitchState> HwTest::applyNewConfig(
    const cfg::SwitchConfig& config) {

  auto newState = applyThriftConfig(programmedState_, &config, getPlatform());
  CHECK(newState);
  // Call stateChanged()
  newState->publish();
  StateDelta delta(programmedState_, newState);
  getHwSwitch()->stateChanged(delta);
  programmedState_ = newState;
  return newState;
}

std::shared_ptr<SwitchState> HwTest::applyNewState(
    std::shared_ptr<SwitchState> newState) {
  // Call stateChanged()
  newState->publish();
  StateDelta delta(programmedState_, newState);
  getHwSwitch()->stateChanged(delta);
  programmedState_ = newState;
  return newState;
}
}} // facebook::fboss
