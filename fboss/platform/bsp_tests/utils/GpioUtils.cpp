#include "fboss/platform/bsp_tests/utils/GpioUtils.h"

#include <re2/re2.h>
#include <sstream>
#include <stdexcept>
#include <string>

#include <fmt/format.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::bsp_tests {

std::vector<GpioDetectResult> GpioUtils::gpiodetect(const std::string& label) {
  std::vector<GpioDetectResult> results;

  try {
    auto [exitStatus, output] = PlatformUtils().execCommand("gpiodetect");
    if (exitStatus != 0) {
      XLOG(ERR) << "gpiodetect command failed with exit status " << exitStatus;
      return results;
    }
    re2::RE2 pattern(R"((\w+)\s+\[(.*?)\]\s+\((.*?) lines\))");
    std::string deviceName, deviceLabel, numLinesStr;
    re2::StringPiece input(output);

    while (RE2::FindAndConsume(
        &input, pattern, &deviceName, &deviceLabel, &numLinesStr)) {
      int numLines = std::stoi(numLinesStr);

      // If label is provided, only add devices with matching label
      if (label.empty() || deviceLabel == label) {
        results.push_back({deviceName, deviceLabel, numLines});
      }
    }
  } catch (const std::exception& e) {
    XLOG(ERR) << "Error in gpiodetect: " << e.what();
  }

  return results;
}

std::vector<GpioInfoResult> GpioUtils::gpioinfo(const std::string& name) {
  std::vector<GpioInfoResult> results;

  try {
    auto [exitStatus, output] =
        PlatformUtils().execCommand(fmt::format("gpioinfo {}", name));
    if (exitStatus != 0) {
      XLOG(ERR) << "gpioinfo command failed with exit status " << exitStatus;
      return results;
    }
    std::istringstream stream(output);
    std::string line;

    // Skip the header line
    std::getline(stream, line);

    while (std::getline(stream, line)) {
      std::istringstream lineStream(line);
      std::string token;
      // Skip the first token (line name)
      lineStream >> token;
      // Get line number (remove the trailing colon)
      lineStream >> token;
      int lineNum = std::stoi(token.substr(0, token.size() - 1));
      // Get line name
      lineStream >> token;
      std::string lineName = token;
      // Determine direction
      std::string direction = "None";
      while (lineStream >> token) {
        if (token == "input") {
          direction = "input";
          break;
        } else if (token == "output") {
          direction = "output";
          break;
        }
      }
      results.push_back({lineName, lineNum, direction});
    }
  } catch (const std::exception& e) {
    XLOG(ERR) << "Error in gpioinfo: " << e.what();
  }

  return results;
}

int GpioUtils::gpioget(const std::string& name, int line) {
  try {
    auto [exitStatus, output] =
        PlatformUtils().execCommand(fmt::format("gpioget {} {}", name, line));
    if (exitStatus != 0) {
      throw std::runtime_error(
          fmt::format(
              "gpioget command failed with exit status {}", exitStatus));
    }
    return std::stoi(output);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Error in gpioget: " << e.what();
    throw std::runtime_error(
        fmt::format(
            "Failed to get GPIO value for {}-{}: {}", name, line, e.what()));
  }
}

} // namespace facebook::fboss::platform::bsp_tests
