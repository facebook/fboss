// Copyright 2021- Facebook. All rights reserved.

// Implementation of Control Logic class. Refer to .h file
// for function description

#include "fboss/platform/fan_service/ControlLogic.h"

#include <folly/logging/xlog.h>

#include "common/time/Time.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_constants.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace {
auto constexpr kFanWriteFailure = "{}.{}.write_pwm_failure";
auto constexpr kFanAbsent = "{}.absent";
auto constexpr kFanReadRpmFailure = "{}.read_rpm_failure";
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
      pBsp_(pB),
      lastControlUpdateSec_(pBsp_->getCurrentTime()) {}

std::tuple<bool, int, uint64_t> ControlLogic::readFanRpm(const Fan& fan) {
  std::string fanName = *fan.fanName();
  bool fanRpmReadSuccess = false;
  int fanRpm = 0;
  uint64_t rpmTimeStamp = 0;
  if (isFanPresentInDevice(fan)) {
    try {
      fanRpm = pBsp_->readSysfs(*fan.rpmSysfsPath());
      rpmTimeStamp = pBsp_->getCurrentTime();
      fanRpmReadSuccess = true;
    } catch (std::exception& e) {
      XLOG(ERR) << "Fan RPM access failed " << *fan.rpmSysfsPath();
    }
  }
  fb303::fbData->setCounter(
      fmt::format(kFanReadRpmFailure, fanName), !fanRpmReadSuccess);
  if (fanRpmReadSuccess) {
    XLOG(INFO) << fmt::format(
        "Control :: {}'s latest rpm is {}", fanName, fanRpm);
  } else {
    XLOG(INFO) << fmt::format("Control :: Failed to read {}'s rpm", fanName);
  }
  return std::make_tuple(!fanRpmReadSuccess, fanRpm, rpmTimeStamp);
}

float ControlLogic::calculatePid(
    const std::string& name,
    float value,
    SensorReadCache& readCache,
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
    readCache.targetPwmCache = pwm;
    pwmCalcCache.previousTargetPwm = pwm;
    pwmCalcCache.last_error = error;
  }
  pwmCalcCache.previousRead2 = previousRead1;
  pwmCalcCache.previousRead1 = value;
  XLOG(DBG1) << "Control :: Sensor : " << name << " Value : " << value
             << " [PID] Pwm : " << pwm;
  XLOG(DBG1) << "               dT : " << dT
             << " Time : " << pBsp_->getCurrentTime()
             << " LUD : " << lastControlUpdateSec_ << " Min : " << minVal
             << " Max : " << maxVal;
  return pwm;
}

float ControlLogic::calculateIncrementalPid(
    const std::string& name,
    float value,
    SensorReadCache& readCache,
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
  readCache.targetPwmCache = pwm;
  pwmCalcCache.previousRead2 = previousRead1;
  pwmCalcCache.previousRead1 = value;
  XLOG(DBG1) << "Control :: Sensor : " << name << " Value : " << value
             << " [IPID] Pwm : " << pwm;
  XLOG(DBG1) << "           Prev1  : " << previousRead1
             << " Prev2 : " << previousRead2;
  return pwm;
}

