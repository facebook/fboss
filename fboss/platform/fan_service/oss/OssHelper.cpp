// Copyright 2021- Facebook. All rights reserved.
// This file contains all the function implementation
// that are OSS specific. The counterpart should be
// implemented in Meta specific file.

#include "fboss/platform/fan_service/HelperFunction.h"

#include <folly/logging/xlog.h>

using namespace facebook::fboss;
using namespace facebook::fboss::platform;

void getTransceivers(
    std::map<int32_t, TransceiverInfo>& cacheTable,
    folly::EventBase& evb) {
  XLOG(ERR) << "OSS Build does not support reading data from qsfp_service";
}

void getSensorValueThroughThrift(
    int sensordThriftPort_,
    folly::EventBase& evb_,
    std::shared_ptr<facebook::fboss::platform::SensorData>& pSensorData,
    std::vector<std::string>& sensorList) {
  // Until we have a way to do RPC between fan_service and
  // sensor_service in OSS build, we will not simply print out
  // error log
  XLOG(ERR) << "Thrift RPC to other services are not yet supported in OSS.";
}
