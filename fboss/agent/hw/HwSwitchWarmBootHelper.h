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

#include <folly/dynamic.h>
#include <string>
#include "fboss/agent/gen-cpp2/switch_state_types.h"

#include "fboss/agent/SwSwitchWarmBootHelper.h"

namespace facebook::fboss {

/*
 * This class encapsulates much of the warm boot functionality for an individual
 * HwSwitch. It will store all the files necessary to perform warm boot on a
 * given unit in the passed in warmBootDir. If no directory name is provided,
 * this class will default to a cold boot and perform none of the setup for
 * performing a warm boot on the next startup.
 */
class HwSwitchWarmBootHelper {
 public:
  HwSwitchWarmBootHelper(
      int switchId,
      const std::string& warmBootDir,
      const std::string& sdkWarmbootFilePrefix);
  ~HwSwitchWarmBootHelper();

  bool canWarmBoot() const {
    return canWarmBoot_;
  }

  void storeHwSwitchWarmBootState(const folly::dynamic& switchState);

  std::tuple<folly::dynamic, std::optional<state::WarmbootState>>
  getWarmBootState() const;

  state::WarmbootState getSwSwitchWarmBootState() const;
  folly::dynamic getHwSwitchWarmBootState() const;

  // bcm switch specific
  std::string startupSdkDumpFile() const;
  // bcm switch specific
  std::string shutdownSdkDumpFile() const;
  // used only in sai
  std::string warmBootDataPath() const;

  int getSwitchId() const {
    return switchId_;
  }

 protected:
  int warmBootFd() const {
    return warmBootFd_;
  }

 private:
  /*
   * Sets a flag that can be read when we next start up that indicates that
   * warm boot is possible. Since warm boot is not currently supported this is
   * always a no-op for now.
   */
  void setCanWarmBoot();

  // Forbidden copy constructor and assignment operator
  HwSwitchWarmBootHelper(HwSwitchWarmBootHelper const&) = delete;
  HwSwitchWarmBootHelper& operator=(HwSwitchWarmBootHelper const&) = delete;

  std::string warmBootFlag() const;
  std::string forceColdBootOnceFlag() const;
  std::string warmBootHwSwitchStateFile_DEPRECATED() const;
  std::string warmBootHwSwitchStateFile() const;
  std::string warmBootThriftSwitchStateFile() const;

  void setupWarmBootFile();
  /*
   * Check to see if we can attempt a warm boot.
   * Returns true if 2 conditions are met
   *
   * 1) User did not create cold_boot_once_unit# file
   * 2) can_warm_boot_unit# file exists, indicating that fboss agent
   * saved warm boot state and shut down successfully.
   * This function also removes the 2 files so that these checks are
   * attempted afresh based on how the agent exits this time and whether
   * or not the user wishes for another cold boot.
   */
  bool checkAndClearWarmBootFlags();

  folly::dynamic getHwSwitchWarmBootState(const std::string& fileName) const;

  int switchId_{-1};
  SwSwitchWarmBootHelper swSwitchWarmBootHelper_;
  std::string sdkWarmbootFilePrefix_;
  int warmBootFd_{-1};
  bool canWarmBoot_{false};
};
} // namespace facebook::fboss