void ControlLogic::updateTargetPwm(const Sensor& sensor) {
  bool accelerate, deadFanExists;
  float previousSensorValue, sensorValue, targetPwm, pwm;
  float minVal = *sensor.setPoint() - *sensor.negHysteresis();
  float maxVal = *sensor.setPoint() + *sensor.posHysteresis();
  uint64_t dT;
  TempToPwmMap tableToUse;
  auto& readCache = sensorReadCaches_[*sensor.sensorName()];
  auto& pwmCalcCache = pwmCalcCaches_[*sensor.sensorName()];
  const auto& pwmCalcType = *sensor.pwmCalcType();

  if (pwmCalcType == constants::SENSOR_PWM_CALC_TYPE_FOUR_LINEAR_TABLE()) {
    accelerate = true;
    previousSensorValue = pwmCalcCache.previousSensorRead;
    sensorValue = readCache.adjustedReadCache;
    targetPwm = 0.0;
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
    readCache.targetPwmCache = targetPwm;
    XLOG(INFO) << "Control :: Sensor : " << *sensor.sensorName()
               << " Value : " << sensorValue << " [4CUV] Pwm : " << targetPwm;
  }

  if (pwmCalcType == constants::SENSOR_PWM_CALC_TYPE_INCREMENT_PID()) {
    pwm = calculateIncrementalPid(
        *sensor.sensorName(),
        readCache.adjustedReadCache,
        readCache,
        pwmCalcCache,
        *sensor.kp(),
        *sensor.ki(),
        *sensor.kd(),
        *sensor.setPoint());
    readCache.targetPwmCache = pwm;
  }

  if (pwmCalcType == constants::SENSOR_PWM_CALC_TYPE_PID()) {
    dT = pBsp_->getCurrentTime() - lastControlUpdateSec_;
    pwm = calculatePid(
        *sensor.sensorName(),
        readCache.adjustedReadCache,
        readCache,
        pwmCalcCache,
        *sensor.kp(),
        *sensor.ki(),
        *sensor.kd(),
        dT,
        minVal,
        maxVal);
    readCache.targetPwmCache = pwm;
  }

  // No matter what, PWM should be within
  // predefined upper and lower thresholds
  if (readCache.targetPwmCache > *config_.pwmUpperThreshold()) {
    readCache.targetPwmCache = *config_.pwmUpperThreshold();
  } else if (readCache.targetPwmCache < *config_.pwmLowerThreshold()) {
    readCache.targetPwmCache = *config_.pwmLowerThreshold();
  }
}

void ControlLogic::getSensorUpdate() {
  std::string sensorItemName;
  float rawValue = 0.0, adjustedValue;
  uint64_t calculatedTime = 0;
  for (auto& sensor : *config_.sensors()) {
    XLOG(INFO) << "Control :: Sensor Name : " << *sensor.sensorName();
    bool sensorAccessFail = false;
    sensorItemName = *sensor.sensorName();
    if (pSensor_->checkIfEntryExists(sensorItemName)) {
      XLOG(INFO) << "Control :: Sensor Exists. Getting the entry type";
      // 1.a Get the reading
      SensorEntryType entryType = pSensor_->getSensorEntryType(sensorItemName);
      switch (entryType) {
        case SensorEntryType::kSensorEntryInt:
          rawValue = pSensor_->getSensorDataInt(sensorItemName);
          rawValue = rawValue / *sensor.scale();
          break;
        case SensorEntryType::kSensorEntryFloat:
          rawValue = pSensor_->getSensorDataFloat(sensorItemName);
          rawValue = rawValue / *sensor.scale();
          break;
        default:
          facebook::fboss::FbossError(
              "Invalid Sensor Entry Type in entry name : ", sensorItemName);
          break;
      }
    } else {
      XLOG(ERR) << "Control :: Sensor Read Fail : " << sensorItemName;
      sensorAccessFail = true;
    }
    XLOG(INFO) << "Control :: Done raw sensor reading";

    auto& readCache = sensorReadCaches_[sensorItemName];

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
      calculatedTime = pSensor_->getLastUpdated(sensorItemName);
      readCache.lastUpdatedTime = calculatedTime;
      readCache.sensorFailed = false;
    }

    adjustedValue = rawValue;

    readCache.adjustedReadCache = adjustedValue;
    XLOG(INFO) << "Control :: Adjusted Value : " << adjustedValue;
    // 1.b Check and trigger alarm
    bool prevMajorAlarm = readCache.majorAlarmTriggered;
    readCache.majorAlarmTriggered =
        (adjustedValue >= *sensor.alarm()->highMajor());
    // If major alarm was triggered, write it as a ERR log
    if (!prevMajorAlarm && readCache.majorAlarmTriggered) {
      XLOG(ERR) << "Major Alarm Triggered on " << *sensor.sensorName()
                << " at value " << adjustedValue;
    } else if (prevMajorAlarm && !readCache.majorAlarmTriggered) {
      XLOG(WARN) << "Major Alarm Cleared on " << *sensor.sensorName()
                 << " at value " << adjustedValue;
    }
    bool prevMinorAlarm = readCache.minorAlarmTriggered;
    if (adjustedValue >= *sensor.alarm()->highMinor()) {
      if (readCache.soakStarted) {
        uint64_t timeDiffInSec =
            pBsp_->getCurrentTime() - readCache.soakStartedAt;
        if (timeDiffInSec >= *sensor.alarm()->minorSoakSeconds()) {
          readCache.minorAlarmTriggered = true;
          readCache.soakStarted = false;
        }
      } else {
        readCache.soakStarted = true;
        readCache.soakStartedAt = calculatedTime;
      }
    } else {
      readCache.minorAlarmTriggered = false;
      readCache.soakStarted = false;
    }
    // If minor alarm was triggered, write it as a WARN log
    if (!prevMinorAlarm && readCache.minorAlarmTriggered) {
      XLOG(WARN) << "Minor Alarm Triggered on " << *sensor.sensorName()
                 << " at value " << adjustedValue;
    }
    if (prevMinorAlarm && !readCache.minorAlarmTriggered) {
      XLOG(WARN) << "Minor Alarm Cleared on " << *sensor.sensorName()
                 << " at value " << adjustedValue;
    }
    // 1.c Calculate the target pwm in percent
    //     (the table or incremental pid should produce
    //      percent as its output)
    updateTargetPwm(sensor);
    XLOG(INFO) << *sensor.sensorName() << " has the target PWM of "
               << readCache.targetPwmCache;
  }
}

