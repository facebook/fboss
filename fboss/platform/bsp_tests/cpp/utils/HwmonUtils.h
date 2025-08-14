#pragma once

#include <string>
#include <vector>

namespace facebook::fboss::platform::bsp_tests::cpp {

// Represents a sensor feature (e.g., fan1, temp1, etc.)
struct SensorFeature {
  std::string name;
  std::string label;
  double value{};
  std::string unit;
};

// Represents a sensor chip
struct SensorChip {
  std::string name;
  std::string bus;
  std::string address;
  std::vector<SensorFeature> features;
};

class HwmonUtils {
 public:
  // Get a specific sensor chip by name
  static SensorChip getDetectedChips(const std::string& chipName);

  // Check if lm_sensors package is installed, install if not
  static bool ensureSensorsInstalled();

 private:
  // Parse the output of the sensors command
  static SensorChip parseSensorsOutput(const std::string& output);
};

} // namespace facebook::fboss::platform::bsp_tests::cpp
