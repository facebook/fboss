// Copyright 2021- Facebook. All rights reserved.

#pragma once

#include "Bsp.h"
#include "SensorData.h"

namespace facebook::fboss::platform::fan_service {

struct FanStatus {
  int rpm;
  float currentPwm{0};
  bool fanFailed{false};
  bool fanAccessLost{false};
  bool firstTimeLedAccess{true};
  uint64_t timeStamp;
};

struct SensorReadCache {
  float adjustedReadCache{0};
  float targetPwmCache{0};
  uint64_t lastUpdatedTime;
  bool enabled{false};
  bool soakStarted{false};
  uint64_t sensorAccessLostAt;
  bool sensorFailed{false};
  bool minorAlarmTriggered{false};
  bool majorAlarmTriggered{false};
  uint64_t soakStartedAt;
  int32_t invalidRangeCheckCount{0};
};

struct PwmCalcCache {
  float previousSensorRead{0};
  float previousTargetPwm{0};
  float previousRead1{0};
  float previousRead2{0};
  float integral{0};
  float last_error{0};
};

// ControlLogic class is a part of Fan Service
// Role : This class contains the logics to detect sensor/fan failures and,
//        the logics to calculate the PWM values from sensor reading.
//        Currently supports PID, Incremental PID and Four Tables method
class ControlLogic {
 public:
  // Constructor / Destructor
  ControlLogic(const FanServiceConfig& config, std::shared_ptr<Bsp> pB);
  ~ControlLogic();
  // updateControl : Main entry for the control logic to process sensor
  //                 readings and set PWM value accordingly
  void updateControl(std::shared_ptr<SensorData> pS);
  void setTransitionValue();

 private:
  // Private Attributess :
  // Pointer to other classes used by Control Logic
  const FanServiceConfig config_;
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
  void programFan(const Zone& zone, float pwmSoFar);
  void adjustZoneFans(bool boostMode);
  void updateTargetPwm(const Sensor& sensorItem);
  void setFanFailState(const Fan& fan, bool fanFailed);
  bool isFanPresentInDevice(const Fan& fan);
  bool isSensorPresentInConfig(const std::string& sensorName);

  std::map<std::string /* fanName */, FanStatus> fanStatuses_;
  std::map<std::string /* sensorName */, SensorReadCache> sensorReadCaches_;
  std::map<std::string /* sensorName */, PwmCalcCache> pwmCalcCaches_;
};
} // namespace facebook::fboss::platform::fan_service