void ControlLogic::getOpticsUpdate() {
  // For all optics entry
  // Read optics array and set calculated pwm
  // No need to worry about timestamp, but update it anyway
  for (const auto& optic : *config_.optics()) {
    XLOG(INFO) << "Control :: Optics Group Name : " << *optic.opticName();
    std::string opticName = *optic.opticName();

    if (!pSensor_->checkIfOpticEntryExists(opticName)) {
      // No data found. Skip this config entry
      continue;
    } else {
      auto opticData = pSensor_->getOpticEntry(opticName);
      int pwmSoFar = 0;
      int dataSize = 0;
      const auto& aggregationType = optic.aggregationType();
      if (opticData != nullptr) {
        dataSize = opticData->data.size();
      }
      if (dataSize == 0) {
        // This data set is empty, already processed. Ignore.
        continue;
      } else {
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
            if (pwmForThis > pwmSoFar) {
              pwmSoFar = pwmForThis;
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
            float minVal =
                *pidSetting->setPoint() - *pidSetting->negHysteresis();
            float maxVal =
                *pidSetting->setPoint() + *pidSetting->posHysteresis();
            float pwm = calculatePid(
                opticType,
                value,
                readCache,
                pwmCalcCache,
                kp,
                ki,
                kd,
                dT,
                minVal,
                maxVal);
            if (pwm > pwmSoFar) {
              pwmSoFar = pwm;
            }
          }
        } else {
          throw facebook::fboss::FbossError(
              "Only max and pid aggregation is supported for optics!");
        }
        opticData->calculatedPwm = pwmSoFar;
        // As we consumed the data, clear the vector
        opticData->data.clear();
        opticData->dataProcessTimeStamp = opticData->lastOpticsUpdateTimeInSec;
      }
    }
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
    float currentPwm,
    float pwmSoFar) {
  float pwmToProgram = 0;
  bool writeSuccess{false};
  if ((*zone.slope() == 0) || (currentPwm == 0)) {
    pwmToProgram = pwmSoFar;
  } else {
    if (pwmSoFar > currentPwm) {
      if ((pwmSoFar - currentPwm) > *zone.slope()) {
        pwmToProgram = currentPwm + *zone.slope();
      } else {
        pwmToProgram = pwmSoFar;
      }
    } else if (pwmSoFar < currentPwm) {
      if ((currentPwm - pwmSoFar) > *zone.slope()) {
        pwmToProgram = currentPwm - *zone.slope();
      } else {
        pwmToProgram = pwmSoFar;
      }
    } else {
      pwmToProgram = pwmSoFar;
    }
  }
  int pwmInt =
      (int)(((*fan.pwmMax()) - (*fan.pwmMin())) * pwmToProgram / 100.0 +
            *fan.pwmMin());
  if (pwmInt < *fan.pwmMin()) {
    pwmInt = *fan.pwmMin();
  } else if (pwmInt > *fan.pwmMax()) {
    pwmInt = *fan.pwmMax();
  }
  writeSuccess = pBsp_->setFanPwmSysfs(*fan.pwmSysfsPath(), pwmInt);
  fb303::fbData->setCounter(
      fmt::format(kFanWriteFailure, *zone.zoneName(), *fan.fanName()),
      !writeSuccess);
  XLOG(INFO) << fmt::format(
      "Programmed Fan {} with PWM {} and Returned {} as PWM to program.",
      *fan.fanName(),
      pwmInt,
      pwmToProgram);
  return std::make_pair(!writeSuccess, pwmToProgram);
}

