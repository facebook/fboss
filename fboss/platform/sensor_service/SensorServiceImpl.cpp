/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <filesystem>

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/sensor_service/FsdbSyncer.h"
#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/Utils.h"
#include "fboss/platform/sensor_service/gen-cpp2/sensor_service_stats_types.h"

DEFINE_int32(
    fsdb_statsStream_interval_seconds,
    5,
    "Interval at which stats subscriptions are served");

namespace {

struct SensorStatsResult {
  int readFailure{0};
  int criticalViolation{0};
  int alarmViolation{0};
};

SensorStatsResult computeSensorStats(
    const facebook::fboss::platform::sensor_service::SensorData& sensorData) {
  auto value = sensorData.value().to_optional();
  if (!value) {
    return {.readFailure = 1};
  }
  auto critViolation =
      *value > sensorData.thresholds()->upperCriticalVal().value_or(INT_MAX) ||
      *value < sensorData.thresholds()->lowerCriticalVal().value_or(INT_MIN);
  auto alarmViolation =
      *value > sensorData.thresholds()->maxAlarmVal().value_or(INT_MAX) ||
      *value < sensorData.thresholds()->minAlarmVal().value_or(INT_MIN);
  return {
      .criticalViolation = critViolation,
      .alarmViolation = alarmViolation,
  };
}

std::string sensorTypeName(
    facebook::fboss::platform::sensor_config::SensorType type) {
  auto name = apache::thrift::util::enumNameSafe(type);
  folly::toLowerAscii(name);
  return name;
}

} // namespace

