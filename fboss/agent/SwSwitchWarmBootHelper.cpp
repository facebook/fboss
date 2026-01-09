// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchWarmBootHelper.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsyncLogger.h"
#include "fboss/agent/FileBasedWarmbootUtils.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/WarmBootFileUtils.h"

namespace facebook::fboss {

SwSwitchWarmBootHelper::SwSwitchWarmBootHelper(
    const AgentDirectoryUtil* directoryUtil,
    HwAsicTable* asicTable)
    : directoryUtil_(directoryUtil),
      warmBootDir_(directoryUtil_->getWarmBootDir()),
      asicTable_(asicTable) {
  if (!warmBootDir_.empty()) {
    // Make sure the warm boot directory exists.
    utilCreateDir(warmBootDir_);

    canWarmBoot_ = checkAndClearWarmBootFlags(directoryUtil_, asicTable_);

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
  if (!asicTable_->isFeatureSupportedOnAllAsic(HwAsic::Feature::WARMBOOT)) {
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
