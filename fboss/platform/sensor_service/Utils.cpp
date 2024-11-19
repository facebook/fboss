// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/Utils.h"

#include <array>
#include <chrono>

#include <exprtk.hpp>
#include <re2/re2.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/sensor_service/ConfigValidator.h"

namespace facebook::fboss::platform::sensor_service {

namespace {
// Compare the two array reprsentation of
// productionState,productVersion,productSubVersion.
// Return true if l1 >= l2, false otherwise.
bool greaterEqual(
    std::array<int16_t, 3> l1,
    std::array<int16_t, 3> l2,
    uint8_t i = 0) {
  if (i == 3) {
    return true;
  }
  if (l1[i] > l2[i]) {
    return true;
  }
  if (l1[i] < l2[i]) {
    return false;
  }
  return greaterEqual(l1, l2, ++i);
}
// Same as above greaterEqual except it takes VersionedPmSensor
bool greaterEqual(
    const VersionedPmSensor& vSensor1,
    const VersionedPmSensor& vSensor2) {
  return greaterEqual(
      {*vSensor1.productProductionState(),
       *vSensor1.productVersion(),
       *vSensor1.productSubVersion()},
      {*vSensor2.productProductionState(),
       *vSensor2.productVersion(),
       *vSensor2.productSubVersion()});
}
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
    const std::vector<VersionedPmSensor>& versionedSensors) {
  if (versionedSensors.empty()) {
    return std::nullopt;
  }
  const auto pmUnitInfo = fetcher.fetch(slotPath);
  if (!pmUnitInfo) {
    return std::nullopt;
  }
  std::optional<VersionedPmSensor> resolvedVersionedSensor{std::nullopt};
  for (const auto& versionedSensor : versionedSensors) {
    if (greaterEqual(
            *pmUnitInfo,
            {*versionedSensor.productProductionState(),
             *versionedSensor.productVersion(),
             *versionedSensor.productSubVersion()})) {
      resolvedVersionedSensor = std::max(
          versionedSensor,
          // Default VersionedPmSensor if null i.e (0,0,0)
          resolvedVersionedSensor.value_or(VersionedPmSensor{}),
          [](const auto& vSensor1, const auto& vSensor2) {
            return !greaterEqual(vSensor1, vSensor2);
          });
    }
  }
  return resolvedVersionedSensor;
}

SensorConfig Utils::getConfig() {
  auto platformName = helpers::PlatformNameLib().getPlatformName();
  SensorConfig sensorConfig =
      apache::thrift::SimpleJSONSerializer::deserialize<SensorConfig>(
          ConfigLib().getSensorServiceConfig(platformName));
  std::optional<platform_manager::PlatformConfig> platformConfig{std::nullopt};
  // TODO(T207042263) Enable cross-service config validation for Darwin
  // once Darwin onboards PM.
  if (platformName != "DARWIN") {
    platformConfig = apache::thrift::SimpleJSONSerializer::deserialize<
        platform_manager::PlatformConfig>(
        ConfigLib().getPlatformManagerConfig(platformName));
  }
  if (!ConfigValidator().isValid(sensorConfig, platformConfig)) {
    throw std::runtime_error("Invalid sensor config");
  }
  return sensorConfig;
}
} // namespace facebook::fboss::platform::sensor_service
