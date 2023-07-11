// Copyright 2021- Facebook. All rights reserved.

// Implementation of Control Logic class. Refer to .h file
// for function description

#include "fboss/platform/fan_service/ControlLogic.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_config_structs_types.h"
// Additional FB helper funtion
#include "common/time/Time.h"

namespace {
auto constexpr kFanWriteFailure = "fan_write.{}.{}.failure";
auto constexpr kFanFailThresholdInSec = 300;
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
  for (auto fanItem = pConfig_->fans.begin(); fanItem != pConfig_->fans.end();
       ++fanItem) {
    XLOG(INFO) << "Control :: Fan name " << fanItem->fanName
               << " Access type : "
               << static_cast<int>(*fanItem->rpmAccess.accessType());
    bool fanAccessFail = false, fanMissing = false;
    int fanRpm;
    uint64_t rpmTimeStamp;
    std::string fanItemName = fanItem->fanName;
    // Check if RPM name in Thrift data is overridden
    if (fanItem->rpmAccess.accessType() ==
        fan_config_structs::SourceType::kSrcThrift) {
      fanItemName = *fanItem->rpmAccess.path();
    }
    if (!checkIfFanPresent(std::addressof(*fanItem))) {
      fanMissing = true;
    }
    // If source type is not specified (SRC_INVALID), we treat it as Thrift.
    if ((fanItem->rpmAccess.accessType() ==
         fan_config_structs::SourceType::kSrcInvalid) ||
        (fanItem->rpmAccess.accessType() ==
         fan_config_structs::SourceType::kSrcThrift)) {
      if (pSensor_->checkIfEntryExists(fanItemName)) {
        entryType = pSensor_->getSensorEntryType(fanItemName);
        switch (entryType) {
          case SensorEntryType::kSensorEntryInt:
            fanRpm = static_cast<int>(pSensor_->getSensorDataInt(fanItemName));
            break;
          case SensorEntryType::kSensorEntryFloat:
            fanRpm =
                static_cast<int>(pSensor_->getSensorDataFloat(fanItemName));
            break;
        }
        rpmTimeStamp = pSensor_->getLastUpdated(fanItemName);
        if (rpmTimeStamp == fanItem->fanStatus.timeStamp) {
          // If read method is Thrift, but read time stamp is stale
          // , consider that as access failure
          fanAccessFail = true;
        } else {
          fanItem->fanStatus.rpm = fanRpm;
          fanItem->fanStatus.timeStamp = rpmTimeStamp;
        }
      } else {
        // If no entry in Thrift response, consider that as failure
        fanAccessFail = true;
      }
    } else if (
        fanItem->rpmAccess.accessType() ==
        fan_config_structs::SourceType::kSrcSysfs) {
      try {
        fanItem->fanStatus.rpm = pBsp_->readSysfs(*fanItem->rpmAccess.path());
        fanItem->fanStatus.timeStamp = pBsp_->getCurrentTime();
      } catch (std::exception& e) {
        XLOG(ERR) << "Fan RPM access fail " << *fanItem->rpmAccess.path();
        // Obvious. Sysfs fail means access fail
        fanAccessFail = true;
      }
    } else {
      facebook::fboss::FbossError(
          "Unable to fetch Fan RPM due to invalide RPM sensor entry type :",
          fanItemName);
    }

    // We honor fan presence bit first,
    // If fan is present, then check if access has failed
    if (fanMissing) {
      setFanFailState(std::addressof(*fanItem), true);
    } else if (fanAccessFail) {
      uint64_t timeDiffInSec =
          pBsp_->getCurrentTime() - fanItem->fanStatus.timeStamp;
      if (timeDiffInSec >= kFanFailThresholdInSec) {
        setFanFailState(std::addressof(*fanItem), true);
        numFanFailed_++;
      }
    } else {
      setFanFailState(std::addressof(*fanItem), false);
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
  float rawValue = 0.0, adjustedValue;
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
        case SensorEntryType::kSensorEntryInt:
          rawValue = pSensor_->getSensorDataInt(sensorItemName);
          rawValue = rawValue / configSensorItem->scale;
          break;
        case SensorEntryType::kSensorEntryFloat:
          rawValue = pSensor_->getSensorDataFloat(sensorItemName);
          rawValue = rawValue / configSensorItem->scale;
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
        configSensorItem->processedData.sensorFailed = true;
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
        (adjustedValue >= *configSensorItem->alarm.highMajor());
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
    if (adjustedValue >= *configSensorItem->alarm.highMinor()) {
      if (configSensorItem->processedData.soakStarted) {
        uint64_t timeDiffInSec = pBsp_->getCurrentTime() -
            configSensorItem->processedData.soakStartedAt;
        if (timeDiffInSec >= *configSensorItem->alarm.minorSoakSeconds()) {
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

void ControlLogic::getOpticsUpdate() {
  // For all optics entry
  // Read optics array and set calculated pwm
  // No need to worry about timestamp, but update it anyway
  for (auto optic = pConfig_->optics.begin(); optic != pConfig_->optics.end();
       ++optic) {
    XLOG(INFO) << "Control :: Optics Group Name : " << *optic->opticName();
    std::string opticName = *optic->opticName();

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
        for (auto dataPair = opticData->data.begin();
             dataPair != opticData->data.end();
             ++dataPair) {
          auto dataType = dataPair->first;
          auto value = dataPair->second;
          int pwmForThis = 0;
          auto tablePointer =
              pConfig_->getConfigOpticTable(opticName, dataType);
          // We have <type, value> pair. If we have table entry for this
          // optics type, get the matching pwm value using the optics value
          if (tablePointer) {
            // Start with the minumum, then continue the comparison
            pwmForThis = tablePointer->begin()->second;
            for (auto tableEntry = tablePointer->begin();
                 tableEntry != tablePointer->end();
                 ++tableEntry) {
              if (value >= tableEntry->first) {
                pwmForThis = tableEntry->second;
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

Sensor* ControlLogic::findSensorConfig(std::string sensorName) {
  for (auto sensorConfig = pConfig_->sensors.begin();
       sensorConfig != pConfig_->sensors.end();
       ++sensorConfig) {
    if (sensorConfig->sensorName == sensorName) {
      return &(*sensorConfig);
    }
  }
  facebook::fboss::FbossError("Enable to find sensorConfig : ", sensorName);
  return nullptr;
}

bool ControlLogic::checkIfFanPresent(Fan* fan) {
  unsigned int readVal;
  bool readSuccessful = false;
  uint64_t nowSec;

  // If no access method is listed in config,
  // skip any check and return true
  if (fan->presence.path() == "") {
    return true;
  }

  std::string presenceKey = fan->fanName + "_presence";
  auto presenceAccessType = *fan->presence.accessType();
  switch (presenceAccessType) {
    case fan_config_structs::SourceType::kSrcThrift:
      // In the case of Thrift, we use the last data from Thrift read
      readVal = pSensor_->getSensorDataFloat(presenceKey);
      readSuccessful = true;
      break;
    case fan_config_structs::SourceType::kSrcSysfs:
      nowSec = facebook::WallClockUtil::NowInSecFast();
      try {
        readVal =
            static_cast<unsigned>(pBsp_->readSysfs(*fan->presence.path()));
        readSuccessful = true;
      } catch (std::exception& e) {
        XLOG(ERR) << "Failed to read sysfs " << *fan->presence.path();
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
             << fan->fanPresentVal << " - bad " << fan->fanMissingVal;

  if (readSuccessful) {
    if (readVal == fan->fanPresentVal) {
      return true;
    }
  }
  return false;
}
void ControlLogic::programFan(fan_config_structs::Zone* zone, float pwmSoFar) {
  for (auto fan = pConfig_->fans.begin(); fan != pConfig_->fans.end(); ++fan) {
    auto srcType = *fan->pwm.accessType();
    float pwmToProgram = 0;
    float currentPwm = fan->fanStatus.currentPwm;
    bool writeSuccess{false};
    // If this fan does not belong to the current zone, do not do anything
    if (std::find(
            zone->fanNames()->begin(), zone->fanNames()->end(), fan->fanName) ==
        zone->fanNames()->end()) {
      continue;
    }
    if ((*zone->slope() == 0) || (currentPwm == 0)) {
      pwmToProgram = pwmSoFar;
    } else {
      if (pwmSoFar > currentPwm) {
        if ((pwmSoFar - currentPwm) > *zone->slope()) {
          pwmToProgram = currentPwm + *zone->slope();
        } else {
          pwmToProgram = pwmSoFar;
        }
      } else if (pwmSoFar < currentPwm) {
        if ((currentPwm - pwmSoFar) > *zone->slope()) {
          pwmToProgram = currentPwm - *zone->slope();
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
        writeSuccess = pBsp_->setFanPwmSysfs(*fan->pwm.path(), pwmInt);
        if (!writeSuccess) {
          setFanFailState(std::addressof(*fan), true);
        }
        break;
      case fan_config_structs::SourceType::kSrcUtil:
        writeSuccess =
            pBsp_->setFanPwmShell(*fan->pwm.path(), fan->fanName, pwmInt);
        if (!writeSuccess) {
          setFanFailState(std::addressof(*fan), true);
        }
        break;
      case fan_config_structs::SourceType::kSrcThrift:
      case fan_config_structs::SourceType::kSrcRest:
      case fan_config_structs::SourceType::kSrcInvalid:
      default:
        facebook::fboss::FbossError(
            "Unsupported PWM access type for : ", fan->fanName);
    }
    fb303::fbData->setCounter(
        fmt::format(kFanWriteFailure, *zone->zoneName(), fan->fanName),
        !writeSuccess);
    fan->fanStatus.currentPwm = pwmToProgram;
  }
}

void ControlLogic::setFanFailState(Fan* fan, bool fanFailed) {
  XLOG(INFO) << "Control :: Enter LED for " << fan->fanName;
  bool ledAccessNeeded = false;
  if (fan->fanStatus.firstTimeLedAccess) {
    ledAccessNeeded = true;
    fan->fanStatus.firstTimeLedAccess = false;
  }
  if (fanFailed) {
    // Fan failed.
    // If the previous status was fan good, we need to set fan LED color
    if (!fan->fanStatus.fanFailed) {
      // We need to change internal state
      fan->fanStatus.fanFailed = true;
      // Also change led color to "FAIL", if Fan LED is available
      if (fan->pwm.path() != "") {
        ledAccessNeeded = true;
      }
    }
  } else {
    // Fan did NOT fail (is in a good shape)
    // If the previous status was fan fail, we need to set fan LED color
    if (fan->fanStatus.fanFailed) {
      // We need to change internal state
      fan->fanStatus.fanFailed = false;
      // Also change led color to "GOOD", if Fan LED is available
      ledAccessNeeded = true;
    }
  }
  if (ledAccessNeeded) {
    unsigned int valueToWrite =
        (fanFailed ? fan->fanFailLedVal : fan->fanGoodLedVal);
    switch (*fan->led.accessType()) {
      case fan_config_structs::SourceType::kSrcSysfs:
        pBsp_->setFanLedSysfs(*fan->led.path(), valueToWrite);
        break;
      case fan_config_structs::SourceType::kSrcUtil:
        pBsp_->setFanLedShell(*fan->led.path(), fan->fanName, valueToWrite);
        break;
      case fan_config_structs::SourceType::kSrcThrift:
      case fan_config_structs::SourceType::kSrcRest:
      case fan_config_structs::SourceType::kSrcInvalid:
      default:
        facebook::fboss::FbossError(
            "Unsupported LED access type for : ", fan->fanName);
    }
    XLOG(INFO) << "Control :: Set the LED of " << fan->fanName << " to "
               << (fanFailed ? "Fail" : "Good") << "(" << valueToWrite << ") "
               << fan->fanFailLedVal << " vs " << fan->fanGoodLedVal;
  }
}

void ControlLogic::adjustZoneFans(bool boostMode) {
  for (auto zone = pConfig_->zones.begin(); zone != pConfig_->zones.end();
       ++zone) {
    float pwmSoFar = 0;
    XLOG(INFO) << "Zone : " << *zone->zoneName();
    // First, calculate the pwm value for this zone
    auto zoneType = *zone->zoneType();
    int totalPwmConsidered = 0;
    for (auto sensorName = zone->sensorNames()->begin();
         sensorName != zone->sensorNames()->end();
         sensorName++) {
      auto pSensorConfig_ = findSensorConfig(*sensorName);
      if ((pSensorConfig_ != nullptr) ||
          (pSensor_->checkIfOpticEntryExists(*sensorName))) {
        totalPwmConsidered++;
        float pwmForThisSensor;
        if (pSensorConfig_ != nullptr) {
          // If this is a sensor name
          pwmForThisSensor = pSensorConfig_->processedData.targetPwmCache;
        } else {
          // If this is an optics name
          pwmForThisSensor = pSensor_->getOpticsPwm(*sensorName);
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
                "Undefined Zone Type for zone : ", *zone->zoneName());
            break;
        }
        XLOG(INFO) << "  Sensor/Optic " << *sensorName << " : "
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
    for (auto sensorName = zone->sensorNames()->begin();
         sensorName != zone->sensorNames()->end();
         sensorName++) {
      auto pSensorConfig_ = findSensorConfig(*sensorName);
      if (pSensorConfig_ != nullptr) {
        pSensorConfig_->incrementPid.previousTargetPwm = pwmSoFar;
      }
    }
    // Secondly, set Zone pwm value to all the fans in the zone
    programFan(&(*zone), pwmSoFar);
  }
}

void ControlLogic::setTransitionValue() {
  for (auto zone = pConfig_->zones.begin(); zone != pConfig_->zones.end();
       ++zone) {
    for (auto fan = pConfig_->fans.begin(); fan != pConfig_->fans.end();
         ++fan) {
      // If this a fan belongs to the zone, then write the transitional value
      if (std::find(
              zone->fanNames()->begin(),
              zone->fanNames()->end(),
              fan->fanName) != zone->fanNames()->end()) {
        for (auto sensorName = zone->sensorNames()->begin();
             sensorName != zone->sensorNames()->end();
             sensorName++) {
          auto pSensorConfig_ = findSensorConfig(*sensorName);
          if (pSensorConfig_ != nullptr) {
            pSensorConfig_->incrementPid.previousTargetPwm =
                pConfig_->getPwmTransitionValue();
          }
        }
        programFan(&(*zone), pConfig_->getPwmTransitionValue());
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
