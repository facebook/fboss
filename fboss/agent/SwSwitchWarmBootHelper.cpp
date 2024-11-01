// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchWarmBootHelper.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <iomanip>

#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/AsyncLogger.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
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

SwSwitchWarmBootHelper::SwSwitchWarmBootHelper(
    const AgentDirectoryUtil* directoryUtil,
    HwAsicTable* asicTable)
    : directoryUtil_(directoryUtil),
      warmBootDir_(directoryUtil_->getWarmBootDir()),
      asicTable_(asicTable) {
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
  if (!asicTable_->isFeatureSupportedOnAllAsic(HwAsic::Feature::WARMBOOT)) {
    XLOG(WARNING)
        << "skip saving warm boot state, as warm boot not supported for network hardware";
    return;
  }
  auto rc = dumpBinaryThriftToFile(
      warmBootThriftSwitchStateFile(), switchStateThrift);
  if (!rc) {
    XLOG(FATAL) << "Error while storing switch state to thrift state file: "
                << warmBootThriftSwitchStateFile();
  }
  // mark that warm boot can happen
  setCanWarmBoot();
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
  forceColdBoot =
      forceColdBoot || checkFileExists(forceColdBootOnceFlagLegacy());
  bool canWarmBoot = removeFile(warmBootFlag(), true /*log*/);
  canWarmBoot = canWarmBoot || checkFileExists(warmBootFlagLegacy());
  if (forceColdBoot || !canWarmBoot) {
    // cold boot was enforced or warm boot flag is absent
    return false;
  }
  if (!asicTable_->isFeatureSupportedOnAllAsic(HwAsic::Feature::WARMBOOT)) {
    // asic does not support warm boot
    XLOG(WARNING) << "Warm boot not supported for network hardware";
    return false;
  }
  // if warm boot flag is present, switch state must exist
  CHECK(checkFileExists(warmBootThriftSwitchStateFile()));
  // command line override
  return FLAGS_can_warm_boot;
}

std::string SwSwitchWarmBootHelper::forceColdBootOnceFlag() const {
  auto rc = folly::to<std::string>(warmBootDir_, "/", forceColdBootFlag);
  CHECK_EQ(rc, AgentDirectoryUtil().getSwColdBootOnceFile(warmBootDir_));
  return rc;
}

std::string SwSwitchWarmBootHelper::warmBootThriftSwitchStateFile() const {
  return folly::to<std::string>(
      warmBootDir_, "/", FLAGS_thrift_switch_state_file);
}

std::string SwSwitchWarmBootHelper::warmBootFlag() const {
  return directoryUtil_->getSwSwitchCanWarmBootFile();
}

void SwSwitchWarmBootHelper::setCanWarmBoot() {
  auto wbFlag = warmBootFlag();
  auto updateFd = creat(wbFlag.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (updateFd < 0) {
    throw SysError(errno, "Unable to create ", wbFlag);
  }
  close(updateFd);
  XLOG(DBG1) << "Wrote can warm boot flag: " << wbFlag;
}

std::string SwSwitchWarmBootHelper::warmBootFlagLegacy() const {
  return folly::to<std::string>(warmBootFlag(), "_0");
}

std::string SwSwitchWarmBootHelper::forceColdBootOnceFlagLegacy() const {
  return folly::to<std::string>(forceColdBootOnceFlag(), "_0");
}

std::pair<std::shared_ptr<SwitchState>, std::unique_ptr<RoutingInformationBase>>
SwSwitchWarmBootHelper::reconstructStateAndRib(
    std::optional<state::WarmbootState> warmBootState,
    bool hasL3) {
  std::unique_ptr<RoutingInformationBase> rib{};
  std::shared_ptr<SwitchState> state{nullptr};
  if (warmBootState.has_value()) {
    /* warm boot: reconstruct from warm boot state */
    state = SwitchState::fromThrift(*(warmBootState->swSwitchState()));
    rib = RoutingInformationBase::fromThrift(
        *(warmBootState->routeTables()),
        state->getFibs(),
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

void SwSwitchWarmBootHelper::logBoot(
    const std::string& bootType,
    const std::string& sdkVersion,
    const std::string& agentVersion) {
  folly::File logFile;
  try {
    logFile = folly::File(
        AgentDirectoryUtil().getAgentBootHistoryLogFile(),
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
  std::tm tm;
  localtime_r(&timer, &tm);

  std::ostringstream oss;
  oss << "[ " << std::put_time(&tm, "%Y %B %d %H:%M:%S")
      << " ]: " << "Start of a " << bootType << ", "
      << "SDK version: " << sdkVersion << ", "
      << "Agent version: " << agentVersion << std::endl;
  auto ossString = oss.str();
  folly::writeFull(logFile.fd(), ossString.c_str(), ossString.size());
}
} // namespace facebook::fboss
