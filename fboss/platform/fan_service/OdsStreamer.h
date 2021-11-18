// Copyright 2021- Facebook. All rights reserved.

#pragma once
// OdsStreamer is a part of Fan Service
// Role : To stream the sensor data to ODS.

// Folly library header file for conversion and log
#include <folly/Conv.h>
#include <folly/logging/xlog.h>

namespace folly {
class EventBase;
}

namespace facebook::fboss::platform {

class SensorData;

class OdsStreamer {
 public:
  // Constructor / Destructor
  explicit OdsStreamer(const std::string& odsTier) : odsTier_(odsTier) {}
  // Main entry to initiate the ODS data send
  int postData(folly::EventBase* evb, const SensorData& sensorData) const;

 private:
  const std::string odsTier_;
};
} // namespace facebook::fboss::platform
