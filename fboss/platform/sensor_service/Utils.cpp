// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/Utils.h"

#include <array>
#include <chrono>
#include <iostream>

#include <exprtk.hpp>
#include <re2/re2.h>

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
} // namespace
namespace facebook::fboss::platform::sensor_service {

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
      resolvedVersionedSensor = versionedSensor;
    }
  }
  return resolvedVersionedSensor;
}

} // namespace facebook::fboss::platform::sensor_service
