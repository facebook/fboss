// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/Utils.h"

#include <chrono>

#include <exprtk.hpp>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/sensor_service/ConfigValidator.h"

namespace fs = std::filesystem;

namespace facebook::fboss::platform::sensor_service {

namespace {
// Strict weak ordering of VersionedSensorComparator(lhs,rhs)
// Returns true if lhs > rhs, false otherwise.
struct {
  bool operator()(
      const platform_manager::PmUnitVersion& l1,
      const platform_manager::PmUnitVersion& l2) {
    if (*l1.productProductionState() != *l2.productProductionState()) {
      return *l1.productProductionState() > *l2.productProductionState();
    }
    if (*l1.productVersion() != *l2.productVersion()) {
      return *l1.productVersion() > *l2.productVersion();
    }
    return *l1.productSubVersion() > *l2.productSubVersion();
  }
  bool operator()(
      const VersionedPmSensor& vSensor1,
      const VersionedPmSensor& vSensor2) {
    if (*vSensor1.productProductionState() !=
        *vSensor2.productProductionState()) {
      return *vSensor1.productProductionState() >
          *vSensor2.productProductionState();
    }
    if (*vSensor1.productVersion() != *vSensor2.productVersion()) {
      return *vSensor1.productVersion() > *vSensor2.productVersion();
    }
    return *vSensor1.productSubVersion() > *vSensor2.productSubVersion();
  }
} VersionedSensorComparator;
} // namespace

uint64_t Utils::nowInSecs() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

float Utils::computeExpression(
    const std::string& equation,
    float input,
    const std::string& symbol) {
  std::string temp_equation = equation;

  // Replace "@" with a valid symbol
  static const re2::RE2 atRegex("@");

  re2::RE2::GlobalReplace(&temp_equation, atRegex, symbol);

  exprtk::symbol_table<float> symbolTable;

  symbolTable.add_variable(symbol, input);

  exprtk::expression<float> expr;
  expr.register_symbol_table(symbolTable);

  exprtk::parser<float> parser;
  parser.compile(temp_equation, expr);

  return expr.value();
}

std::optional<VersionedPmSensor> Utils::resolveVersionedSensors(
    const PmUnitInfoFetcher& fetcher,
    const std::string& slotPath,
    std::vector<VersionedPmSensor> versionedSensors) {
  if (versionedSensors.empty()) {
    return std::nullopt;
  }
  // Sort in a descending order.
  std::sort(
      versionedSensors.begin(),
      versionedSensors.end(),
      VersionedSensorComparator);
  const auto pmUnitInfo = fetcher.fetch(slotPath);
  // Use the latest PmUnitInfo as the best effort because eventually the latest
  // respins will only be deployed to DC. So more merits to tailor towards them
  // with an assumption that latest respins will mainly be in the DC.
  if (!pmUnitInfo || !pmUnitInfo->version()) {
    XLOG(INFO) << fmt::format(
        "No version available for PmUnit at {}. "
        "Fall back to the latest VersionedPmSensor",
        slotPath);
    return versionedSensors.front();
  }
  const auto& fetchedVersion = *pmUnitInfo->version();
  for (const auto& versionedSensor : versionedSensors) {
    // Find a VersionedSensor that satisfies fetched PmUnitInfo version.
    // i.e. PmUnitInfo version >= VersionedSensor sensor
    platform_manager::PmUnitVersion sensorVersion;
    sensorVersion.productProductionState() =
        *versionedSensor.productProductionState();
    sensorVersion.productVersion() = *versionedSensor.productVersion();
    sensorVersion.productSubVersion() = *versionedSensor.productSubVersion();
    if (!VersionedSensorComparator(sensorVersion, fetchedVersion)) {
      XLOG(INFO) << fmt::format(
          "Resolved to VersionedPmSensor of version {}.{}.{}",
          *versionedSensor.productProductionState(),
          *versionedSensor.productVersion(),
          *versionedSensor.productSubVersion());
      return versionedSensor;
    }
  }
  return std::nullopt;
}

SensorConfig Utils::getConfig() {
  auto platformName = helpers::PlatformNameLib().getPlatformName();
  SensorConfig sensorConfig =
      apache::thrift::SimpleJSONSerializer::deserialize<SensorConfig>(
          ConfigLib().getSensorServiceConfig(platformName));
  if (*sensorConfig.platformName() != platformName.value_or("")) {
    throw std::runtime_error(
        fmt::format(
            "platformName in config '{}' does not match inferred name '{}'",
            *sensorConfig.platformName(),
            platformName.value_or("")));
  }
  if (!ConfigValidator().isValid(sensorConfig)) {
    throw std::runtime_error("Invalid sensor config");
  }
  return sensorConfig;
}

std::optional<std::string> Utils::getPciAddress(
    const std::string& vendorId,
    const std::string& deviceId) {
  std::optional<std::string> sbdf;
  for (const auto& dirEntry : fs::directory_iterator("/sys/bus/pci/devices")) {
    std::string vendor, device;
    auto deviceFilePath = dirEntry.path() / "device";
    auto vendorFilePath = dirEntry.path() / "vendor";
    if (!folly::readFile(vendorFilePath.c_str(), vendor)) {
      XLOG(ERR) << "Failed to read vendor file from " << dirEntry.path();
    }
    if (!folly::readFile(deviceFilePath.c_str(), device)) {
      XLOG(ERR) << "Failed to read device file from " << dirEntry.path();
    }
    if (folly::trimWhitespace(vendor).str() == vendorId &&
        folly::trimWhitespace(device).str() == deviceId) {
      sbdf = dirEntry.path().filename().string();
      break;
    }
  }
  return sbdf;
}
} // namespace facebook::fboss::platform::sensor_service
