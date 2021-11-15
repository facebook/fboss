// Copyright 2021- Facebook. All rights reserved.

// Implementation of Control Logic class. Refer to .h file
// for function description

#include "fboss/platform/fan_service/ControlLogic.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"

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
  uint64_t alculatedTime;
  SensorEntryType entryType;

  // Zeroth, check Fan RPM and Status
  for (auto fanItem = pConfig_->fans.begin(); fanItem != pConfig_->fans.end();
       ++fanItem) {
    XLOG(INFO) << "Control :: Fan name " << fanItem->fanName
               << " Access type : "
               << static_cast<int>(fanItem->rpmAccess.accessType);
    bool fanAccessFail = false;
    int fanRpm;
    uint64_t rpmTimeStamp;
    std::string fanItemName = fanItem->fanName;
    // Check if RPM name in Thrift data is overridden
    if (fanItem->rpmAccess.accessType ==
        fan_config_structs::SourceType::kSrcThrift) {
      fanItemName = fanItem->rpmAccess.path;
    }
    // If source type is not specified (SRC_INVALID), we treat it as Thrift.
    if ((fanItem->rpmAccess.accessType ==
         fan_config_structs::SourceType::kSrcInvalid) ||
        (fanItem->rpmAccess.accessType ==
         fan_config_structs::SourceType::kSrcThrift)) {
      if (pSensor_->checkIfEntryExists(fanItemName)) {
        entryType = pSensor_->getSensorEntryType(fanItemName);
        switch (entryType) {
          case kSensorEntryInt:
            fanRpm = static_cast<int>(pSensor_->getSensorDataInt(fanItemName));
            break;
          case kSensorEntryFloat:
            fanRpm =
                static_cast<int>(pSensor_->getSensorDataFloat(fanItemName));
            break;
          default:
            facebook::fboss::FbossError(
                "Invalid Fan RPM Sensor Entry Type in entry name : ",
                fanItemName);
            break;
        }
        rpmTimeStamp = pSensor_->getLastUpdated(fanItemName);
        if (rpmTimeStamp == fanItem->fanStatus.timeStamp) {
          fanAccessFail = true;
        } else {
          fanItem->fanStatus.rpm = fanRpm;
          fanItem->fanStatus.fanFailed = false;
          fanItem->fanStatus.timeStamp = rpmTimeStamp;
        }
      } else {
        fanAccessFail = true;
      }
    } else if (
        fanItem->rpmAccess.accessType ==
        fan_config_structs::SourceType::kSrcSysfs) {
      try {
        XLOG(INFO) << "Reading RPM of fan " << fanItem->fanName << " at "
                   << fanItem->rpmAccess.path;
        fanItem->fanStatus.rpm = pBsp_->readSysfs(fanItem->rpmAccess.path);
        fanItem->fanStatus.fanFailed = false;
        fanItem->fanStatus.timeStamp = pBsp_->getCurrentTime();
        XLOG(INFO) << "Successfully read  " << fanItem->fanName << " at "
                   << fanItem->rpmAccess.path;
      } catch (std::exception& e) {
        XLOG(ERR) << "Fan RPM access fail " << fanItem->rpmAccess.path;
        fanAccessFail = true;
      }
    } else {
      facebook::fboss::FbossError(
          "Unable to fetch Fan RPM due to invalide RPM sensor entry type :",
          fanItemName);
    }

    if (fanAccessFail) {
      uint64_t timeDiffInSec =
          pBsp_->getCurrentTime() - fanItem->fanStatus.timeStamp;
      if (timeDiffInSec >= fanItem->fanFailThresholdInSec) {
        fanItem->fanStatus.fanFailed = true;
        numFanFailed_++;
      }
    }
    XLOG(INFO) << "Control :: RPM :" << fanItem->fanStatus.rpm
               << " Failed : " << (fanItem->fanStatus.fanFailed ? "Yes" : "No");
  }
  XLOG(INFO) << "Control :: Done checking Fans Status";
  return;
}

