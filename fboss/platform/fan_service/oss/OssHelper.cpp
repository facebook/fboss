// Copyright 2021- Facebook. All rights reserved.
// This file contains all the function implementation
// that are OSS specific. The counterpart should be
// implemented in Meta specific file.
#include <folly/experimental/FunctionScheduler.h>
#include "fboss/platform/fan_service/HelperFunction.h"

using namespace facebook::fboss;
using namespace facebook::fboss::platform;

int doFBInit(int argc, char** argv) {
  return 0;
}

bool getCacheTable(
    std::unordered_map<TransceiverID, TransceiverInfo>& cacheTable,
    std::shared_ptr<QsfpCache> qsfpCache_) {
  XLOG(ERR) << "OSS Build does not support reading data from qsfp_service";
  return false;
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

bool initQsfpSvc(
    std::shared_ptr<QsfpCache>& qsfpCache_,
    folly::EventBase& evb_) {
  return false;
}

void setVersionInfo() {}