void ControlLogic::programLed(const Fan& fan, bool fanFailed) {
  unsigned int valueToWrite =
      (fanFailed ? *fan.fanFailLedVal() : *fan.fanGoodLedVal());
  pBsp_->setFanLedSysfs(*fan.ledSysfsPath(), valueToWrite);
  XLOG(INFO) << fmt::format(
      "Control :: Setting the LED of {} to {} (value: {})",
      *fan.fanName(),
      (fanFailed ? "Fail" : "Good"),
      valueToWrite);
}

float ControlLogic::calculateZonePwm(const Zone& zone, bool boostMode) {
  XLOG(INFO) << "Zone : " << *zone.zoneName();
  // First, calculate the pwm value for this zone
  auto zoneType = *zone.zoneType();
  float pwmSoFar = 0;
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
        if (pwmSoFar < pwmForThisSensor) {
          pwmSoFar = pwmForThisSensor;
        }
      } else if (zoneType == constants::ZONE_TYPE_MIN()) {
        if (pwmSoFar > pwmForThisSensor) {
          pwmSoFar = pwmForThisSensor;
        }
      } else if (zoneType == constants::ZONE_TYPE_AVG()) {
        pwmSoFar += pwmForThisSensor;
      } else {
        XLOG(ERR) << "Undefined Zone Type for zone : ", *zone.zoneName();
      }
      XLOG(INFO) << "  Sensor/Optic " << sensorName << " : " << pwmForThisSensor
                 << " Overall so far : " << pwmSoFar;
    }
  }
  if (zoneType == constants::ZONE_TYPE_AVG()) {
    pwmSoFar /= (float)totalPwmConsidered;
  }
  XLOG(INFO) << "  Final PWM : " << pwmSoFar;
  if (boostMode) {
    if (pwmSoFar < *config_.pwmBoostValue()) {
      pwmSoFar = *config_.pwmBoostValue();
    }
  }
  // Update the previous pwm value in each associated sensors,
  // so that they may be used in the next calculation.
  for (const auto& sensorName : *zone.sensorNames()) {
    if (isSensorPresentInConfig(sensorName)) {
      pwmCalcCaches_[sensorName].previousTargetPwm = pwmSoFar;
    }
  }
  return pwmSoFar;
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
        const auto [fanFailed, pwmToProgram] = programFan(
            zone,
            fan,
            *fanStatuses[*fan.fanName()].pwmToProgram(),
            *config_.pwmTransitionValue());
        fanStatuses[*fan.fanName()].pwmToProgram() = pwmToProgram;
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
  bool boostMode = false;
  bool boost_due_to_no_qsfp = false;

  // If we have not yet successfully read so far,
  // it does not make sense to update fan PWM out of no data.
  // So check if we read sensor data at least once. If not,
  // do it now.
  if (!pBsp_->checkIfInitialSensorDataRead()) {
    XLOG(INFO) << "Control :: Reading sensors for the first time";
    pBsp_->getSensorData(pSensor_);
  }

  // Determine proposed pwm value by each sensor read
  XLOG(INFO) << "Control :: Reading Sensor Status and determine per sensor PWM";
  getSensorUpdate();

  // Determine proposed pwm value by each optics read
  XLOG(INFO) << "Control :: Checking optics temperature to get pwm";
  getOpticsUpdate();

  // Check if we need to turn on boost mode
  XLOG(INFO) << "Control :: Failed Fans : " << numFanFailed_
             << " Failed Sensors : " << numSensorFailed_;

  uint64_t secondsSinceLastOpticsUpdate =
      pBsp_->getCurrentTime() - pSensor_->getLastQsfpSvcTime();
  if ((*config_.pwmBoostOnNoQsfpAfterInSec() != 0) &&
      (secondsSinceLastOpticsUpdate >= *config_.pwmBoostOnNoQsfpAfterInSec())) {
    boost_due_to_no_qsfp = true;
    XLOG(INFO) << "Control :: Boost mode condition for no optics update "
               << secondsSinceLastOpticsUpdate << " > "
               << *config_.pwmBoostOnNoQsfpAfterInSec();
  }

  fanStatuses_.withWLock([&](auto& fanStatuses) {
    // Now, check if Fan is in good shape, based on the previously read
    // sensor data
    XLOG(INFO) << "Control :: Checking Fan Status";

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
      programLed(fan, fanFailed);
      fanStatuses[*fan.fanName()].fanFailed() = fanFailed;
    }

    boostMode =
        (((*config_.pwmBoostOnNumDeadFan() != 0) &&
          (numFanFailed_ >= *config_.pwmBoostOnNumDeadFan())) ||
         ((*config_.pwmBoostOnNumDeadSensor() != 0) &&
          (numSensorFailed_ >= *config_.pwmBoostOnNumDeadSensor())) ||
         boost_due_to_no_qsfp);
    XLOG(INFO) << "Control :: Boost mode " << (boostMode ? "On" : "Off");

    XLOG(INFO) << "Control :: Updating Zones with new Fan value";
    for (const auto& zone : *config_.zones()) {
      // Finally, set pwm values per zone.
      // It's not recommended to put a fan in multiple zones,
      // even though it's possible to do so.
      float pwmSoFar = calculateZonePwm(zone, boostMode);
      for (const auto& fan : *config_.fans()) {
        // Skip if the fan doesn't belong to this zone
        if (std::find(
                zone.fanNames()->begin(),
                zone.fanNames()->end(),
                *fan.fanName()) == zone.fanNames()->end()) {
          continue;
        }

        const auto [fanFailed, pwmToProgram] = programFan(
            zone, fan, *fanStatuses[*fan.fanName()].pwmToProgram(), pwmSoFar);
        fanStatuses[*fan.fanName()].pwmToProgram() = pwmToProgram;

        if (fanFailed) {
          programLed(fan, fanFailed);
          // Only override the fanFailed if fan programming failed.
          fanStatuses[*fan.fanName()].fanFailed() = fanFailed;
        }
      }
    }
  });

  // Update the time stamp
  lastControlUpdateSec_ = pBsp_->getCurrentTime();
}

} // namespace facebook::fboss::platform::fan_service
