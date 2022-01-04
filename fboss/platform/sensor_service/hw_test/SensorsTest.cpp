/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/sensor_service/hw_test/SensorsTest.h"

#include <folly/experimental/coro/BlockingWait.h>
#include "thrift/lib/cpp2/server/ThriftServer.h"

#include "common/services/cpp/ServiceFrameworkLight.h"
#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include "fboss/platform/sensor_service/SetupThrift.h"

namespace facebook::fboss::platform::sensor_service {

SensorsTest::~SensorsTest() {}

void SensorsTest::SetUp() {
  std::tie(thriftServer_, thriftHandler_) = setupThrift();
  service_ =
      std::make_unique<services::ServiceFrameworkLight>("Sensor service test");
  runServer(
      *service_, thriftServer_, thriftHandler_.get(), false /*loop forever*/);
}
void SensorsTest::TearDown() {
  service_->stop();
  service_->waitForStop();
  service_.reset();
  thriftServer_.reset();
  thriftHandler_.reset();
}

SensorReadResponse SensorsTest::getSensors(
    const std::vector<std::string>& sensors) {
  return *(folly::coro::blockingWait(thriftHandler_->co_getSensorValuesByNames(
      std::make_unique<std::vector<std::string>>(sensors))));
}

SensorServiceImpl* SensorsTest::getService() {
  return thriftHandler_->getServiceImpl();
}

TEST_F(SensorsTest, getAllSensors) {
  getService()->getAllSensorData();
}

TEST_F(SensorsTest, getBogusSensor) {
  EXPECT_EQ(getSensors({"bogusSensor_foo"}).sensorData_ref()->size(), 0);
}

TEST_F(SensorsTest, getSomeSensors) {
  EXPECT_EQ(getSensors({"PCH_TEMP"}).sensorData_ref()->size(), 1);
}

TEST_F(SensorsTest, getSensorsByFruTypes) {
  std::vector<FruType> fruTypes{FruType::ALL};
  auto response =
      *(folly::coro::blockingWait(thriftHandler_->co_getSensorValuesByFruTypes(
          std::make_unique<std::vector<FruType>>(fruTypes))));
  // TODO assert for non empty response once this thrift API is implemented
  EXPECT_EQ(response.sensorData_ref()->size(), 0);
}
} // namespace facebook::fboss::platform::sensor_service
