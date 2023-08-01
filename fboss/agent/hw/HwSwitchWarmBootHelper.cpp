/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"

#include "fboss/agent/AsyncLogger.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/Utils.h"

#include <folly/FileUtil.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <optional>
#include <tuple>
#include "fboss/lib/CommonFileUtils.h"

DEFINE_bool(can_warm_boot, true, "Enable/disable warm boot functionality");
DEFINE_string(
    switch_state_file,
    "switch_state",
    "File for dumping switch state JSON in on exit, it maintains only hardware switch");
DEFINE_string(
    thrift_switch_state_file,
    "thrift_switch_state",
    "File for dumping switch state in serialized thrift format on exit");

namespace {
constexpr auto wbFlagPrefix = "can_warm_boot_";
constexpr auto forceColdBootPrefix = "cold_boot_once_";
constexpr auto shutdownDumpPrefix = "sdk_shutdown_dump_";
constexpr auto startupDumpPrefix = "sdk_startup_dump_";

} // namespace

namespace facebook::fboss {

HwSwitchWarmBootHelper::HwSwitchWarmBootHelper(
    int switchId,
    const std::string& warmBootDir,
    const std::string& sdkWarmbootFilePrefix)
    : switchId_(switchId),
      warmBootDir_(warmBootDir),
      sdkWarmbootFilePrefix_(sdkWarmbootFilePrefix) {
  if (!warmBootDir_.empty()) {
    // Make sure the warm boot directory exists.
    utilCreateDir(warmBootDir_);

    canWarmBoot_ = checkAndClearWarmBootFlags();
    if (!FLAGS_can_warm_boot) {
      canWarmBoot_ = false;
    }

    auto bootType = canWarmBoot_ ? "WARM" : "COLD";
    XLOG(DBG1) << "Will attempt " << bootType << " boot";

    // Notify Async logger about the boot type
    AsyncLogger::setBootType(canWarmBoot_);

    setupWarmBootFile();
  }
}

HwSwitchWarmBootHelper::~HwSwitchWarmBootHelper() {
  if (warmBootFd_ > 0) {
    int rv = close(warmBootFd_);
    if (rv < 0) {
      XLOG(ERR) << "error closing warm boot file for unit " << switchId_ << ": "
                << errno;
    }
    warmBootFd_ = -1;
  }
}

std::string HwSwitchWarmBootHelper::warmBootHwSwitchStateFile_DEPRECATED()
    const {
  return folly::to<std::string>(warmBootDir_, "/", FLAGS_switch_state_file);
}

std::string HwSwitchWarmBootHelper::warmBootHwSwitchStateFile() const {
  return folly::to<std::string>(
      warmBootDir_, "/", FLAGS_switch_state_file, "_", switchId_);
}

std::string HwSwitchWarmBootHelper::warmBootThriftSwitchStateFile() const {
  return folly::to<std::string>(
      warmBootDir_, "/", FLAGS_thrift_switch_state_file);
}

std::string HwSwitchWarmBootHelper::warmBootFlag() const {
  return folly::to<std::string>(warmBootDir_, "/", wbFlagPrefix, switchId_);
}

std::string HwSwitchWarmBootHelper::warmBootDataPath() const {
  return folly::to<std::string>(
      warmBootDir_, "/", sdkWarmbootFilePrefix_, switchId_);
}

std::string HwSwitchWarmBootHelper::forceColdBootOnceFlag() const {
  return folly::to<std::string>(
      warmBootDir_, "/", forceColdBootPrefix, switchId_);
}

std::string HwSwitchWarmBootHelper::startupSdkDumpFile() const {
  return folly::to<std::string>(
      warmBootDir_, "/", startupDumpPrefix, switchId_);
}

std::string HwSwitchWarmBootHelper::shutdownSdkDumpFile() const {
  return folly::to<std::string>(
      warmBootDir_, "/", shutdownDumpPrefix, switchId_);
}

void HwSwitchWarmBootHelper::setCanWarmBoot() {
  auto wbFlag = warmBootFlag();
  auto updateFd = creat(wbFlag.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (updateFd < 0) {
    throw SysError(errno, "Unable to create ", wbFlag);
  }
  close(updateFd);

  XLOG(DBG1) << "Wrote can warm boot flag: " << wbFlag;
}

bool HwSwitchWarmBootHelper::checkAndClearWarmBootFlags() {
  // Return true if coldBootOnceFile does not exist and
  // canWarmBoot file exists
  bool canWarmBoot = removeFile(warmBootFlag(), true /*log*/);
  bool forceColdBoot = removeFile(forceColdBootOnceFlag(), true /*log*/);
  return !forceColdBoot && canWarmBoot;
}

bool HwSwitchWarmBootHelper::storeWarmBootState(
    const folly::dynamic& follySwitchState,
    const state::WarmbootState& thriftSwitchState) {
  /* dump hardware switch state */
  warmBootStateWritten_ &= storeHwSwitchWarmBootState(follySwitchState);
  /* dump software switch state */
  warmBootStateWritten_ = storeSwSwitchWarmBootState(thriftSwitchState);
  return warmBootStateWritten_;
}

std::tuple<folly::dynamic, std::optional<state::WarmbootState>>
HwSwitchWarmBootHelper::getWarmBootState() const {
  folly::dynamic hwSwitchState = getHwSwitchWarmBootState();
  state::WarmbootState thriftState = getSwSwitchWarmBootState();
  return std::make_tuple(hwSwitchState, thriftState);
}

void HwSwitchWarmBootHelper::setupWarmBootFile() {
  auto warmBootPath = warmBootDataPath();
  warmBootFd_ = open(warmBootPath.c_str(), O_RDWR | O_CREAT, 0600);
  if (warmBootFd_ < 0) {
    throw SysError(errno, "failed to open warm boot data file ", warmBootPath);
  }
}

bool HwSwitchWarmBootHelper::storeSwSwitchWarmBootState(
    const state::WarmbootState& switchStateThrift) {
  auto rc = dumpBinaryThriftToFile(
      warmBootThriftSwitchStateFile(), switchStateThrift);
  if (!rc) {
    XLOG(ERR) << "Error while storing switch state to thrift state file: "
              << warmBootThriftSwitchStateFile();
  }
  return rc;
}

bool HwSwitchWarmBootHelper::storeHwSwitchWarmBootState(
    const folly::dynamic& switchState) {
  auto dumpStateToFileFn = [](const std::string& file,
                              const folly::dynamic& state) {
    if (!dumpStateToFile(file, state)) {
      XLOG(ERR) << "Error while storing switch state to folly state file: "
                << file;
      return false;
    }
    return true;
  };
  auto rc =
      dumpStateToFileFn(warmBootHwSwitchStateFile_DEPRECATED(), switchState);
  rc &= dumpStateToFileFn(warmBootHwSwitchStateFile(), switchState);
  return rc;
}

state::WarmbootState HwSwitchWarmBootHelper::getSwSwitchWarmBootState() const {
  state::WarmbootState thriftState;
  if (!readThriftFromBinaryFile(warmBootThriftSwitchStateFile(), thriftState)) {
    throw FbossError(
        "Failed to read thrift state from ", warmBootThriftSwitchStateFile());
  }
  return thriftState;
}

folly::dynamic HwSwitchWarmBootHelper::getHwSwitchWarmBootState() const {
  bool existsOld = checkFileExists(warmBootHwSwitchStateFile_DEPRECATED());
  bool existsNew = checkFileExists(warmBootHwSwitchStateFile());
  if (existsOld && existsNew) {
    // prefer old one if both exists to support warm boot from old version to
    // new version new version also dumps at old location.
    return getHwSwitchWarmBootState(warmBootHwSwitchStateFile_DEPRECATED());
  } else if (existsOld) {
    return getHwSwitchWarmBootState(warmBootHwSwitchStateFile_DEPRECATED());
  } else if (existsNew) {
    return getHwSwitchWarmBootState(warmBootHwSwitchStateFile());
  }
  throw FbossError("No hw switch warm boot state file found");
}

folly::dynamic HwSwitchWarmBootHelper::getHwSwitchWarmBootState(
    const std::string& fileName) const {
  std::string warmBootJson;
  XLOG(INFO) << "reading hw switch warm boot state from : " << fileName;
  auto ret = folly::readFile(fileName.c_str(), warmBootJson);
  sysCheckError(
      ret, "Unable to read hw switch warm boot state from : ", fileName);
  return folly::parseJson(warmBootJson);
}

} // namespace facebook::fboss
