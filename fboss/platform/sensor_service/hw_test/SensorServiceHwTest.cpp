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

using namespace apache::thrift;

namespace facebook::fboss::platform::sensor_service {

SensorServiceHwTest::~SensorServiceHwTest() = default;

void SensorServiceHwTest::SetUp() {
  sensorServiceImpl_ = std::make_shared<SensorServiceImpl>();
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

bool sensorReadOk(const std::string& sensorName) {
  if (fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kReadFailure, sensorName)) == 0) {
    return true;
  }
  return false;
}

TEST_F(SensorServiceHwTest, GetAllSensors) {
  auto res = getSensors(std::vector<std::string>{});
  std::vector<std::string> sensorNames;
  for (const auto& pmUnitSensors : *sensorConfig_.pmUnitSensorsList()) {
    for (const auto& sensor : *pmUnitSensors.sensors()) {
      sensorNames.push_back(*sensor.name());
    }
  }
  EXPECT_EQ(sensorNames.size(), res.sensorData()->size());
  for (const auto& sensorName : sensorNames) {
    auto it = std::find_if(
        res.sensorData()->begin(),
        res.sensorData()->end(),
        [sensorNameCopy = sensorName](auto sensorData) {
          return *sensorData.name() == sensorNameCopy;
        });
    EXPECT_NE(it, std::end(*res.sensorData()));
    // only non-failed sensors will have value
    if (sensorReadOk(sensorName)) {
      EXPECT_TRUE(it->value().has_value());
    }
  }
}

TEST_F(SensorServiceHwTest, GetBogusSensor) {
  EXPECT_EQ(getSensors({"bogusSensor_foo"}).sensorData()->size(), 0);
}

TEST_F(SensorServiceHwTest, GetSomeSensors) {
  std::vector<std::string> sensorNames;
  for (const auto& pmUnitSensors : *sensorConfig_.pmUnitSensorsList()) {
    if (pmUnitSensors.sensors()->size() > 0) {
      sensorNames.push_back(*pmUnitSensors.sensors()->front().name());
    }
  }

  auto response1 = getSensors(sensorNames);
  EXPECT_EQ(response1.sensorData()->size(), sensorNames.size());
  for (const auto& sensorData : *response1.sensorData()) {
    if (sensorReadOk(*sensorData.name())) {
      EXPECT_TRUE(sensorData.value().has_value());
    }
  }

  // Burn a second
  std::this_thread::sleep_for(std::chrono::seconds(1));

  auto response2 = getSensors(sensorNames);
  EXPECT_EQ(response2.sensorData()->size(), sensorNames.size());
  for (const auto& sensorData : *response2.sensorData()) {
    if (sensorReadOk(*sensorData.name())) {
      EXPECT_TRUE(sensorData.value().has_value());
    }
  }

  // Response2 sensor collection time stamp should be later
  EXPECT_GT(*response2.timeStamp(), *response1.timeStamp());

  // Check individual sensor reads happen after the previous reads.
  int total = 0;
  int valid = 0;
  for (const auto& sensorData : *response2.sensorData()) {
    if (!sensorData.value().has_value()) {
      continue;
    }
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

TEST_F(SensorServiceHwTest, GetSomeSensorsViaThrift) {
  std::vector<std::string> sensorNames;
  for (const auto& pmUnitSensors : *sensorConfig_.pmUnitSensorsList()) {
    if (pmUnitSensors.sensors()->size() > 0) {
      sensorNames.push_back(*pmUnitSensors.sensors()->front().name());
    }
  }
  // Trigger a fetch before the thrift request hits the server.
  sensorServiceImpl_->fetchSensorData();
  apache::thrift::ScopedServerInterfaceThread server(sensorServiceHandler_);
  auto client = server.newClient<apache::thrift::Client<SensorServiceThrift>>();
  SensorReadResponse response;
  client->sync_getSensorValuesByNames(response, sensorNames);
  EXPECT_EQ(response.sensorData()->size(), sensorNames.size());
  for (const auto& sensorData : *response.sensorData()) {
    if (sensorReadOk(*sensorData.name())) {
      EXPECT_TRUE(sensorData.value().has_value());
    }
  }
}

TEST_F(SensorServiceHwTest, SensorFetchODSCheck) {
  sensorServiceImpl_->fetchSensorData();
  auto sensorMap = sensorServiceImpl_->getAllSensorData();
  EXPECT_GT(fb303::fbData->getCounter(SensorServiceImpl::kReadTotal), 0);
  for (const auto& [sensorName, sensorData] : sensorMap) {
    if (!sensorData.value().has_value()) {
      continue;
    }
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(SensorServiceImpl::kReadValue, sensorName)),
        (int64_t)*sensorData.value());
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(SensorServiceImpl::kReadFailure, sensorName)),
        0);
  }
}

// This will test all sensors. If any sensors failed or not responding,
// this test will fail.
TEST_F(SensorServiceHwTest, CheckAllSensors) {
  sensorServiceImpl_->fetchSensorData();
  EXPECT_EQ(fb303::fbData->getCounter(SensorServiceImpl::kHasReadFailure), 0);
  EXPECT_EQ(fb303::fbData->getCounter(SensorServiceImpl::kTotalReadFailure), 0);
  auto sensorMap = sensorServiceImpl_->getAllSensorData();
  EXPECT_GT(fb303::fbData->getCounter(SensorServiceImpl::kReadTotal), 0);
  for (const auto& [sensorName, sensorData] : sensorMap) {
    auto hasValue = sensorData.value().has_value();
    EXPECT_TRUE(hasValue) << "Sensor " << sensorName << " has no value";
    if (!hasValue) {
      // To avoid exception Below, we will not compare the value against
      // fb303 counter.
      continue;
    }
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(SensorServiceImpl::kReadValue, sensorName)),
        (int64_t)*sensorData.value());
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(SensorServiceImpl::kReadFailure, sensorName)),
        0);
  }
}
} // namespace facebook::fboss::platform::sensor_service

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
