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
#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/sensor_service/FsdbSyncer.h"
#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/Utils.h"
#include "fboss/platform/sensor_service/gen-cpp2/sensor_service_stats_types.h"

DEFINE_int32(
    fsdb_statsStream_interval_seconds,
    5,
    "Interval at which stats subscriptions are served");

namespace facebook::fboss::platform::sensor_service {
namespace {
void monitorSensorValue(const SensorData& sensorData) {
  // Don't monitor if thresholds aren't defined to prevent false data.
  if (!sensorData.thresholds()->upperCriticalVal() &&
      !sensorData.thresholds()->lowerCriticalVal()) {
    return;
  }
  // Skip reporting if there's no sensor value.
  if (!sensorData.value()) {
    return;
  }
  // At least one of upperCriticalVal or lowerCriticalVal exist.
  bool thresholdViolation = *sensorData.value() >
          sensorData.thresholds()->upperCriticalVal().value_or(INT_MAX) ||
      *sensorData.value() <
          sensorData.thresholds()->lowerCriticalVal().value_or(INT_MIN);
  fb303::fbData->setCounter(
      fmt::format(
          SensorServiceImpl::kCriticalThresholdViolation,
          *sensorData.name(),
          apache::thrift::util::enumNameSafe(*sensorData.sensorType())),
      thresholdViolation);
}
} // namespace

SensorServiceImpl::SensorServiceImpl(
    const SensorConfig& sensorConfig,
    const std::shared_ptr<Utils>& utils,
    const std::shared_ptr<PlatformUtils>& platformUtils)
    : sensorConfig_(sensorConfig),
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
  uint readFailures{0};
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
      if (!sensorData.value()) {
        readFailures++;
      }
      publishPerSensorStats(sensorName, sensorData.value().to_optional());
    }
  }

  if (sensorConfig_.switchAsicTemp()) {
    XLOG(INFO) << "Processing Asic Temperature";
    auto sensorData = getAsicTemp(sensorConfig_.switchAsicTemp().value());
    polledData[kAsicTemp] = sensorData;
    if (!sensorData.value()) {
      readFailures++;
    }
    publishPerSensorStats(kAsicTemp, sensorData.value().to_optional());
  }

  fb303::fbData->setCounter(kReadTotal, polledData.size());
  fb303::fbData->setCounter(kTotalReadFailure, readFailures);
  fb303::fbData->setCounter(kHasReadFailure, readFailures > 0 ? 1 : 0);
  XLOG(INFO) << fmt::format(
      "Summary: Processed {} Sensors. {} Failures.",
      polledData.size(),
      readFailures);
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
  }
  sensorData.thresholds() = thresholds ? *thresholds : Thresholds();
  sensorData.sensorType() = sensorType;
  monitorSensorValue(sensorData);
  return sensorData;
}

void SensorServiceImpl::publishPerSensorStats(
    const std::string& sensorName,
    std::optional<float> value) {
  // We log 0 if there is a read failure.  If we dont log 0 on failure,
  // fb303 will pick up the last reported (on read success) value and
  // keep reporting that as the value. For 0 values, it is accurate to
  // read the value along with the kReadFailure counter. Alternative is
  // to delete this counter if there is a failure.
  fb303::fbData->setCounter(
      fmt::format(kReadValue, sensorName), value.value_or(0));
  if (!value) {
    fb303::fbData->setCounter(fmt::format(kReadFailure, sensorName), 1);
  } else {
    fb303::fbData->setCounter(fmt::format(kReadFailure, sensorName), 0);
  }
}

SensorData SensorServiceImpl::getAsicTemp(const SwitchAsicTemp& asicTemp) {
  SensorData sensorData{};
  sensorData.name() = kAsicTemp;
  sensorData.sensorType() = SensorType::TEMPERTURE;
  if (!asicTemp.vendorId() || !asicTemp.deviceId()) {
    return sensorData;
  }
  auto sbdf = utils_->getPciAddress(*asicTemp.vendorId(), *asicTemp.deviceId());
  if (!sbdf) {
    XLOG(ERR) << fmt::format(
        "Could not find asic device with vendorId: {}, deviceId: {}",
        *asicTemp.vendorId(),
        *asicTemp.deviceId());
    return sensorData;
  }

  sensorData.sysfsPath() = fmt::format("/usr/bin/mget_temp -d {}", *sbdf);

  auto [exitStatus, output] =
      platformUtils_->runCommand({"/usr/bin/mget_temp", "-d", *sbdf});
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Failed to get ASIC temperature for PCI device {} with error: {}",
        *sbdf,
        output);
    return sensorData;
  }

  sensorData.timeStamp() = Utils::nowInSecs();
  sensorData.value() = folly::to<uint16_t>(output);
  return sensorData;
}

} // namespace facebook::fboss::platform::sensor_service