void ControlLogic::updateTargetPwm(Sensor* sensorItem) {
  bool accelerate, deadFanExists;
  float previousSensorValue, sensorValue, targetPwm;
  float value, lastPwm, kp, ki, kd, previousRead1, previousRead2, pwm;
  float minVal, maxVal, error, d;
  uint64_t dT;
  std::vector<std::pair<float, float>> tableToUse;
  switch (sensorItem->calculationType) {
    case fan_config_structs::SensorPwmCalcType::kSensorPwmCalcFourLinearTable:
      accelerate = true;
      previousSensorValue = sensorItem->fourCurves.previousSensorRead;
      sensorValue = sensorItem->processedData.adjustedReadCache;
      targetPwm = 0.0;
      deadFanExists = (numFanFailed_ > 0);
      accelerate =
          ((previousSensorValue == 0) || (sensorValue > previousSensorValue));
      if (accelerate && !deadFanExists) {
        tableToUse = sensorItem->fourCurves.normalUp;
      } else if (!accelerate && !deadFanExists) {
        tableToUse = sensorItem->fourCurves.normalDown;
      } else if (accelerate && deadFanExists) {
        tableToUse = sensorItem->fourCurves.failUp;
      } else {
        tableToUse = sensorItem->fourCurves.failDown;
      }
      // Start with the lowest value
      targetPwm = tableToUse[0].second;
      for (auto tableEntry = tableToUse.begin(); tableEntry != tableToUse.end();
           ++tableEntry) {
        if (sensorValue > tableEntry->first) {
          targetPwm = tableEntry->second;
        }
      }
      if (accelerate) {
        // If accleration is needed, new pwm should be bigger than old value
        if (targetPwm < sensorItem->processedData.targetPwmCache) {
          targetPwm = sensorItem->processedData.targetPwmCache;
        }
      } else {
        // If deceleration is needed, new pwm should be smaller than old one
        if (targetPwm > sensorItem->processedData.targetPwmCache) {
          targetPwm = sensorItem->processedData.targetPwmCache;
        }
      }
      sensorItem->fourCurves.previousSensorRead = sensorValue;
      sensorItem->processedData.targetPwmCache = targetPwm;
      XLOG(INFO) << "Control :: Sensor : " << sensorItem->sensorName
                 << " Value : " << sensorValue << " [4CUV] Pwm : " << targetPwm;
      break;

    case fan_config_structs::SensorPwmCalcType::kSensorPwmCalcIncrementPid:
      value = sensorItem->processedData.adjustedReadCache;
      lastPwm = sensorItem->incrementPid.previousTargetPwm;
      kp = sensorItem->incrementPid.kp;
      ki = sensorItem->incrementPid.ki;
      kd = sensorItem->incrementPid.kd;
      previousRead1 = sensorItem->incrementPid.previousRead1;
      previousRead2 = sensorItem->incrementPid.previousRead2;
      pwm = lastPwm + (kp * (value - previousRead1)) +
          (ki * (value - sensorItem->incrementPid.setPoint)) +
          (kd * (value - 2 * previousRead1 + previousRead2));
      // Even though the previous Target Pwm should be the zone pwm,
      // the best effort is made here. Zone should update this value.
      sensorItem->incrementPid.previousTargetPwm = pwm;
      sensorItem->processedData.targetPwmCache = pwm;
      sensorItem->incrementPid.previousRead2 = previousRead1;
      sensorItem->incrementPid.previousRead1 = value;
      XLOG(INFO) << "Control :: Sensor : " << sensorItem->sensorName
                 << " Value : " << value << " [IPID] Pwm : " << pwm;
      XLOG(INFO) << "           Prev1  : " << previousRead1
                 << " Prev2 : " << previousRead2;

      break;

    case fan_config_structs::SensorPwmCalcType::kSensorPwmCalcPid:
      value = sensorItem->processedData.adjustedReadCache;
      lastPwm = sensorItem->incrementPid.previousTargetPwm;
      pwm = lastPwm;
      kp = sensorItem->incrementPid.kp;
      ki = sensorItem->incrementPid.ki;
      kd = sensorItem->incrementPid.kd;
      previousRead1 = sensorItem->incrementPid.previousRead1;
      previousRead2 = sensorItem->incrementPid.previousRead2;
      dT = pBsp_->getCurrentTime() - lastControlUpdateSec_;
      minVal = sensorItem->incrementPid.minVal;
      maxVal = sensorItem->incrementPid.maxVal;

      if (value < minVal) {
        sensorItem->incrementPid.i = 0;
        sensorItem->incrementPid.previousTargetPwm = 0;
      }
      if (value > maxVal) {
        error = maxVal - value;
        sensorItem->incrementPid.i = sensorItem->incrementPid.i + error * dT;
        d = (error - sensorItem->incrementPid.last_error);
        pwm = kp * error + ki * sensorItem->incrementPid.i + kd * d;
        sensorItem->processedData.targetPwmCache = pwm;
        sensorItem->incrementPid.previousTargetPwm = pwm;
        sensorItem->incrementPid.last_error = error;
      }
      sensorItem->incrementPid.previousRead2 = previousRead1;
      sensorItem->incrementPid.previousRead1 = value;
      XLOG(INFO) << "Control :: Sensor : " << sensorItem->sensorName
                 << " Value : " << value << " [PID] Pwm : " << pwm;
      XLOG(INFO) << "               dT : " << dT
                 << " Time : " << pBsp_->getCurrentTime()
                 << " LUD : " << lastControlUpdateSec_ << " Min : " << minVal
                 << " Max : " << maxVal;
      break;

    case fan_config_structs::SensorPwmCalcType::kSensorPwmCalcDisable:
      // Do nothing
      XLOG(WARN) << "Control :: Sensor : " << sensorItem->sensorName
                 << "Do Nothing ";
      break;
    default:
      facebook::fboss::FbossError(
          "Invalid PWM Calculation Type for sensor", sensorItem->sensorName);
      break;
  }
  XLOG(INFO) << "Value : " << sensorItem->processedData.targetPwmCache
             << " Lower : " << pConfig_->getPwmLowerThreshold()
             << " Upper : " << pConfig_->getPwmUpperThreshold();
  // No matter what, PWM should be within
  // predefined upper and lower thresholds
  if (sensorItem->processedData.targetPwmCache >
      pConfig_->getPwmUpperThreshold()) {
    sensorItem->processedData.targetPwmCache = pConfig_->getPwmUpperThreshold();
  } else if (
      sensorItem->processedData.targetPwmCache <
      pConfig_->getPwmLowerThreshold()) {
    sensorItem->processedData.targetPwmCache = pConfig_->getPwmLowerThreshold();
  }
  return;
}

