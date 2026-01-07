// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FileBasedWarmbootUtils.h"

#include <folly/logging/xlog.h>

#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/lib/CommonFileUtils.h"

namespace facebook::fboss {

std::string getForceColdBootOnceFlagLegacy(
    const AgentDirectoryUtil* directoryUtil) {
  return folly::to<std::string>(directoryUtil->getSwColdBootOnceFile(), "_0");
}

std::string getWarmBootFlagLegacy(const AgentDirectoryUtil* directoryUtil) {
  return folly::to<std::string>(
      directoryUtil->getSwSwitchCanWarmBootFile(), "_0");
}

std::string getWarmBootThriftSwitchStateFile(
    const std::string& warmBootDir,
    const std::string& thriftSwitchStateFile) {
  return folly::to<std::string>(warmBootDir, "/", thriftSwitchStateFile);
}

bool checkAndClearWarmBootFlags(
    const AgentDirectoryUtil* directoryUtil,
    HwAsicTable* asicTable) {
  bool forceColdBoot =
      removeFile(directoryUtil->getSwColdBootOnceFile(), true /*log*/);
  forceColdBoot = forceColdBoot ||
      checkFileExists(getForceColdBootOnceFlagLegacy(directoryUtil));
  bool canWarmBoot =
      removeFile(directoryUtil->getSwSwitchCanWarmBootFile(), true /*log*/);
  canWarmBoot =
      canWarmBoot || checkFileExists(getWarmBootFlagLegacy(directoryUtil));

  if (forceColdBoot || !canWarmBoot) {
    // cold boot was enforced or warm boot flag is absent
    return false;
  }
  if (!asicTable->isFeatureSupportedOnAllAsic(HwAsic::Feature::WARMBOOT)) {
    // asic does not support warm boot
    XLOG(WARNING) << "Warm boot not supported for network hardware";
    return false;
  }
  // if warm boot flag is present, switch state must exist
  CHECK(checkFileExists(getWarmBootThriftSwitchStateFile(
      directoryUtil->getWarmBootDir(), FLAGS_thrift_switch_state_file)));
  // command line override
  return FLAGS_can_warm_boot;
}

} // namespace facebook::fboss
