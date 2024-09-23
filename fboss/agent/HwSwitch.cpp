/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/HwSwitch.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TransceiverMap.h"
#include "fboss/lib/HwWriteBehavior.h"

#include <fb303/ThreadCachedServiceData.h>
#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <folly/testing/TestUtil.h>

#include <re2/re2.h>
#include <chrono>

#include "fboss/lib/CommonFileUtils.h"

DEFINE_bool(flexports, false, "Load the agent with flexport support enabled");
DEFINE_int32(
    update_watermark_stats_interval_s,
    60,
    "Update watermark stats interval in seconds");

DEFINE_int32(
    update_voq_stats_interval_s,
    60,
    "Update voq stats interval in seconds");

DEFINE_int32(
    update_phy_info_interval_s,
    10,
    "Update phy info interval in seconds");

DEFINE_bool(
    flowletStatsEnable,
    false,
    "Flag to turn on flowlet stats collection for DLB");

DEFINE_bool(
    skip_stats_update_for_debug,
    false,
    "Skip reading stats from ASIC to allow diag shell debugging!");

DEFINE_int32(
    update_cable_length_stats_s,
    600,
    "Update cable length stats interval in seconds");

namespace {
constexpr auto kBuildSdkVersion = "SDK Version";

/* This function takes a silicon SDK version string and converts it into
   integers that can be sent to external systems such as ODS. The conversion is
   performed as follows: First, the input string is matched against a regex to
   make sure it has the right syntax: "BCM SDK Version: sdk-A.B.C" where A is an
   integer representing the major version, B the minor version and C the release
   version. Then integers are combined into a single number by multiplying major
   by 10,000, minor by 100 and then adding all of them.
   e.g:  For input "BCM SDK Version: sdk-6.4.3", returned value is 60403.
   e.g:  For input "BCM SDK Version: sdk-6.11.10", returned value is 61110.
   Returns an unsigned integer greater than zero, or zero on error.

   Update - After logging SDK version instead of Bcm version, the string stored
   within fb303 becomes
   Bcm Switch: "  BCM SDK Version: sdk-6.5.17"
   Bcm Sai Switch: "  SAI Version: 1.6.5  BCM-SAI SDK Version: 4.2.2.7  BCM SDK
   Version: sdk-6.5.19"
   Leaba Sai switch: "  LEABA-SAI SDK Version: 1.40.0"
   This function will match each type of sdk and produce results accordingly.
   */
std::tuple<int, int, int> normalizeSdkVersion(std::string sdkVersion) {
  /* Matches broadcom SDK version strings (e.g: BCM SDK Version: sdk-6.5.17) */
  constexpr auto bcmSdkVerRegex =
      "BCM SDK Version: sdk-([0-9]{1,2})\\.([0-9]{1,2})\\.([0-9]{1,2})";
  static const re2::RE2 bcmSdkVersionRegex(bcmSdkVerRegex);
  /* Broadcom SAI SDK version strings (e.g: BCM-SAI SDK Version: 4.2.2.7) */
  constexpr auto bcmSaiSdkVerRegex =
      "BCM-SAI SDK Version: ([0-9]{1,2})\\.([0-9]{1,2})\\.([0-9]{1,2})\\.([0-9]{1,2})";
  static const re2::RE2 bcmSaiSdkVersionRegex(bcmSaiSdkVerRegex);
  /* Leaba SAI SDK version strings (e.g: LEABA-SAI SDK Version: 1.40.0) */
  constexpr auto leabaSaiSdkVerRegex =
      "LEABA-SAI SDK Version: ([0-9]{1,2})\\.([0-9]{1,2})\\.([0-9]{1,2})";
  static const re2::RE2 leabaSaiSdkVersionRegex(leabaSaiSdkVerRegex);

  /* Only process versions that match the regex */
  int major, minor, build, release;
  int bcmVer = 0, bcmSaiVer = 0, leabaSaiVer = 0;
  if (re2::RE2::PartialMatch(
          sdkVersion, bcmSdkVerRegex, &major, &minor, &release)) {
    bcmVer = major * 10000 + minor * 100 + release;
  } else {
    XLOG(WARNING) << "Failed to match bcm SDK version to int (" << sdkVersion
                  << ")";
  }
  if (re2::RE2::PartialMatch(
          sdkVersion, bcmSaiSdkVerRegex, &major, &minor, &build, &release)) {
    bcmSaiVer = major * 1000000 + minor * 10000 + build * 100 + release;
  } else {
    XLOG(WARNING) << "Failed to match bcm sai SDK version to int ("
                  << sdkVersion << ")";
  }
  if (re2::RE2::PartialMatch(
          sdkVersion, leabaSaiSdkVerRegex, &major, &minor, &release)) {
    leabaSaiVer = major * 10000 + minor * 100 + release;
  } else {
    XLOG(WARNING) << "Failed to match leaba sai SDK version to int ("
                  << sdkVersion << ")";
  }
  return std::make_tuple(bcmVer, bcmSaiVer, leabaSaiVer);
}

} // namespace
namespace facebook::fboss {

std::string HwSwitch::getDebugDump() const {
  folly::test::TemporaryDirectory tmpDir;
  auto fname = tmpDir.path().string() + "hw_debug_dump";
  dumpDebugState(fname);
  std::string out;
  if (!folly::readFile(fname.c_str(), out)) {
    throw FbossError("Unable get debug dump");
  }
  return out;
}

HwSwitchFb303Stats* HwSwitch::getSwitchStats() const {
  if (!hwSwitchStats_) {
    hwSwitchStats_.reset(new HwSwitchFb303Stats(
        fb303::ThreadCachedServiceData::get()->getThreadStats(),
        getPlatform()->getAsic()->getVendor(),
        getPlatform()->getMultiSwitchStatsPrefix()));
  }
  return hwSwitchStats_.get();
}

void HwSwitch::switchRunStateChanged(SwitchRunState newState) {
  if (runState_ != newState) {
    switchRunStateChangedImpl(newState);
    runState_ = newState;
    XLOG(DBG2) << " HwSwitch run state changed to: "
               << switchRunStateStr(runState_);
  }
}

void HwSwitch::updateStats() {
  try {
    updateStatsImpl();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error collecting hw stats " << folly::exceptionStr(ex);
    getSwitchStats()->statsCollectionFailed();
    throw;
  }
}

multiswitch::HwSwitchStats HwSwitch::getHwSwitchStats() {
  multiswitch::HwSwitchStats hwSwitchStats;
  auto now = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  hwSwitchStats.timestamp() = now.count();
  hwSwitchStats.hwPortStats() = getPortStats();
  hwSwitchStats.hwAsicErrors() = getSwitchStats()->getHwAsicErrors();
  hwSwitchStats.teFlowStats() = getTeFlowStats();
  hwSwitchStats.fabricReachabilityStats() = getFabricReachabilityStats();
  hwSwitchStats.sysPortStats() = getSysPortStats();
  hwSwitchStats.fb303GlobalStats() = getSwitchStats()->getAllFb303Stats();
  hwSwitchStats.cpuPortStats() = getCpuPortStats();
  hwSwitchStats.switchDropStats() = getSwitchDropStats();
  hwSwitchStats.flowletStats() = getHwFlowletStats();
  hwSwitchStats.aclStats() = getAclStats();
  hwSwitchStats.switchWatermarkStats() = getSwitchWatermarkStats();
  hwSwitchStats.hwResourceStats() = getResourceStats();
  return hwSwitchStats;
}

void HwSwitch::updateAllPhyInfo() {
  // Determine if phy info needs to be collected
  auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  if (now - phyInfoUpdateTime_ >= FLAGS_update_phy_info_interval_s) {
    phyInfoUpdateTime_ = now;
    try {
      *lastPhyInfo_.wlock() = updateAllPhyInfoImpl();
    } catch (const std::exception& ex) {
      getSwitchStats()->phyInfoCollectionFailed();
      XLOG(ERR) << "Error running updateAllPhyInfo: "
                << folly::exceptionStr(ex);
    }
  }
}

uint32_t HwSwitch::generateDeterministicSeed(LoadBalancerID loadBalancerID) {
  return generateDeterministicSeed(
      loadBalancerID, getPlatform()->getLocalMac());
}

void HwSwitch::gracefulExit() {
  auto* dirUtil = getPlatform()->getDirectoryUtil();
  auto switchIndex = getPlatform()->getAsic()->getSwitchIndex();
  auto sleepOnSigTermFile = dirUtil->sleepHwSwitchOnSigTermFile(switchIndex);
  if (checkFileExists(sleepOnSigTermFile)) {
    SCOPE_EXIT {
      removeFile(sleepOnSigTermFile);
    };
    std::string timeStr;
    if (folly::readFile(sleepOnSigTermFile.c_str(), timeStr)) {
      // @lint-ignore CLANGTIDY
      std::this_thread::sleep_for(
          std::chrono::seconds(folly::to<uint32_t>(timeStr)));
    }
  }
  if (getPlatform()->getAsic()->isSupported(HwAsic::Feature::WARMBOOT)) {
    gracefulExitImpl();
  }
}

std::shared_ptr<SwitchState> HwSwitch::stateChangedTransaction(
    const StateDelta& delta,
    const HwWriteBehaviorRAII& behavior) {
  auto result = stateChangedTransaction(delta.getOperDelta(), behavior);
  if (!result.changes()->empty()) {
    // changes have been rolled back to last good known state
    return delta.oldState();
  }
  return delta.newState();
}

void HwSwitch::rollback(const StateDelta& /*delta*/) noexcept {
  XLOG(FATAL)
      << "Transactions is supported but rollback is implemented on this switch";
}

std::shared_ptr<SwitchState> HwSwitch::getProgrammedState() const {
  auto programmedState = programmedState_.rlock();
  return *programmedState;
}

void HwSwitch::setProgrammedState(const std::shared_ptr<SwitchState>& state) {
  auto programmedState = programmedState_.wlock();
  *programmedState = state;
  (*programmedState)->publish();
}

fsdb::OperDelta HwSwitch::stateChanged(
    const fsdb::OperDelta& delta,
    const HwWriteBehaviorRAII& /*behavior*/) {
  auto stateDelta = StateDelta(getProgrammedState(), delta);
  auto state = stateChangedImpl(stateDelta);
  setProgrammedState(state);
  if (getProgrammedState() == stateDelta.newState()) {
    return fsdb::OperDelta{};
  }
  // return the delta between expected applied state and actually applied state
  // caller can then can construct actually applied state from its expected new
  // state from returning oper delta, and also know what was not applied from
  // the state delta between expected applied state and applied state.
  return StateDelta(stateDelta.newState(), getProgrammedState()).getOperDelta();
}

fsdb::OperDelta HwSwitch::stateChangedTransaction(
    const fsdb::OperDelta& delta,
    const HwWriteBehaviorRAII& /*behavior*/) {
  if (!transactionsSupported()) {
    throw FbossError("Transactions not supported on this switch");
  }
  auto goodKnownState = getProgrammedState();
  fsdb::OperDelta result{};
  try {
    result = stateChanged(delta);
  } catch (const FbossError& e) {
    XLOG(WARNING) << " Transaction failed with error : " << *e.message()
                  << " attempting rollback";
    this->rollback(StateDelta(getProgrammedState(), delta));
    setProgrammedState(goodKnownState);
    return delta;
  }
  return result;
}

bool HwSwitch::isFullyConfigured() const {
  auto state = getRunState();
  return state >= SwitchRunState::CONFIGURED &&
      state != SwitchRunState::EXITING;
}

void HwSwitch::ensureConfigured(folly::StringPiece function) const {
  if (isFullyConfigured()) {
    return;
  }

  if (!function.empty()) {
    XLOG(DBG1) << "failing thrift prior to switch configuration: " << function;
  }
  throw FbossError(
      "switch is still initializing or is exiting and is not "
      "fully configured yet");
}

void HwSwitch::ensureVoqOrFabric(folly::StringPiece function) const {
  ensureConfigured(function);
  if (getSwitchType() != cfg::SwitchType::VOQ &&
      getSwitchType() != cfg::SwitchType::FABRIC) {
    throw FbossError(
        "switch is not a VOQ or Fabric switch and cannot process request: ",
        function);
  }
}

std::shared_ptr<SwitchState> HwSwitch::getMinAlpmState(
    RoutingInformationBase* rib,
    const std::shared_ptr<SwitchState>& state) {
  CHECK(getRunState() < SwitchRunState::INITIALIZED)
      << "Invalid run state for programming ALPM state";
  CHECK(getBootType() == BootType::COLD_BOOT)
      << "ALPM minimum state can only be programmed on cold boot";
  CHECK(
      getPlatform()->getAsic()->isSupported(HwAsic::Feature::ROUTE_PROGRAMMING))
      << "Route programming is not supported";
  CHECK(
      getSwitchType() == cfg::SwitchType::VOQ ||
      getSwitchType() == cfg::SwitchType::NPU)
      << "ALPM minimum state can only be programmed on support VOQ or NPU switch";

  std::shared_ptr<SwitchState> minAlpmState{};
  HwSwitchRouteUpdateWrapper routeUpdater(
      this, rib, [&minAlpmState](const StateDelta& delta) {
        minAlpmState = delta.newState();
        return minAlpmState;
      });
  routeUpdater.programMinAlpmState();
  return minAlpmState;
}

std::shared_ptr<SwitchState> HwSwitch::programMinAlpmState(
    RoutingInformationBase* rib) {
  return programMinAlpmState(
      rib, [this](const StateDelta& delta) { return stateChanged(delta); });
}

std::shared_ptr<SwitchState> HwSwitch::programMinAlpmState(
    RoutingInformationBase* rib,
    StateChangedFn func) {
  auto state = getProgrammedState();
  auto minAlpmState = getMinAlpmState(rib, state);
  return func(StateDelta(state, minAlpmState));
}

HwInitResult HwSwitch::init(
    Callback* callback,
    const std::shared_ptr<SwitchState>& state,
    bool failHwCallsOnWarmboot) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::steady_clock;
  steady_clock::time_point begin = steady_clock::now();
  HwInitResult ret = initLight(callback, failHwCallsOnWarmboot);
  if (ret.bootType == BootType::WARM_BOOT) {
    CHECK(state);
    ret.switchState = state;
  } else {
    // cold boot state is already programmed during initLight
    ret.switchState = getProgrammedState();
    ret.rib = std::make_unique<RoutingInformationBase>();
  }
  if (ret.bootType == BootType::WARM_BOOT) {
    // apply state only for warm boot. cold boot state is already applied.
    auto writeBehavior = getWarmBootWriteBehavior(failHwCallsOnWarmboot);
    ret.switchState = stateChanged(
        StateDelta(getProgrammedState(), ret.switchState), writeBehavior);
    setProgrammedState(ret.switchState);
  }
  ret.bootTime =
      duration_cast<duration<float>>(steady_clock::now() - begin).count();
  initialStateApplied();
  return ret;
}

