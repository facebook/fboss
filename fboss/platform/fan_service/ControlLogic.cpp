// Copyright 2021- Facebook. All rights reserved.

// Implementation of Control Logic class. Refer to .h file
// for function description

#include "fboss/platform/fan_service/ControlLogic.h"

#include <folly/logging/xlog.h>

#include "common/time/Time.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_constants.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace {
auto constexpr kFanWriteFailure = "fan_write.{}.{}.failure";
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
  auto rpmAccessType = *fan.rpmAccess()->accessType();
  XLOG(INFO) << "Control :: Fan name " << *fan.fanName()
             << " Access type : " << rpmAccessType;
  bool fanAccessFailed = false;
  int fanRpm = 0;
  uint64_t rpmTimeStamp = 0;

  if (!isFanPresentInDevice(fan)) {
    XLOG(INFO) << "Control :: Fan is absent for " << *fan.fanName();
    return std::make_tuple(true, 0, 0);
  }

  if (rpmAccessType == constants::ACCESS_TYPE_SYSFS()) {
    try {
      fanRpm = pBsp_->readSysfs(*fan.rpmAccess()->path());
      rpmTimeStamp = pBsp_->getCurrentTime();
    } catch (std::exception& e) {
      XLOG(ERR) << "Fan RPM access failed " << *fan.rpmAccess()->path();
      // Obvious. Sysfs fail means access fail
      fanAccessFailed = true;
    }
  } else {
    facebook::fboss::FbossError(
        "Unable to fetch Fan RPM due to invalide RPM sensor entry type :",
        fanName);
  }

  XLOG(INFO) << "Control :: RPM :" << fanRpm
             << " Failed : " << (fanAccessFailed ? "Yes" : "No");

  return std::make_tuple(fanAccessFailed, fanRpm, rpmTimeStamp);
}

void ControlLogic::updateTargetPwm(const Sensor& sensor) {
  bool accelerate, deadFanExists;
  float previousSensorValue, sensorValue, targetPwm;
  float value, lastPwm, kp, ki, kd, previousRead1, previousRead2, pwm;
  float error;
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
    value = readCache.adjustedReadCache;
    lastPwm = pwmCalcCache.previousTargetPwm;
    kp = *sensor.kp();
    ki = *sensor.ki();
    kd = *sensor.kd();
    previousRead1 = pwmCalcCache.previousRead1;
    previousRead2 = pwmCalcCache.previousRead2;
    pwm = lastPwm + (kp * (value - previousRead1)) +
        (ki * (value - *sensor.setPoint())) +
        (kd * (value - 2 * previousRead1 + previousRead2));
    // Even though the previous Target Pwm should be the zone pwm,
    // the best effort is made here. Zone should update this value.
    pwmCalcCache.previousTargetPwm = pwm;
    readCache.targetPwmCache = pwm;
    pwmCalcCache.previousRead2 = previousRead1;
    pwmCalcCache.previousRead1 = value;
    XLOG(INFO) << "Control :: Sensor : " << *sensor.sensorName()
               << " Value : " << value << " [IPID] Pwm : " << pwm;
    XLOG(INFO) << "           Prev1  : " << previousRead1
               << " Prev2 : " << previousRead2;
  }

  if (pwmCalcType == constants::SENSOR_PWM_CALC_TYPE_PID()) {
    value = readCache.adjustedReadCache;
    lastPwm = pwmCalcCache.previousTargetPwm;
    pwm = lastPwm;
    kp = *sensor.kp();
    ki = *sensor.ki();
    kd = *sensor.kd();
    previousRead1 = pwmCalcCache.previousRead1;
    previousRead2 = pwmCalcCache.previousRead2;
    dT = pBsp_->getCurrentTime() - lastControlUpdateSec_;

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
    XLOG(INFO) << "Control :: Sensor : " << *sensor.sensorName()
               << " Value : " << value << " [PID] Pwm : " << pwm;
    XLOG(INFO) << "               dT : " << dT
               << " Time : " << pBsp_->getCurrentTime()
               << " LUD : " << lastControlUpdateSec_ << " Min : " << minVal
               << " Max : " << maxVal;
  }

  // No matter what, PWM should be within
  // predefined upper and lower thresholds
  if (readCache.targetPwmCache > *config_.pwmUpperThreshold()) {
    readCache.targetPwmCache = *config_.pwmUpperThreshold();
  } else if (readCache.targetPwmCache < *config_.pwmLowerThreshold()) {
    readCache.targetPwmCache = *config_.pwmLowerThreshold();
  }
  return;
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

    // 1.b If adjustment table exists, adjust the raw value
    if (sensor.adjustmentTable()->size() == 0) {
      adjustedValue = rawValue;
    } else {
      float offset = 0;
      for (const auto& [k, v] : *sensor.adjustmentTable()) {
        if (rawValue >= k) {
          offset = v;
        }
        adjustedValue = rawValue + offset;
      }
    }
    readCache.adjustedReadCache = adjustedValue;
    XLOG(INFO) << "Control :: Adjusted Value : " << adjustedValue;
    // 1.c Check and trigger alarm
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
    // 1.d Check the range (if required), and do emergency
    // shutdown, if the value is out of range for more than
    // the "tolerance" times

    if (sensor.rangeCheck()) {
      if ((adjustedValue > *sensor.rangeCheck()->high()) ||
          (adjustedValue < *sensor.rangeCheck()->low())) {
        readCache.invalidRangeCheckCount++;
        if (readCache.invalidRangeCheckCount >=
            *sensor.rangeCheck()->tolerance()) {
          // ERR log only once.
          if (readCache.invalidRangeCheckCount ==
              *sensor.rangeCheck()->tolerance()) {
            XLOG(ERR) << "Sensor " << *sensor.sensorName()
                      << " out of range for too long!";
          }
          // If we are not yet in emergency state, do the emergency shutdown.
          if ((*sensor.rangeCheck()->invalidRangeAction() ==
               constants::RANGE_CHECK_ACTION_SHUTDOWN()) &&
              (pBsp_->getEmergencyState() == false)) {
            pBsp_->emergencyShutdown(true);
          }
        }
      } else {
        readCache.invalidRangeCheckCount = 0;
      }
    }
    // 1.e Calculate the target pwm in percent
    //     (the table or incremental pid should produce
    //      percent as its output)
    updateTargetPwm(sensor);
    XLOG(INFO) << *sensor.sensorName() << " has the target PWM of "
               << readCache.targetPwmCache;
  }
  return;
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
      if (opticData != nullptr) {
        dataSize = opticData->data.size();
      }
      if (dataSize == 0) {
        // This data set is empty, already processed. Ignore.
        continue;
      } else {
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
  uint64_t nowSec;

  // If no access method is listed in config,
  // skip any check and return true
  if (*fan.presenceAccess()->path() == "") {
    return true;
  }

  std::string presenceKey = *fan.fanName() + "_presence";
  auto presenceAccessType = *fan.presenceAccess()->accessType();
  if (presenceAccessType == constants::ACCESS_TYPE_THRIFT()) {
    // In the case of Thrift, we use the last data from Thrift read
    readVal = pSensor_->getSensorDataFloat(presenceKey);
    readSuccessful = true;
  } else if (presenceAccessType == constants::ACCESS_TYPE_SYSFS()) {
    nowSec = facebook::WallClockUtil::NowInSecFast();
    try {
      readVal = static_cast<unsigned>(
          pBsp_->readSysfs(*fan.presenceAccess()->path()));
      readSuccessful = true;
    } catch (std::exception& e) {
      XLOG(ERR) << "Failed to read sysfs " << *fan.presenceAccess()->path();
    }
    // If the read is successful, also update the SW state
    if (readSuccessful) {
      pSensor_->updateEntryFloat(presenceKey, readVal, nowSec);
    }
    readSuccessful = true;
  } else {
    throw facebook::fboss::FbossError(
        "Only Thrift and sysfs are supported for fan presence detection!");
  }
  XLOG(INFO) << "Control :: " << presenceKey << " : " << readVal << " vs good "
             << *fan.fanPresentVal() << " - bad " << *fan.fanMissingVal();

  if (readSuccessful) {
    if (readVal == *fan.fanPresentVal()) {
      return true;
    }
  }
  return false;
}

