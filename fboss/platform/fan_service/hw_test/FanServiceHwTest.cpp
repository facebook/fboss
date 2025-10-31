/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <fmt/format.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fan_service/Bsp.h"
#include "fboss/platform/fan_service/FanServiceHandler.h"
#include "fboss/platform/helpers/Init.h"

using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::fan_service;

namespace facebook::fboss::platform::fan_service {
class FanServiceHwTest : public ::testing::Test {
 public:
  void SetUp() override {
    std::string fanServiceConfJson = ConfigLib().getFanServiceConfig();
    fanServiceConfig_ =
        apache::thrift::SimpleJSONSerializer::deserialize<FanServiceConfig>(
            fanServiceConfJson);
    auto pBsp = std::make_shared<Bsp>(fanServiceConfig_);
    EXPECT_NO_THROW(
        controlLogic_ =
            std::make_unique<ControlLogic>(fanServiceConfig_, pBsp));
    EXPECT_FALSE(fanServiceConfig_.fans()->empty());
  }

  std::unique_ptr<ControlLogic> controlLogic_;
  FanServiceConfig fanServiceConfig_;
};

TEST_F(FanServiceHwTest, TransitionalPWM) {
  auto fanStatuses = controlLogic_->getFanStatuses();

  for (const auto& fan : *fanServiceConfig_.fans()) {
    auto fanName = *fan.fanName();
    FanStatus fanStatus;
    EXPECT_NO_THROW(fanStatus = fanStatuses.at(fanName));
    EXPECT_FALSE(*fanStatus.fanFailed());
    EXPECT_EQ(
        *fanStatus.pwmToProgram(), *fanServiceConfig_.pwmTransitionValue());
    EXPECT_FALSE(fanStatus.rpm().has_value());
  }
}

TEST_F(FanServiceHwTest, FanControl) {
  int run = 1;
  do {
    EXPECT_NO_THROW(controlLogic_->controlFan());

    auto fanStatuses = controlLogic_->getFanStatuses();
    for (const auto& fan : *fanServiceConfig_.fans()) {
      auto fanName = *fan.fanName();
      FanStatus fanStatus;
      EXPECT_NO_THROW(fanStatus = fanStatuses.at(fanName));
      EXPECT_FALSE(*fanStatus.fanFailed());
      EXPECT_GT(*fanStatus.pwmToProgram(), 0);
      EXPECT_GT(*fanStatus.rpm(), 0);
    }
  } while (++run < 10);
}

TEST_F(FanServiceHwTest, FanStatusesThrift) {
  controlLogic_->controlFan();

  auto fanServiceHandler =
      std::make_shared<FanServiceHandler>(std::move(controlLogic_));
  apache::thrift::ScopedServerInterfaceThread server(fanServiceHandler);
  auto client = server.newClient<apache::thrift::Client<FanService>>();

  FanStatusesResponse resp;
  client->sync_getFanStatuses(resp);
  EXPECT_FALSE(resp.fanStatuses()->empty());
  for (const auto& fan : *fanServiceConfig_.fans()) {
    auto fanName = *fan.fanName();
    FanStatus fanStatus;
    EXPECT_NO_THROW(fanStatus = resp.fanStatuses()->at(fanName));
    EXPECT_FALSE(*fanStatus.fanFailed());
    EXPECT_GT(*fanStatus.pwmToProgram(), 0);
    EXPECT_GT(*fanStatus.rpm(), 0);
    EXPECT_GT(*fanStatus.lastSuccessfulAccessTime(), 0);
  }
}

TEST_F(FanServiceHwTest, ODSCounters) {
  controlLogic_->controlFan();

  for (const auto& zone : *fanServiceConfig_.zones()) {
    for (const auto& fan : *fanServiceConfig_.fans()) {
      if (std::find(
              zone.fanNames()->begin(),
              zone.fanNames()->end(),
              *fan.fanName()) == zone.fanNames()->end()) {
        continue;
      }
      EXPECT_EQ(
          fb303::fbData->getCounter(
              fmt::format(
                  "{}.{}.pwm_write.failure", *zone.zoneName(), *fan.fanName())),
          0);
      EXPECT_EQ(
          fb303::fbData->getCounter(
              fmt::format("{}.rpm_read.failure", *fan.fanName())),
          0);
      EXPECT_EQ(
          fb303::fbData->getCounter(fmt::format("{}.absent", *fan.fanName())),
          0);
    }
  }
}

TEST_F(FanServiceHwTest, LedWriteDidNotFail) {
  controlLogic_->controlFan();

  for (const auto& fan : *fanServiceConfig_.fans()) {
    if (fan.goodLedSysfsPath().has_value() ||
        fan.failLedSysfsPath().has_value()) {
      EXPECT_EQ(
          fb303::fbData->getCounter(
              fmt::format("{}.led_write.failure", *fan.fanName())),
          0);
    }
  }
}

} // namespace facebook::fboss::platform::fan_service

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
