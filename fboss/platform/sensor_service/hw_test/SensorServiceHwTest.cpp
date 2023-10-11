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
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/sensor_service/Flags.h"

using namespace apache::thrift;

namespace facebook::fboss::platform::sensor_service {

SensorServiceHwTest::~SensorServiceHwTest() {}

void SensorServiceHwTest::SetUp() {
  thriftHandler_ = std::make_shared<SensorServiceThriftHandler>(
      std::make_shared<SensorServiceImpl>(FLAGS_config_file));
}

void SensorServiceHwTest::TearDown() {}

SensorReadResponse SensorServiceHwTest::getSensors(
    const std::vector<std::string>& sensors) {
  SensorReadResponse response;
  thriftHandler_->getSensorValuesByNames(
      response, std::make_unique<std::vector<std::string>>(sensors));
  return response;
}

SensorServiceImpl* SensorServiceHwTest::getService() {
  return thriftHandler_->getServiceImpl();
}

TEST_F(SensorServiceHwTest, getAllSensors) {
  getService()->getAllSensorData();
}

TEST_F(SensorServiceHwTest, getBogusSensor) {
  EXPECT_EQ(getSensors({"bogusSensor_foo"}).sensorData()->size(), 0);
}

TEST_F(SensorServiceHwTest, getSomeSensors) {
  auto response1 = getSensors({"PCH_TEMP"});
  EXPECT_EQ(response1.sensorData()->size(), 1);
  // Burn a second
  std::this_thread::sleep_for(std::chrono::seconds(1));
  // Refresh sensors
  getService()->fetchSensorData();
  auto response2 = getSensors({"PCH_TEMP"});
  EXPECT_EQ(response2.sensorData()->size(), 1);
  // Response2 sensor collection time stamp should be later
  EXPECT_GT(*response2.timeStamp(), *response1.timeStamp());
}

TEST_F(SensorServiceHwTest, getSensorsByFruTypes) {
  std::vector<FruType> fruTypes{FruType::ALL};
  SensorReadResponse response;
  thriftHandler_->getSensorValuesByFruTypes(
      response, std::make_unique<std::vector<FruType>>(fruTypes));
  // TODO assert for non empty response once this thrift API is implemented
  EXPECT_EQ(response.sensorData()->size(), 0);
}

TEST_F(SensorServiceHwTest, testThrift) {
  auto server = std::make_unique<ScopedServerInterfaceThread>(
      thriftHandler_, folly::SocketAddress("::1", FLAGS_thrift_port));
  auto resolverEvb = std::make_unique<folly::EventBase>();
  auto clientPtr =
      server->newClient<apache::thrift::Client<SensorServiceThrift>>(
          resolverEvb.get());
  SensorReadResponse response;
  clientPtr->sync_getSensorValuesByNames(response, {"PCH_TEMP"});
  EXPECT_EQ(response.sensorData()->size(), 1);
}
} // namespace facebook::fboss::platform::sensor_service

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(argc, argv);

  // Run the tests
  return RUN_ALL_TESTS();
}
