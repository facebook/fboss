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
#include "fboss/agent/hw/test/HwTestPortUtils.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#ifndef IS_OSS
#if __has_feature(address_sanitizer)
#include <sanitizer/lsan_interface.h>
#endif
#endif

DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for SDK warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");

DEFINE_bool(
    setup_thrift_on_failure,
    false,
    "Set up thrift on demand upon encountering test failure");

DECLARE_int32(update_watermark_stats_interval_s);

namespace {

auto kStageLogPrefix = "RUNNING STAGE: ";

}

namespace facebook::fboss {

HwSwitch* HwTest::getHwSwitch() {
  return hwSwitchEnsemble_->getHwSwitch();
}

Platform* HwTest::getPlatform() {
  return hwSwitchEnsemble_->getPlatform();
}

const HwAsic* HwTest::getAsic() const {
  return getPlatform()->getAsic();
}

bool HwTest::isSupported(HwAsic::Feature feature) const {
  return getAsic()->isSupported(feature);
}

std::shared_ptr<SwitchState> HwTest::getProgrammedState() const {
  return hwSwitchEnsemble_->getProgrammedState();
}

std::vector<PortID> HwTest::masterLogicalPortIds(
    const std::set<cfg::PortType>& filter) const {
  return hwSwitchEnsemble_->masterLogicalPortIds(filter);
}

std::vector<PortID> HwTest::masterLogicalInterfacePortIds() const {
  return hwSwitchEnsemble_->masterLogicalPortIds(
      {cfg::PortType::INTERFACE_PORT});
}

std::vector<PortID> HwTest::getAllPortsInGroup(PortID portID) const {
  return hwSwitchEnsemble_->getAllPortsInGroup(portID);
}

void HwTest::SetUp() {
  // Reset any global state being tracked in singletons
  // Each test then sets up its own state as needed.
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();
  HwSwitchEnsemble::HwSwitchEnsembleInitInfo initInfo;
  initInfo.overrideTransceiverInfo = overrideTransceiverInfo();
  initInfo.dsfNodes = overrideDsfNodes();
  // Set watermark stats update interval to 0 so we always refresh BST stats
  // in each updateStats call
  FLAGS_update_watermark_stats_interval_s = 0;
  hwSwitchEnsemble_ = createHwEnsemble(featuresDesired());
  hwSwitchEnsemble_->init(initInfo);
  hwSwitchEnsemble_->addHwEventObserver(this);
  if (getHwSwitch()->getBootType() == BootType::WARM_BOOT) {
    // For warm boots, in wedge_agent at this point we would
    // apply the config. Which if nothing changed would be a noop
    // In HwTests we don't save and apply the config on warmboot
    // and since the warm boot expectation is that switch came
    // back in a identical state. Mark the switch state is as
    // CONFIGURED
    hwSwitchEnsemble_->switchRunStateChanged(SwitchRunState::CONFIGURED);
  }
}

void HwTest::TearDown() {
  tearDownSwitchEnsemble();
  /*
   * Work around to avoid long singleton cleanup
   * during atexit. TODO: figure out why extra time
   * is spent in at exit cleanups
   */
  XLOG(DBG2) << " Destroy singleton instances ";
  folly::SingletonVault::singleton()->destroyInstances();
  XLOG(DBG2) << " Done destroying singleton instances";
}

void HwTest::logStage(folly::StringPiece msg) {
  XLOG(DBG2);
  XLOG(DBG2) << kStageLogPrefix << msg;
  XLOG(DBG2);
}

void HwTest::tearDownSwitchEnsemble(bool doWarmboot) {
  if (!hwSwitchEnsemble_) {
    // hwSwitchEnsemble already torn down, nothing to do
    return;
  }
  if (::testing::Test::HasFailure()) {
    collectTestFailureInfo();
    if (FLAGS_setup_thrift_on_failure) {
      hwSwitchEnsemble_->ensureThrift();
    }
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
#ifndef IS_OSS
#if __has_feature(address_sanitizer)
    __lsan_ignore_object(leakedHwEnsemble);
#endif
#endif
  }
  hwSwitchEnsemble_.reset();
}

std::shared_ptr<SwitchState> HwTest::applyNewConfig(
    const cfg::SwitchConfig& config) {
  return hwSwitchEnsemble_->applyNewConfig(config);
}

std::shared_ptr<SwitchState> HwTest::applyNewStateImpl(
    const std::shared_ptr<SwitchState>& newState,
    bool transaction) {
  if (!newState) {
    return hwSwitchEnsemble_->getProgrammedState();
  }
  return transaction ? hwSwitchEnsemble_->applyNewStateTransaction(newState)
                     : hwSwitchEnsemble_->applyNewState(newState);
}

HwPortStats HwTest::getLatestPortStats(PortID port) {
  return getLatestPortStats(std::vector<PortID>{port})[port];
}
std::map<PortID, HwPortStats> HwTest::getLatestPortStats(
    const std::vector<PortID>& ports) {
  return hwSwitchEnsemble_->getLatestPortStats(ports);
}
HwSysPortStats HwTest::getLatestSysPortStats(SystemPortID port) {
  return getLatestSysPortStats(std::vector<SystemPortID>{port})[port];
}

std::map<SystemPortID, HwSysPortStats> HwTest::getLatestSysPortStats(
    const std::vector<SystemPortID>& ports) {
  return hwSwitchEnsemble_->getLatestSysPortStats(ports);
}

HwTrunkStats HwTest::getLatestAggregatePortStats(AggregatePortID aggPort) {
  return getLatestAggregatePortStats(
      std::vector<AggregatePortID>{aggPort})[aggPort];
}
std::map<AggregatePortID, HwTrunkStats> HwTest::getLatestAggregatePortStats(
    const std::vector<AggregatePortID>& aggPorts) {
  return hwSwitchEnsemble_->getLatestAggregatePortStats(aggPorts);
}

std::unique_ptr<HwSwitchEnsembleRouteUpdateWrapper> HwTest::getRouteUpdater() {
  return std::make_unique<HwSwitchEnsembleRouteUpdateWrapper>(
      getHwSwitchEnsemble()->getRouteUpdater());
}

std::vector<cfg::AsicType> HwTest::getOtherAsicTypes() const {
  auto asicList = HwAsic::getAllHwAsicList();
  auto myAsic = hwSwitchEnsemble_->getPlatform()->getAsic()->getAsicType();

  asicList.erase(
      std::remove_if(
          std::begin(asicList),
          std::end(asicList),
          [myAsic](auto asic) { return asic == myAsic; }),
      std::end(asicList));

  return asicList;
}
} // namespace facebook::fboss
