// Copyright 2021- Facebook. All rights reserved.

// This include file is used by both MetaHelper.cpp and OssHelper.cpp,
// to define functions that are implemented differently in Meta in OSS.
#include "fboss/platform/fan_service/SensorData.h"
#include "fboss/qsfp_service/lib/QsfpClient.h"

void getTransceivers(
    std::map<int32_t, facebook::fboss::TransceiverInfo>& cacheTable,
    folly::EventBase& evb);

void getSensorValueThroughThrift(
    int sensordThriftPort_,
    folly::EventBase& evb_,
    std::shared_ptr<facebook::fboss::platform::fan_service::SensorData>&
        pSensorData,
    std::vector<std::string>& sensorList);
