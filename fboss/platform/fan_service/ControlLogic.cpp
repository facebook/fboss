// Copyright 2021- Facebook. All rights reserved.

// Implementation of Control Logic class. Refer to .h file
// for function description

#include "fboss/platform/fan_service/ControlLogic.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_constants.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"
// Additional FB helper funtion
#include "common/time/Time.h"

namespace {
auto constexpr kFanWriteFailure = "fan_write.{}.{}.failure";
auto constexpr kFanFailThresholdInSec = 300;
auto constexpr kSensorFailThresholdInSec = 300;
} // namespace

namespace facebook::fboss::platform {
ControlLogic::ControlLogic(
    std::shared_ptr<ServiceConfig> pC,
    std::shared_ptr<Bsp> pB) {
  pConfig_ = pC;
  pBsp_ = pB;
  numFanFailed_ = 0;
  numSensorFailed_ = 0;
  lastControlUpdateSec_ = pBsp_->getCurrentTime();
}

ControlLogic::~ControlLogic() {}

void ControlLogic::getFanUpdate() {
  SensorEntryType entryType;

  // Zeroth, check Fan RPM and Status
  for (const auto& fan : pConfig_->fans) {
    std::string fanName = *fan.fanName();
    XLOG(INFO) << "Control :: Fan name " << *fan.fanName() << " Access type : "
               << static_cast<int>(*fan.rpmAccess()->accessType());
    bool fanAccessFail = false, fanMissing = false;
    int fanRpm;
    uint64_t rpmTimeStamp;

    // Check if RPM name in Thrift data is overridden
    if (*fan.rpmAccess()->accessType() ==
        fan_config_structs::SourceType::kSrcThrift) {
      fanName = *fan.rpmAccess()->path();
    }
    if (!checkIfFanPresent(fan)) {
      fanMissing = true;
    }
    // If source type is not specified (SRC_INVALID), we treat it as Thrift.
    if ((*fan.rpmAccess()->accessType() ==
         fan_config_structs::SourceType::kSrcInvalid) ||
        (*fan.rpmAccess()->accessType() ==
         fan_config_structs::SourceType::kSrcThrift)) {
      if (pSensor_->checkIfEntryExists(fanName)) {
        entryType = pSensor_->getSensorEntryType(fanName);
        switch (entryType) {
          case SensorEntryType::kSensorEntryInt:
            fanRpm = static_cast<int>(pSensor_->getSensorDataInt(fanName));
            break;
          case SensorEntryType::kSensorEntryFloat:
            fanRpm = static_cast<int>(pSensor_->getSensorDataFloat(fanName));
            break;
        }
        rpmTimeStamp = pSensor_->getLastUpdated(fanName);
        if (rpmTimeStamp == pConfig_->fanStatuses[fanName].timeStamp) {
          // If read method is Thrift, but read time stamp is stale
          // , consider that as access failure
          fanAccessFail = true;
        } else {
          pConfig_->fanStatuses[fanName].rpm = fanRpm;
          pConfig_->fanStatuses[fanName].timeStamp = rpmTimeStamp;
        }
      } else {
        // If no entry in Thrift response, consider that as failure
        fanAccessFail = true;
      }
    } else if (
        *fan.rpmAccess()->accessType() ==
        fan_config_structs::SourceType::kSrcSysfs) {
      try {
        pConfig_->fanStatuses[fanName].rpm =
            pBsp_->readSysfs(*fan.rpmAccess()->path());
        pConfig_->fanStatuses[fanName].timeStamp = pBsp_->getCurrentTime();
      } catch (std::exception& e) {
        XLOG(ERR) << "Fan RPM access fail " << *fan.rpmAccess()->path();
        // Obvious. Sysfs fail means access fail
        fanAccessFail = true;
      }
    } else {
      facebook::fboss::FbossError(
          "Unable to fetch Fan RPM due to invalide RPM sensor entry type :",
          fanName);
    }

    // We honor fan presence bit first,
    // If fan is present, then check if access has failed
    if (fanMissing) {
      setFanFailState(fan, true);
    } else if (fanAccessFail) {
      uint64_t timeDiffInSec =
          pBsp_->getCurrentTime() - pConfig_->fanStatuses[fanName].timeStamp;
      if (timeDiffInSec >= kFanFailThresholdInSec) {
        setFanFailState(fan, true);
        numFanFailed_++;
      }
    } else {
      setFanFailState(fan, false);
    }
    XLOG(INFO) << "Control :: RPM :" << pConfig_->fanStatuses[fanName].rpm
               << " Failed : "
               << (pConfig_->fanStatuses[fanName].fanFailed ? "Yes" : "No");
  }
  XLOG(INFO) << "Control :: Done checking Fans Status";
  return;
}

void ControlLogic::updateTargetPwm(const Sensor& sensor) {
  bool accelerate, deadFanExists;
  float previousSensorValue, sensorValue, targetPwm;
  float value, lastPwm, kp, ki, kd, previousRead1, previousRead2, pwm;
  float error;
  float minVal = sensor.setPoint - sensor.negHysteresis;
  float maxVal = sensor.setPoint + sensor.posHysteresis;
  uint64_t dT;
  std::vector<std::pair<float, float>> tableToUse;
  auto& readCache = pConfig_->sensorReadCaches[sensor.sensorName];
  auto& pwmCalcCache = pConfig_->pwmCalcCaches[sensor.sensorName];
  switch (sensor.calculationType) {
    case fan_config_structs::SensorPwmCalcType::kSensorPwmCalcFourLinearTable:
      accelerate = true;
      previousSensorValue = pwmCalcCache.previousSensorRead;
      sensorValue = readCache.adjustedReadCache;
      targetPwm = 0.0;
      deadFanExists = (numFanFailed_ > 0);
      accelerate =
          ((previousSensorValue == 0) || (sensorValue > previousSensorValue));
      if (accelerate && !deadFanExists) {
        tableToUse = sensor.normalUp;
      } else if (!accelerate && !deadFanExists) {
        tableToUse = sensor.normalDown;
      } else if (accelerate && deadFanExists) {
        tableToUse = sensor.failUp;
      } else {
        tableToUse = sensor.failDown;
      }
      // Start with the lowest value
      targetPwm = tableToUse[0].second;
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
      XLOG(INFO) << "Control :: Sensor : " << sensor.sensorName
                 << " Value : " << sensorValue << " [4CUV] Pwm : " << targetPwm;
      break;

    case fan_config_structs::SensorPwmCalcType::kSensorPwmCalcIncrementPid:
      value = readCache.adjustedReadCache;
      lastPwm = pwmCalcCache.previousTargetPwm;
      kp = sensor.kp;
      ki = sensor.ki;
      kd = sensor.kd;
      previousRead1 = pwmCalcCache.previousRead1;
      previousRead2 = pwmCalcCache.previousRead2;
      pwm = lastPwm + (kp * (value - previousRead1)) +
          (ki * (value - sensor.setPoint)) +
          (kd * (value - 2 * previousRead1 + previousRead2));
      // Even though the previous Target Pwm should be the zone pwm,
      // the best effort is made here. Zone should update this value.
      pwmCalcCache.previousTargetPwm = pwm;
      readCache.targetPwmCache = pwm;
      pwmCalcCache.previousRead2 = previousRead1;
      pwmCalcCache.previousRead1 = value;
      XLOG(INFO) << "Control :: Sensor : " << sensor.sensorName
                 << " Value : " << value << " [IPID] Pwm : " << pwm;
      XLOG(INFO) << "           Prev1  : " << previousRead1
                 << " Prev2 : " << previousRead2;

      break;

    case fan_config_structs::SensorPwmCalcType::kSensorPwmCalcPid:
      value = readCache.adjustedReadCache;
      lastPwm = pwmCalcCache.previousTargetPwm;
      pwm = lastPwm;
      kp = sensor.kp;
      ki = sensor.ki;
      kd = sensor.kd;
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
      XLOG(INFO) << "Control :: Sensor : " << sensor.sensorName
                 << " Value : " << value << " [PID] Pwm : " << pwm;
      XLOG(INFO) << "               dT : " << dT
                 << " Time : " << pBsp_->getCurrentTime()
                 << " LUD : " << lastControlUpdateSec_ << " Min : " << minVal
                 << " Max : " << maxVal;
      break;

    case fan_config_structs::SensorPwmCalcType::kSensorPwmCalcDisable:
      // Do nothing
      XLOG(WARN) << "Control :: Sensor : " << sensor.sensorName
                 << "Do Nothing ";
      break;
    default:
      facebook::fboss::FbossError(
          "Invalid PWM Calculation Type for sensor", sensor.sensorName);
      break;
  }
  // No matter what, PWM should be within
  // predefined upper and lower thresholds
  if (readCache.targetPwmCache > pConfig_->getPwmUpperThreshold()) {
    readCache.targetPwmCache = pConfig_->getPwmUpperThreshold();
  } else if (readCache.targetPwmCache < pConfig_->getPwmLowerThreshold()) {
    readCache.targetPwmCache = pConfig_->getPwmLowerThreshold();
  }
  return;
}

void ControlLogic::getSensorUpdate() {
  std::string sensorItemName;
  float rawValue = 0.0, adjustedValue;
  uint64_t calculatedTime = 0;
  for (auto& sensor : pConfig_->sensors) {
    XLOG(INFO) << "Control :: Sensor Name : " << sensor.sensorName;
    bool sensorAccessFail = false;
    sensorItemName = sensor.sensorName;
    if (pSensor_->checkIfEntryExists(sensorItemName)) {
      XLOG(INFO) << "Control :: Sensor Exists. Getting the entry type";
      // 1.a Get the reading
      SensorEntryType entryType = pSensor_->getSensorEntryType(sensorItemName);
      switch (entryType) {
        case SensorEntryType::kSensorEntryInt:
          rawValue = pSensor_->getSensorDataInt(sensorItemName);
          rawValue = rawValue / sensor.scale;
          break;
        case SensorEntryType::kSensorEntryFloat:
          rawValue = pSensor_->getSensorDataFloat(sensorItemName);
          rawValue = rawValue / sensor.scale;
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

    auto& readCache = pConfig_->sensorReadCaches[sensorItemName];

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
    if (sensor.offsetTable.size() == 0) {
      adjustedValue = rawValue;
    } else {
      float offset = 0;
      for (const auto& [k, v] : sensor.offsetTable) {
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
        (adjustedValue >= *sensor.alarm.highMajor());
    // If major alarm was triggered, write it as a ERR log
    if (!prevMajorAlarm && readCache.majorAlarmTriggered) {
      XLOG(ERR) << "Major Alarm Triggered on " << sensor.sensorName
                << " at value " << adjustedValue;
    } else if (prevMajorAlarm && !readCache.majorAlarmTriggered) {
      XLOG(WARN) << "Major Alarm Cleared on " << sensor.sensorName
                 << " at value " << adjustedValue;
    }
    bool prevMinorAlarm = readCache.minorAlarmTriggered;
    if (adjustedValue >= *sensor.alarm.highMinor()) {
      if (readCache.soakStarted) {
        uint64_t timeDiffInSec =
            pBsp_->getCurrentTime() - readCache.soakStartedAt;
        if (timeDiffInSec >= *sensor.alarm.minorSoakSeconds()) {
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
      XLOG(WARN) << "Minor Alarm Triggered on " << sensor.sensorName
                 << " at value " << adjustedValue;
    }
    if (prevMinorAlarm && !readCache.minorAlarmTriggered) {
      XLOG(WARN) << "Minor Alarm Cleared on " << sensor.sensorName
                 << " at value " << adjustedValue;
    }
    // 1.d Check the range (if required), and do emergency
    // shutdown, if the value is out of range for more than
    // the "tolerance" times

    if (sensor.rangeCheck) {
      if ((adjustedValue > *sensor.rangeCheck->high()) ||
          (adjustedValue < *sensor.rangeCheck->low())) {
        sensor.rangeCheck->invalidCount() =
            *sensor.rangeCheck->invalidCount() + 1;
        if (*sensor.rangeCheck->invalidCount() >=
            *sensor.rangeCheck->tolerance()) {
          // ERR log only once.
          if (*sensor.rangeCheck->invalidCount() ==
              *sensor.rangeCheck->tolerance()) {
            XLOG(ERR) << "Sensor " << sensor.sensorName
                      << " out of range for too long!";
          }
          // If we are not yet in emergency state, do the emergency shutdown.
          if ((*sensor.rangeCheck->invalidRangeAction() ==
               fan_config_structs::fan_config_structs_constants::
                   RANGE_CHECK_ACTION_SHUTDOWN()) &&
              (pBsp_->getEmergencyState() == false)) {
            pBsp_->emergencyShutdown(pConfig_, true);
          }
        }
      } else {
        sensor.rangeCheck->invalidCount() = 0;
      }
    }
    // 1.e Calculate the target pwm in percent
    //     (the table or incremental pid should produce
    //      percent as its output)
    updateTargetPwm(sensor);
    XLOG(INFO) << sensor.sensorName << " has the target PWM of "
               << readCache.targetPwmCache;
  }
  return;
}

void ControlLogic::getOpticsUpdate() {
  // For all optics entry
  // Read optics array and set calculated pwm
  // No need to worry about timestamp, but update it anyway
  for (const auto& optic : pConfig_->optics) {
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
        for (const auto& [dataType, value] : opticData->data) {
          int pwmForThis = 0;
          auto tablePointer =
              pConfig_->getConfigOpticTable(opticName, dataType);
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

Sensor* ControlLogic::findSensorConfig(const std::string& sensorName) {
  for (auto& sensor : pConfig_->sensors) {
    if (sensor.sensorName == sensorName) {
      return &sensor;
    }
  }
  return nullptr;
}

bool ControlLogic::checkIfFanPresent(const fan_config_structs::Fan& fan) {
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
  switch (presenceAccessType) {
    case fan_config_structs::SourceType::kSrcThrift:
      // In the case of Thrift, we use the last data from Thrift read
      readVal = pSensor_->getSensorDataFloat(presenceKey);
      readSuccessful = true;
      break;
    case fan_config_structs::SourceType::kSrcSysfs:
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
      break;
    case fan_config_structs::SourceType::kSrcInvalid:
    case fan_config_structs::SourceType::kSrcRest:
    case fan_config_structs::SourceType::kSrcUtil:
    default:
      throw facebook::fboss::FbossError(
          "Only Thrift and sysfs are supported for fan presence detection!");
      break;
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

void ControlLogic::programFan(
    const fan_config_structs::Zone& zone,
    float pwmSoFar) {
  for (const auto& fan : pConfig_->fans) {
    auto srcType = *fan.pwmAccess()->accessType();
    float pwmToProgram = 0;
    float currentPwm = pConfig_->fanStatuses[*fan.fanName()].currentPwm;
    bool writeSuccess{false};
    // If this fan does not belong to the current zone, do not do anything
    if (std::find(
            zone.fanNames()->begin(), zone.fanNames()->end(), *fan.fanName()) ==
        zone.fanNames()->end()) {
      continue;
    }
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
    switch (srcType) {
      case fan_config_structs::SourceType::kSrcSysfs:
        writeSuccess = pBsp_->setFanPwmSysfs(*fan.pwmAccess()->path(), pwmInt);
        if (!writeSuccess) {
          setFanFailState(fan, true);
        }
        break;
      case fan_config_structs::SourceType::kSrcUtil:
        writeSuccess = pBsp_->setFanPwmShell(
            *fan.pwmAccess()->path(), *fan.fanName(), pwmInt);
        if (!writeSuccess) {
          setFanFailState(fan, true);
        }
        break;
      case fan_config_structs::SourceType::kSrcThrift:
      case fan_config_structs::SourceType::kSrcRest:
      case fan_config_structs::SourceType::kSrcInvalid:
      default:
        facebook::fboss::FbossError(
            "Unsupported PWM access type for : ", *fan.fanName());
    }
    fb303::fbData->setCounter(
        fmt::format(kFanWriteFailure, *zone.zoneName(), *fan.fanName()),
        !writeSuccess);
    pConfig_->fanStatuses[*fan.fanName()].currentPwm = pwmToProgram;
  }
}

void ControlLogic::setFanFailState(
    const fan_config_structs::Fan& fan,
    bool fanFailed) {
  XLOG(INFO) << "Control :: Enter LED for " << *fan.fanName();
  bool ledAccessNeeded = false;
  if (pConfig_->fanStatuses[*fan.fanName()].firstTimeLedAccess) {
    ledAccessNeeded = true;
    pConfig_->fanStatuses[*fan.fanName()].firstTimeLedAccess = false;
  }
  if (fanFailed) {
    // Fan failed.
    // If the previous status was fan good, we need to set fan LED color
    if (!pConfig_->fanStatuses[*fan.fanName()].fanFailed) {
      // We need to change internal state
      pConfig_->fanStatuses[*fan.fanName()].fanFailed = true;
      // Also change led color to "FAIL", if Fan LED is available
      if (*fan.pwmAccess()->path() != "") {
        ledAccessNeeded = true;
      }
    }
  } else {
    // Fan did NOT fail (is in a good shape)
    // If the previous status was fan fail, we need to set fan LED color
    if (pConfig_->fanStatuses[*fan.fanName()].fanFailed) {
      // We need to change internal state
      pConfig_->fanStatuses[*fan.fanName()].fanFailed = false;
      // Also change led color to "GOOD", if Fan LED is available
      ledAccessNeeded = true;
    }
  }
  if (ledAccessNeeded) {
    unsigned int valueToWrite =
        (fanFailed ? *fan.fanFailLedVal() : *fan.fanGoodLedVal());
    switch (*fan.ledAccess()->accessType()) {
      case fan_config_structs::SourceType::kSrcSysfs:
        pBsp_->setFanLedSysfs(*fan.ledAccess()->path(), valueToWrite);
        break;
      case fan_config_structs::SourceType::kSrcUtil:
        pBsp_->setFanLedShell(
            *fan.ledAccess()->path(), *fan.fanName(), valueToWrite);
        break;
      case fan_config_structs::SourceType::kSrcThrift:
      case fan_config_structs::SourceType::kSrcRest:
      case fan_config_structs::SourceType::kSrcInvalid:
      default:
        facebook::fboss::FbossError(
            "Unsupported LED access type for : ", *fan.fanName());
    }
    XLOG(INFO) << "Control :: Set the LED of " << *fan.fanName() << " to "
               << (fanFailed ? "Fail" : "Good") << "(" << valueToWrite << ") "
               << *fan.fanFailLedVal() << " vs " << *fan.fanGoodLedVal();
  }
}

void ControlLogic::adjustZoneFans(bool boostMode) {
  for (const auto& zone : pConfig_->zones) {
    float pwmSoFar = 0;
    XLOG(INFO) << "Zone : " << *zone.zoneName();
    // First, calculate the pwm value for this zone
    auto zoneType = *zone.zoneType();
    int totalPwmConsidered = 0;
    for (const auto& sensorName : *zone.sensorNames()) {
      auto pSensorConfig_ = findSensorConfig(sensorName);
      if ((pSensorConfig_ != nullptr) ||
          (pSensor_->checkIfOpticEntryExists(sensorName))) {
        totalPwmConsidered++;
        float pwmForThisSensor;
        if (pSensorConfig_ != nullptr) {
          // If this is a sensor name
          pwmForThisSensor =
              pConfig_->sensorReadCaches[sensorName].targetPwmCache;
        } else {
          // If this is an optics name
          pwmForThisSensor = pSensor_->getOpticsPwm(sensorName);
        }
        switch (zoneType) {
          case fan_config_structs::ZoneType::kZoneMax:
            if (pwmSoFar < pwmForThisSensor) {
              pwmSoFar = pwmForThisSensor;
            }
            break;
          case fan_config_structs::ZoneType::kZoneMin:
            if (pwmSoFar > pwmForThisSensor) {
              pwmSoFar = pwmForThisSensor;
            }
            break;
          case fan_config_structs::ZoneType::kZoneAvg:
            pwmSoFar += pwmForThisSensor;
            break;
          case fan_config_structs::ZoneType::kZoneInval:
          default:
            facebook::fboss::FbossError(
                "Undefined Zone Type for zone : ", *zone.zoneName());
            break;
        }
        XLOG(INFO) << "  Sensor/Optic " << sensorName << " : "
                   << pwmForThisSensor << " Overall so far : " << pwmSoFar;
      }
    }
    if (zoneType == fan_config_structs::ZoneType::kZoneAvg) {
      pwmSoFar /= (float)totalPwmConsidered;
    }
    XLOG(INFO) << "  Final PWM : " << pwmSoFar;
    if (boostMode) {
      if (pwmSoFar < pConfig_->getPwmBoostValue()) {
        pwmSoFar = pConfig_->getPwmBoostValue();
      }
    }
    // Update the previous pwm value in each associated sensors,
    // so that they may be used in the next calculation.
    for (const auto& sensorName : *zone.sensorNames()) {
      auto pSensorConfig_ = findSensorConfig(sensorName);
      if (pSensorConfig_ != nullptr) {
        pConfig_->pwmCalcCaches[sensorName].previousTargetPwm = pwmSoFar;
      }
    }
    // Secondly, set Zone pwm value to all the fans in the zone
    programFan(zone, pwmSoFar);
  }
}

void ControlLogic::setTransitionValue() {
  for (const auto& zone : pConfig_->zones) {
    for (const auto& fan : pConfig_->fans) {
      // If this fan belongs to the zone, then write the transitional value
      if (std::find(
              zone.fanNames()->begin(),
              zone.fanNames()->end(),
              *fan.fanName()) != zone.fanNames()->end()) {
        for (const auto& sensorName : *zone.sensorNames()) {
          auto pSensorConfig_ = findSensorConfig(sensorName);
          if (pSensorConfig_ != nullptr) {
            pConfig_->pwmCalcCaches[sensorName].previousTargetPwm =
                pConfig_->getPwmTransitionValue();
          }
        }
        programFan(zone, pConfig_->getPwmTransitionValue());
      }
    }
  }
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
    pBsp_->getSensorData(pConfig_, pSensor_);
  }

  // Now, check if Fan is in good shape, based on the previously read
  // sensor data
  XLOG(INFO) << "Control :: Checking Fan Status";
  getFanUpdate();

  // Determine proposed pwm value by each sensor read
  XLOG(INFO) << "Control :: Reading Sensor Status and determine per sensor PWM";
  getSensorUpdate();

  // Determine proposed pwm value by each optics read
  XLOG(INFO) << "Control :: Checking optics temperature to get pwm";
  getOpticsUpdate();

  // Check if we need to turn on boost mode
  XLOG(INFO) << "Control :: Failed Sensor : " << numFanFailed_
             << " Failed Sensor : " << numSensorFailed_;

  uint64_t secondsSinceLastOpticsUpdate =
      pBsp_->getCurrentTime() - pSensor_->getLastQsfpSvcTime();
  if ((pConfig_->pwmBoostNoQsfpAfterInSec != 0) &&
      (secondsSinceLastOpticsUpdate >= pConfig_->pwmBoostNoQsfpAfterInSec)) {
    boost_due_to_no_qsfp = true;
    XLOG(INFO) << "Control :: Boost mode condition for no optics update "
               << secondsSinceLastOpticsUpdate << " > "
               << pConfig_->pwmBoostNoQsfpAfterInSec;
  }

  boostMode =
      (((pConfig_->pwmBoostOnDeadFan != 0) &&
        (numFanFailed_ >= pConfig_->pwmBoostOnDeadFan)) ||
       ((pConfig_->pwmBoostOnDeadSensor != 0) &&
        (numSensorFailed_ >= pConfig_->pwmBoostOnDeadSensor)) ||
       boost_due_to_no_qsfp);
  XLOG(INFO) << "Control :: Boost mode " << (boostMode ? "On" : "Off");
  XLOG(INFO) << "Control :: Updating Zones with new Fan value";

  // Finally, set pwm values per zone.
  // It's not recommended to put a fan in multiple zones,
  // even though it's possible to do so.
  adjustZoneFans(boostMode);
  // Update the time stamp
  lastControlUpdateSec_ = pBsp_->getCurrentTime();
}

} // namespace facebook::fboss::platform