HwWriteBehaviorRAII HwSwitch::getWarmBootWriteBehavior(
    bool failHwCallsOnWarmboot) const {
  if (getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::ZERO_SDK_WRITE_WARMBOOT)) {
    if (failHwCallsOnWarmboot) {
      return HwWriteBehaviorRAII(HwWriteBehavior::FAIL);
    } else {
      return HwWriteBehaviorRAII(HwWriteBehavior::WRITE);
    }
  } else {
    return HwWriteBehaviorRAII(HwWriteBehavior::LOG_FAIL);
  }
}

HwInitResult HwSwitch::initLight(
    Callback* callback,
    bool failHwCallsOnWarmboot) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::steady_clock;
  steady_clock::time_point begin = steady_clock::now();
  auto ret = initLightImpl(callback, failHwCallsOnWarmboot);
  /* Merchant Silicon SDK Version */
  std::string buildSdkVersion;
  fb303::fbData->getExportedValue(buildSdkVersion, kBuildSdkVersion);
  /* Remove newline in SDK version */
  buildSdkVersion.erase(
      std::remove(buildSdkVersion.begin(), buildSdkVersion.end(), '\n'),
      buildSdkVersion.end());
  /* Tuple of <bcmVersion, bcmSaiVersion, leabaSaiVersion> */
  auto [bcmVer, bcmSaiVer, leabaSaiVer] = normalizeSdkVersion(buildSdkVersion);
  if (bcmVer) {
    getSwitchStats()->bcmSdkVer(bcmVer);
  }
  if (bcmSaiVer) {
    getSwitchStats()->bcmSaiSdkVer(bcmSaiVer);
  }
  if (leabaSaiVer) {
    getSwitchStats()->leabaSdkVer(leabaSaiVer);
  }
  auto end = steady_clock::now();
  ret.bootTime = duration_cast<duration<float>>(end - begin).count();
  getSwitchStats()->bootTime(
      duration_cast<std::chrono::milliseconds>(end - begin).count());
  return ret;
}

