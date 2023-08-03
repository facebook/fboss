// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchWarmBootHelper.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AsyncLogger.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/Utils.h"
#include "fboss/lib/CommonFileUtils.h"

namespace {
constexpr auto forceColdBootFlag = "sw_cold_boot_once";
} // namespace

DEFINE_bool(can_warm_boot, true, "Enable/disable warm boot functionality");
DEFINE_string(
    thrift_switch_state_file,
    "thrift_switch_state",
    "File for dumping switch state in serialized thrift format on exit");

namespace facebook::fboss {

SwSwitchWarmBootHelper::SwSwitchWarmBootHelper(const std::string& warmBootDir)
    : warmBootDir_(warmBootDir) {
  if (!warmBootDir_.empty()) {
    // Make sure the warm boot directory exists.
    utilCreateDir(warmBootDir_);

    canWarmBoot_ = checkAndClearWarmBootFlags();

    auto bootType = canWarmBoot_ ? "WARM" : "COLD";
    XLOG(DBG1) << "Will attempt " << bootType << " boot";

    // Notify Async logger about the boot type
    AsyncLogger::setBootType(canWarmBoot_);
  }
}

bool SwSwitchWarmBootHelper::canWarmBoot() const {
  return canWarmBoot_;
}

void SwSwitchWarmBootHelper::storeWarmBootState(
    const state::WarmbootState& switchStateThrift) {
  auto rc = dumpBinaryThriftToFile(
      warmBootThriftSwitchStateFile(), switchStateThrift);
  if (!rc) {
    XLOG(FATAL) << "Error while storing switch state to thrift state file: "
                << warmBootThriftSwitchStateFile();
  }
}

state::WarmbootState SwSwitchWarmBootHelper::getWarmBootState() const {
  state::WarmbootState thriftState;
  if (!readThriftFromBinaryFile(warmBootThriftSwitchStateFile(), thriftState)) {
    throw FbossError(
        "Failed to read thrift state from ", warmBootThriftSwitchStateFile());
  }
  return thriftState;
}

bool SwSwitchWarmBootHelper::checkAndClearWarmBootFlags() {
  bool forceColdBoot = removeFile(forceColdBootOnceFlag(), true /*log*/);
  if (!forceColdBoot) {
    // cold boot was enforced.
    return false;
  }
  // if warm boot state was dumped read it
  if (!checkFileExists(warmBootThriftSwitchStateFile())) {
    // it switch state was not dumped, enforce cold boot
    return false;
  }
  // command line override
  return FLAGS_can_warm_boot;
}

std::string SwSwitchWarmBootHelper::forceColdBootOnceFlag() const {
  return folly::to<std::string>(warmBootDir_, "/", forceColdBootFlag);
}

std::string SwSwitchWarmBootHelper::warmBootThriftSwitchStateFile() const {
  return folly::to<std::string>(
      warmBootDir_, "/", FLAGS_thrift_switch_state_file);
}

} // namespace facebook::fboss
