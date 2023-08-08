/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
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
      std::shared_ptr<SensorServiceImpl> sensorServiceImpl)
      : sensorServiceImpl_(std::move(sensorServiceImpl)) {}

#if FOLLY_HAS_COROUTINES
  folly::coro::Task<std::unique_ptr<SensorReadResponse>>
  co_getSensorValuesByNames(
      std::unique_ptr<std::vector<std::string>> request) override {
    auto response = std::make_unique<SensorReadResponse>();
    getSensorValuesByNames(*response, std::move(request));
    co_return response;
  }

  folly::coro::Task<std::unique_ptr<SensorReadResponse>>
  co_getSensorValuesByFruTypes(
      std::unique_ptr<std::vector<FruType>> request) override {
    auto response = std::make_unique<SensorReadResponse>();
    getSensorValuesByFruTypes(*response, std::move(request));
    co_return response;
  }
#endif
  void getSensorValuesByNames(
      SensorReadResponse& response,
      std::unique_ptr<std::vector<std::string>> request) override;

  void getSensorValuesByFruTypes(
      SensorReadResponse& response,
      std::unique_ptr<std::vector<FruType>> request) override;

  SensorServiceImpl* getServiceImpl() {
    return sensorServiceImpl_.get();
  }

 private:
  std::shared_ptr<SensorServiceImpl> sensorServiceImpl_;
};
} // namespace facebook::fboss::platform::sensor_service
