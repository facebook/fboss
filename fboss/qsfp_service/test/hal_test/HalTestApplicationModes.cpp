// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <algorithm>
#include <utility>

#include <fmt/core.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/test/hal_test/HalTest.h"
#include "fboss/qsfp_service/test/hal_test/HalTestUtils.h"

#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/test/hal_test/gen-cpp2/hal_test_config_constants.h"

namespace facebook::fboss {

namespace {

void verifyMediaInterfaceCodes(
    hal_test::TransceiverTestResult& result,
    QsfpModule* module,
    int tcvrId,
    TcvrOperationalMode mode) {
  module->refresh();
  auto expectedCodes = hal_test::getExpectedMediaInterfaceCodes(mode);

  auto info = module->getTransceiverInfo();
  HAL_CHECK_FATAL_VOID(
      result,
      info.tcvrState()->settings().has_value(),
      fmt::format("Transceiver {} has no settings", tcvrId));
  HAL_CHECK_FATAL_VOID(
      result,
      info.tcvrState()->settings()->mediaInterface().has_value(),
      fmt::format(
          "Transceiver {} has no mediaInterface after programming mode {}",
          tcvrId,
          apache::thrift::util::enumNameSafe(mode)));

  const auto& mediaInterfaces = *info.tcvrState()->settings()->mediaInterface();

  HAL_CHECK_FATAL_VOID(
      result,
      mediaInterfaces.size() == expectedCodes.size(),
      fmt::format(
          "Transceiver {} unexpected number of media interface lanes for mode {}: got {} expected {}",
          tcvrId,
          apache::thrift::util::enumNameSafe(mode),
          mediaInterfaces.size(),
          expectedCodes.size()));

  for (size_t i = 0; i < expectedCodes.size(); i++) {
    HAL_CHECK(
        result,
        *mediaInterfaces[i].code() == expectedCodes[i],
        fmt::format(
            "Transceiver {} lane {} media interface code mismatch for mode {}: expected {} got {}",
            tcvrId,
            i,
            apache::thrift::util::enumNameSafe(mode),
            apache::thrift::util::enumNameSafe(expectedCodes[i]),
            apache::thrift::util::enumNameSafe(*mediaInterfaces[i].code())));
  }
}

void bringModuleToReady(
    hal_test::TransceiverTestResult& result,
    CmisModule* cmisModule,
    int tcvrId) {
  cmisModule->setModuleLowPowerModeLocked();
  cmisModule->releaseModuleLowPowerModeLocked();
  HAL_CHECK_FATAL_VOID(
      result,
      cmisModule->moduleReadyStatePoll(),
      fmt::format(
          "Transceiver {} did not reach ready state after releasing low power",
          tcvrId));
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
  forEachTransceiverParallel(
      [this, mode](hal_test::TransceiverTestResult& result, int tcvrId) {
        auto* module = getModule(tcvrId);
        auto* cmisModule = dynamic_cast<CmisModule*>(module);
        HAL_CHECK_FATAL_VOID(
            result,
            cmisModule != nullptr,
            fmt::format("Transceiver {} is not CMIS", tcvrId));
        getImpl(tcvrId)->triggerQsfpHardReset();
        module->detectPresence();
        bringModuleToReady(result, cmisModule, tcvrId);
        if (!result.passed) {
          return;
        }
        module->refresh();
        auto programState = hal_test::createProgramTransceiverState(mode);
        module->programTransceiver(programState, true);
        verifyMediaInterfaceCodes(result, module, tcvrId, mode);
      },
      [this, mode](int tcvrId) {
        auto* module = getModule(tcvrId);
        module->refresh();
        if (!isModeSupported(module, getConfig(), mode)) {
          XLOG(INFO) << "Transceiver " << tcvrId << " does not support mode "
                     << apache::thrift::util::enumNameSafe(mode)
                     << ", skipping";
          return false;
        }
        return true;
      });
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
  forEachTransceiverParallel(
      [this, fromMode, toMode](
          hal_test::TransceiverTestResult& result, int tcvrId) {
        auto* module = getModule(tcvrId);
        auto fromState = hal_test::createProgramTransceiverState(fromMode);
        module->programTransceiver(fromState, true);
        verifyMediaInterfaceCodes(result, module, tcvrId, fromMode);
        if (!result.passed) {
          return;
        }
        auto toState = hal_test::createProgramTransceiverState(toMode);
        module->programTransceiver(toState, true);
        verifyMediaInterfaceCodes(result, module, tcvrId, toMode);
        if (!result.passed) {
          return;
        }
        module->programTransceiver(fromState, true);
        verifyMediaInterfaceCodes(result, module, tcvrId, fromMode);
      },
      [this, fromMode, toMode](int tcvrId) {
        auto* module = getModule(tcvrId);
        module->refresh();
        if (!hal_test::isSpeedChangeSupportedForModule(
                module, getConfig(), fromMode, toMode)) {
          XLOG(INFO) << "Transceiver " << tcvrId
                     << " does not support transition "
                     << apache::thrift::util::enumNameSafe(fromMode) << " -> "
                     << apache::thrift::util::enumNameSafe(toMode)
                     << ", skipping";
          return false;
        }
        return true;
      });
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
