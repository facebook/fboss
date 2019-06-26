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
#include "fboss/agent/hw/test/HwTestStatUtils.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

DECLARE_bool(setup_for_warmboot);
DECLARE_int32(thrift_port);

namespace facebook {
namespace fboss {

class HwSwitch;
class Platform;
class SwitchState;

class HwTest : public ::testing::Test, public HwSwitch::Callback {
 public:
  HwTest() = default;
  ~HwTest() override = default;

  void SetUp() override;
  void TearDown() override;

  virtual HwSwitch* getHwSwitch() const {
    return hwSwitch_.get();
  }
  virtual Platform* getPlatform() const {
    return platform_.get();
  }

  void packetReceived(std::unique_ptr<RxPacket> pkt) noexcept override;
  void linkStateChanged(PortID port, bool up) noexcept override;
  void exitFatal() const noexcept override;

  /*
   * Sync and get current stats for port(s)
   */
  HwPortStats getLatestPortStats(PortID port);
  virtual std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports) = 0;

 private:
  /*
   * setupHw is a hook for each of the specific HW implementations to do the
   * necessary platform/switch creation. Since this is likely to vary across
   * different chips, provide a hook for derived test class to hook in.
   */
  virtual std::pair<std::unique_ptr<Platform>, std::unique_ptr<HwSwitch>>
  createHw() const = 0;
  /*
   * Create thrift server to handle thrift based interaction with the test
   */
  virtual std::unique_ptr<std::thread> createThriftThread() const = 0;
  virtual bool warmBootSupported() const = 0;
  /*
   * Destroy and recreate HwSwitch from just the warm boot state
   */
  virtual void recreateHwSwitchFromWBState() = 0;

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
    if (hwSwitch_->getBootType() != BootType::WARM_BOOT) {
      programmedState_ = setup();
    }
    verify();
    if (hwSwitch_->getBootType() == BootType::WARM_BOOT) {
      // If we did a warm boot, do post warmboot actions now
      setupPostWarmboot();
      verifyPostWarmboot();
    }
    if (FLAGS_setup_for_warmboot) {
      folly::dynamic switchState = folly::dynamic::object;
      hwSwitch_->unregisterCallbacks();
      switchState[kSwSwitch] = programmedState_->toFollyDynamic();
      hwSwitch_->gracefulExit(switchState);
    } else {
      if (hwSwitch_->getBootType() != BootType::WARM_BOOT) {
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

  std::shared_ptr<SwitchState> getProgrammedState() const {
    return programmedState_;
  }

 private:
  std::shared_ptr<SwitchState> initHwSwitch();

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
  std::unique_ptr<HwSwitch> hwSwitch_;
  std::unique_ptr<Platform> platform_;
  std::unique_ptr<std::thread> thriftThread_;
  std::shared_ptr<SwitchState> programmedState_{nullptr};
};

} // namespace fboss
} // namespace facebook
