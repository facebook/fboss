/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <utility>

#include <folly/Range.h>
#include <folly/dynamic.h>

#include "fboss/agent/Constants.h"
#include "fboss/agent/GtestDefs.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/hw/test/HwTestConstants.h"
#include "fboss/agent/hw/test/HwTestStatUtils.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

DECLARE_bool(setup_for_warmboot);
DECLARE_int32(thrift_port);

namespace facebook::fboss {

class HwSwitch;
class HwSwitchEnsemble;
class SwitchState;
class Platform;

class HwTest : public ::testing::Test,
               public HwSwitchEnsemble::HwSwitchEventObserverIf {
 public:
  HwTest() = default;
  ~HwTest() override = default;

  void SetUp() override;
  void TearDown() override;

  virtual HwSwitch* getHwSwitch();
  virtual Platform* getPlatform();
  virtual const HwSwitch* getHwSwitch() const {
    return const_cast<HwTest*>(this)->getHwSwitch();
  }
  virtual const Platform* getPlatform() const {
    return const_cast<HwTest*>(this)->getPlatform();
  }
  const HwAsic* getAsic() const;
  bool isSupported(HwAsic::Feature feature) const;

  void packetReceived(RxPacket* /*pkt*/) noexcept override {}
  void linkStateChanged(PortID /*port*/, bool /*up*/) override {}
  void l2LearningUpdateReceived(
      L2Entry /*l2Entry*/,
      L2EntryUpdateType /*l2EntryUpdateType*/) override {}

  HwSwitchEnsemble* getHwSwitchEnsemble() {
    return hwSwitchEnsemble_.get();
  }
  const HwSwitchEnsemble* getHwSwitchEnsemble() const {
    return hwSwitchEnsemble_.get();
  }

  /*
   * Sync and get current stats for port(s)
   */
  HwPortStats getLatestPortStats(PortID port);
  std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports);
  HwSysPortStats getLatestSysPortStats(SystemPortID port);
  std::map<SystemPortID, HwSysPortStats> getLatestSysPortStats(
      const std::vector<SystemPortID>& ports);

  HwTrunkStats getLatestAggregatePortStats(AggregatePortID aggPort);
  std::map<AggregatePortID, HwTrunkStats> getLatestAggregatePortStats(
      const std::vector<AggregatePortID>& aggPorts);

  std::vector<PortID> masterLogicalPortIds(
      const std::set<cfg::PortType>& filter = {}) const;

  std::vector<PortID> masterLogicalInterfacePortIds() const;

  std::vector<PortID> getAllPortsInGroup(PortID portID) const;

 protected:
  /*
   * Most tests don't want packet RX or link scan enabled while running. Other
   * tests might (e.g. trunk tests watching for links going down). Provide
   * a default setting for features desired and let individual tests override
   * it.
   */
  virtual HwSwitchEnsemble::Features featuresDesired() const {
    return {};
  }

  std::unique_ptr<HwSwitchEnsembleRouteUpdateWrapper> getRouteUpdater();

  template <
      typename SETUP_FN,
      typename VERIFY_FN,
      typename SETUP_POSTWB_FN,
      typename VERIFY_POSTWB_FN>
  void verifyAcrossWarmBoots(
      SETUP_FN setup,
      VERIFY_FN verify,
      SETUP_POSTWB_FN setupPostWarmboot,
      VERIFY_POSTWB_FN verifyPostWarmboot) {
    if (getHwSwitch()->getBootType() != BootType::WARM_BOOT) {
      logStage("cold boot setup()");
      setup();
    }

    logStage("verify()");
    verify();

    if (getHwSwitch()->getBootType() == BootType::WARM_BOOT) {
      // If we did a warm boot, do post warmboot actions now
      logStage("setupPostWarmboot()");
      setupPostWarmboot();

      logStage("verifyPostWarmboot()");
      verifyPostWarmboot();
    }
    if (FLAGS_setup_for_warmboot && isSupported(HwAsic::Feature::WARMBOOT)) {
      logStage("tearDownSwitchEnsemble() for warmboot");
      tearDownSwitchEnsemble(true);
    }
  }

  template <typename SETUP_FN, typename VERIFY_FN>
  void verifyAcrossWarmBoots(SETUP_FN setup, VERIFY_FN verify) {
    verifyAcrossWarmBoots(
        setup, verify, []() {}, []() {});
  }
  std::shared_ptr<SwitchState> applyNewConfig(const cfg::SwitchConfig& config);
  std::shared_ptr<SwitchState> applyNewState(
      std::shared_ptr<SwitchState> newState) {
    return applyNewStateImpl(newState, false);
  }
  std::shared_ptr<SwitchState> applyNewStateTransaction(
      std::shared_ptr<SwitchState> newState) {
    return applyNewStateImpl(newState, true);
  }
  std::shared_ptr<SwitchState> getProgrammedState() const;

  template <typename EcmpHelperT>
  void resolveNeigborAndProgramRoutes(const EcmpHelperT& ecmp, int width) {
    applyNewState(ecmp.resolveNextHops(getProgrammedState(), width));
    ecmp.programRoutes(getRouteUpdater(), width);
  }

  std::vector<cfg::AsicType> getOtherAsicTypes() const;

 private:
  virtual std::optional<TransceiverInfo> overrideTransceiverInfo() const {
    return std::nullopt;
  }
  virtual std::optional<std::map<int64_t, cfg::DsfNode>> overrideDsfNodes(
      const std::map<int64_t, cfg::DsfNode>& /*curDsfNodes*/) const {
    return std::nullopt;
  }

  std::shared_ptr<SwitchState> applyNewStateImpl(
      const std::shared_ptr<SwitchState>& newState,
      bool transaction);
  void tearDownSwitchEnsemble(bool doWarmboot = false);
  void collectTestFailureInfo() const {
    hwSwitchEnsemble_->dumpHwCounters();
  }

  void logStage(folly::StringPiece msg);

  // Forbidden copy constructor and assignment operator
  HwTest(HwTest const&) = delete;
  HwTest& operator=(HwTest const&) = delete;

  std::unique_ptr<HwSwitchEnsemble> hwSwitchEnsemble_;
};

} // namespace facebook::fboss