void ControlLogic::getSensorUpdate() {
  std::string sensorItemName;
  float rawValue, adjustedValue, targetPwm;
  uint64_t calculatedTime = 0;
  for (auto configSensorItem = pConfig_->sensors.begin();
       configSensorItem != pConfig_->sensors.end();
       ++configSensorItem) {
    XLOG(INFO) << "Control :: Sensor Name : " << configSensorItem->sensorName;
    bool sensorAccessFail = false;
    sensorItemName = configSensorItem->sensorName;
    if (pSensor_->checkIfEntryExists(sensorItemName)) {
      XLOG(INFO) << "Control :: Sensor Exists. Getting the entry type";
      // 1.a Get the reading
      SensorEntryType entryType = pSensor_->getSensorEntryType(sensorItemName);
      switch (entryType) {
        case kSensorEntryInt:
          rawValue = (float)pSensor_->getSensorDataInt(sensorItemName);
          break;
        case kSensorEntryFloat:
          rawValue = (float)pSensor_->getSensorDataFloat(sensorItemName);
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

    if (sensorAccessFail) {
      // If the sensor data cache is stale for a while, we consider it as the
      // failure of such sensor
      uint64_t timeDiffInSec = pBsp_->getCurrentTime() -
          configSensorItem->processedData.lastUpdatedTime;
      if (timeDiffInSec >= configSensorItem->sensorFailThresholdInSec) {
        configSensorItem->processedData.sensorFailed == true;
        numSensorFailed_++;
      }
    } else {
      calculatedTime = pSensor_->getLastUpdated(sensorItemName);
      configSensorItem->processedData.lastUpdatedTime = calculatedTime;
      configSensorItem->processedData.sensorFailed = false;
    }

    // 1.b If adjustment table exists, adjust the raw value
    if (configSensorItem->offsetTable.size() == 0) {
      adjustedValue = rawValue;
    } else {
      float offset = 0;
      for (auto tableEntry = configSensorItem->offsetTable.begin();
           tableEntry != configSensorItem->offsetTable.end();
           ++tableEntry) {
        if (rawValue >= tableEntry->first) {
          offset = tableEntry->second;
        }
        adjustedValue = rawValue + offset;
      }
    }
    configSensorItem->processedData.adjustedReadCache = adjustedValue;
    XLOG(INFO) << "Control :: Adjusted Value : " << adjustedValue;
    // 1.c Check and trigger alarm
    bool prevMajorAlarm = configSensorItem->processedData.majorAlarmTriggered;
    configSensorItem->processedData.majorAlarmTriggered =
        (adjustedValue >= configSensorItem->alarm.high_major);
    // If major alarm was triggered, write it as a ERR log
    if (!prevMajorAlarm &&
        configSensorItem->processedData.majorAlarmTriggered) {
      XLOG(ERR) << "Major Alarm Triggered on " << configSensorItem->sensorName
                << " at value " << adjustedValue;
    } else if (
        prevMajorAlarm &&
        !configSensorItem->processedData.majorAlarmTriggered) {
      XLOG(WARN) << "Major Alarm Cleared on " << configSensorItem->sensorName
                 << " at value " << adjustedValue;
    }
    bool prevMinorAlarm = configSensorItem->processedData.minorAlarmTriggered;
    if (adjustedValue >= configSensorItem->alarm.high_minor) {
      if (configSensorItem->processedData.soakStarted) {
        uint64_t timeDiffInSec = pBsp_->getCurrentTime() -
            configSensorItem->processedData.soakStartedAt;
        if (timeDiffInSec >= configSensorItem->alarm.high_minor_soak) {
          configSensorItem->processedData.minorAlarmTriggered = true;
          configSensorItem->processedData.soakStarted = false;
        }
      } else {
        configSensorItem->processedData.soakStarted = true;
        configSensorItem->processedData.soakStartedAt = calculatedTime;
      }
    } else {
      configSensorItem->processedData.minorAlarmTriggered = false;
      configSensorItem->processedData.soakStarted = false;
    }
    // If minor alarm was triggered, write it as a WARN log
    if (!prevMinorAlarm &&
        configSensorItem->processedData.minorAlarmTriggered) {
      XLOG(WARN) << "Minor Alarm Triggered on " << configSensorItem->sensorName
                 << " at value " << adjustedValue;
    }
    if (prevMinorAlarm &&
        !configSensorItem->processedData.minorAlarmTriggered) {
      XLOG(WARN) << "Minor Alarm Cleared on " << configSensorItem->sensorName
                 << " at value " << adjustedValue;
    }
    // 1.d Check the range (if required), and do emergency
    // shutdown, if the value is out of range for more than
    // the "tolerance" times
    if (configSensorItem->rangeCheck.enabled) {
      if ((adjustedValue > configSensorItem->rangeCheck.rangeHigh) ||
          (adjustedValue < configSensorItem->rangeCheck.rangeLow)) {
        configSensorItem->rangeCheck.invalidCount += 1;
        if (configSensorItem->rangeCheck.invalidCount >=
            configSensorItem->rangeCheck.tolerance) {
          // ERR log only once.
          if (configSensorItem->rangeCheck.invalidCount ==
              configSensorItem->rangeCheck.tolerance) {
            XLOG(ERR) << "Sensor " << configSensorItem->sensorName
                      << " out of range for too long!";
          }
          // If we are not yet in emergency state, do the emergency shutdown.
          if ((configSensorItem->rangeCheck.action ==
               kRangeCheckActionShutdown) &&
              (pBsp_->getEmergencyState() == false)) {
            pBsp_->emergencyShutdown(pConfig_, true);
          }
        }
      } else {
        configSensorItem->rangeCheck.invalidCount = 0;
      }
    }
    // 1.e Calculate the target pwm in percent
    //     (the table or incremental pid should produce
    //      percent as its output)
    updateTargetPwm(&(*configSensorItem));
    XLOG(INFO) << configSensorItem->sensorName << " has the target PWM of "
               << configSensorItem->processedData.targetPwmCache;
  }
  return;
}

Sensor* ControlLogic::findSensorConfig(std::string sensorName) {
  for (auto sensorConfig = pConfig_->sensors.begin();
       sensorConfig != pConfig_->sensors.end();
       ++sensorConfig) {
    if (sensorConfig->sensorName == sensorName) {
      return &(*sensorConfig);
    }
  }
  facebook::fboss::FbossError("Enable to find sensorConfig : ", sensorName);
  return NULL;
}

void ControlLogic::adjustZoneFans(bool boostMode) {
  for (auto zone = pConfig_->zones.begin(); zone != pConfig_->zones.end();
       ++zone) {
    float pwmSoFar = 0;
    XLOG(INFO) << "Zone : " << zone->zoneName;
    // First, calculate the pwm value for this zone
    auto zoneType = zone->type;
    for (auto sensorName = zone->sensorNames.begin();
         sensorName != zone->sensorNames.end();
         sensorName++) {
      auto pSensorConfig_ = findSensorConfig(*sensorName);
      if (pSensorConfig_ != NULL) {
        float pwmForThisSensor = pSensorConfig_->processedData.targetPwmCache;
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
                "Undefined Zone Type for zone : ", zone->zoneName);
            break;
        }
        XLOG(INFO) << "  Sensor " << pSensorConfig_->sensorName << " : "
                   << pSensorConfig_->processedData.targetPwmCache
                   << " Overall so far : " << pwmSoFar;
      }
      if (zoneType == fan_config_structs::ZoneType::kZoneAvg) {
        pwmSoFar /= (float)zone->sensorNames.size();
      }
      XLOG(INFO) << "  Final PWM : " << pwmSoFar;
    }
    if (boostMode) {
      if (pwmSoFar < pConfig_->getPwmBoostValue()) {
        pwmSoFar = pConfig_->getPwmBoostValue();
      }
    }
    // Update the previous pwm value in each associated sensors,
    // so that they may be used in the next calculation.
    for (auto sensorName = zone->sensorNames.begin();
         sensorName != zone->sensorNames.end();
         sensorName++) {
      auto pSensorConfig_ = findSensorConfig(*sensorName);
      if (pSensorConfig_ != NULL) {
        pSensorConfig_->incrementPid.previousTargetPwm = pwmSoFar;
      }
    }

    // Secondly, set Zone pwm value to all the fans in the zone
    for (auto fan = pConfig_->fans.begin(); fan != pConfig_->fans.end();
         ++fan) {
      auto srcType = fan->pwm.accessType;
      float pwmToProgram = 0;
      float currentPwm = fan->fanStatus.currentPwm;
      if ((zone->slope == 0) || (currentPwm == 0)) {
        pwmToProgram = pwmSoFar;
      } else {
        if (pwmSoFar > currentPwm) {
          if ((pwmSoFar - currentPwm) > zone->slope) {
            pwmToProgram = currentPwm + zone->slope;
          } else {
            pwmToProgram = pwmSoFar;
          }
        } else if (pwmSoFar < currentPwm) {
          if ((currentPwm - pwmSoFar) > zone->slope) {
            pwmToProgram = currentPwm - zone->slope;
          } else {
            pwmToProgram = pwmSoFar;
          }
        } else {
          pwmToProgram = pwmSoFar;
        }
      }
      int pwmInt =
          (int)(((fan->pwmMax) - (fan->pwmMin)) * pwmToProgram / 100.0 + fan->pwmMin);
      if (pwmInt < fan->pwmMin) {
        pwmInt = fan->pwmMin;
      } else if (pwmInt > fan->pwmMax) {
        pwmInt = fan->pwmMax;
      }
      switch (srcType) {
        case fan_config_structs::SourceType::kSrcSysfs:
          pBsp_->setFanPwmSysfs(fan->pwm.path, pwmInt);
          break;
        case fan_config_structs::SourceType::kSrcUtil:
          pBsp_->setFanPwmShell(fan->pwm.path, fan->fanName, pwmInt);
          break;
        case fan_config_structs::SourceType::kSrcThrift:
        case fan_config_structs::SourceType::kSrcRest:
        case fan_config_structs::SourceType::kSrcInvalid:
        default:
          facebook::fboss::FbossError(
              "Unsupported PWM access type for : ", fan->fanName);
      }
      fan->fanStatus.currentPwm = pwmToProgram;
    }
  }
}

void ControlLogic::updateControl(std::shared_ptr<SensorData> pS) {
  pSensor_ = pS;

  numFanFailed_ = 0;
  numSensorFailed_ = 0;
  bool boostMode = false;

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

  // Check if we need to turn on boost mode
  XLOG(INFO) << "Control :: Failed Sensor : " << numFanFailed_
             << " Failed Sensor : " << numSensorFailed_;
  boostMode =
      (((pConfig_->pwmBoostOnDeadFan != 0) &&
        (numFanFailed_ >= pConfig_->pwmBoostOnDeadFan)) ||
       ((pConfig_->pwmBoostOnDeadSensor != 0) &&
        (numSensorFailed_ >= pConfig_->pwmBoostOnDeadSensor)));
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
