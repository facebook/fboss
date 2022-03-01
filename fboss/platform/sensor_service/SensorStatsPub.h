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

#include <fb303/ThreadCachedServiceData.h>
#include <fb303/ThreadLocalStatsMap.h>

namespace facebook::fboss::platform::sensor_service {

class SensorStatsPub {
 public:
  explicit SensorStatsPub(std::shared_ptr<SensorServiceImpl> sensorService)
      : sensorService_(sensorService) {}
  void init();
  void publishStats(int32_t stats_publish_interval);

 private:
  std::shared_ptr<SensorServiceImpl> sensorService_;
}

} // namespace facebook::fboss::platform::sensor_service
