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
#include "fboss/lib/LogThriftCall.h"
#include "fboss/platform/helpers/Utils.h"

namespace facebook::fboss::platform::sensor_service {

#if FOLLY_HAS_COROUTINES
folly::coro::Task<std::unique_ptr<SensorReadResponse>>
SensorServiceThriftHandler::co_getSensorValuesByNames(
    std::unique_ptr<std::vector<std::string>> request) {
  auto log = LOG_THRIFT_CALL(DBG1);
  auto response = std::make_unique<SensorReadResponse>();

  response->timeStamp_ref() = helpers::nowInSecs();

  // Request list is not empty
  if (!request->empty()) {
    std::vector<SensorData> v;

    if (request->size() == 1) {
      // Request is for a single sensor
      std::optional<SensorData> sensor =
          sensorService_->getSensorData(request->at(0));
      if (sensor) {
        SensorData sa;
        sa.name_ref() = request->at(0);
        sa.value_ref() = *sensor->value_ref();
        sa.timeStamp_ref() = *sensor->timeStamp_ref();
        v.push_back(sa);
      }
    } else {
      v = sensorService_->getSensorsData(*request);
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
#endif

} // namespace facebook::fboss::platform::sensor_service
