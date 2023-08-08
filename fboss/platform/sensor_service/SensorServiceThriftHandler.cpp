/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
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
#include "fboss/platform/sensor_service/Utils.h"

namespace facebook::fboss::platform::sensor_service {

void SensorServiceThriftHandler::getSensorValuesByNames(
    SensorReadResponse& response,
    std::unique_ptr<std::vector<std::string>> request) {
  auto log = LOG_THRIFT_CALL(DBG1);

  response.timeStamp() = Utils::nowInSecs();

  // Request list is not empty
  if (!request->empty()) {
    std::vector<SensorData> v;

    if (request->size() == 1) {
      // Request is for a single sensor
      std::optional<SensorData> sensor =
          sensorServiceImpl_->getSensorData(request->at(0));
      if (sensor) {
        SensorData sa;
        sa.name() = request->at(0);
        sa.value() = *sensor->value();
        sa.timeStamp() = *sensor->timeStamp();
        v.push_back(sa);
      }
    } else {
      v = sensorServiceImpl_->getSensorsData(*request);
    }
    response.sensorData() = v;

  } else {
    // Request list is empty, we send all the sensor data
    std::vector<SensorData> sensorVec;
    for (const auto& sensorDataItr : sensorServiceImpl_->getAllSensorData()) {
      sensorVec.push_back(sensorDataItr.second);
    }
    response.sensorData() = sensorVec;
  }
}

void SensorServiceThriftHandler::getSensorValuesByFruTypes(
    SensorReadResponse& /*response*/,
    std::unique_ptr<std::vector<FruType>> /*request*/) {
  auto log = LOG_THRIFT_CALL(DBG1);
  // ToDo: implement here
}
} // namespace facebook::fboss::platform::sensor_service