namespace facebook::fboss::platform::sensor_service {

SensorServiceImpl::SensorServiceImpl(
    const SensorConfig& sensorConfig,
    const std::shared_ptr<Utils>& utils,
    const std::shared_ptr<PlatformUtils>& platformUtils)
    : sensorConfig_(sensorConfig),
      structuredLogger_("sensor_service"),
      utils_(utils),
      platformUtils_(platformUtils) {
  fsdbSyncer_ = std::make_unique<FsdbSyncer>();
}

SensorServiceImpl::~SensorServiceImpl() {
  if (fsdbSyncer_) {
    fsdbSyncer_->stop();
  }
  fsdbSyncer_.reset();
}

std::vector<SensorData> SensorServiceImpl::getSensorsData(
    const std::vector<std::string>& sensorNames) {
  std::vector<SensorData> sensorDataVec;
  auto allSensorData = getAllSensorData();
  for (const auto& sensorName : sensorNames) {
    auto it = allSensorData.find(sensorName);
    if (it != allSensorData.end()) {
      sensorDataVec.push_back(it->second);
    }
  }
  return sensorDataVec;
}

std::map<std::string, SensorData> SensorServiceImpl::getAllSensorData() {
  return polledData_.copy();
}

void SensorServiceImpl::fetchSensorData() {
  std::map<std::string, SensorData> polledData;
  XLOG(INFO) << fmt::format(
      "Reading SensorData for {} PMUnits",
      sensorConfig_.pmUnitSensorsList()->size());
  for (const auto& pmUnitSensors : *sensorConfig_.pmUnitSensorsList()) {
    auto pmSensors = resolveSensors(pmUnitSensors);
    XLOG(INFO) << fmt::format(
        "Processing {} PMUnit: {} sensors",
        *pmUnitSensors.pmUnitName(),
        pmSensors.size());
    for (const auto& sensor : pmSensors) {
      const auto& sensorName = *sensor.name();
      auto sensorData = fetchSensorDataImpl(
          sensorName,
          *sensor.sysfsPath(),
          *sensor.type(),
          sensor.thresholds().to_optional(),
          sensor.compute().to_optional());
      sensorData.slotPath() = *pmUnitSensors.slotPath();
      polledData[sensorName] = sensorData;
      publishPerSensorStats(sensorData);
    }
  }

  if (sensorConfig_.asicCommand()) {
    XLOG(INFO) << "Processing Asic Command";
    auto sensorData = processAsicCmd(sensorConfig_.asicCommand().value());
    const auto& sensorName = *sensorConfig_.asicCommand()->sensorName();
    polledData[sensorName] = sensorData;
    publishPerSensorStats(sensorData);
  }

  processPower(polledData, *sensorConfig_.powerConfig());

  processTemperature(polledData, *sensorConfig_.temperatureConfigs());

  processInputVoltage(
      polledData, *sensorConfig_.powerConfig()->inputVoltageSensors());

  publishAggStats(polledData);

  polledData_.swap(polledData);

  if (FLAGS_publish_stats_to_fsdb) {
    auto now = std::chrono::steady_clock::now();
    if (!publishedStatsToFsdbAt_ ||
        std::chrono::duration_cast<std::chrono::seconds>(
            now - *publishedStatsToFsdbAt_)
                .count() >= FLAGS_fsdb_statsStream_interval_seconds) {
      stats::SensorServiceStats sensorServiceStats;
      sensorServiceStats.sensorData() = getAllSensorData();
      fsdbSyncer_->statsUpdated(sensorServiceStats);
      publishedStatsToFsdbAt_ = now;
    }
  }
}

void SensorServiceImpl::publishVersionedSensorStats(
    const std::string& pmUnitName,
    int16_t productProductionState,
    int16_t productVersion,
    int16_t productSubVersion) {
  fb303::fbData->setCounter(
      fmt::format(
          kPmUnitVersion,
          pmUnitName,
          productProductionState,
          productVersion,
          productSubVersion),
      1);
}

std::vector<PmSensor> SensorServiceImpl::resolveSensors(
    const PmUnitSensors& pmUnitSensors) {
  auto pmSensors = *pmUnitSensors.sensors();
  if (auto versionedPmSensors = utils_->resolveVersionedSensors(
          pmUnitInfoFetcher_,
          *pmUnitSensors.slotPath(),
          *pmUnitSensors.versionedSensors())) {
    XLOG(INFO) << fmt::format(
        "Resolved to versionedPmSensors config with version {}.{}.{} for pmUnit {} at {}",
        *versionedPmSensors->productProductionState(),
        *versionedPmSensors->productVersion(),
        *versionedPmSensors->productSubVersion(),
        *pmUnitSensors.pmUnitName(),
        *pmUnitSensors.slotPath());
    publishVersionedSensorStats(
        *pmUnitSensors.pmUnitName(),
        *versionedPmSensors->productProductionState(),
        *versionedPmSensors->productVersion(),
        *versionedPmSensors->productSubVersion());
    pmSensors.insert(
        pmSensors.end(),
        versionedPmSensors->sensors()->begin(),
        versionedPmSensors->sensors()->end());
  }
  return pmSensors;
}

SensorData SensorServiceImpl::fetchSensorDataImpl(
    const std::string& sensorName,
    const std::string& sysfsPath,
    SensorType sensorType,
    const std::optional<Thresholds>& thresholds,
    const std::optional<std::string>& compute) {
  SensorData sensorData{};
  sensorData.name() = sensorName;
  sensorData.sysfsPath() = sysfsPath;
  std::string sensorValue;
  bool sysfsFileExists =
      std::filesystem::exists(std::filesystem::path(sysfsPath));
  if (sysfsFileExists && folly::readFile(sysfsPath.c_str(), sensorValue)) {
    sensorData.value() = folly::to<float>(sensorValue);
    sensorData.timeStamp() = Utils::nowInSecs();
    if (compute) {
      sensorData.value() =
          Utils::computeExpression(*compute, *sensorData.value());
    }
    XLOG(DBG1) << fmt::format(
        "{} ({}) : {}", sensorName, sysfsPath, *sensorData.value());
  } else {
    XLOG(ERR) << fmt::format(
        "Could not read data for {} from path:{}, error:{}",
        sensorName,
        sysfsPath,
        sysfsFileExists ? folly::errnoStr(errno) : "File does not exist");
    structuredLogger_.logAlert(
        "sensor_sysfs_read_failure",
        sysfsFileExists ? folly::errnoStr(errno) : "File does not exist",
        {{"sensor_name", sensorName}, {"sysfs_path", sysfsPath}});
  }
  sensorData.thresholds() = thresholds ? *thresholds : Thresholds();
  sensorData.sensorType() = sensorType;
  return sensorData;
}

void SensorServiceImpl::publishPerSensorStats(const SensorData& sensorData) {
  const auto& sensorName = *sensorData.name();
  auto stats = computeSensorStats(sensorData);

  // We log 0 if there is a read failure.  If we dont log 0 on failure,
  // fb303 will pick up the last reported (on read success) value and
  // keep reporting that as the value. For 0 values, it is accurate to
  // read the value along with the kReadFailure counter. Alternative is
  // to delete this counter if there is a failure.
  fb303::fbData->setCounter(
      fmt::format(kReadValue, sensorName),
      sensorData.value().to_optional().value_or(0));
  fb303::fbData->setCounter(
      fmt::format(kReadFailure, sensorName), stats.readFailure);

  if (sensorData.thresholds()->upperCriticalVal() ||
      sensorData.thresholds()->lowerCriticalVal()) {
    fb303::fbData->setCounter(
        fmt::format(kCriticalThresholdViolation, sensorName),
        stats.criticalViolation);
  }

  if (sensorData.thresholds()->maxAlarmVal() ||
      sensorData.thresholds()->minAlarmVal()) {
    fb303::fbData->setCounter(
        fmt::format(kAlarmThresholdViolation, sensorName),
        stats.alarmViolation);
  }
}

void SensorServiceImpl::publishAggStats(
    const std::map<std::string, SensorData>& polledData) {
  std::map<SensorType, std::pair<bool, bool>> violationsBySensorType;
  int totalReadFailures{0};

  for (const auto& [_, sensorData] : polledData) {
    auto stats = computeSensorStats(sensorData);
    auto& [hasCritical, hasAlarm] =
        violationsBySensorType[*sensorData.sensorType()];
    hasCritical = hasCritical || stats.criticalViolation;
    hasAlarm = hasAlarm || stats.alarmViolation;
    totalReadFailures += stats.readFailure;
  }

  for (const auto& [sensorType, violations] : violationsBySensorType) {
    auto typeName = sensorTypeName(sensorType);
    fb303::fbData->setCounter(
        fmt::format(kAggHasCriticalThresholdViolation, typeName),
        violations.first);
    fb303::fbData->setCounter(
        fmt::format(kAggHasAlarmThresholdViolation, typeName),
        violations.second);
  }

  fb303::fbData->setCounter(kReadTotal, polledData.size());
  fb303::fbData->setCounter(kTotalReadFailure, totalReadFailures);
  fb303::fbData->setCounter(kHasReadFailure, totalReadFailures > 0 ? 1 : 0);
  XLOG(INFO) << fmt::format(
      "Summary: Processed {} Sensors. {} Failures.",
      polledData.size(),
      totalReadFailures);
}

void SensorServiceImpl::publishDerivedStats(
    const std::string& entity,
    std::optional<float> value) {
  fb303::fbData->setCounter(
      fmt::format(kDerivedValue, entity), value.value_or(0));
  if (!value) {
    fb303::fbData->setCounter(fmt::format(kDerivedFailure, entity), 1);
  } else {
    fb303::fbData->setCounter(fmt::format(kDerivedFailure, entity), 0);
  }
}

SensorData SensorServiceImpl::processAsicCmd(const AsicCommand& asicCommand) {
  SensorData sensorData{};
  sensorData.name() = *asicCommand.sensorName();
  sensorData.sensorType() = *asicCommand.sensorType();

  if (asicCommand.cmd()->empty()) {
    XLOG(ERR) << "AsicCommand cmd is empty";
    return sensorData;
  }

  const auto& cmd = *asicCommand.cmd();

  auto [exitStatus, output] = platformUtils_->execCommand(cmd);
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Failed to run AsicCommand '{}' with exit status: {}", cmd, exitStatus);
    structuredLogger_.logAlert(
        "asic_cmd_failure",
        fmt::format("Command failed with exit status: {}", exitStatus),
        {{"command", cmd}, {"exit_status", std::to_string(exitStatus)}});
    return sensorData;
  }

