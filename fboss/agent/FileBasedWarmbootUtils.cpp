// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FileBasedWarmbootUtils.h"

#include <chrono>
#include <iomanip>
#include <sstream>

#include <folly/File.h>
#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
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

bool checkForceColdBootFlag(const AgentDirectoryUtil* directoryUtil) {
  bool forceColdBoot =
      removeFile(directoryUtil->getSwColdBootOnceFile(), true /*log*/);
  forceColdBoot = forceColdBoot ||
      checkFileExists(getForceColdBootOnceFlagLegacy(directoryUtil));
  return forceColdBoot;
}

bool checkCanWarmBootFlag(const AgentDirectoryUtil* directoryUtil) {
  bool canWarmBoot =
      removeFile(directoryUtil->getSwSwitchCanWarmBootFile(), true /*log*/);
  canWarmBoot =
      canWarmBoot || checkFileExists(getWarmBootFlagLegacy(directoryUtil));
  return canWarmBoot;
}

bool checkAsicSupportsWarmboot(HwAsicTable* asicTable) {
  if (!asicTable->isFeatureSupportedOnAllAsic(HwAsic::Feature::WARMBOOT)) {
    XLOG(WARNING) << "Warm boot not supported for network hardware";
    return false;
  }
  return true;
}

bool checkWarmbootStateFileExists(
    const std::string& warmBootDir,
    const std::string& thriftSwitchStateFile) {
  return checkFileExists(
      getWarmBootThriftSwitchStateFile(warmBootDir, thriftSwitchStateFile));
}

void logBootHistory(
    const AgentDirectoryUtil* directoryUtil,
    const std::string& bootType,
    const std::string& sdkVersion,
    const std::string& agentVersion) {
  folly::File logFile;
  try {
    logFile = folly::File(
        directoryUtil->getAgentBootHistoryLogFile(),
        O_RDWR | O_CREAT | O_APPEND);
  } catch (const std::system_error&) {
    //   /var/facebook/logs/fboss/ might not exist for testing switch
    XLOG(WARNING)
        << "Agent boot history log failed to create under /var/facebook/logs/fboss/, using /tmp/";
    logFile =
        folly::File("/tmp/wedge_agent_starts.log", O_RDWR | O_CREAT | O_TRUNC);
  }

  auto now = std::chrono::system_clock::now();
  auto timer = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  localtime_r(&timer, &tm);

  std::ostringstream oss;
  oss << "[ " << std::put_time(&tm, "%Y %B %d %H:%M:%S")
      << " ]: " << "Start of a " << bootType << ", "
      << "SDK version: " << sdkVersion << ", "
      << "Agent version: " << agentVersion << std::endl;
  auto ossString = oss.str();
  folly::writeFull(logFile.fd(), ossString.c_str(), ossString.size());
}

std::pair<std::shared_ptr<SwitchState>, std::unique_ptr<RoutingInformationBase>>
reconstructStateAndRib(
    std::optional<state::WarmbootState> warmBootState,
    bool hasL3) {
  std::unique_ptr<RoutingInformationBase> rib{};
  std::shared_ptr<SwitchState> state{nullptr};
  if (warmBootState.has_value()) {
    /* warm boot: reconstruct from warm boot state */
    state = SwitchState::fromThrift(*(warmBootState->swSwitchState()));
    rib = RoutingInformationBase::fromThrift(
        *(warmBootState->routeTables()),
        state->getFibsInfoMap(),
        state->getLabelForwardingInformationBase());
  } else {
    state = SwitchState::fromThrift(state::SwitchState{});
    /* cold boot, setup default rib */
    std::map<int32_t, state::RouteTableFields> routeTables{};
    if (hasL3) {
      /* at least one switch supports route programming, setup default vrf */
      routeTables.emplace(kDefaultVrf, state::RouteTableFields{});
    }
    rib = RoutingInformationBase::fromThrift(routeTables);
  }
  return std::make_pair(state, std::move(rib));
}

} // namespace facebook::fboss
