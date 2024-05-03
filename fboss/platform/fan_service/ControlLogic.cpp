// Copyright 2021- Facebook. All rights reserved.

// Implementation of Control Logic class. Refer to .h file
// for function description

#include "fboss/platform/fan_service/ControlLogic.h"

#include <folly/logging/xlog.h>

#include "common/time/Time.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_constants.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace {
auto constexpr kFanWriteFailure = "{}.{}.pwm_write.failure";
auto constexpr kFanWriteValue = "{}.{}.pwm_write.value";
auto constexpr kFanAbsent = "{}.absent";
auto constexpr kFanReadRpmFailure = "{}.rpm_read.failure";
auto constexpr kFanReadRpmValue = "{}.rpm_read.value";
auto constexpr kFanFailThresholdInSec = 300;
auto constexpr kSensorFailThresholdInSec = 300;

using namespace facebook::fboss::platform::fan_service;
using constants =
    facebook::fboss::platform::fan_service::fan_service_config_constants;

std::optional<TempToPwmMap> getConfigOpticTable(
    const Optic& optic,
    const std::string& opticType) {
  for (const auto& [tableType, tempToPwmMap] : *optic.tempToPwmMaps()) {
    if (tableType == opticType) {
      return tempToPwmMap;
    }
  }
  return std::nullopt;
}

std::optional<PidSetting> getConfigOpticPid(
    const Optic& optic,
    const std::string& opticType) {
  for (const auto& [tableType, pidSetting] : *optic.pidSettings()) {
    if (tableType == opticType) {
      return pidSetting;
    }
  }
  return std::nullopt;
}

} // namespace

