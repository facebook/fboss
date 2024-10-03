// Copyright 2021- Facebook. All rights reserved.

// Implementation of Control Logic class. Refer to .h file
// for function description

#include "fboss/platform/fan_service/ControlLogic.h"

#include <folly/logging/xlog.h>
#include <gpiod.h>

#include "common/time/Time.h"
#include "fboss/lib/GpiodLine.h"
#include "fboss/platform/fan_service/SensorData.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_constants.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace {

auto constexpr kDefaultSensorReadFrequencyInSec = 30;
auto constexpr kDefaultFanControlFrequencyInSec = 30;

auto constexpr kFanWriteFailure = "{}.{}.pwm_write.failure";
auto constexpr kFanWriteValue = "{}.{}.pwm_write.value";
auto constexpr kFanAbsent = "{}.absent";
auto constexpr kFanReadRpmFailure = "{}.rpm_read.failure";
auto constexpr kFanReadRpmValue = "{}.rpm_read.value";
auto constexpr kSensorReadFailure = "{}.sensor_read.failure";
auto constexpr kSensorReadValue = "{}.sensor_read.value";
auto constexpr kFanFailThresholdInSec = 300;
auto constexpr kSensorFailThresholdInSec = 300;

using namespace facebook::fboss::platform::fan_service;
namespace constants =
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

ControlLogic::ControlLogic(FanServiceConfig config, std::shared_ptr<Bsp> bsp)
    : config_(config), pBsp_(bsp) {
  pSensorData_ = std::make_shared<SensorData>();

  setupPidLogics();

  XLOG(INFO)
      << "Upon fan_service start up, program all fan pwm with transitional value of "
      << *config_.pwmTransitionValue();
  setTransitionValue();
}

unsigned int ControlLogic::getControlFrequency() const {
  if (config_.controlInterval()) {
    return *config_.controlInterval()->pwmUpdateInterval();
  }
  return kDefaultFanControlFrequencyInSec;
}

unsigned int ControlLogic::getSensorFetchFrequency() const {
  if (config_.controlInterval()) {
    return *config_.controlInterval()->sensorReadInterval();
  }
  return kDefaultSensorReadFrequencyInSec;
}

void ControlLogic::controlFan() {
  uint64_t currentTimeSec = pBsp_->getCurrentTime();
  // Update Sensor Value based according to fetch frequency
  if ((currentTimeSec - lastSensorFetchTimeSec_) >= getSensorFetchFrequency()) {
    bool sensorReadOK = false;
    bool opticsReadOK = false;

    // Get the updated sensor data
    try {
      pBsp_->getSensorData(pSensorData_);
      sensorReadOK = true;
      XLOG(INFO) << "Successfully fetched sensor data.";
    } catch (std::exception& e) {
      XLOG(ERR) << "Failed to get sensor data with error : " << e.what();
    }

    // Also get the updated optics data
    try {
      // Get the updated optics data
      pBsp_->getOpticsData(pSensorData_);
      opticsReadOK = true;
      XLOG(INFO) << "Successfully fetched optics data.";
    } catch (std::exception& e) {
      XLOG(ERR) << "Failed to get optics data with error : " << e.what();
    }
    // If BOTH of the two data read above pass, then we consider
    // that the sensor reading is successful. Otherwise, we will
    // keep retrying.
    if (sensorReadOK && opticsReadOK) {
      lastSensorFetchTimeSec_ = currentTimeSec;
    }
  }
  // Change Fan PWM as needed according to control execution frequency
  if ((currentTimeSec - lastControlExecutionTimeSec_) >=
      getControlFrequency()) {
    updateControl(pSensorData_);
    lastControlExecutionTimeSec_ = currentTimeSec;
  }

  pBsp_->kickWatchdog();
}

