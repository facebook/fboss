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

#include <folly/dynamic.h>

#include "fboss/agent/Constants.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTestStatUtils.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

DECLARE_bool(setup_for_warmboot);
DECLARE_int32(thrift_port);

namespace facebook {
namespace fboss {

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

  void packetReceived(RxPacket* /*pkt*/) noexcept override {}
  void linkStateChanged(PortID /*port*/, bool /*up*/) override {}

  HwSwitchEnsemble* getHwSwitchEnsemble() {
    return hwSwitchEnsemble_.get();
  }

  /*
   * Sync and get current stats for port(s)
   */
  HwPortStats getLatestPortStats(PortID port);
  virtual std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports) = 0;

 private:
  /*
   * createHw is a hook for each of the specific HW implementations to do the
   * necessary hw creation. Since this is likely to vary across
   * different chips, provide a hook for derived test class to hook in.
   */
  virtual std::unique_ptr<HwSwitchEnsemble> createHw() const = 0;
  /*
   * Create thrift server to handle thrift based interaction with the test
   */
  virtual std::unique_ptr<std::thread> createThriftThread() const = 0;
  virtual bool warmBootSupported() const = 0;
  /*
   * Destroy and recreate HwSwitch from just the warm boot state
   */
  virtual void recreateHwSwitchFromWBState() = 0;
  /*
   * Hook to execute any necessary HW specific behavior between test setup
   * and verify
   */
  virtual void postSetup() {}

 protected:
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
      setup();
    }
    postSetup();
    verify();
    if (getHwSwitch()->getBootType() == BootType::WARM_BOOT) {
      // If we did a warm boot, do post warmboot actions now
      setupPostWarmboot();
      verifyPostWarmboot();
    }
    if (FLAGS_setup_for_warmboot) {
      tearDownSwitchEnsemble(true);
    } else {
      if (getHwSwitch()->getBootType() != BootType::WARM_BOOT) {
        // Now do a Hw switch layer only warm boot and verify. This is
        // useful to verify integrity of reconstructing BCM layer data
        // structures using warm boot cache and pre warmboot switch state.
        // Skip is we already did a true ASIC level warm boot. Since that
        // already verified this code path
        if (warmBootSupported()) {
          flushBcmAndVerifyUsingWarmbootCache(
              verify, setupPostWarmboot, verifyPostWarmboot);
        }
      }
    }
  }

  template <typename SETUP_FN, typename VERIFY_FN>
  void verifyAcrossWarmBoots(SETUP_FN setup, VERIFY_FN verify) {
    verifyAcrossWarmBoots(setup, verify, []() {}, []() {});
  }
  std::shared_ptr<SwitchState> applyNewConfig(const cfg::SwitchConfig& config);
  std::shared_ptr<SwitchState> applyNewState(
      std::shared_ptr<SwitchState> newState);

  std::shared_ptr<SwitchState> getProgrammedState() const;

 private:
  void tearDownSwitchEnsemble(bool doWarmboot = false);
  virtual void collectTestFailureInfo() const {}

  template <
      typename VERIFY_FN,
      typename SETUP_POSTWB_FN,
      typename VERIFY_POSTWB_FN>
  void flushBcmAndVerifyUsingWarmbootCache(
      VERIFY_FN verify,
      SETUP_POSTWB_FN setupPostWarmboot,
      VERIFY_POSTWB_FN verifyPostWarmboot) {
    recreateHwSwitchFromWBState();
    verify();
    setupPostWarmboot();
    verifyPostWarmboot();
  }

  // Forbidden copy constructor and assignment operator
  HwTest(HwTest const&) = delete;
  HwTest& operator=(HwTest const&) = delete;

 private:
  std::unique_ptr<HwSwitchEnsemble> hwSwitchEnsemble_;
  std::unique_ptr<std::thread> thriftThread_;
};

} // namespace fboss
} // namespace facebook