std::pair<bool, float> ControlLogic::programFan(
    const Zone& zone,
    const Fan& fan,
    float currentPwm,
    float pwmSoFar) {
  auto srcType = *fan.pwmAccess()->accessType();
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
      (int)(((*fan.pwmMax()) - (*fan.pwmMin())) * pwmToProgram / 100.0 + *fan.pwmMin());
  if (pwmInt < *fan.pwmMin()) {
    pwmInt = *fan.pwmMin();
  } else if (pwmInt > *fan.pwmMax()) {
    pwmInt = *fan.pwmMax();
  }
  if (srcType == constants::ACCESS_TYPE_SYSFS()) {
    writeSuccess = pBsp_->setFanPwmSysfs(*fan.pwmAccess()->path(), pwmInt);
  } else {
    XLOG(ERR) << "Unsupported PWM access type for : ", *fan.fanName();
  }
  fb303::fbData->setCounter(
      fmt::format(kFanWriteFailure, *zone.zoneName(), *fan.fanName()),
      !writeSuccess);
  XLOG(INFO) << folly ::sformat(
      "Programmed Fan {} with PWM {} and Returned {} as PWM to program.",
      *fan.fanName(),
      pwmInt,
      pwmToProgram);
  return std::make_pair(!writeSuccess, pwmToProgram);
}

void ControlLogic::programLed(const Fan& fan, bool fanFailed) {
  XLOG(INFO) << "Control :: Enter LED for " << *fan.fanName();
  unsigned int valueToWrite =
      (fanFailed ? *fan.fanFailLedVal() : *fan.fanGoodLedVal());
  if (*fan.ledAccess()->accessType() == constants::ACCESS_TYPE_SYSFS()) {
    pBsp_->setFanLedSysfs(*fan.ledAccess()->path(), valueToWrite);
  } else {
    XLOG(ERR) << folly::sformat(
        "Unsupported LED access type {} for : {}",
        *fan.ledAccess()->accessType(),
        *fan.fanName());
  }
  XLOG(INFO) << "Control :: Set the LED of " << *fan.fanName() << " to "
             << (fanFailed ? "Fail" : "Good") << "(" << valueToWrite << ") "
             << *fan.fanFailLedVal() << " vs " << *fan.fanGoodLedVal();
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
