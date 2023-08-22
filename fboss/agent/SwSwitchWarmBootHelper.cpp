// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchWarmBootHelper.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AsyncLogger.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/CommonFileUtils.h"

namespace {
constexpr auto forceColdBootFlag = "sw_cold_boot_once";
constexpr auto wbFlag = "can_warm_boot";
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
  // if warm boot flag is present, switch state must exist
  CHECK(checkFileExists(warmBootThriftSwitchStateFile()));
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

std::string SwSwitchWarmBootHelper::warmBootFlag() const {
  return folly::to<std::string>(warmBootDir_, "/", wbFlag);
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
} // namespace facebook::fboss
