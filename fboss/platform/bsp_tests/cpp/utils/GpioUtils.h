#pragma once

#include <string>
#include <vector>

namespace facebook::fboss::platform::bsp_tests::cpp {

// Represents a GPIO device detected by gpiodetect
struct GpioDetectResult {
  std::string name;
  std::string label;
  int lines;
};

// Represents a GPIO line info from gpioinfo
struct GpioInfoResult {
  std::string name;
  int lineNum;
  std::string direction;
};

class GpioUtils {
 public:
  // Run gpiodetect command and parse the results
  // If label is provided, only return devices with matching label
  static std::vector<GpioDetectResult> gpiodetect(
      const std::string& label = "");

  // Run gpioinfo command for a specific GPIO device and parse the results
  static std::vector<GpioInfoResult> gpioinfo(const std::string& name);

  // Get the value of a specific GPIO line
  static int gpioget(const std::string& name, int line);
};

} // namespace facebook::fboss::platform::bsp_tests::cpp
