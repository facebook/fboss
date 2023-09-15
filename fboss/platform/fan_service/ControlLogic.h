// Copyright 2021- Facebook. All rights reserved.

#pragma once

#include "fboss/platform/fan_service/Bsp.h"
#include "fboss/platform/fan_service/SensorData.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_types.h"

namespace facebook::fboss::platform::fan_service {

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
  ~ControlLogic() = default;
  // updateControl : Main entry for the control logic to process sensor
  //                 readings and set PWM value accordingly
  void updateControl(std::shared_ptr<SensorData> pS);
  void setTransitionValue();
  const std::map<std::string, FanStatus> getFanStatuses() {
    return fanStatuses_.copy();
  }

 private:
  // Private Attributess :
  // Pointer to other classes used by Control Logic
  const FanServiceConfig config_;
  std::shared_ptr<Bsp> pBsp_;
  std::shared_ptr<SensorData> pSensor_;
  // Internal variable storing the number of failed sensors and fans
  int numFanFailed_ = 0;
  int numSensorFailed_ = 0;
  // Last control update time. Used for dT calculation
  uint64_t lastControlUpdateSec_;

  // Private Methods
  void getSensorUpdate();
  std::tuple<bool /*fanAccessFailed*/, int /*rpm*/, uint64_t /*timestamp*/>
  readFanRpm(const Fan& fan);
  void getOpticsUpdate();
  std::pair<bool /* pwm update fail */, float /* pwm */> programFan(
      const Zone& zone,
      const Fan& fan,
      float currentPwm,
      float pwmSoFar);
  float calculateZonePwm(const Zone& zone, bool boostMode);
  void updateTargetPwm(const Sensor& sensorItem);
  void programLed(const Fan& fan, bool fanFailed);
  bool isFanPresentInDevice(const Fan& fan);
  bool isSensorPresentInConfig(const std::string& sensorName);

  folly::Synchronized<std::map<std::string /* fanName */, FanStatus>>
      fanStatuses_;
  std::map<std::string /* sensorName */, SensorReadCache> sensorReadCaches_;
  std::map<std::string /* sensorName */, PwmCalcCache> pwmCalcCaches_;
};
} // namespace facebook::fboss::platform::fan_service
