#include "fboss/platform/bsp_tests/cpp/utils/HwmonUtils.h"

#include <re2/re2.h>
#include <sstream>
#include <stdexcept>
#include <string>

#include <fmt/format.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

SensorChip HwmonUtils::getDetectedChips(const std::string& chipName) {
  try {
    std::string command = "sensors";
    if (!chipName.empty()) {
      command = fmt::format("sensors '{}'", chipName);
    }

    auto [exitStatus, output] = PlatformUtils().execCommand(command);
    if (exitStatus != 0) {
      if (chipName.empty()) {
        XLOG(ERR) << "Failed to get sensor data: " << output;
        throw std::runtime_error("Failed to get sensor data");
      } else {
        XLOG(ERR) << "Failed to get sensor data for " << chipName << ": "
                  << output;
        throw std::runtime_error(
            fmt::format("Failed to get sensor data for {}", chipName));
      }
    }
    return parseSensorsOutput(output);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Exception during getDetectedChips: " << e.what();
    throw;
  }
}

bool HwmonUtils::ensureSensorsInstalled() {
  try {
    auto [exitStatus, output] = PlatformUtils().execCommand("which sensors");
    if (exitStatus == 0) {
      return true;
    }

    auto [installStatus, installOutput] =
        PlatformUtils().execCommand("dnf install lm_sensors -y");
    if (installStatus != 0) {
      XLOG(ERR) << "Failed to install lm_sensors: " << installOutput;
      return false;
    }

    return true;
  } catch (const std::exception& e) {
    XLOG(ERR) << "Exception during ensureSensorsInstalled: " << e.what();
    return false;
  }
}

SensorChip HwmonUtils::parseSensorsOutput(const std::string& output) {
  std::istringstream stream(output);
  std::string line;
  SensorChip chip;
  bool firstLine = true;

  // Regular expressions for parsing
  re2::RE2 featureRegex(R"(^(\w+):\s+([^(]*)(?:\(([^)]*)\))?\s*(.*)$)");
  re2::RE2 chipRegex(R"(^([^:]+)(?:-i2c-(\d+)-([0-9a-fA-F]+))?)");

  while (std::getline(stream, line)) {
    // Skip irrelevant lines
    if (line.empty() || line.starts_with("Adapter")) {
      continue;
    }

    // Check if this is a chip line
    if (firstLine) {
      std::string chipName, bus, address;
      if (RE2::FullMatch(line, chipRegex, &chipName, &bus, &address)) {
        chip.name = chipName;
        if (!bus.empty() && !address.empty()) {
          chip.bus = std::move(bus);
          chip.address = std::move(address);
        }
      } else {
        chip.name = line;
      }
      firstLine = false;
      continue;
    }

    std::string featureName, valueStr, label, unit;
    if (RE2::FullMatch(
            line, featureRegex, &featureName, &valueStr, &label, &unit)) {
      SensorFeature feature;
      feature.name = std::move(featureName);
      feature.label = std::move(label);

      // Try to parse the value
      try {
        feature.value = std::stod(valueStr);
      } catch (const std::exception&) {
        feature.value = 0.0;
      }

      feature.unit = std::move(unit);
      chip.features.push_back(feature);
    }
  }
  return chip;
}

} // namespace facebook::fboss::platform::bsp_tests::cpp
