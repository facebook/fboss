// Copyright 2021- Facebook. All rights reserved.
#pragma once

#include <gflags/gflags.h>
#include <string>

#include "fboss/platform/fan_service/Bsp.h"
#include "fboss/platform/fan_service/ControlLogic.h"
#include "fboss/platform/fan_service/Mokujin.h"
#include "fboss/platform/fan_service/SensorData.h"

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace folly {
class EventBase;
}
namespace facebook::fboss::platform::fan_service {

class FanServiceImpl {
 public:
  explicit FanServiceImpl(const std::string& configFile);
  ~FanServiceImpl() {} // Make compiler happy in handling smart pointers
  // Instantiates all classes used by FanServiceImpl
  void kickstart();
  // Runs Fan PWM control logic
  int controlFan();
  // A special function to run FanServiceImpl as a Mock
  // (simulation for unit testing)
  int runMock(std::string mockInputFile, std::string mockOutputFile);

  void getSensorDataThrift(std::shared_ptr<SensorData> pSensorData) const {
    return pBsp_->getSensorDataThrift(pSensorData);
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
  // fan_service config
  FanServiceConfig config_;
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
  std::string confFileName_{};

  unsigned int getControlFrequency() const;
  // The factory method to return the proper BSP object,
  // based on the platform type specified in config file
  std::shared_ptr<Bsp> BspFactory();
  // Main Loop for standalone execution
  int mainLoop();
};
} // namespace facebook::fboss::platform::fan_service
