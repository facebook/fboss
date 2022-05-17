// Copyright 2021- Facebook. All rights reserved.

// This include file is used by both MetaHelper.cpp and OssHelper.cpp,
// to define functions that are implemented differently in Meta in OSS.
#include "fboss/platform/fan_service/SensorData.h"
#include "fboss/platform/fan_service/ServiceConfig.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

bool getCacheTable(
    std::unordered_map<
        facebook::fboss::TransceiverID,
        facebook::fboss::TransceiverInfo>& cacheTable,
    std::shared_ptr<facebook::fboss::QsfpCache> qsfpCache_);

void getSensorValueThroughThrift(
    int sensordThriftPort_,
    folly::EventBase& evb_,
    std::shared_ptr<facebook::fboss::platform::SensorData>& pSensorData,
    std::vector<std::string>& sensorList);

bool initQsfpSvc(
    std::shared_ptr<facebook::fboss::QsfpCache> qsfpCache_,
    folly::EventBase& evb_);

int doFBInit(int argc, char** argv);