void ControlLogic::setupPidLogics() {
  for (const auto& sensor : *config_.sensors()) {
    if (*sensor.pwmCalcType() == constants::SENSOR_PWM_CALC_TYPE_PID()) {
      auto pidSetting = *sensor.pidSetting();
      pidLogics_.emplace(
          *sensor.sensorName(), PidLogic(pidSetting, getControlFrequency()));
    }
  }
  for (const auto& optic : *config_.optics()) {
    if (*optic.aggregationType() == constants::OPTIC_AGGREGATION_TYPE_PID()) {
      for (const auto& [opticType, pidSetting] : *optic.pidSettings()) {
        auto cacheKey = *optic.opticName() + opticType;
        pidLogics_.emplace(
            cacheKey, PidLogic(pidSetting, getControlFrequency()));
      }
    }
  }
}

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
    } catch (std::exception&) {
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

void ControlLogic::updateTargetPwm(const Sensor& sensor) {
  int16_t targetPwm{0};
  TempToPwmMap tableToUse;
  auto& readCache = sensorReadCaches_[*sensor.sensorName()];
  const auto& pwmCalcType = *sensor.pwmCalcType();

  if (pwmCalcType == constants::SENSOR_PWM_CALC_TYPE_FOUR_LINEAR_TABLE()) {
    float previousSensorValue = readCache.processedReadValue;
    float sensorValue = readCache.lastReadValue;
    bool deadFanExists = (numFanFailed_ > 0);
    bool accelerate =
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
    readCache.processedReadValue = sensorValue;
  }

  if (pwmCalcType == constants::SENSOR_PWM_CALC_TYPE_PID()) {
    targetPwm = pidLogics_.at(*sensor.sensorName())
                    .calculatePwm(readCache.lastReadValue);
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
    auto sensorEntry = pSensor_->getSensorEntry(sensorName);
    if (sensorEntry) {
      float readValue = sensorEntry->value / *sensor.scale();
      readCache.lastReadValue = readValue;
      readCache.lastUpdatedTime = sensorEntry->lastUpdated;
      readCache.sensorFailed = false;
      XLOG(ERR) << fmt::format(
          "{}: Sensor read value (after scaling) is {}", sensorName, readValue);
    } else {
      XLOG(INFO) << fmt::format(
          "{}: Failure to get data (either wrong entry or read failure)",
          sensorName);
      sensorAccessFail = true;
    }

    fb303::fbData->setCounter(
        fmt::format(kSensorReadFailure, sensorName), sensorAccessFail);
    fb303::fbData->setCounter(
        fmt::format(kSensorReadValue, sensorName), readCache.lastReadValue);
    if (sensorAccessFail) {
      // If the sensor data cache is stale for a while, we consider it as the
      // failure of such sensor
      uint64_t timeDiffInSec =
          pBsp_->getCurrentTime() - readCache.lastUpdatedTime;
      if (timeDiffInSec >= kSensorFailThresholdInSec) {
        readCache.sensorFailed = true;
        numSensorFailed_++;
      }
    }

    // STEP 2: Calculate target pwm
    updateTargetPwm(sensor);
  }
}

void ControlLogic::getOpticsUpdate() {
  for (const auto& optic : *config_.optics()) {
    std::string opticName = *optic.opticName();

    auto opticEntry = pSensor_->getOpticEntry(opticName);
    if (!opticEntry || opticEntry->data.size() == 0) {
      continue;
    }

    int aggOpticPwm = 0;
    const auto& aggregationType = *optic.aggregationType();

    if (aggregationType == constants::OPTIC_AGGREGATION_TYPE_MAX()) {
      // Conventional one-table conversion, followed by
      // the aggregation using the max value
      for (const auto& [opticType, value] : opticEntry->data) {
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
      std::string cacheKey;
      for (const auto& [opticType, value] : opticEntry->data) {
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
        if (!pidSetting) {
          XLOG(ERR) << fmt::format(
              "Optic {} does not have PID setting", opticType);
          continue;
        }
        // Cache key is unique as long as opticName is unique
        cacheKey = opticName + opticType;
        float pwm = pidLogics_.at(cacheKey).calculatePwm(value);
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
    opticReadCaches_[opticName] = aggOpticPwm;
    pSensor_->updateOpticDataProcessingTimestamp(
        opticName, opticEntry->qsfpServiceTimeStamp);
  }
}

bool ControlLogic::isFanPresentInDevice(const Fan& fan) {
  unsigned int readVal;
  bool readSuccessful = false;
  bool fanPresent = false;
  if (fan.presenceSysfsPath()) {
    try {
      readVal =
          static_cast<unsigned>(pBsp_->readSysfs(*fan.presenceSysfsPath()));
      readSuccessful = true;
    } catch (std::exception&) {
      XLOG(ERR) << "Failed to read sysfs " << *fan.presenceSysfsPath();
    }
    fanPresent = (readSuccessful && readVal == *fan.fanPresentVal());
    if (fanPresent) {
      XLOG(INFO) << fmt::format(
          "{}: is present in the host (through sysfs)", *fan.fanName());
    } else {
      XLOG(INFO) << fmt::format(
          "{}: is absent in the host (through sysfs)", *fan.fanName());
    }
  } else if (fan.presenceGpio()) {
    struct gpiod_chip* chip =
        gpiod_chip_open(fan.presenceGpio()->path()->c_str());
    // Ensure GpiodLine is destroyed before gpiod_chip_close
    int value = GpiodLine(chip, *fan.presenceGpio()->lineIndex(), "gpioline")
                    .getValue();
    gpiod_chip_close(chip);
    if (value == *fan.presenceGpio()->desiredValue()) {
      fanPresent = true;
      XLOG(INFO) << fmt::format(
          "{}: is present in the host (through gpio)", *fan.fanName());
    } else {
      XLOG(INFO) << fmt::format(
          "{}: is absent in the host (through gpio)", *fan.fanName());
    }
  }
  fb303::fbData->setCounter(
      fmt::format(kFanAbsent, *fan.fanName()), !fanPresent);
  return fanPresent;
}

std::pair<bool, int16_t> ControlLogic::programFan(
    const Zone& zone,
    const Fan& fan,
    int16_t currentFanPwm,
    int16_t zonePwm) {
  int16_t newFanPwm = 0;
  bool writeSuccess{false};
  if ((*zone.slope() == 0) || (currentFanPwm == 0)) {
    newFanPwm = zonePwm;
  } else {
    if (zonePwm > currentFanPwm) {
      if ((zonePwm - currentFanPwm) > *zone.slope()) {
        newFanPwm = currentFanPwm + *zone.slope();
      } else {
        newFanPwm = zonePwm;
      }
    } else if (zonePwm < currentFanPwm) {
      if ((currentFanPwm - zonePwm) > *zone.slope()) {
        newFanPwm = currentFanPwm - *zone.slope();
      } else {
        newFanPwm = zonePwm;
      }
    } else {
      newFanPwm = zonePwm;
    }
  }

  newFanPwm = std::min(newFanPwm, *config_.pwmUpperThreshold());
  newFanPwm = std::max(newFanPwm, *config_.pwmLowerThreshold());

  std::optional<int> fanHoldPwm = fanHoldPwm_.load();
  if (fanHoldPwm.has_value()) {
    newFanPwm = fanHoldPwm.value();
    XLOG(INFO) << "Using hold PWM " << newFanPwm;
  }

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
  if (!fan.ledSysfsPath()->empty()) {
    unsigned int valueToWrite =
        (fanFailed ? *fan.fanFailLedVal() : *fan.fanGoodLedVal());
    pBsp_->setFanLedSysfs(*fan.ledSysfsPath(), valueToWrite);
    XLOG(INFO) << fmt::format(
        "{}: Setting LED to {} (value: {})",
        *fan.fanName(),
        (fanFailed ? "Fail" : "Good"),
        valueToWrite);
  } else {
    XLOG(DBG1) << fmt::format(
        "{}: FAN LED sysfs path is empty. It's likely that FAN LED is controlled by hardware.",
        *fan.fanName());
  }
}

int16_t ControlLogic::calculateZonePwm(const Zone& zone, bool boostMode) {
  auto zoneType = *zone.zoneType();
  int16_t zonePwm{0};
  int totalPwmConsidered{0};
  for (const auto& sensorName : *zone.sensorNames()) {
    int16_t pwmForThisSensor;
    if (sensorReadCaches_.find(sensorName) != sensorReadCaches_.end()) {
      pwmForThisSensor = sensorReadCaches_[sensorName].targetPwmCache;
    } else if (opticReadCaches_.find(sensorName) != opticReadCaches_.end()) {
      pwmForThisSensor = opticReadCaches_[sensorName];
    } else {
      continue;
    }
    if (zoneType == constants::ZONE_TYPE_MAX()) {
      zonePwm = std::max(zonePwm, pwmForThisSensor);
    } else if (zoneType == constants::ZONE_TYPE_MIN()) {
      zonePwm = std::min(zonePwm, pwmForThisSensor);
    } else if (zoneType == constants::ZONE_TYPE_AVG()) {
      zonePwm += pwmForThisSensor;
    } else {
      XLOG(ERR) << "Undefined Zone Type for zone : ", *zone.zoneName();
    }
    totalPwmConsidered++;
  }
  if (zoneType == constants::ZONE_TYPE_AVG() && totalPwmConsidered != 0) {
    zonePwm /= totalPwmConsidered;
  }
  if (boostMode) {
    zonePwm = std::max(zonePwm, *config_.pwmBoostValue());
  }
  XLOG(INFO) << fmt::format(
      "{}: Components: {}. Aggregation Type: {}. Aggregate PWM is {}.",
      *zone.zoneName(),
      folly::join(",", *zone.sensorNames()),
      zoneType,
      zonePwm);
  for (const auto& sensorName : *zone.sensorNames()) {
    if (pidLogics_.find(sensorName) != pidLogics_.end()) {
      pidLogics_.at(sensorName).updateLastPwm(zonePwm);
    }
    for (const auto& optic : *config_.optics()) {
      if (optic.opticName() != sensorName) {
        continue;
      }
      for (const auto& [opticType, _] : *optic.pidSettings()) {
        auto cacheKey = *optic.opticName() + opticType;
        if (pidLogics_.find(cacheKey) != pidLogics_.end()) {
          pidLogics_.at(cacheKey).updateLastPwm(zonePwm);
        }
      }
    }
  }

  return zonePwm;
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
        for (auto& [key, pidLogic] : pidLogics_) {
          pidLogic.updateLastPwm(*config_.pwmTransitionValue());
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

  // If we have not yet successfully read so far,
  // it does not make sense to update fan PWM out of no data.
  // So check if we read sensor data at least once. If not,
  // do it now.
  if (!pBsp_->checkIfInitialSensorDataRead()) {
    XLOG(INFO) << "Reading sensors for the first time";
    pBsp_->getSensorData(pSensor_);
  }

  // STEP 1: Check presence/rpm of fans
  XLOG(INFO) << "Processing Fans ...";
  fanStatuses_.withWLock([&](auto& fanStatuses) {
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
      // If rpmMin or rpmMax is set, compare fanRpm with them to decide
      // whether fan is failed
      if (fan.rpmMin()) {
        if (fanRpm < *fan.rpmMin()) {
          fanFailed = true;
          XLOG(ERR) << fmt::format(
              "fan {} : rpm {} is below the minimum value {}",
              *fan.fanName(),
              fanRpm,
              *fan.rpmMin());
        }
      }
      if (fan.rpmMax()) {
        if (fanRpm > *fan.rpmMax()) {
          fanFailed = true;
          XLOG(ERR) << fmt::format(
              "fan {} : rpm {} is above the maximum value {}",
              *fan.fanName(),
              fanRpm,
              *fan.rpmMax());
        }
      }
      if (fanFailed) {
        numFanFailed_++;
      }
      fanStatuses[*fan.fanName()].fanFailed() = fanFailed;
    }
  });

  // STEP 2: Read sensor values and calculate their PWM
  XLOG(INFO) << "Processing Sensors ...";
  getSensorUpdate();

  // STEP 3: Read optics values and calculate their PWM
  XLOG(INFO) << "Processing Optics ...";
  getOpticsUpdate();

  // STEP 4: Determine whether boost mode is necessary
  uint64_t secondsSinceLastOpticsUpdate =
      pBsp_->getCurrentTime() - pSensor_->getLastQsfpSvcTime();
  bool missingOpticsUpdate{false}, fanFailures{false}, sensorFailures{false};
  if ((*config_.pwmBoostOnNoQsfpAfterInSec() != 0) &&
      (secondsSinceLastOpticsUpdate >= *config_.pwmBoostOnNoQsfpAfterInSec())) {
    missingOpticsUpdate = true;
    XLOG(INFO) << fmt::format(
        "Boost mode enabled for optics update missing for {}s",
        secondsSinceLastOpticsUpdate);
  }
  if ((*config_.pwmBoostOnNumDeadFan() != 0) &&
      (numFanFailed_ >= *config_.pwmBoostOnNumDeadFan())) {
    fanFailures = true;
    XLOG(INFO) << fmt::format(
        "Boost mode enabled for {} fan failures", numFanFailed_);
  }
  if ((*config_.pwmBoostOnNumDeadSensor() != 0) &&
      (numSensorFailed_ >= *config_.pwmBoostOnNumDeadSensor())) {
    sensorFailures = true;
    XLOG(INFO) << fmt::format(
        "Boost mode enabled for {} sensor failures", numSensorFailed_);
  }
  bool boostMode = (missingOpticsUpdate || fanFailures || sensorFailures);

  // STEP 5: Calculate and program fan PWMs
  fanStatuses_.withWLock([&](auto& fanStatuses) {
    for (const auto& zone : *config_.zones()) {
      int16_t zonePwm = calculateZonePwm(zone, boostMode);
      for (const auto& fan : *config_.fans()) {
        if (std::find(
                zone.fanNames()->begin(),
                zone.fanNames()->end(),
                *fan.fanName()) == zone.fanNames()->end()) {
          continue;
        }
        const auto [fanFailed, newFanPwm] = programFan(
            zone, fan, *fanStatuses[*fan.fanName()].pwmToProgram(), zonePwm);
        fanStatuses[*fan.fanName()].pwmToProgram() = newFanPwm;
        if (fanFailed) {
          // Only override the fanFailed if fan programming failed.
          fanStatuses[*fan.fanName()].fanFailed() = fanFailed;
        }
      }
    }

    // STEP 6: Program fan LEDs
    for (const auto& fan : *config_.fans()) {
      programLed(fan, *fanStatuses[*fan.fanName()].fanFailed());
    }
  });
}

void ControlLogic::setFanHold(std::optional<int> pwm) {
  fanHoldPwm_.store(pwm);
}

std::optional<int> ControlLogic::getFanHold() {
  return fanHoldPwm_.load();
}

} // namespace facebook::fboss::platform::fan_service
