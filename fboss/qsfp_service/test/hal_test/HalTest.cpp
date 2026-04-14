// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/hal_test/HalTest.h"

#include <folly/logging/xlog.h>

#include <algorithm>

#include "fboss/agent/FbossError.h"

DEFINE_string(hal_test_config, "", "Path to HAL test config JSON file");

namespace facebook::fboss {

void HalTest::SetUpTestSuite() {
  if (FLAGS_hal_test_config.empty()) {
    throw FbossError("--hal_test_config flag is required");
  }

  config_ = hal_test::loadHalTestConfig(FLAGS_hal_test_config);

  // Ensure we refresh all pages every time
  gflags::SetCommandLineOptionWithMode(
      "refresh_all_pages_cycles", "1", gflags::SET_FLAGS_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "qsfp_data_refresh_interval", "0", gflags::SET_FLAGS_DEFAULT);

  modules_ = hal_test::createAllQsfpModules(config_);
  XLOG(INFO) << "Created " << modules_.size() << " QsfpModule(s)";
}

void HalTest::TearDownTestSuite() {
  modules_.clear();
}

void HalTest::SetUp() {
  ASSERT_FALSE(modules_.empty()) << "No transceiver modules configured";

  // Hard reset all present transceivers to ensure a clean starting state
  for (auto tcvrId : getPresentTransceiverIds()) {
    XLOG(INFO) << "Hard resetting transceiver " << tcvrId;
    getImpl(tcvrId)->triggerQsfpHardReset();
  }
  /* sleep override */
  sleep(2);

  // Detect all transceivers
  for (auto tcvrId : getPresentTransceiverIds()) {
    XLOG(INFO) << "Detecting transceiver " << tcvrId;
    getModule(tcvrId)->detectPresence();
  }

  if (shouldApplyStartupFirmware()) {
    int upgraded = hal_test::applyStartupFirmwareUpgrades(config_, modules_);
    if (upgraded > 0) {
      XLOG(INFO) << "Upgraded firmware on " << upgraded << " module(s)";
    }
  }
}

QsfpModule* HalTest::getModule(int tcvrId) const {
  auto it = modules_.find(tcvrId);
  if (it == modules_.end()) {
    throw FbossError("No module found for transceiver ID: ", tcvrId);
  }
  return it->second.module.get();
}

BspTransceiverImpl* HalTest::getImpl(int tcvrId) const {
  auto it = modules_.find(tcvrId);
  if (it == modules_.end()) {
    throw FbossError("No impl found for transceiver ID: ", tcvrId);
  }
  return it->second.impl.get();
}

const HalTestConfig& HalTest::getConfig() const {
  return config_;
}

std::vector<int> HalTest::getPresentTransceiverIds() const {
  std::vector<int> presentIds;
  for (const auto& [id, halModule] : modules_) {
    if (halModule.impl->detectTransceiver()) {
      presentIds.push_back(id);
    }
  }
  return presentIds;
}

void HalTest::forEachTransceiverParallel(
    std::function<void(hal_test::TransceiverTestResult&, int)> fn,
    std::function<bool(int)> filter) {
  auto tcvrIds = getPresentTransceiverIds();
  XLOG(INFO) << "forEachTransceiverParallel: running for " << tcvrIds.size()
             << " present transceiver(s)";

  auto results = hal_test::runForAllTransceivers(tcvrIds, [&](int tcvrId) {
    hal_test::TransceiverTestResult result;
    result.tcvrId = tcvrId;
    if (filter && !filter(tcvrId)) {
      XLOG(INFO) << "Transceiver " << tcvrId << ": skipped by filter";
      result.skipped = true;
      return result;
    }
    XLOG(INFO) << "Transceiver " << tcvrId << ": starting test";
    fn(result, tcvrId);
    XLOG(INFO) << "Transceiver " << tcvrId
               << ": finished (passed=" << result.passed << ")";
    return result;
  });

  int skipped = 0;
  int passed = 0;
  int failed = 0;
  for (const auto& r : results) {
    if (r.skipped) {
      ++skipped;
    } else if (r.passed) {
      ++passed;
    } else {
      ++failed;
      for (const auto& f : r.failures) {
        XLOG(ERR) << "Transceiver " << r.tcvrId << ": " << f;
      }
      if (!r.fatalFailure.empty()) {
        XLOG(ERR) << "Transceiver " << r.tcvrId
                  << " (fatal): " << r.fatalFailure;
      }
    }
  }
  XLOG(INFO) << "forEachTransceiverParallel: " << passed << " passed, "
             << failed << " failed, " << skipped << " skipped";

  if (skipped == static_cast<int>(results.size())) {
    GTEST_SKIP() << "No transceiver matched filter";
  }

  hal_test::reportTransceiverResults(results);
}

} // namespace facebook::fboss
