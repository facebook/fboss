// Copyright 2021- Facebook. All rights reserved.

#pragma once

#include "Bsp.h"
#include "SensorData.h"

namespace facebook::fboss::platform {

// ControlLogic class is a part of Fan Service
// Role : This class contains the logics to detect sensor/fan failures and,
//        the logics to calculate the PWM values from sensor reading.
//        Currently supports PID, Incremental PID and Four Tables method
class ControlLogic {
 public:
  // Constructor / Destructor
  ControlLogic(std::shared_ptr<ServiceConfig> pC, std::shared_ptr<Bsp> pB);
  ~ControlLogic();
  // updateControl : Main entry for the control logic to process sensor
  //                 readings and set PWM value accordingly
  void updateControl(std::shared_ptr<SensorData> pS);
  void setTransitionValue();

 private:
  // Private Attributess :
  // Pointer to other classes used by Control Logic
  std::shared_ptr<ServiceConfig> pConfig_;
  std::shared_ptr<Bsp> pBsp_;
  std::shared_ptr<SensorData> pSensor_;
  // Internal variable storing the number of failed sensors and fans
  int numFanFailed_;
  int numSensorFailed_;
  // Last control update time. Used for dT calculation
  uint64_t lastControlUpdateSec_;

  // Private Methods
  void getSensorUpdate();
  void getFanUpdate();
  void getOpticsUpdate();
  void programFan(const fan_config_structs::Zone& zone, float pwmSoFar);
  void adjustZoneFans(bool boostMode);
  void updateTargetPwm(const Sensor& sensorItem);
  void setFanFailState(const fan_config_structs::Fan& fan, bool fanFailed);
  bool checkIfFanPresent(const fan_config_structs::Fan& fan);
  Sensor* findSensorConfig(const std::string& sensorName);
};
} // namespace facebook::fboss::platform
