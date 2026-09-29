/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <charconv>
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
#include "fboss/platform/sensor_service/utilities/PowerConfigUtils.h"

DEFINE_int32(
    fsdb_statsStream_interval_seconds,
    5,
    "Interval at which stats subscriptions are served");

namespace {

// CPLD regbit sysfs attributes can emit hex (e.g. "0x1"), which
// folly::to<float> rejects. Parse hex explicitly, decimal otherwise, and
// return nullopt on any unparseable value so a single bad sensor degrades to a
// read failure instead of throwing out of the entire poll cycle.
//
// This hex-parsing path exists solely to address SEV S649086. A forthcoming BSP
// update will make the CPLD attrs emit decimal values, after which hex handling
// here can be removed.
std::optional<float> parseSensorValue(const std::string& raw) {
  auto trimmed = folly::trimWhitespace(raw);
  if (trimmed.empty()) {
    return std::nullopt;
  }
  if (trimmed.startsWith("0x") || trimmed.startsWith("0X")) {
    int64_t hexValue{};
    auto body = trimmed.subpiece(2);
    auto [ptr, ec] = std::from_chars(body.begin(), body.end(), hexValue, 16);
    if (ec == std::errc() && ptr == body.end()) {
      return static_cast<float>(hexValue);
    }
    return std::nullopt;
  }
  if (auto parsed = folly::tryTo<float>(trimmed)) {
    return *parsed;
  }
  return std::nullopt;
}

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

// nullopt = presence unknown (no PmUnitInfo, or no presenceInfo);
// platform_manager fetch failed or hasn't reported. Callers must
// treat unknown as fail-safe (do not suppress).
std::optional<bool> isSlotPresent(
    const std::optional<
        facebook::fboss::platform::platform_manager::PmUnitInfo>& info) {
  if (!info || !info->presenceInfo()) {
    return std::nullopt;
  }
  return *info->presenceInfo()->isPresent();
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

std::map<std::string, std::optional<platform_manager::PmUnitInfo>>
SensorServiceImpl::prefetchPmUnitInfos() {
  std::map<std::string, std::optional<platform_manager::PmUnitInfo>>
      pmUnitInfoMap;
  XLOG(INFO) << "Fetching PmUnitInfos from PlatformManager";
  for (const auto& pmUnitSensors : *sensorConfig_.pmUnitSensorsList()) {
    const auto& pmUnitName = *pmUnitSensors.pmUnitName();
    bool isPsuOrPem = (pmUnitName == "PSU" || pmUnitName == "PEM");
    // Fetch when versioned-sensor resolution needs a PmUnitInfo, OR the
    // slot is a field-replaceable PSU/PEM (we use presence info from
    // the same map to skip absent-slot sensor collection, processPower
    // skip, and publishPsuPemCounters).
    if (pmUnitSensors.versionedSensors()->empty() && !isPsuOrPem) {
      continue;
    }
    const auto& slotPath = *pmUnitSensors.slotPath();
    if (pmUnitInfoMap.count(slotPath)) {
      continue;
    }
    auto pmUnitInfo = pmUnitInfoFetcher_->fetch(slotPath);
    pmUnitInfoMap[slotPath] = pmUnitInfo;
    if (pmUnitInfo && pmUnitInfo->version()) {
      XLOG(INFO) << fmt::format(
          "Found v{}.{}.{} of {} at {}",
          *pmUnitInfo->version()->productionState(),
          *pmUnitInfo->version()->productionSubState(),
          *pmUnitInfo->version()->respinVariantIndicator(),
          *pmUnitInfo->name(),
          slotPath);
    } else if (pmUnitInfo) {
      XLOG(INFO) << fmt::format(
          "No version found for {} at {}. The unit may not have an IDPROM",
          *pmUnitInfo->name(),
          slotPath);
    }
  }
  return pmUnitInfoMap;
}

void SensorServiceImpl::fetchSensorData() {
  std::map<std::string, SensorData> polledData;
  std::vector<PmSensor> loadLineSensors;

  auto pmUnitInfoMap = prefetchPmUnitInfos();

  XLOG(INFO) << fmt::format(
      "Reading SensorData for {} PMUnits",
      sensorConfig_.pmUnitSensorsList()->size());
  const std::optional<platform_manager::PmUnitInfo> emptyPmUnitInfo;
  for (const auto& pmUnitSensors : *sensorConfig_.pmUnitSensorsList()) {
    const auto& slotPath = *pmUnitSensors.slotPath();
    const auto& pmUnitName = *pmUnitSensors.pmUnitName();
    auto it = pmUnitInfoMap.find(slotPath);
    const auto& pmUnitInfo =
        (it != pmUnitInfoMap.end()) ? it->second : emptyPmUnitInfo;

    // Skip sensor collection entirely for confirmed-absent PSU/PEM
    // slots. Avoids ~all sysfs read failures (and the resulting
    // FBOSS_ALERT sensor_sysfs_read_failure events) for hardware that
    // platform_manager reports physically not present. Unknown
    // presence (RPC failed, no presenceInfo) → fall through and
    // collect, preserving fail-safe semantics.
    if ((pmUnitName == "PSU" || pmUnitName == "PEM") &&
        isSlotPresent(pmUnitInfo) == false) {
      XLOG(INFO) << fmt::format(
          "PSU/PEM at {} reported absent; skipping sensor collection",
          slotPath);
      continue;
    }

    // Resolve versioned sensors and merge with base sensors
    auto pmSensors = *pmUnitSensors.sensors();
    auto versionedPmSensors = utils_->resolveVersionedSensors(
        pmUnitInfo, slotPath, *pmUnitSensors.versionedSensors());
    if (versionedPmSensors) {
      publishVersionedSensorStats(
          *pmUnitSensors.pmUnitName(),
          *versionedPmSensors->productionState(),
          *versionedPmSensors->productionSubState(),
          *versionedPmSensors->respinVariantIndicator());
      pmSensors.insert(
          pmSensors.end(),
          versionedPmSensors->sensors()->begin(),
          versionedPmSensors->sensors()->end());
    }

    // Log with platform version and resolved VersionedSensor version
    if (pmUnitInfo && pmUnitInfo->version() && versionedPmSensors) {
      XLOG(INFO) << fmt::format(
          "Processing {} PMUnit (v{}.{}.{}). "
          "Using VersionedPmSensor v{}.{}.{} (productName: {}): {} sensors",
          *pmUnitSensors.pmUnitName(),
          *pmUnitInfo->version()->productionState(),
          *pmUnitInfo->version()->productionSubState(),
          *pmUnitInfo->version()->respinVariantIndicator(),
          *versionedPmSensors->productionState(),
          *versionedPmSensors->productionSubState(),
          *versionedPmSensors->respinVariantIndicator(),
          versionedPmSensors->productName().value_or("UNSET"),
          pmSensors.size());
    } else if (versionedPmSensors) {
      XLOG(INFO) << fmt::format(
          "Processing {} PMUnit. Using VersionedSensor v{}.{}.{} (productName: {}): {} sensors",
          *pmUnitSensors.pmUnitName(),
          *versionedPmSensors->productionState(),
          *versionedPmSensors->productionSubState(),
          *versionedPmSensors->respinVariantIndicator(),
          versionedPmSensors->productName().value_or("UNSET"),
          pmSensors.size());
    } else {
      XLOG(INFO) << fmt::format(
          "Processing {} PMUnit: {} sensors",
          *pmUnitSensors.pmUnitName(),
          pmSensors.size());
    }
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
      // For AVS rails that use load-line regulation (Adaptive Voltage
      // Positioning), we need to adjust their read vout to account for
      // load-line droops. So we defer publishing the stats for such sensors.
      if (sensor.loadLineuOhms().has_value()) {
        loadLineSensors.push_back(sensor);
      } else {
        publishPerSensorStats(sensorData);
      }
    }
  }

  if (sensorConfig_.asicCommand()) {
    XLOG(INFO) << "Processing Asic Command";
    auto sensorData = processAsicCmd(sensorConfig_.asicCommand().value());
    const auto& sensorName = *sensorConfig_.asicCommand()->sensorName();
    polledData[sensorName] = sensorData;
    publishPerSensorStats(sensorData);
  }

  processPower(polledData, *sensorConfig_.powerConfig(), pmUnitInfoMap);

  processLoadLineDroopAdjust(polledData, loadLineSensors);

  processTemperature(polledData, *sensorConfig_.temperatureConfigs());

  processInputVoltage(polledData, *sensorConfig_.powerConfig());

  // Must run after processInputVoltage so inputPowerType_ is resolved.
  publishPsuPemCounters(pmUnitInfoMap, *sensorConfig_.powerConfig());

  publishAggStats(polledData);

  logSensorValues(polledData);

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

std::vector<PmSensor> SensorServiceImpl::resolveSensors(
    const PmUnitSensors& pmUnitSensors) {
  auto pmUnitInfo = pmUnitInfoFetcher_->fetch(*pmUnitSensors.slotPath());
  auto pmSensors = *pmUnitSensors.sensors();
  if (auto versionedPmSensors = utils_->resolveVersionedSensors(
          pmUnitInfo,
          *pmUnitSensors.slotPath(),
          *pmUnitSensors.versionedSensors())) {
    pmSensors.insert(
        pmSensors.end(),
        versionedPmSensors->sensors()->begin(),
        versionedPmSensors->sensors()->end());
  }
  return pmSensors;
}

void SensorServiceImpl::publishVersionedSensorStats(
    const std::string& pmUnitName,
    int16_t productionState,
    int16_t productionSubState,
    int16_t respinVariantIndicator) {
  fb303::fbData->setCounter(
      fmt::format(
          kPmUnitVersion,
          pmUnitName,
          productionState,
          productionSubState,
          respinVariantIndicator),
      1);
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
    if (auto parsedValue = parseSensorValue(sensorValue)) {
      sensorData.value() = *parsedValue;
      sensorData.timeStamp() = Utils::nowInSecs();
      if (compute) {
        sensorData.value() =
            Utils::computeExpression(*compute, *sensorData.value());
      }
      XLOG(DBG1) << fmt::format(
          "{} ({}) : {}", sensorName, sysfsPath, *sensorData.value());
    } else {
      XLOG(ERR) << fmt::format(
          "Could not parse value '{}' for {} from path:{}",
          folly::trimWhitespace(sensorValue).str(),
          sensorName,
          sysfsPath);
      structuredLogger_.logAlert(
          "sensor_sysfs_read_failure",
          "Unparseable sensor value",
          {{"sensor_name", sensorName}, {"sysfs_path", sysfsPath}});
    }
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

void SensorServiceImpl::logSensorValues(
    const std::map<std::string, SensorData>& polledData) {
  const auto& loggedSensorNames = *sensorConfig_.loggedSensorNames();
  if (loggedSensorNames.empty()) {
    return;
  }
  for (const auto& sensorName : loggedSensorNames) {
    auto it = polledData.find(sensorName);
    if (it == polledData.end()) {
      // ConfigValidator guarantees the name resolves on every hardware
      // version, but a sensor can still be missing from a given cycle — e.g.
      // it lives on a PSU/PEM slot that platform_manager reported absent, so
      // collection was skipped. Surface it rather than dropping the configured
      // log line silently.
      XLOG(WARN) << fmt::format(
          "{} {}: sensor data missing", kLoggedSensorPrefix, sensorName);
      continue;
    }
    const auto& sensorData = it->second;
    if (auto value = sensorData.value().to_optional()) {
      XLOG(INFO) << fmt::format(
          "{} {} = {} (sysfs: {})",
          kLoggedSensorPrefix,
          sensorName,
          *value,
          *sensorData.sysfsPath());
    } else {
      XLOG(WARN) << fmt::format(
          "{} {} = READ_FAILED (sysfs: {})",
          kLoggedSensorPrefix,
          sensorName,
          *sensorData.sysfsPath());
    }
  }
}

void SensorServiceImpl::publishAggStats(
    const std::map<std::string, SensorData>& polledData) {
  std::map<SensorType, std::pair<bool, bool>> violationsBySensorType;
  int totalReadFailures{0};
  bool hasAnyCriticalViolation{false};
  bool hasAnyAlarmViolation{false};

  for (const auto& [_, sensorData] : polledData) {
    auto stats = computeSensorStats(sensorData);
    auto& [hasCritical, hasAlarm] =
        violationsBySensorType[*sensorData.sensorType()];
    hasCritical = hasCritical || stats.criticalViolation;
    hasAlarm = hasAlarm || stats.alarmViolation;
    hasAnyCriticalViolation =
        hasAnyCriticalViolation || stats.criticalViolation;
    hasAnyAlarmViolation = hasAnyAlarmViolation || stats.alarmViolation;
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
  fb303::fbData->setCounter(
      kHasCriticalThresholdViolation, hasAnyCriticalViolation ? 1 : 0);
  fb303::fbData->setCounter(
      kHasAlarmThresholdViolation, hasAnyAlarmViolation ? 1 : 0);
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

void SensorServiceImpl::processLoadLineDroopAdjust(
    std::map<std::string, SensorData>& polledData,
    const std::vector<PmSensor>& loadLineSensors) {
  auto getSensorValue = [&](const std::string& sensorName) {
    auto it = polledData.find(sensorName);
    if (it != polledData.end()) {
      return it->second.value().to_optional();
    }
    return std::optional<float>(std::nullopt);
  };

  for (const auto& sensor : loadLineSensors) {
    const auto& voltageSensorName = *sensor.name();
    const auto& loadLineSensorName = *sensor.loadLineCurrentSensor();
    const double uOhms = *sensor.loadLineuOhms();
    auto voutIt = polledData.find(voltageSensorName);
    if (voutIt == polledData.end()) {
      XLOG(ERR) << fmt::format(
          "Load-line sensor {} missing from polled data; skipping",
          voltageSensorName);
      continue;
    }

    auto rawVout = voutIt->second.value().to_optional();
    auto loadLineAmps = getSensorValue(loadLineSensorName);

    if (!rawVout || !loadLineAmps) {
      XLOG(WARNING) << fmt::format(
          "Skipping load-line adjustment for {}", voltageSensorName);
      publishPerSensorStats(voutIt->second);
      continue;
    }

    double vout = static_cast<double>(*rawVout) + uOhms * 1e-6 * *loadLineAmps;
    voutIt->second.value() = static_cast<float>(vout);
    publishPerSensorStats(voutIt->second);
  }
}

void SensorServiceImpl::processPower(
    const std::map<std::string, SensorData>& polledData,
    const PowerConfig& powerConfig,
    const std::map<std::string, std::optional<platform_manager::PmUnitInfo>>&
        pmUnitInfoMap) {
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
    // Skip per-slot _POWER publication for PSU/PEM slots that
    // platform_manager confirms absent. The validator (D98824972)
    // requires PSU/PEM PerSlotPowerConfigs to set a non-empty slotPath
    // for production configs; the has_value() guard keeps unit tests
    // that bypass the validator (e.g. SensorServiceImplPowerConsumption-
    // Test) safe. Unknown presence (RPC failed, missing presenceInfo)
    // → fall through and process (fail-safe).
    if (isPsuOrPem(perSlotConfig) && perSlotConfig.slotPath().has_value()) {
      auto it = pmUnitInfoMap.find(*perSlotConfig.slotPath());
      if (it != pmUnitInfoMap.end() && isSlotPresent(it->second) == false) {
        continue;
      }
    }
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

void SensorServiceImpl::publishPsuPemCounters(
    const std::map<std::string, std::optional<platform_manager::PmUnitInfo>>&
        pmUnitInfoMap,
    const PowerConfig& powerConfig) {
  // Count physical PSU/PEM slots present, by iterating pmUnitSensorsList
  // (one entry per physical slot). NOT perSlotPowerConfigs — that can
  // have multiple logical entries per physical slot (e.g. Darwin's
  // PEM1 + PEM2 both at /PEM_SLOT@0 for PIN = VIN×CH1 + VIN×CH2).
  bool hasAnyPsuOrPemSlot = false;
  int presentCount = 0;
  for (const auto& pmUnitSensors : *sensorConfig_.pmUnitSensorsList()) {
    const auto& pmUnitName = *pmUnitSensors.pmUnitName();
    if (pmUnitName != "PSU" && pmUnitName != "PEM") {
      continue;
    }
    hasAnyPsuOrPemSlot = true;
    auto it = pmUnitInfoMap.find(*pmUnitSensors.slotPath());
    if (it != pmUnitInfoMap.end() && isSlotPresent(it->second) == true) {
      ++presentCount;
    }
  }
  if (!hasAnyPsuOrPemSlot) {
    return;
  }
  fb303::fbData->setCounter(kTotalNumPresentPsu, presentCount);

  // Skip the boolean alert when we can't pick a threshold (unknown power
  // type or no min configured for the detected type).
  if (inputPowerType_ != kInputPowerTypeUnknown) {
    int expectedMin = (inputPowerType_ == kInputPowerTypeAC)
        ? *powerConfig.minAcPsuCount()
        : *powerConfig.minDcPsuCount();
    if (expectedMin > 0) {
      fb303::fbData->setCounter(
          kUnexpectedNumPresentPsu, presentCount < expectedMin ? 1 : 0);
    }
  }
}

void SensorServiceImpl::processInputVoltage(
    std::map<std::string, SensorData>& polledData,
    const PowerConfig& powerConfig) {
  const auto& inputVoltageSensors = *powerConfig.inputVoltageSensors();

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

  // Determine input power type using voltage ranges from PowerConfig:
  // - AC: voltage >= acVoltageMin (default 90V)
  // - DC: dcVoltageMin (default 9V) <= voltage <= dcVoltageMax (default 64V)
  // - Unknown: outside both ranges
  // We only determine power type once as it should not change during the
  // sensor service execution lifetime
  if (maxVoltage && inputPowerType_ == kInputPowerTypeUnknown) {
    if (*maxVoltage >= *powerConfig.acVoltageMin()) {
      inputPowerType_ = kInputPowerTypeAC;
    } else if (
        *maxVoltage >= *powerConfig.dcVoltageMin() &&
        *maxVoltage <= *powerConfig.dcVoltageMax()) {
      inputPowerType_ = kInputPowerTypeDC;
    }
    // Apply voltage thresholds from PowerConfig (defaults defined in thrift)
    if (inputPowerType_ != kInputPowerTypeUnknown) {
      auto [lowerCriticalVoltage, upperCriticalVoltage] =
          (inputPowerType_ == kInputPowerTypeAC)
          ? std::make_pair(
                static_cast<float>(*powerConfig.acVoltageMin()),
                static_cast<float>(*powerConfig.acVoltageMax()))
          : std::make_pair(
                static_cast<float>(*powerConfig.dcVoltageMin()),
                static_cast<float>(*powerConfig.dcVoltageMax()));
      for (const auto& sensorName : inputVoltageSensors) {
        auto it = polledData.find(sensorName);
        if (it != polledData.end() &&
            !it->second.thresholds()->lowerCriticalVal().has_value() &&
            !it->second.thresholds()->upperCriticalVal().has_value()) {
          it->second.thresholds()->lowerCriticalVal() = lowerCriticalVoltage;
          it->second.thresholds()->upperCriticalVal() = upperCriticalVoltage;
        }
      }
    }
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
