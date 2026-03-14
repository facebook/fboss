// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <algorithm>
#include <utility>

#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/test/hal_test/HalTest.h"
#include "fboss/qsfp_service/test/hal_test/HalTestUtils.h"

#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/test/hal_test/gen-cpp2/hal_test_config_constants.h"

namespace facebook::fboss {

namespace {

void verifyMediaInterfaceCodes(
    QsfpModule* module,
    int tcvrId,
    TcvrOperationalMode mode) {
  // Refresh to update the internal data structures
  module->refresh();
  auto expectedCodes = hal_test::getExpectedMediaInterfaceCodes(mode);

  auto info = module->getTransceiverInfo();
  ASSERT_TRUE(info.tcvrState()->settings().has_value())
      << "Transceiver " << tcvrId << " has no settings";
  ASSERT_TRUE(info.tcvrState()->settings()->mediaInterface().has_value())
      << "Transceiver " << tcvrId
      << " has no mediaInterface after programming mode "
      << apache::thrift::util::enumNameSafe(mode);

  const auto& mediaInterfaces = *info.tcvrState()->settings()->mediaInterface();

  ASSERT_EQ(mediaInterfaces.size(), expectedCodes.size())
      << "Transceiver " << tcvrId
      << " unexpected number of media interface lanes for mode "
      << apache::thrift::util::enumNameSafe(mode);

  for (size_t i = 0; i < expectedCodes.size(); i++) {
    EXPECT_EQ(*mediaInterfaces[i].code(), expectedCodes[i])
        << "Transceiver " << tcvrId << " lane " << i
        << " media interface code mismatch for mode "
        << apache::thrift::util::enumNameSafe(mode) << ": expected "
        << apache::thrift::util::enumNameSafe(expectedCodes[i]) << " got "
        << apache::thrift::util::enumNameSafe(*mediaInterfaces[i].code());
  }
}

// Bring a transceiver out of low power mode and confirm it reaches ready state.
void bringModuleToReady(CmisModule* cmisModule, int tcvrId) {
  cmisModule->setModuleLowPowerModeLocked();
  cmisModule->releaseModuleLowPowerModeLocked();
  ASSERT_TRUE(cmisModule->moduleReadyStatePoll())
      << "Transceiver " << tcvrId
      << " did not reach ready state after releasing low power";
}

bool isModeSupported(
    QsfpModule* module,
    const HalTestConfig& config,
    TcvrOperationalMode mode) {
  auto mediaInterface = module->getModuleMediaInterface();
  const auto& mediaConfigs = hal_test::getMediaInterfaceConfigs(config);
  auto it = mediaConfigs.find(mediaInterface);
  if (it == mediaConfigs.end()) {
    return false;
  }
  const auto& supportedModes = *it->second.supportedModes();
  return std::find(supportedModes.begin(), supportedModes.end(), mode) !=
      supportedModes.end();
}

} // namespace

// Parameterized test fixture for per-mode tests that program a transceiver
// with a hard reset before each mode change. Each test skips transceivers that
// don't support the mode and skips entirely if no transceiver supports it.

class T1HalTestProgramMode
    : public T1HalTest,
      public ::testing::WithParamInterface<TcvrOperationalMode> {};

TEST_P(T1HalTestProgramMode, programMode) {
  auto mode = GetParam();
  bool tested = false;
  for (auto tcvrId : getPresentTransceiverIds()) {
    auto* module = getModule(tcvrId);
    auto* cmisModule = dynamic_cast<CmisModule*>(module);
    ASSERT_NE(cmisModule, nullptr)
        << "Transceiver " << tcvrId << " is not CMIS";
    module->refresh();
    if (!isModeSupported(module, getConfig(), mode)) {
      XLOG(INFO) << "Transceiver " << tcvrId << " does not support mode "
                 << apache::thrift::util::enumNameSafe(mode) << ", skipping";
      continue;
    }
    tested = true;
    getImpl(tcvrId)->triggerQsfpHardReset();
    module->detectPresence();
    bringModuleToReady(cmisModule, tcvrId);
    module->refresh();
    auto programState = hal_test::createProgramTransceiverState(mode);
    module->programTransceiver(programState, true);
    verifyMediaInterfaceCodes(module, tcvrId, mode);
  }
  if (!tested) {
    GTEST_SKIP() << "No transceiver supports mode "
                 << apache::thrift::util::enumNameSafe(mode);
  }
}

INSTANTIATE_TEST_SUITE_P(
    T1ProgramMode,
    T1HalTestProgramMode,
    testing::ValuesIn(apache::thrift::TEnumTraits<TcvrOperationalMode>::values),
    [](const testing::TestParamInfo<TcvrOperationalMode>& info) {
      return std::string(apache::thrift::util::enumNameSafe(info.param));
    });

// Parameterized test fixture for speed-change transitions. Transitions are
// defined in the thrift config (speedChangeTransitions). At static init time
// we use DEFAULT_MEDIA_INTERFACE_CONFIGS; at runtime the test body checks
// the actual config via isSpeedChangeSupportedForModule().

class T2HalTestSpeedChange
    : public T2HalTest,
      public ::testing::WithParamInterface<
          std::pair<TcvrOperationalMode, TcvrOperationalMode>> {};

TEST_P(T2HalTestSpeedChange, speedChange) {
  auto [fromMode, toMode] = GetParam();
  bool tested = false;
  for (auto tcvrId : getPresentTransceiverIds()) {
    auto* module = getModule(tcvrId);
    auto* cmisModule = dynamic_cast<CmisModule*>(module);
    ASSERT_NE(cmisModule, nullptr)
        << "Transceiver " << tcvrId << " is not CMIS";
    module->refresh();
    if (!hal_test::isSpeedChangeSupportedForModule(
            module, getConfig(), fromMode, toMode)) {
      XLOG(INFO) << "Transceiver " << tcvrId << " does not support transition "
                 << apache::thrift::util::enumNameSafe(fromMode) << " -> "
                 << apache::thrift::util::enumNameSafe(toMode) << ", skipping";
      continue;
    }
    tested = true;
    auto fromState = hal_test::createProgramTransceiverState(fromMode);
    module->programTransceiver(fromState, true);
    verifyMediaInterfaceCodes(module, tcvrId, fromMode);
    auto toState = hal_test::createProgramTransceiverState(toMode);
    module->programTransceiver(toState, true);
    verifyMediaInterfaceCodes(module, tcvrId, toMode);
    module->programTransceiver(fromState, true);
    verifyMediaInterfaceCodes(module, tcvrId, fromMode);
  }
  if (!tested) {
    GTEST_SKIP() << "No transceiver supports transition "
                 << apache::thrift::util::enumNameSafe(fromMode) << " -> "
                 << apache::thrift::util::enumNameSafe(toMode);
  }
}

INSTANTIATE_TEST_SUITE_P(
    T2SpeedChange,
    T2HalTestSpeedChange,
    testing::ValuesIn(
        hal_test::getAllSpeedChangeTransitions(
            hal_test_config_constants::DEFAULT_MEDIA_INTERFACE_CONFIGS())),
    [](const testing::TestParamInfo<
        std::pair<TcvrOperationalMode, TcvrOperationalMode>>& info) {
      return std::string(apache::thrift::util::enumNameSafe(info.param.first)) +
          "_to_" +
          std::string(apache::thrift::util::enumNameSafe(info.param.second));
    });

} // namespace facebook::fboss
