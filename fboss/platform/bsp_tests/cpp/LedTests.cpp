#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <re2/re2.h>
#include <filesystem>
#include <string>
#include <vector>

#include "fboss/platform/bsp_tests/cpp/BspTest.h"
#include "fboss/platform/bsp_tests/cpp/utils/CdevUtils.h"
#include "fboss/platform/bsp_tests/cpp/utils/I2CUtils.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

re2::RE2 kPortLedName("port(\\d+)_led(\\d*)");
re2::RE2 kFanLedName("fan(\\d*)_led");

const std::vector<std::string> expectedColors = {"blue", "amber"};
const std::vector<std::string> validLedNames = {
    "psu_led",
    "sys_led",
    "smb_led"};

// Helper function to validate LED names with a specific pattern
void validateLedName(
    const std::string& ledName,
    std::vector<std::string>& errorMessages) {
  int firstId = -1;
  std::string secondIdStr;

  if (ledName.starts_with("port")) {
    if (!re2::RE2::FullMatch(ledName, kPortLedName, &firstId, &secondIdStr)) {
      errorMessages.emplace_back(
          fmt::format("LED name `{}` does not match expected format", ledName));
    }
  } else if (ledName.starts_with("fan")) {
    re2::RE2 fanLedRegex("fan(\\d*)_led");
    if (!re2::RE2::FullMatch(ledName, kFanLedName, &firstId)) {
      errorMessages.emplace_back(
          fmt::format("index in LED name {} is not 1-based", ledName));
    }
  } else {
    if (std::find(validLedNames.begin(), validLedNames.end(), ledName) ==
        validLedNames.end()) {
      errorMessages.emplace_back(fmt::format("Invalid LED name {}", ledName));
    }
    return;
  }

  if (firstId == 0) {
    errorMessages.emplace_back(
        fmt::format("index in LED name {} is not 1-based", ledName));
  }

  if (!secondIdStr.empty()) {
    int secondId = std::stoi(secondIdStr);
    if (secondId <= 0) {
      errorMessages.emplace_back(
          fmt::format("index in LED name {} is not 1-based", ledName));
    }
  }
}

// Helper function to find expected LEDs in the system
void findExpectedLeds(
    const std::string& deviceName,
    int ledId,
    int portNum,
    std::vector<std::string> errors) {
  std::string expectedLedNamePrefix;
  std::string idStr = std::to_string(ledId);

  if (deviceName == "port_led") {
    if (portNum <= 0) {
      errors.emplace_back("Port Index must be 1-based");
    }
    if (ledId <= 0) {
      errors.emplace_back("Port LED ID must be 1-based");
    }
    expectedLedNamePrefix = "port" + std::to_string(portNum) + "_led" + idStr;
  } else if (deviceName == "fan_led") {
    if (ledId == 0) {
      errors.emplace_back("Fan LED ID must be 1-based");
    }
    if (ledId < 0) {
      expectedLedNamePrefix = "fan_led";
    } else {
      expectedLedNamePrefix = "fan" + idStr + "_led";
    }
  } else if (deviceName == "psu_led") {
    if (ledId == 0) {
      errors.emplace_back("PSU LED ID must be 1-based");
    }
    if (ledId < 0) {
      expectedLedNamePrefix = "psu_led";
    } else {
      expectedLedNamePrefix = "psu" + idStr + "_led";
    }
  } else if (deviceName == "sys_led") {
    expectedLedNamePrefix = "sys_led";
  } else if (deviceName == "smb_led") {
    expectedLedNamePrefix = "smb_led";
  } else {
    errors.emplace_back(fmt::format("Unknown device name: {}", deviceName));
    return;
  }

  std::vector<std::string> ledFiles;
  for (const auto& entry :
       std::filesystem::directory_iterator("/sys/class/leds")) {
    ledFiles.emplace_back(entry.path().filename().string());
  }

  // Check for each expected color
  for (const auto& color : expectedColors) {
    std::string expectedLedName =
        fmt::format("{}:{}:status", expectedLedNamePrefix, color);
    bool found = false;
    for (const auto& file : ledFiles) {
      if (file == expectedLedName) {
        found = true;
        break;
      }
    }
    if (!found) {
      errors.emplace_back(
          fmt::format("Expected LED {} not found", expectedLedName));
    }
  }
  return;
}

