/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <fb303/ServiceData.h>
#include <fmt/format.h>
#include <folly/FileUtil.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/data_corral_service/DataCorralServiceThriftHandler.h"
#include "fboss/platform/data_corral_service/FruPresenceExplorer.h"
#include "fboss/platform/data_corral_service/if/gen-cpp2/DataCorralServiceThrift.h"
#include "fboss/platform/data_corral_service/if/gen-cpp2/data_corral_service_types.h"
#include "fboss/platform/helpers/Init.h"

using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::data_corral_service;

// When running OSS, users must supply config_file unlike in fb environment.
// getUncachedFruid test, DataCorral talks to WeUtil to parse eeprom
// content. Sincne config_file flag only store a single file, WeUtil will
// end up deserializing led_manger.json...
// To avoid this different user expectations, we could:
// 1) Figure out how to avoid passing config files in OSS.
// 2) See whether this can be avoided when we merge DataCorral into PM.
DEFINE_string(
    led_manager_config_file,
    "",
    "[OSS-Only] Path to led_manager.json file used for DataCorral service configuration during HwTest.");
DEFINE_string(
    weutil_config_file,
    "",
    "[OSS-Only] Path to weutil.json file used for weutil configuration during HWTest. "
    "This will overwrite FLAGS_config_file when weutil initializes the config.");

class DataCorralServiceHwTest : public ::testing::Test {
 public:
  void SetUp() override {
    thriftHandler_ = std::make_shared<DataCorralServiceThriftHandler>();

    std::string jsonLedManagerConfig;
    if (FLAGS_led_manager_config_file.empty()) {
      jsonLedManagerConfig = ConfigLib().getLedManagerConfig();
    } else {
      folly::readFile(
          FLAGS_led_manager_config_file.c_str(), jsonLedManagerConfig);
    }
    if (!FLAGS_weutil_config_file.empty()) {
      FLAGS_config_file = FLAGS_weutil_config_file;
    }
    apache::thrift::SimpleJSONSerializer::deserialize<LedManagerConfig>(
        jsonLedManagerConfig, ledManagerConfig_);

    auto ledManager = std::make_shared<LedManager>(
        *ledManagerConfig_.systemLedConfig(),
        *ledManagerConfig_.fruTypeLedConfigs());
    fruPresenceExplorer_ = std::make_shared<FruPresenceExplorer>(
        *ledManagerConfig_.fruConfigs(), ledManager);
  }

  void TearDown() override {}

 protected:
  DataCorralFruidReadResponse getFruid(bool uncached) {
    auto client = apache::thrift::makeTestClient(thriftHandler_);
    DataCorralFruidReadResponse resp;
    client->sync_getFruid(resp, uncached);
    return resp;
  }

  std::shared_ptr<FruPresenceExplorer> fruPresenceExplorer_;
  std::shared_ptr<DataCorralServiceThriftHandler> thriftHandler_;
  LedManagerConfig ledManagerConfig_;
};

TEST_F(DataCorralServiceHwTest, FruLedProgrammingSysfsCheck) {
  fruPresenceExplorer_->detectFruPresence();

  std::string val;
  for (const auto& [fruType, ledConfig] :
       *ledManagerConfig_.fruTypeLedConfigs()) {
    auto fruTypePresence = fruPresenceExplorer_->isPresent(fruType);
    folly::readFile(ledConfig.presentLedSysfsPath()->c_str(), val);
    EXPECT_EQ(folly::to<int>(val), fruTypePresence);
    folly::readFile(ledConfig.absentLedSysfsPath()->c_str(), val);
    EXPECT_EQ(folly::to<int>(val), !fruTypePresence);
  }
  auto systemPresence = fruPresenceExplorer_->isAllPresent();
  folly::readFile(
      ledManagerConfig_.systemLedConfig()->presentLedSysfsPath()->c_str(), val);
  EXPECT_EQ(folly::to<int>(val), systemPresence);
  folly::readFile(
      ledManagerConfig_.systemLedConfig()->absentLedSysfsPath()->c_str(), val);
  EXPECT_EQ(folly::to<int>(val), !systemPresence);
}

TEST_F(DataCorralServiceHwTest, FruLEDProgrammingODSCheck) {
  fruPresenceExplorer_->detectFruPresence();

  for (const auto& [fruType, ledConfig] :
       *ledManagerConfig_.fruTypeLedConfigs()) {
    EXPECT_EQ(
        facebook::fb303::fbData->getCounter(
            fmt::format("fru_presence_explorer.{}.presence", fruType)),
        fruPresenceExplorer_->isPresent(fruType));
    EXPECT_EQ(
        facebook::fb303::fbData->getCounter(
            fmt::format("led_manager.{}.program_led_fail", fruType)),
        0);
  }
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          "fru_presence_explorer.SYSTEM.presence"),
      fruPresenceExplorer_->isAllPresent());
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          "led_manager.SYSTEM.program_led_fail"),
      0);
}

TEST_F(DataCorralServiceHwTest, getCachedFruid) {
  EXPECT_GT(getFruid(false).fruidData()->size(), 0);
}

TEST_F(DataCorralServiceHwTest, getUncachedFruid) {
  EXPECT_GT(getFruid(true).fruidData()->size(), 0);
}

TEST_F(DataCorralServiceHwTest, testThrift) {
  apache::thrift::ScopedServerInterfaceThread server(thriftHandler_);
  auto client =
      server.newClient<apache::thrift::Client<DataCorralServiceThrift>>();
  DataCorralFruidReadResponse response;
  client->sync_getFruid(response, false);
  EXPECT_GT(response.fruidData()->size(), 0);
}

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  if (!FLAGS_config_file.empty()) {
    XLOG(ERR)
        << "Please use --led_manager_config_file and --weutil_config_file instead of --config_file. "
        << "For more info on the flags, run with --helpon=DataCorralServiceHwTest";
    return -1;
  }
#ifdef IS_OSS
  if (FLAGS_led_manager_config_file.empty() ||
      FLAGS_weutil_config_file.empty()) {
    XLOG(ERR)
        << "Please specify --led_manager_config_file and --weutil_config_file. "
        << "For more info on the flags, run with --helpon=DataCorralServiceHwTest";
    return -1;
  }
#endif
  // Run the tests
  return RUN_ALL_TESTS();
}
