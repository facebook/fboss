// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchWarmBootHelper.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsyncLogger.h"
#include "fboss/agent/FileBasedWarmbootUtils.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/ThriftBasedWarmbootUtils.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/WarmBootFileUtils.h"

namespace facebook::fboss {

SwSwitchWarmBootHelper::SwSwitchWarmBootHelper(
    const AgentDirectoryUtil* directoryUtil,
    HwAsicTable* asicTable)
    : directoryUtil_(directoryUtil),
      warmBootDir_(directoryUtil_->getWarmBootDir()) {
  if (!warmBootDir_.empty()) {
    // Make sure the warm boot directory exists.
    utilCreateDir(warmBootDir_);

    forceColdBootFlag_ = checkForceColdBootFlag(directoryUtil_);
    canWarmBootFlag_ = checkCanWarmBootFlag(directoryUtil_);
    asicCanWarmBoot_ = checkAsicSupportsWarmboot(asicTable);
  }
}

bool SwSwitchWarmBootHelper::canWarmBootFromFile() const {
  bool canWarmbootFromFile = false;
  if (!forceColdBootFlag_ && canWarmBootFlag_ && asicCanWarmBoot_ &&
      FLAGS_can_warm_boot) {
    CHECK(checkWarmbootStateFileExists(
        warmBootDir_, FLAGS_thrift_switch_state_file));
    canWarmbootFromFile = true;
  }

  XLOG(DBG1) << "Will attempt " << (canWarmbootFromFile ? "WARM" : "COLD")
             << " boot";

  // Notify Async logger about the boot type
  AsyncLogger::setBootType(canWarmbootFromFile);

  return canWarmbootFromFile;
}

bool SwSwitchWarmBootHelper::canWarmBootFromThrift(
    bool isRunModeMultiSwitch,
    HwAsicTable* asicTable,
    HwSwitchThriftClientTable* hwSwitchThriftClientTable) {
  if (!FLAGS_recover_from_hw_switch) {
    return false;
  }
  recoveredStateFromHW_ = checkAndGetWarmbootStateFromHwSwitch(
      isRunModeMultiSwitch, asicTable, hwSwitchThriftClientTable);
  return recoveredStateFromHW_.has_value();
}

void SwSwitchWarmBootHelper::storeWarmBootState(
    const state::WarmbootState& switchStateThrift) {
  if (!asicCanWarmBoot_) {
    XLOG(WARNING)
        << "skip saving warm boot state, as warm boot not supported for network hardware";
    return;
  }
  WarmBootFileUtils::storeWarmBootState(
      warmBootThriftSwitchStateFile(), switchStateThrift);
  // mark that warm boot can happen
  setCanWarmBoot();
}

state::WarmbootState SwSwitchWarmBootHelper::getWarmBootState() const {
  return WarmBootFileUtils::getWarmBootState(warmBootThriftSwitchStateFile());
}

std::string SwSwitchWarmBootHelper::warmBootThriftSwitchStateFile() const {
  return folly::to<std::string>(
      warmBootDir_, "/", FLAGS_thrift_switch_state_file);
}

void SwSwitchWarmBootHelper::setCanWarmBoot() {
  auto warmBootFlagPath = directoryUtil_->getSwSwitchCanWarmBootFile();
  WarmBootFileUtils::setCanWarmBoot(warmBootFlagPath);
}

void SwSwitchWarmBootHelper::logBoot(
    const std::string& bootType,
    const std::string& sdkVersion,
    const std::string& agentVersion) {
  logBootHistory(directoryUtil_, bootType, sdkVersion, agentVersion);
}

} // namespace facebook::fboss