void checkLedCorrectness(
    const std::string& ledPath,
    std::vector<std::string>& errorMessages) {
  // Check that the LED is a directory
  if (!std::filesystem::is_directory(ledPath)) {
    errorMessages.emplace_back(
        fmt::format("LED path {} is not a directory", ledPath));
    return;
  }

  // Check that max_brightness is not 0
  std::optional<std::string> maxBrightness =
      PlatformFsUtils().getStringFileContent(ledPath + "/max_brightness");
  if (!maxBrightness.has_value()) {
    errorMessages.emplace_back(
        fmt::format("Failed to read max_brightness for LED {}", ledPath));
  }
  if (maxBrightness.has_value() && *maxBrightness == "0") {
    errorMessages.emplace_back(
        fmt::format("max_brightness for LED {} is 0", ledPath));
  }

  // Check naming convention
  std::string ledFileName = std::filesystem::path(ledPath).filename().string();
  std::vector<std::string> ledNameParts;
  folly::split(':', ledFileName, ledNameParts);
  if (ledNameParts.size() != 3) {
    errorMessages.emplace_back(
        fmt::format("LED name {} does not match specification", ledFileName));
    return;
  }
  if (ledNameParts[2] != "status") {
    errorMessages.emplace_back(
        fmt::format("LED name {} does not end with status", ledFileName));
  }
  validateLedName(ledNameParts[0], errorMessages);
}

class LedTest : public BspTest {};

// For top-level LEDs created from userspace, e.g port_led
TEST_F(LedTest, LedsCreated) {
  std::vector<std::string> errorMessages;

  int id = 0;
  for (const auto& device : *GetRuntimeConfig().devices()) {
    for (const auto& auxDevice : *device.auxDevices()) {
      if (*auxDevice.type() != fbiob::AuxDeviceType::LED ||
          !auxDevice.ledData().has_value()) {
        continue;
      }

      try {
        id++;
        CdevUtils::createNewDevice(*device.pciInfo(), auxDevice, id);
        registerDeviceForCleanup(*device.pciInfo(), auxDevice, id);

        findExpectedLeds(
            *auxDevice.id()->deviceName(),
            *auxDevice.ledData()->ledId(),
            *auxDevice.ledData()->portNumber(),
            errorMessages);
      } catch (const std::exception& e) {
        errorMessages.emplace_back(fmt::format(
            "Exception during LED creation for {}: {}",
            *auxDevice.name(),
            e.what()));
      }
    }
  }

  if (!errorMessages.empty()) {
    FAIL() << "Found " << errorMessages.size() << " errors:\n"
           << folly::join("\n", errorMessages);
  }
}

// For devices which create LEDs
TEST_F(LedTest, DeviceLedsCreated) {
  std::vector<std::string> errorMessages;
  for (const auto& [pmName, testData] : *GetRuntimeConfig().testData()) {
    if (!testData.ledTestData().has_value()) {
      continue;
    }
    I2CAdapter adapter;
    I2CDevice i2cDevice;
    try {
      std::tie(adapter, i2cDevice) = getI2cAdapterAndDevice(pmName);
    } catch (const std::exception&) {
      errorMessages.emplace_back(
          fmt::format("Could not find I2C Device {}", pmName));
    }
    const auto busses = I2CUtils::createI2CAdapter(adapter, 0);
    registerAdapterForCleanup(adapter, 0);
    I2CUtils::createI2CDevice(
        i2cDevice, busses.at(*i2cDevice.channel()).busNum);
    std::string path =
        I2CUtils::getI2CDir(
            busses.at(*i2cDevice.channel()).busNum, *i2cDevice.address()) +
        "/leds";

    std::vector<std::string> ledFolders;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      ledFolders.emplace_back(entry.path().filename().string());
      checkLedCorrectness(entry.path().string(), errorMessages);
    }

    // For each unique name (the first part of the filename before ":"), check
    // that there is an LED for the expected colors
    std::map<std::string, std::set<std::string>> ledNameToColors;

    // Extract unique LED names and their colors
    for (const auto& ledFolder : ledFolders) {
      std::vector<std::string> parts;
      folly::split(':', ledFolder, parts);
      if (parts.size() >= 2) {
        ledNameToColors[parts[0]].insert(parts[1]);
      }
    }

    // Check that each unique LED name has all the expected colors
    for (const auto& [ledName, colors] : ledNameToColors) {
      for (const auto& expectedColor : expectedColors) {
        if (colors.find(expectedColor) == colors.end()) {
          errorMessages.emplace_back(fmt::format(
              "LED {} is missing expected color {}", ledName, expectedColor));
        }
      }
    }
  }

  if (!errorMessages.empty()) {
    FAIL() << "Found " << errorMessages.size() << " errors:\n"
           << folly::join("\n", errorMessages);
  }

  cleanupDevices();
}

} // namespace facebook::fboss::platform::bsp_tests::cpp
