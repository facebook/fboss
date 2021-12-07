/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/SensorServiceThrift.h"

#include <memory>
#include <string>
#include <vector>

namespace facebook::fboss::platform::sensor_service {

class SensorServiceThriftHandler : public SensorServiceThriftSvIf {
 public:
  explicit SensorServiceThriftHandler(
      std::shared_ptr<SensorServiceImpl> sensorService)
      : sensorService_(sensorService) {}

  folly::coro::Task<std::unique_ptr<SensorReadResponse>>
  co_getSensorValuesByNames(
      std::unique_ptr<std::vector<std::string>> request) override;

  folly::coro::Task<std::unique_ptr<SensorReadResponse>>
  co_getSensorValuesByFruTypes(
      std::unique_ptr<std::vector<FruType>> request) override;

 private:
  std::shared_ptr<SensorServiceImpl> sensorService_;
};
} // namespace facebook::fboss::platform::sensor_service