  sensorData.timeStamp() = Utils::nowInSecs();
  try {
    sensorData.value() = folly::to<uint16_t>(output);
  } catch (const std::exception& e) {
    XLOG(ERR) << fmt::format(
        "Failed to parse AsicCommand '{}' output: {}", output, e.what());
  }
  return sensorData;
}

void SensorServiceImpl::processPower(
    const std::map<std::string, SensorData>& polledData,
    const PowerConfig& powerConfig) {
  auto getSensorValue = [&](const std::string& sensorName) {
    auto it = polledData.find(sensorName);
    if (it != polledData.end()) {
      return it->second.value().to_optional();
    }
    return std::optional<float>(std::nullopt);
  };

  float totalPowerVal{0};

  // Process per-slot power configs (PSU/PEM/HSC)
  for (const auto& perSlotConfig : *powerConfig.perSlotPowerConfigs()) {
    std::optional<float> slotPower{std::nullopt};
    std::string calcMethod{};
    if (perSlotConfig.powerSensorName()) {
      slotPower = getSensorValue(*perSlotConfig.powerSensorName());
      calcMethod =
          fmt::format("Power Sensor: {}", *perSlotConfig.powerSensorName());
    } else if (
        perSlotConfig.voltageSensorName() &&
        perSlotConfig.currentSensorName()) {
      auto voltage = getSensorValue(*perSlotConfig.voltageSensorName());
      auto current = getSensorValue(*perSlotConfig.currentSensorName());
      if (voltage && current) {
        slotPower = *voltage * *current;
      }
      calcMethod = fmt::format(
          "Voltage Sensor: {} * Current Sensor: {}",
          *perSlotConfig.voltageSensorName(),
          *perSlotConfig.currentSensorName());
    }
    publishDerivedStats(
        fmt::format("{}_POWER", *perSlotConfig.name()), slotPower);
    if (slotPower) {
      XLOG(INFO) << fmt::format(
          "{}: Power {}W (Based on {})",
          *perSlotConfig.name(),
          *slotPower,
          calcMethod);
      totalPowerVal += *slotPower;
    } else {
      XLOG(ERR) << fmt::format(
          "{}: Error reading power (Based on {})",
          *perSlotConfig.name(),
          calcMethod);
      structuredLogger_.logAlert(
          "power_read_failure",
          fmt::format("Error reading power for {}", *perSlotConfig.name()),
          {{"slot_name", *perSlotConfig.name()}, {"calc_method", calcMethod}});
    }
  }

  // Process other power sensors (e.g., FANx power sensors)
  for (const auto& sensorName : *powerConfig.otherPowerSensorNames()) {
    auto sensorValue = getSensorValue(sensorName);
    if (sensorValue) {
      XLOG(INFO) << fmt::format(
          "{}: Power {}W (Direct Sensor)", sensorName, *sensorValue);
      totalPowerVal += *sensorValue;
    } else {
      XLOG(ERR) << fmt::format("{}: Error reading power sensor", sensorName);
      structuredLogger_.logAlert(
          "power_read_failure",
          fmt::format("Error reading power sensor {}", sensorName),
          {{"sensor_name", sensorName}});
    }
  }

  // Add power delta if configured
  if (*powerConfig.powerDelta() != 0) {
    XLOG(INFO) << fmt::format(
        "Adding power delta: {}W", *powerConfig.powerDelta());
    totalPowerVal += *powerConfig.powerDelta();
  }

  publishDerivedStats(kTotalPower, totalPowerVal);
}

