/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/sensor_service/hw_test/SensorServiceHwTest.h"

#include <folly/init/Init.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/sensor_service/SensorStatsPub.h"

using namespace apache::thrift;

namespace facebook::fboss::platform::sensor_service {

SensorServiceHwTest::~SensorServiceHwTest() = default;

void SensorServiceHwTest::SetUp() {
  sensorServiceImpl_ = std::make_shared<SensorServiceImpl>("");
  sensorServiceHandler_ =
      std::make_shared<SensorServiceThriftHandler>(sensorServiceImpl_);

  auto sensorServiceConfigJson = ConfigLib().getSensorServiceConfig();
  apache::thrift::SimpleJSONSerializer::deserialize<SensorConfig>(
      sensorServiceConfigJson, sensorConfig_);
}

SensorReadResponse SensorServiceHwTest::getSensors(
    const std::vector<std::string>& sensors) {
  sensorServiceImpl_->fetchSensorData();

  SensorReadResponse response;
  sensorServiceHandler_->getSensorValuesByNames(
      response, std::make_unique<std::vector<std::string>>(sensors));
  return response;
}

TEST_F(SensorServiceHwTest, GetAllSensors) {
  auto res = getSensors(std::vector<std::string>{});
  for (const auto& [fruName, sensorMap] : *sensorConfig_.sensorMapList()) {
    for (const auto& [sensorName, sensor] : sensorMap) {
      auto it = std::find_if(
          res.sensorData()->begin(),
          res.sensorData()->end(),
          [sensorNameCopy = sensorName](auto sensorData) {
            return *sensorData.name() == sensorNameCopy;
          });
      EXPECT_NE(it, std::end(*res.sensorData()));
      EXPECT_TRUE(it->value().has_value());
    }
  }
}

TEST_F(SensorServiceHwTest, GetBogusSensor) {
  EXPECT_EQ(getSensors({"bogusSensor_foo"}).sensorData()->size(), 0);
}

TEST_F(SensorServiceHwTest, GetSomeSensors) {
  std::vector<std::string> sensorNames;
  for (const auto& [fruName, sensorMap] : *sensorConfig_.sensorMapList()) {
    sensorNames.push_back(sensorMap.begin()->first);
  }

  auto response1 = getSensors(sensorNames);
  EXPECT_EQ(response1.sensorData()->size(), sensorNames.size());
  for (const auto& sensorData : *response1.sensorData()) {
    EXPECT_TRUE(sensorData.value().has_value());
  }

  // Burn a second
  std::this_thread::sleep_for(std::chrono::seconds(1));

  auto response2 = getSensors(sensorNames);
  EXPECT_EQ(response2.sensorData()->size(), sensorNames.size());
  for (const auto& sensorData : *response2.sensorData()) {
    EXPECT_TRUE(sensorData.value().has_value());
  }

  // Response2 sensor collection time stamp should be later
  EXPECT_GT(*response2.timeStamp(), *response1.timeStamp());

  // Check individual sensor reads happen after the previous reads.
  int total = 0;
  int valid = 0;
  for (const auto& sensorData : *response2.sensorData()) {
    auto it = std::find_if(
        response1.sensorData()->begin(),
        response1.sensorData()->end(),
        [sensorName = *sensorData.name()](auto sensorData) {
          return *sensorData.name() == sensorName;
        });
    if (*sensorData.timeStamp() > *it->timeStamp()) {
      valid++;
    }
    total++;
  }
  EXPECT_GT((float)valid / total, 0.9);
}

TEST_F(SensorServiceHwTest, GetSensorsByFruTypes) {
  std::vector<FruType> fruTypes{FruType::ALL};
  SensorReadResponse response;
  sensorServiceHandler_->getSensorValuesByFruTypes(
      response, std::make_unique<std::vector<FruType>>(fruTypes));
  // TODO assert for non empty response once this thrift API is implemented
  EXPECT_EQ(response.sensorData()->size(), 0);
}

TEST_F(SensorServiceHwTest, GetSomeSensorsViaThrift) {
  apache::thrift::ScopedServerInterfaceThread server(sensorServiceHandler_);
  auto client = server.newClient<apache::thrift::Client<SensorServiceThrift>>();
  SensorReadResponse response;
  client->sync_getSensorValuesByNames(response, {"PCH_TEMP"});
  EXPECT_EQ(response.sensorData()->size(), 1);
}

TEST_F(SensorServiceHwTest, SensorFetchODSCheck) {
  sensorServiceImpl_->fetchSensorData();
  EXPECT_EQ(fb303::fbData->getCounter("sensor_read.total.failures"), 0);
  EXPECT_EQ(fb303::fbData->getCounter("sensor_read.has.failures"), 0);
}

TEST_F(SensorServiceHwTest, PublishStats) {
  sensorServiceImpl_->fetchSensorData();

  SensorStatsPub publisher(sensorServiceImpl_.get());
  publisher.publishStats();

  auto sensorMap = sensorServiceImpl_->getAllSensorData();
  for (const auto& [sensorName, sensorData] : sensorMap) {
    EXPECT_EQ(
        fb303::fbData->getCounter(sensorName), (int64_t)*sensorData.value());
  }
}
} // namespace facebook::fboss::platform::sensor_service

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);

  // Run the tests
  return RUN_ALL_TESTS();
}