namespace facebook::fboss::platform::fan_service {
ControlLogic::ControlLogic(
    const FanServiceConfig& config,
    std::shared_ptr<Bsp> pB)
    : config_(config),
      pBsp_(std::move(pB)),
      lastControlUpdateSec_(pBsp_->getCurrentTime()) {}

std::tuple<bool, int, uint64_t> ControlLogic::readFanRpm(const Fan& fan) {
  std::string fanName = *fan.fanName();
  bool fanRpmReadSuccess{false};
  int fanRpm{0};
  uint64_t rpmTimeStamp{0};
  if (isFanPresentInDevice(fan)) {
    try {
      fanRpm = pBsp_->readSysfs(*fan.rpmSysfsPath());
      rpmTimeStamp = pBsp_->getCurrentTime();
      fanRpmReadSuccess = true;
    } catch (std::exception& e) {
      XLOG(ERR) << fmt::format(
          "{}: Failed to read rpm from {}", fanName, *fan.rpmSysfsPath());
    }
  }
  fb303::fbData->setCounter(
      fmt::format(kFanReadRpmFailure, fanName), !fanRpmReadSuccess);
  if (fanRpmReadSuccess) {
    fb303::fbData->setCounter(fmt::format(kFanReadRpmValue, fanName), fanRpm);
    XLOG(INFO) << fmt::format("{}: RPM read is {}", fanName, fanRpm);
  }
  return std::make_tuple(!fanRpmReadSuccess, fanRpm, rpmTimeStamp);
}

float ControlLogic::calculatePid(
    const std::string& name,
    float value,
    PwmCalcCache& pwmCalcCache,
    float kp,
    float ki,
    float kd,
    uint64_t dT,
    float minVal,
    float maxVal) {
  float lastPwm, previousRead1, error, pwm;
  lastPwm = pwmCalcCache.previousTargetPwm;
  pwm = lastPwm;
  previousRead1 = pwmCalcCache.previousRead1;

  if (value < minVal) {
    pwmCalcCache.integral = 0;
    pwmCalcCache.previousTargetPwm = 0;
  }
  if (value > maxVal) {
    error = maxVal - value;
    pwmCalcCache.integral = pwmCalcCache.integral + error * dT;
    auto derivative = (error - pwmCalcCache.last_error) / dT;
    pwm = kp * error + ki * pwmCalcCache.integral + kd * derivative;
    pwmCalcCache.previousTargetPwm = pwm;
    pwmCalcCache.last_error = error;
  }
  pwmCalcCache.previousRead2 = previousRead1;
  pwmCalcCache.previousRead1 = value;
  XLOG(DBG1) << fmt::format(
      "{}: Sensor Value: {}, PWM [PID]: {}", name, value, pwm);
  XLOG(DBG1) << "               dT : " << dT
             << " Time : " << pBsp_->getCurrentTime()
             << " LUD : " << lastControlUpdateSec_ << " Min : " << minVal
             << " Max : " << maxVal;
  return pwm;
}

float ControlLogic::calculateIncrementalPid(
    const std::string& name,
    float value,
    PwmCalcCache& pwmCalcCache,
    float kp,
    float ki,
    float kd,
    float setPoint) {
  float pwm, lastPwm, previousRead1, previousRead2;
  lastPwm = pwmCalcCache.previousTargetPwm;
  previousRead1 = pwmCalcCache.previousRead1;
  previousRead2 = pwmCalcCache.previousRead2;
  pwm = lastPwm + (kp * (value - previousRead1)) + (ki * (value - setPoint)) +
      (kd * (value - 2 * previousRead1 + previousRead2));
  // Even though the previous Target Pwm should be the zone pwm,
  // the best effort is made here. Zone should update this value.
  pwmCalcCache.previousTargetPwm = pwm;
  pwmCalcCache.previousRead2 = previousRead1;
  pwmCalcCache.previousRead1 = value;
  XLOG(DBG1) << fmt::format(
      "{}: Sensor Value: {}, PWM [PID]: {}", name, value, pwm);
  XLOG(DBG1) << "           Prev1  : " << previousRead1
             << " Prev2 : " << previousRead2;
  return pwm;
}

void ControlLogic::updateTargetPwm(const Sensor& sensor) {
  bool accelerate, deadFanExists;
  float previousSensorValue, sensorValue, targetPwm{0};
  float minVal = *sensor.setPoint() - *sensor.negHysteresis();
  float maxVal = *sensor.setPoint() + *sensor.posHysteresis();
  uint64_t dT;
  TempToPwmMap tableToUse;
  auto& readCache = sensorReadCaches_[*sensor.sensorName()];
  auto& pwmCalcCache = pwmCalcCaches_[*sensor.sensorName()];
  const auto& pwmCalcType = *sensor.pwmCalcType();

  if (pwmCalcType == constants::SENSOR_PWM_CALC_TYPE_FOUR_LINEAR_TABLE()) {
    previousSensorValue = pwmCalcCache.previousSensorRead;
    sensorValue = readCache.lastReadValue;
    deadFanExists = (numFanFailed_ > 0);
    accelerate =
        ((previousSensorValue == 0) || (sensorValue > previousSensorValue));
    if (accelerate && !deadFanExists) {
      tableToUse = *sensor.normalUpTable();
    } else if (!accelerate && !deadFanExists) {
      tableToUse = *sensor.normalDownTable();
    } else if (accelerate && deadFanExists) {
      tableToUse = *sensor.failUpTable();
    } else {
      tableToUse = *sensor.failDownTable();
    }
    // Start with the lowest value
    targetPwm = tableToUse.begin()->second;
    for (const auto& [temp, pwm] : tableToUse) {
      if (sensorValue > temp) {
        targetPwm = pwm;
      }
    }
    if (accelerate) {
      // If accleration is needed, new pwm should be bigger than old value
      if (targetPwm < readCache.targetPwmCache) {
        targetPwm = readCache.targetPwmCache;
      }
    } else {
      // If deceleration is needed, new pwm should be smaller than old one
      if (targetPwm > readCache.targetPwmCache) {
        targetPwm = readCache.targetPwmCache;
      }
    }
    pwmCalcCache.previousSensorRead = sensorValue;
  }

  if (pwmCalcType == constants::SENSOR_PWM_CALC_TYPE_INCREMENT_PID()) {
    targetPwm = calculateIncrementalPid(
        *sensor.sensorName(),
        readCache.lastReadValue,
        pwmCalcCache,
        *sensor.kp(),
        *sensor.ki(),
        *sensor.kd(),
        *sensor.setPoint());
  }

  if (pwmCalcType == constants::SENSOR_PWM_CALC_TYPE_PID()) {
    dT = pBsp_->getCurrentTime() - lastControlUpdateSec_;
    targetPwm = calculatePid(
        *sensor.sensorName(),
        readCache.lastReadValue,
        pwmCalcCache,
        *sensor.kp(),
        *sensor.ki(),
        *sensor.kd(),
        dT,
        minVal,
        maxVal);
  }

  XLOG(INFO) << fmt::format(
      "{}: Calculated PWM is {}", *sensor.sensorName(), targetPwm);
  readCache.targetPwmCache = targetPwm;
}

void ControlLogic::getSensorUpdate() {
  for (const auto& sensor : *config_.sensors()) {
    bool sensorAccessFail = false;
    const auto sensorName = *sensor.sensorName();
    auto& readCache = sensorReadCaches_[sensorName];

    // STEP 1: Get reading.
    float readValue{0};
    if (pSensor_->checkIfEntryExists(sensorName)) {
      SensorEntryType entryType = pSensor_->getSensorEntryType(sensorName);
      switch (entryType) {
        case SensorEntryType::kSensorEntryInt:
          readValue = pSensor_->getSensorDataInt(sensorName);
          readValue = readValue / *sensor.scale();
          break;
        case SensorEntryType::kSensorEntryFloat:
          readValue = pSensor_->getSensorDataFloat(sensorName);
          readValue = readValue / *sensor.scale();
          break;
        default:
          facebook::fboss::FbossError(
              "Invalid Sensor Entry Type in entry name : ", sensorName);
          break;
      }
      readCache.lastReadValue = readValue;
      XLOG(ERR) << fmt::format(
          "{}: Sensor read value (after scaling) is {}", sensorName, readValue);
    } else {
      XLOG(INFO) << fmt::format(
          "{}: Failure to get data (either wrong entry or read failure)",
          sensorName);
      sensorAccessFail = true;
    }

    if (sensorAccessFail) {
      // If the sensor data cache is stale for a while, we consider it as the
      // failure of such sensor
      uint64_t timeDiffInSec =
          pBsp_->getCurrentTime() - readCache.lastUpdatedTime;
      if (timeDiffInSec >= kSensorFailThresholdInSec) {
        readCache.sensorFailed = true;
        numSensorFailed_++;
      }
    } else {
      readCache.lastUpdatedTime = pSensor_->getLastUpdated(sensorName);
      readCache.sensorFailed = false;
    }

    // STEP 2: Calculate target pwm
    updateTargetPwm(sensor);
  }
}

void ControlLogic::getOpticsUpdate() {
  for (const auto& optic : *config_.optics()) {
    std::string opticName = *optic.opticName();

    if (!pSensor_->checkIfOpticEntryExists(opticName)) {
      continue;
    }

    auto opticData = pSensor_->getOpticEntry(opticName);
    if (opticData != nullptr && opticData->data.size() == 0) {
      continue;
    }

    int aggOpticPwm = 0;
    const auto& aggregationType = *optic.aggregationType();

    if (aggregationType == constants::OPTIC_AGGREGATION_TYPE_MAX()) {
      // Conventional one-table conversion, followed by
      // the aggregation using the max value
      for (const auto& [opticType, value] : opticData->data) {
        int pwmForThis = 0;
        auto tablePointer = getConfigOpticTable(optic, opticType);
        // We have <type, value> pair. If we have table entry for this
        // optics type, get the matching pwm value using the optics value
        if (tablePointer) {
          // Start with the minumum, then continue the comparison
          pwmForThis = tablePointer->begin()->second;
          for (const auto& [temp, pwm] : *tablePointer) {
            if (value >= temp) {
              pwmForThis = pwm;
            }
          }
        }
        XLOG(INFO) << fmt::format(
            "{}: Optic sensor read value is {}. PWM is {}",
            opticType,
            value,
            pwmForThis);
        if (pwmForThis > aggOpticPwm) {
          aggOpticPwm = pwmForThis;
        }
      }
    } else if (aggregationType == constants::OPTIC_AGGREGATION_TYPE_PID()) {
      // PID based conversion. First calculate the max temperature
      // per optic type, and use the max temperature for calculating
      // pwm using PID method
      // Step 1. Get the max temperature per optic type
      std::unordered_map<std::string, float> maxValue;
      for (const auto& [opticType, value] : opticData->data) {
        if (maxValue.find(opticType) == maxValue.end()) {
          maxValue[opticType] = value;
        } else {
          if (value > maxValue[opticType]) {
            maxValue[opticType] = value;
          }
        }
      }
      // Step 2. Get the pwm per optic type using PID
      for (const auto& [opticType, value] : maxValue) {
        // Get PID setting
        auto pidSetting = getConfigOpticPid(optic, opticType);
        // Cache values are stored per optic type
        auto& readCache = sensorReadCaches_[opticType];
        auto& pwmCalcCache = pwmCalcCaches_[opticType];
        float kp = *pidSetting->kp();
        float ki = *pidSetting->ki();
        float kd = *pidSetting->kd();
        uint64_t dT = pBsp_->getCurrentTime() - lastControlUpdateSec_;
        float minVal = *pidSetting->setPoint() - *pidSetting->negHysteresis();
        float maxVal = *pidSetting->setPoint() + *pidSetting->posHysteresis();
        float pwm = calculatePid(
            opticType, value, pwmCalcCache, kp, ki, kd, dT, minVal, maxVal);
        readCache.targetPwmCache = pwm;
        if (pwm > aggOpticPwm) {
          aggOpticPwm = pwm;
        }
      }
    } else {
      throw facebook::fboss::FbossError(
          "Only max and pid aggregation is supported for optics!");
    }
    XLOG(INFO) << fmt::format(
        "Optics: Aggregation Type: {}. Aggregate PWM is {}",
        aggregationType,
        aggOpticPwm);
    opticData->calculatedPwm = aggOpticPwm;
    // As we consumed the data, clear the vector
    opticData->data.clear();
    opticData->dataProcessTimeStamp = opticData->lastOpticsUpdateTimeInSec;
  }
}

bool ControlLogic::isSensorPresentInConfig(const std::string& sensorName) {
  for (auto& sensor : *config_.sensors()) {
    if (*sensor.sensorName() == sensorName) {
      return true;
    }
  }
  return false;
}

bool ControlLogic::isFanPresentInDevice(const Fan& fan) {
  unsigned int readVal;
  bool readSuccessful = false;
  try {
    readVal = static_cast<unsigned>(pBsp_->readSysfs(*fan.presenceSysfsPath()));
    readSuccessful = true;
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to read sysfs " << *fan.presenceSysfsPath();
  }
  auto fanPresent = (readSuccessful && readVal == *fan.fanPresentVal());
  if (!fanPresent) {
    XLOG(INFO) << fmt::format(
        "Control :: {} is absent in the host", *fan.fanName());
  }
  fb303::fbData->setCounter(
      fmt::format(kFanAbsent, *fan.fanName()), !fanPresent);
  return fanPresent;
}

std::pair<bool, float> ControlLogic::programFan(
    const Zone& zone,
    const Fan& fan,
    float currentFanPwm,
    float calculatedZonePwm) {
  float newFanPwm = 0;
  bool writeSuccess{false};
  if ((*zone.slope() == 0) || (currentFanPwm == 0)) {
    newFanPwm = calculatedZonePwm;
  } else {
    if (calculatedZonePwm > currentFanPwm) {
      if ((calculatedZonePwm - currentFanPwm) > *zone.slope()) {
        newFanPwm = currentFanPwm + *zone.slope();
      } else {
        newFanPwm = calculatedZonePwm;
      }
    } else if (calculatedZonePwm < currentFanPwm) {
      if ((currentFanPwm - calculatedZonePwm) > *zone.slope()) {
        newFanPwm = currentFanPwm - *zone.slope();
      } else {
        newFanPwm = calculatedZonePwm;
      }
    } else {
      newFanPwm = calculatedZonePwm;
    }
  }

  newFanPwm = std::min(newFanPwm, (float)*config_.pwmUpperThreshold());
  newFanPwm = std::max(newFanPwm, (float)*config_.pwmLowerThreshold());

  int pwmRawValue =
      (int)(((*fan.pwmMax()) - (*fan.pwmMin())) * newFanPwm / 100.0 +
            *fan.pwmMin());
  if (pwmRawValue < *fan.pwmMin()) {
    pwmRawValue = *fan.pwmMin();
  } else if (pwmRawValue > *fan.pwmMax()) {
    pwmRawValue = *fan.pwmMax();
  }
  writeSuccess = pBsp_->setFanPwmSysfs(*fan.pwmSysfsPath(), pwmRawValue);
  fb303::fbData->setCounter(
      fmt::format(kFanWriteFailure, *zone.zoneName(), *fan.fanName()),
      !writeSuccess);
  if (writeSuccess) {
    fb303::fbData->setCounter(
        fmt::format(kFanWriteValue, *zone.zoneName(), *fan.fanName()),
        newFanPwm);
    XLOG(INFO) << fmt::format(
        "{}: Programmed with PWM {} (raw value {})",
        *fan.fanName(),
        newFanPwm,
        pwmRawValue);
  } else {
    XLOG(INFO) << fmt::format(
        "{}: Failed to program with PWM {} (raw value {})",
        *fan.fanName(),
        newFanPwm,
        pwmRawValue);
  }

  return std::make_pair(!writeSuccess, newFanPwm);
}

void ControlLogic::programLed(const Fan& fan, bool fanFailed) {
  unsigned int valueToWrite =
      (fanFailed ? *fan.fanFailLedVal() : *fan.fanGoodLedVal());
  pBsp_->setFanLedSysfs(*fan.ledSysfsPath(), valueToWrite);
  XLOG(INFO) << fmt::format(
      "{}: Setting LED to {} (value: {})",
      *fan.fanName(),
      (fanFailed ? "Fail" : "Good"),
      valueToWrite);
}

float ControlLogic::calculateZonePwm(const Zone& zone, bool boostMode) {
  // First, calculate the pwm value for this zone
  auto zoneType = *zone.zoneType();
  float calculatedZonePwm = 0;
  int totalPwmConsidered = 0;
  for (const auto& sensorName : *zone.sensorNames()) {
    if (isSensorPresentInConfig(sensorName) ||
        pSensor_->checkIfOpticEntryExists(sensorName)) {
      totalPwmConsidered++;
      float pwmForThisSensor;
      if (isSensorPresentInConfig(sensorName)) {
        // If this is a sensor name
        pwmForThisSensor = sensorReadCaches_[sensorName].targetPwmCache;
      } else {
        // If this is an optics name
        pwmForThisSensor = pSensor_->getOpticsPwm(sensorName);
      }
      if (zoneType == constants::ZONE_TYPE_MAX()) {
        if (calculatedZonePwm < pwmForThisSensor) {
          calculatedZonePwm = pwmForThisSensor;
        }
      } else if (zoneType == constants::ZONE_TYPE_MIN()) {
        if (calculatedZonePwm > pwmForThisSensor) {
          calculatedZonePwm = pwmForThisSensor;
        }
      } else if (zoneType == constants::ZONE_TYPE_AVG()) {
        calculatedZonePwm += pwmForThisSensor;
      } else {
        XLOG(ERR) << "Undefined Zone Type for zone : ", *zone.zoneName();
      }
    }
  }
  if (zoneType == constants::ZONE_TYPE_AVG() && totalPwmConsidered != 0) {
    calculatedZonePwm /= (float)totalPwmConsidered;
  }

  if (boostMode) {
    if (calculatedZonePwm < *config_.pwmBoostValue()) {
      calculatedZonePwm = *config_.pwmBoostValue();
    }
  }
  XLOG(INFO) << fmt::format(
      "{}: Components: {}. Aggregation Type: {}. Aggregate PWM is {}.",
      *zone.zoneName(),
      folly::join(",", *zone.sensorNames()),
      zoneType,
      calculatedZonePwm);
  // Update the previous pwm value in each associated sensors,
  // so that they may be used in the next calculation.
  for (const auto& sensorName : *zone.sensorNames()) {
    if (isSensorPresentInConfig(sensorName)) {
      pwmCalcCaches_[sensorName].previousTargetPwm = calculatedZonePwm;
    }
  }
  return calculatedZonePwm;
}

void ControlLogic::setTransitionValue() {
  fanStatuses_.withWLock([&](auto& fanStatuses) {
    for (const auto& zone : *config_.zones()) {
      for (const auto& fan : *config_.fans()) {
        // If this fan belongs to the zone, then write the transitional value
        if (std::find(
                zone.fanNames()->begin(),
                zone.fanNames()->end(),
                *fan.fanName()) == zone.fanNames()->end()) {
          continue;
        }
        for (const auto& sensorName : *zone.sensorNames()) {
          if (isSensorPresentInConfig(sensorName)) {
            pwmCalcCaches_[sensorName].previousTargetPwm =
                *config_.pwmTransitionValue();
          }
        }
        const auto [fanFailed, newFanPwm] = programFan(
            zone,
            fan,
            *fanStatuses[*fan.fanName()].pwmToProgram(),
            *config_.pwmTransitionValue());
        fanStatuses[*fan.fanName()].pwmToProgram() = newFanPwm;
        if (fanFailed) {
          programLed(fan, fanFailed);
        }
        fanStatuses[*fan.fanName()].fanFailed() = fanFailed;
      }
    }
  });
}

void ControlLogic::updateControl(std::shared_ptr<SensorData> pS) {
  pSensor_ = pS;

  numFanFailed_ = 0;
  numSensorFailed_ = 0;
  bool boostForMissingOpticsUpdate = false;

  // If we have not yet successfully read so far,
  // it does not make sense to update fan PWM out of no data.
  // So check if we read sensor data at least once. If not,
  // do it now.
  if (!pBsp_->checkIfInitialSensorDataRead()) {
    XLOG(INFO) << "Reading sensors for the first time";
    pBsp_->getSensorData(pSensor_);
  }

  // Determine proposed pwm value by each sensor read
  XLOG(INFO) << "Processing Sensors ...";
  getSensorUpdate();

  // Determine proposed pwm value by each optics read
  XLOG(INFO) << "Processing Optics ...";
  getOpticsUpdate();

  // Check if we need to turn on boost mode for qsfp errors
  uint64_t secondsSinceLastOpticsUpdate =
      pBsp_->getCurrentTime() - pSensor_->getLastQsfpSvcTime();
  if ((*config_.pwmBoostOnNoQsfpAfterInSec() != 0) &&
      (secondsSinceLastOpticsUpdate >= *config_.pwmBoostOnNoQsfpAfterInSec())) {
    boostForMissingOpticsUpdate = true;
    XLOG(INFO) << fmt::format(
        "Boost mode enabled for optics update missing for {}s",
        secondsSinceLastOpticsUpdate);
  }

  XLOG(INFO) << "Processing Fans ...";
  fanStatuses_.withWLock([&](auto& fanStatuses) {
    // Now, check if Fan is in good shape, based on the previously read
    // sensor data

    // Update fan status with new rpm and timestamp.
    for (const auto& fan : *config_.fans()) {
      auto [fanAccessFailed, fanRpm, fanTimestamp] = readFanRpm(fan);
      if (fanAccessFailed) {
        fanStatuses[*fan.fanName()].rpm().reset();
      } else {
        fanStatuses[*fan.fanName()].rpm() = fanRpm;
        fanStatuses[*fan.fanName()].lastSuccessfulAccessTime() = fanTimestamp;
      }

      // Ignore last access failure if it happened < kFanFailThresholdInSec
      auto timeSinceLastSuccessfulAccess = pBsp_->getCurrentTime() -
          *fanStatuses[*fan.fanName()].lastSuccessfulAccessTime();
      auto fanFailed = fanAccessFailed &&
          (timeSinceLastSuccessfulAccess >= kFanFailThresholdInSec);
      if (fanFailed) {
        numFanFailed_++;
      }
      fanStatuses[*fan.fanName()].fanFailed() = fanFailed;
    }

    bool boostMode =
        (((*config_.pwmBoostOnNumDeadFan() != 0) &&
          (numFanFailed_ >= *config_.pwmBoostOnNumDeadFan())) ||
         ((*config_.pwmBoostOnNumDeadSensor() != 0) &&
          (numSensorFailed_ >= *config_.pwmBoostOnNumDeadSensor())) ||
         boostForMissingOpticsUpdate);
    XLOG(INFO) << fmt::format("Boost mode is {}", (boostMode ? "On" : "Off"));

    for (const auto& zone : *config_.zones()) {
      // Finally, set pwm values per zone.
      // It's not recommended to put a fan in multiple zones,
      // even though it's possible to do so.
      float calculatedZonePwm = calculateZonePwm(zone, boostMode);
      for (const auto& fan : *config_.fans()) {
        // Skip if the fan doesn't belong to this zone
        if (std::find(
                zone.fanNames()->begin(),
                zone.fanNames()->end(),
                *fan.fanName()) == zone.fanNames()->end()) {
          continue;
        }

        const auto [fanFailed, newFanPwm] = programFan(
            zone,
            fan,
            *fanStatuses[*fan.fanName()].pwmToProgram(),
            calculatedZonePwm);
        fanStatuses[*fan.fanName()].pwmToProgram() = newFanPwm;

        if (fanFailed) {
          // Only override the fanFailed if fan programming failed.
          fanStatuses[*fan.fanName()].fanFailed() = fanFailed;
        }
      }
    }

    for (const auto& fan : *config_.fans()) {
      programLed(fan, *fanStatuses[*fan.fanName()].fanFailed());
    }
  });

  // Update the time stamp
  lastControlUpdateSec_ = pBsp_->getCurrentTime();
}

} // namespace facebook::fboss::platform::fan_service