void SensorServiceImpl::processTemperature(
    const std::map<std::string, SensorData>& polledData,
    const std::vector<TemperatureConfig>& tempConfigs) {
  auto getSensorValue = [&](const std::string& sensorName) {
    auto it = polledData.find(sensorName);
    if (it != polledData.end()) {
      return it->second.value().to_optional();
    }
    return std::optional<float>(std::nullopt);
  };

  for (const auto& tempConfig : tempConfigs) {
    std::optional<float> maxTemp{std::nullopt};
    int numFailures{0};
    std::optional<std::string> maxSensor{};
    for (const auto& sensorName : *tempConfig.temperatureSensorNames()) {
      auto sensorValue = getSensorValue(sensorName);
      if (sensorValue) {
        if (!maxTemp || *sensorValue > *maxTemp) {
          maxTemp = sensorValue;
          maxSensor = sensorName;
        }
      } else {
        numFailures++;
      }
    }
    publishDerivedStats(fmt::format("{}_TEMP", *tempConfig.name()), maxTemp);
    XLOG(INFO) << fmt::format(
        "{}: Temperature {}°C (Based on {}).  Failures: {}",
        *tempConfig.name(),
        maxTemp.value_or(0),
        maxSensor.value_or("NONE"),
        numFailures);
  }
}

void SensorServiceImpl::processInputVoltage(
    const std::map<std::string, SensorData>& polledData,
    const std::vector<std::string>& inputVoltageSensors) {
  auto getSensorValue = [&](const std::string& sensorName) {
    auto it = polledData.find(sensorName);
    if (it != polledData.end()) {
      return it->second.value().to_optional();
    }
    return std::optional<float>(std::nullopt);
  };

  std::optional<float> maxVoltage{std::nullopt};
  int numFailures{0};
  std::optional<std::string> maxSensor{};

  for (const auto& sensorName : inputVoltageSensors) {
    auto sensorValue = getSensorValue(sensorName);
    if (sensorValue) {
      if (!maxVoltage || *sensorValue > *maxVoltage) {
        maxVoltage = sensorValue;
        maxSensor = sensorName;
      }
    } else {
      numFailures++;
    }
  }

  publishDerivedStats(kMaxInputVoltage, maxVoltage);

  // Determine input power type based on voltage
  // AC if maxVoltage > kACVoltageThreshold, DC otherwise
  // We only do this once as power input type should not change during the
  // sensor service execution lifetime
  if (maxVoltage && *maxVoltage > kMinVoltageThreshold &&
      inputPowerType_ == kInputPowerTypeUnknown) {
    inputPowerType_ = (*maxVoltage > kACVoltageThreshold) ? kInputPowerTypeAC
                                                          : kInputPowerTypeDC;
  }
  publishDerivedStats(kInputPowerType, inputPowerType_);

  XLOG(INFO) << fmt::format(
      "Max Input Voltage: {}V (Based on {}).  Processed: {}/{}.  Power Type: {}",
      maxVoltage.value_or(0),
      maxSensor.value_or("NONE"),
      inputVoltageSensors.size() - numFailures,
      inputVoltageSensors.size(),
      inputPowerType_ == kInputPowerTypeUnknown
          ? "Unknown"
          : (inputPowerType_ == kInputPowerTypeDC ? "DC" : "AC"));
}
} // namespace facebook::fboss::platform::sensor_service
