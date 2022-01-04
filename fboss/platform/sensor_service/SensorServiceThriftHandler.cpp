/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include <folly/logging/xlog.h>
#include "common/time/Time.h"
#include "fboss/lib/LogThriftCall.h"

namespace facebook::fboss::platform::sensor_service {

folly::coro::Task<std::unique_ptr<SensorReadResponse>>
SensorServiceThriftHandler::co_getSensorValuesByNames(
    std::unique_ptr<std::vector<std::string>> request) {
  auto log = LOG_THRIFT_CALL(DBG1);
  auto response = std::make_unique<SensorReadResponse>();

  response->timeStamp_ref() = facebook::WallClockUtil::NowInSecFast();

  // Request list is not empty
  if (!request->empty()) {
    std::vector<SensorData> v;
    for (auto it : *request) {
      SensorData sa;
      std::optional<SensorData> sensor = sensorService_->getSensorData(it);

      if (sensor) {
        sa.name_ref() = it;
        sa.value_ref() = *sensor->value_ref();
        sa.timeStamp_ref() = *sensor->timeStamp_ref();
        v.push_back(sa);
      }
    }
    response->sensorData_ref() = v;

  } else {
    // Request list is empty, we send all the sensor data
    std::vector<SensorData> sensorVec = sensorService_->getAllSensorData();

    response->sensorData_ref() = sensorVec;
  }
  co_return response;
}

folly::coro::Task<std::unique_ptr<SensorReadResponse>>
SensorServiceThriftHandler::co_getSensorValuesByFruTypes(
    std::unique_ptr<std::vector<FruType>> request) {
  auto log = LOG_THRIFT_CALL(DBG1);
  auto response = std::make_unique<SensorReadResponse>();
  // ToDo: implement here
  co_return response;
}

} // namespace facebook::fboss::platform::sensor_service
