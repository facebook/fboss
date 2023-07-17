// Copyright 2021- Facebook. All rights reserved.
#pragma once

#include <gflags/gflags.h>
#include <string>
#include "Bsp.h"
#include "ControlLogic.h"
#include "Mokujin.h"
#include "SensorData.h"
#include "ServiceConfig.h"

namespace folly {
class EventBase;
}
namespace facebook::fboss::platform {

// FanService    : The main Class the represent the fan_service
//                 Instantiates the following classes.
//
//               - ControlLogic class : PWM control logic
//               - ServiceConfig class : parse config file, and keep the info
//               - Bsp class : All I/O functions including Thrift handler
//                    - Mokujin class : A mock of Bsp class for unit testing
//               - SensorData class : Stores sensor data and the timestamps

class FanService {
 public:
  // Constructor / destructor
  FanService();
  ~FanService() {} // Make compiler happy in handling smart pointers
  // Instantiates all classes used by Fan Service
  void kickstart();
  // Runs Fan PWM control logic
  int controlFan();
  // A special function to run Fan Service as a Mock
  // (simulation for unit testing)
  int runMock(std::string mockInputFile, std::string mockOutputFile);

  void getSensorDataThrift(std::shared_ptr<SensorData> pSensorData) const {
    return pBsp_->getSensorDataThrift(pConfig_, pSensorData);
  }
  const SensorData& sensorData() const {
    return *(pSensorData_.get());
  }
  uint64_t lastSensorFetchTimeSec() const {
    return lastSensorFetchTimeSec_;
  }
  unsigned int getSensorFetchFrequency() const;

 private:
  // Attributes
  // BSP contains platform specific I/O methonds
  std::shared_ptr<Bsp> pBsp_;
  // ServiceConfig parses the configuration, and keep the data
  std::shared_ptr<ServiceConfig> pConfig_;
  // Control logic determines fan pwm based on config and sensor read
  std::shared_ptr<ControlLogic> pControlLogic_;
  // SensorData keeps all the latest sensor reading. Also provides
  // metadata that describes how to read that sensor data
  std::shared_ptr<SensorData> pSensorData_;
  // Check if fan pwm was programmed with transitionValue yet.
  bool transitionValueSet_;
  // The timestamp of the last PWM control logic execution
  uint64_t lastControlExecutionTimeSec_;
  // The timestamp of the last sensor data fetch
  uint64_t lastSensorFetchTimeSec_;
  // How often we run fan control logic?
  uint64_t controlFrequencySec_;

  unsigned int getControlFrequency() const;
  // The factory method to return the proper BSP object,
  // based on the platform type specified in config file
  std::shared_ptr<Bsp> BspFactory();
  // Main Loop for standalone execution
  int mainLoop();
};
} // namespace facebook::fboss::platform
