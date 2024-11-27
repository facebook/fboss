// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/Utils.h"

#include <array>
#include <chrono>

#include <exprtk.hpp>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/sensor_service/ConfigValidator.h"

namespace facebook::fboss::platform::sensor_service {

namespace {
// Strict weak ordering of VersionedSensorComparator(lhs,rhs)
// Returns true if lhs > rhs, false otherwise.
struct {
  bool operator()(
      const std::array<int16_t, 3>& l1,
      const std::array<int16_t, 3>& l2,
      uint8_t i = 0) {
    if (i == 3) {
      return false;
    }
    if (l1[i] > l2[i]) {
      return true;
    }
    if (l1[i] < l2[i]) {
      return false;
    }
    return (*this)(l1, l2, ++i);
  }
  bool operator()(
      const VersionedPmSensor& vSensor1,
      const VersionedPmSensor& vSensor2) {
    return (*this)(
        {*vSensor1.productProductionState(),
         *vSensor1.productVersion(),
         *vSensor1.productSubVersion()},
        {*vSensor2.productProductionState(),
         *vSensor2.productVersion(),
         *vSensor2.productSubVersion()});
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
  // with an assumption that latest respions will mainly be in the DC.
  if (!pmUnitInfo) {
    XLOG(INFO) << fmt::format(
        "Fail to fetch PmUnitInfo at {} from PlatformManager. "
        "Fall back to the latest VersionedPmSensor",
        slotPath);
    return versionedSensors.front();
  }
  for (const auto& versionedSensor : versionedSensors) {
    // Find a VersionedSensor that satisfies fetched PmUnitInfo version.
    // i.e. PmUnitInfo version >= VersionedSensor sensor
    if (!VersionedSensorComparator(
            {*versionedSensor.productProductionState(),
             *versionedSensor.productVersion(),
             *versionedSensor.productSubVersion()},
            *pmUnitInfo)) {
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
  if (!ConfigValidator().isValid(sensorConfig, std::nullopt)) {
    throw std::runtime_error("Invalid sensor config");
  }
  return sensorConfig;
}
} // namespace facebook::fboss::platform::sensor_service
