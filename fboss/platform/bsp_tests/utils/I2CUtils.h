// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <filesystem>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "fboss/platform/bsp_tests/gen-cpp2/bsp_tests_runtime_config_types.h"

namespace facebook::fboss::platform::bsp_tests {

// Structure to represent an I2C bus, similar to the Python I2CBus NamedTuple
struct I2CBus {
  int busNum;
  std::string type;
  std::string name;
  std::string description;

  // Define equality operator for use in sets
  bool operator==(const I2CBus& other) const {
    return busNum == other.busNum && type == other.type && name == other.name &&
        description == other.description;
  }

  // Define less than operator for use in sets
  bool operator<(const I2CBus& other) const {
    if (busNum != other.busNum) {
      return busNum < other.busNum;
    }
    if (type != other.type) {
      return type < other.type;
    }
    if (name != other.name) {
      return name < other.name;
    }
    return description < other.description;
  }
};

class I2CUtils {
 public:
  // Create an I2C adapter and return the set of new buses and the base bus
  // number
  static std::map<int, I2CBus> createI2CAdapter(
      const I2CAdapter& adapter,
      int32_t id = 1);

  // Find all I2C buses in the system
  static std::set<I2CBus> findI2CBuses();

  // Parse a line from i2cdetect -l output
  static I2CBus parseI2CDetectLine(const std::string& line);

  // Detect if an I2C device exists at the specified bus and address
  static bool detectI2CDevice(int bus, const std::string& hexAddr);

  // Parse i2cdump data and return a list of byte values
  static std::vector<std::string> parseI2CDumpData(const std::string& data);

  // Dump data from an I2C device
  static std::string i2cDump(
      const int bus,
      const std::string& addr,
      const std::string& start,
      const std::string& end);

  // Get a value from an I2C device
  static std::string
  i2cGet(const int bus, const std::string& addr, const std::string& reg);

  // Set a value to an I2C device
  static void i2cSet(
      const int bus,
      const std::string& addr,
      const std::string& reg,
      const std::string& data);

  // Create an I2C device
  static bool createI2CDevice(const I2CDevice& device, int bus);

  static std::string
  findI2cDir(const std::string& pciDir, const I2CAdapter& adapter, int id);

  static std::string getI2CDir(int busNum, const std::string& address);

  static std::string findPciDirectory(PciDeviceInfo pci);

  static std::string getBusNameFromNum(int busNum);
};
} // namespace facebook::fboss::platform::bsp_tests
