// Copyright 2021- Facebook. All rights reserved.
// This file contains all the function implementation
// that are OSS specific. The counterpart should be
// implemented in Meta specific file.

#include "fboss/platform/fan_service/DataFetcher.h"

#include <folly/logging/xlog.h>

using namespace facebook::fboss;
using namespace facebook::fboss::platform::fan_service;

namespace facebook::fboss::platform::fan_service {

void getTransceivers(
    std::map<int32_t, TransceiverInfo>& cacheTable,
    folly::EventBase& evb) {
  XLOG(ERR) << "OSS Build does not support reading data from qsfp_service";
}

void getSensorValueThroughThrift(
    int sensorServiceThriftPort,
    folly::EventBase& evb,
    std::shared_ptr<SensorData>& pSensorData) {
  // Until we have a way to do RPC between fan_service and
  // sensor_service in OSS build, we will not simply print out
  // error log
  XLOG(ERR) << "Thrift RPC to other services are not yet supported in OSS.";
}

} // namespace facebook::fboss::platform::fan_service
