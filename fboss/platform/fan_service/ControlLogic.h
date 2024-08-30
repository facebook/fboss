// Copyright 2021- Facebook. All rights reserved.

#pragma once

#include "fboss/platform/fan_service/Bsp.h"
#include "fboss/platform/fan_service/PidLogic.h"
#include "fboss/platform/fan_service/SensorData.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_types.h"

namespace facebook::fboss::platform::fan_service {

struct SensorReadCache {
  // This is the last read value from the sensor which has been processed.
  float processedReadValue{0};
  // This is the last read value from the sensor which is yet to be processed.
  float lastReadValue{0};
  int16_t targetPwmCache{0};
  uint64_t lastUpdatedTime;
  bool sensorFailed{false};
};

// ControlLogic class is a part of Fan Service
// Role : This class contains the logics to detect sensor/fan failures and,
//        the logics to calculate the PWM values from sensor reading.
//        Currently supports PID, Incremental PID and Four Tables method
class ControlLogic {
 public:
  ControlLogic(FanServiceConfig config, std::shared_ptr<Bsp> bsp);
  ~ControlLogic() = default;

  void updateControl(std::shared_ptr<SensorData> pS);
  void setTransitionValue();
  const std::map<std::string, FanStatus> getFanStatuses() {
    return fanStatuses_.copy();
  }
  const std::map<std::string, SensorReadCache>& getSensorCaches() {
    return sensorReadCaches_;
  }

  void controlFan();
  void getSensorDataThrift(std::shared_ptr<SensorData> pSensorData) const {
    return pBsp_->getSensorDataThrift(pSensorData);
  }
  const SensorData& sensorData() const {
    return *(pSensorData_.get());
  }
  uint64_t lastSensorFetchTimeSec() const {
    return lastSensorFetchTimeSec_;
  }
  unsigned int getControlFrequency() const;
  unsigned int getSensorFetchFrequency() const;
  void setFanHold(std::optional<int> pwm);
  std::optional<int> getFanHold();

 private:
  const FanServiceConfig config_;
  std::shared_ptr<Bsp> pBsp_;
  std::shared_ptr<SensorData> pSensor_;
  // Internal variable storing the number of failed sensors and fans
  int numFanFailed_ = 0;
  int numSensorFailed_ = 0;
  // The timestamp of the last PWM control logic execution
  uint64_t lastControlExecutionTimeSec_{0};
  // The timestamp of the last sensor data fetch
  uint64_t lastSensorFetchTimeSec_{0};

  void setupPidLogics();
  void getSensorUpdate();
  std::tuple<bool /*fanAccessFailed*/, int /*rpm*/, uint64_t /*timestamp*/>
  readFanRpm(const Fan& fan);
  void getOpticsUpdate();
  std::pair<bool /* pwm update fail */, int16_t /* pwm */> programFan(
      const Zone& zone,
      const Fan& fan,
      int16_t currentFanPwm,
      int16_t zonePwm);
  int16_t calculateZonePwm(const Zone& zone, bool boostMode);
  void updateTargetPwm(const Sensor& sensorItem);
  void programLed(const Fan& fan, bool fanFailed);
  bool isFanPresentInDevice(const Fan& fan);

  folly::Synchronized<std::map<std::string /* fanName */, FanStatus>>
      fanStatuses_;
  std::atomic<std::optional<int>> fanHoldPwm_;
  std::map<std::string /* sensorName */, SensorReadCache> sensorReadCaches_;
  std::map<std::string /* sensorName */, int16_t /* pwm */> opticReadCaches_;
  std::map<std::string /* sensorName */, PidLogic> pidLogics_;
  std::shared_ptr<SensorData> pSensorData_{nullptr};
};
} // namespace facebook::fboss::platform::fan_service
