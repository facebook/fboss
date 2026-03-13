// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/hal_test/HalTest.h"

#include <folly/logging/xlog.h>

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

} // namespace facebook::fboss
