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
    response.sensorData() = sensorServiceImpl_->getSensorsData(*request);
    return;
  }

  // Request list is empty, we send all the sensor data
  for (const auto& sensorDataItr : sensorServiceImpl_->getAllSensorData()) {
    response.sensorData()->push_back(sensorDataItr.second);
  }
}

} // namespace facebook::fboss::platform::sensor_service