HwInitResult HwSwitch::initLightImpl(
    Callback* callback,
    bool failHwCallsOnWarmboot) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::steady_clock;
  steady_clock::time_point begin = steady_clock::now();
  switchType_ = getPlatform()->getAsic()->getSwitchType();
  switchId_ = getPlatform()->getAsic()->getSwitchId();
  BootType bootType = BootType::COLD_BOOT;
  if (getPlatform()->getAsic()->getAsicType() !=
      cfg::AsicType::ASIC_TYPE_MOCK) {
    if (auto warmBootHelper = getPlatform()->getWarmBootHelper()) {
      bool canWarmBoot =
          (getPlatform()->getAsic()->isSupported(HwAsic::Feature::WARMBOOT));
      canWarmBoot &= warmBootHelper->canWarmBoot();
      bootType = canWarmBoot ? BootType::WARM_BOOT : BootType::COLD_BOOT;
    }
  }
  // initialize hardware switch
  auto ret = initImpl(callback, bootType, failHwCallsOnWarmboot);
  ret.bootType = bootType;
  auto end = steady_clock::now();
  ret.initializedTime = duration_cast<duration<float>>(end - begin).count();
  getSwitchStats()->hwInitializedTime(
      duration_cast<std::chrono::milliseconds>(end - begin).count());
  if (ret.bootType == BootType::WARM_BOOT) {
    // on warm boot, no need to apply minimum alpm state
    // wait for state to be injected by SwSwitch
    setProgrammedState(std::make_shared<SwitchState>());
    getSwitchStats()->warmBoot();
    return ret;
  }
  getSwitchStats()->coldBoot();
  if (switchType_ != cfg::SwitchType::NPU &&
      switchType_ != cfg::SwitchType::VOQ) {
    // no route programming, no need to apply minimum alpm state
    return ret;
  }

  // program min alpm state for npu and voq only on cold boot
  std::map<int32_t, state::RouteTableFields> routeTables{};
  routeTables.emplace(kDefaultVrf, state::RouteTableFields{});
  auto rib = RoutingInformationBase::fromThrift(routeTables);
  programMinAlpmState(rib.get(), [this](const StateDelta& delta) {
    return stateChanged(delta);
  });
  return ret;
}
} // namespace facebook::fboss
