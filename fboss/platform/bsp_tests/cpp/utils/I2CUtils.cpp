// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/platform/bsp_tests/cpp/utils/I2CUtils.h"
#include "fboss/platform/platform_manager/I2cAddr.h"
#include "fboss/platform/platform_manager/I2cExplorer.h"

#include <re2/re2.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <folly/Format.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/bsp_tests/cpp/utils/CdevUtils.h"
#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

// Returns map {channel_num -> I2CBus}
// Creates an adapter AND its parents e.g. a mux adapter that
// depends on a parent PCI adapter
std::map<int, I2CBus> I2CUtils::createI2CAdapter(
    // const PciDevice& pciDevice,
    const I2CAdapter& adapter,
    int32_t id) {
  if (*adapter.isCpuAdapter()) {
    // No need to create, just return the bus
    auto existingBuses = findI2CBuses();
    for (const auto& bus : existingBuses) {
      if (bus.name == *adapter.busName()) {
        return {{0, bus}};
      }
    }
    throw std::runtime_error(
        fmt::format("CPU adapter {} not found", *adapter.busName()));
  } else if (adapter.pciAdapterInfo().has_value()) {
    // Ensure the adapter has I2C info
    if (!adapter.pciAdapterInfo()->auxData()->i2cData()) {
      throw std::runtime_error("I2C adapter does not have I2C info");
    }
    int numBuses =
        *adapter.pciAdapterInfo()->auxData()->i2cData()->numChannels();
    PciDeviceInfo pci;
    pci.vendorId() = *adapter.pciAdapterInfo()->pciInfo()->vendorId();
    pci.deviceId() = *adapter.pciAdapterInfo()->pciInfo()->deviceId();
    pci.subSystemVendorId() =
        *adapter.pciAdapterInfo()->pciInfo()->subSystemVendorId();
    pci.subSystemDeviceId() =
        *adapter.pciAdapterInfo()->pciInfo()->subSystemDeviceId();
    CdevUtils::createNewDevice(pci, *adapter.pciAdapterInfo()->auxData(), id);

    // find new buses from pci device path
    std::string pciDir = CdevUtils::makePciPath(pci);
    std::string i2cDir = findI2cDir(pciDir, adapter, id);
    std::map<int, I2CBus> newBuses;
    for (int i = 0; i < numBuses; i++) {
      std::string channelFile = fmt::format("{}/channel-{}", i2cDir, i);
      if (!std::filesystem::exists(channelFile) ||
          !std::filesystem::is_symlink(channelFile)) {
        throw std::runtime_error(
            fmt::format("{} does not exist or is not symlink", channelFile));
      }
      I2CBus bus;
      bus.busNum = platform_manager::I2cExplorer().extractBusNumFromPath(
          std::filesystem::read_symlink(channelFile));
      bus.name = getBusNameFromNum(bus.busNum);
      newBuses.at(i) = bus;
    }
    return newBuses;
  } else if (adapter.muxAdapterInfo().has_value()) {
    // create parent adapter
    int parentId = id + 1;
    auto newBuses =
        createI2CAdapter(*adapter.muxAdapterInfo()->parentAdapter(), parentId);

    I2CDevice device;
    device.pmName() = *adapter.pmName();
    device.channel() = *adapter.muxAdapterInfo()->parentAdapterChannel();
    device.deviceName() = *adapter.muxAdapterInfo()->deviceName();
    device.address() = *adapter.muxAdapterInfo()->address();
    int busNum = newBuses.at(*device.channel()).busNum;
    createI2CDevice(device, busNum);

    std::map<uint16_t, uint16_t> createdBuses =
        platform_manager::I2cExplorer().getMuxChannelI2CBuses(
            busNum, platform_manager::I2cAddr(*device.address()));
    std::map<int, I2CBus> newMuxBuses;
    for (const auto& [channel, bus] : createdBuses) {
      I2CBus newBus;
      newBus.busNum = bus;
      newMuxBuses.at(channel) = newBus;
    }
    return newMuxBuses;
  } else {
    throw std::runtime_error("Unknown adapter type");
  }
}

std::string I2CUtils::getBusNameFromNum(int busNum) {
  auto busName = PlatformFsUtils().getStringFileContent(
      fmt::format("/sys/bus/i2c/devices/i2c-{}", busNum));
  if (busName->empty()) {
    throw std::runtime_error(
        fmt::format("Failed to open bus name file for bus {}", busNum));
  }
  return *busName;
}

std::string I2CUtils::findI2cDir(
    const std::string& pciDir,
    const I2CAdapter& adapter,
    int id) {
  std::string expectedEnding =
      fmt::format(".{}.{}", *adapter.pciAdapterInfo()->auxData()->name(), id);
  for (const auto& dirEntry : std::filesystem::directory_iterator(pciDir)) {
    if ((dirEntry.path().string().ends_with(expectedEnding))) {
      return dirEntry.path().string();
    }
  }
  return "";
}

std::set<I2CBus> I2CUtils::findI2CBuses() {
  auto [exitCode, output] = PlatformUtils().execCommand("i2cdetect -l");

  if (exitCode != 0) {
    throw std::runtime_error(fmt::format(
        "Command failed with exit code {}: i2cdetect -l", exitCode));
  }

  std::set<I2CBus> buses;
  std::istringstream iss(output);
  std::string line;
  while (std::getline(iss, line)) {
    if (!line.empty()) {
      buses.insert(parseI2CDetectLine(line));
    }
  }

  return buses;
}

I2CBus I2CUtils::parseI2CDetectLine(const std::string& line) {
  std::vector<std::string> parts;
  std::istringstream iss(line);
  std::string part;

  while (std::getline(iss, part, '\t')) {
    parts.push_back(part);
  }

  if (parts.size() < 4) {
    throw std::runtime_error(
        fmt::format("Invalid i2cdetect line format: {}", line));
  }

  // Parse bus number from the first part (e.g., "i2c-1" -> 1)
  std::string busNumStr = parts[0].substr(parts[0].find('-') + 1);
  int busNum = std::stoi(busNumStr);

  return {busNum, parts[1], parts[2], parts[3]};
}

bool I2CUtils::detectI2CDevice(int bus, const std::string& hexAddr) {
  // Try different options for i2cdetect
  std::vector<std::vector<std::string>> options = {
      {"-y"}, {"-y", "-r"}, {"-y", "-q"}};

  for (const auto& opt : options) {
    std::vector<std::string> cmd = {"i2cdetect"};
    cmd.insert(cmd.end(), opt.begin(), opt.end());
    cmd.push_back(std::to_string(bus));
    cmd.push_back(hexAddr);
    cmd.push_back(hexAddr);

    try {
      auto [exitCode, output] =
          PlatformUtils().execCommand(folly::join(" ", cmd));
      if (exitCode != 0) {
        continue; // Try next option if command failed
      }

      std::istringstream iss(output);
      std::string line;

      while (std::getline(iss, line)) {
        if (!line.empty()) {
          std::istringstream lineStream(line);
          std::string part;
          while (lineStream >> part) {
            // Remove "0x" prefix from hexAddr for comparison
            std::string addrSuffix = hexAddr.substr(2);
            if (part == addrSuffix) {
              return true;
            }
          }
        }
      }
    } catch (const std::exception& e) {
      XLOG(WARN) << "Error running i2cdetect: " << e.what();
      // Continue to the next option
    }
  }

  return false;
}

std::vector<std::string> I2CUtils::parseI2CDumpData(const std::string& data) {
  std::vector<std::string> dataBytes;
  std::istringstream iss(data);
  std::string line;

  // Skip the first line (header)
  std::getline(iss, line);

  // Process remaining lines
  while (std::getline(iss, line)) {
    std::istringstream lineStream(line);
    std::string part;

    // Skip the first column (address)
    lineStream >> part;

    // Process remaining parts
    while (lineStream >> part) {
      // Check if the part contains only hex characters
      bool isHex = true;
      for (char c : part) {
        if (!std::isxdigit(c)) {
          isHex = false;
          break;
        }
      }

      if (isHex) {
        dataBytes.push_back(part);
      }
    }
  }

  return dataBytes;
}

std::string I2CUtils::i2cDump(
    const int bus,
    const std::string& addr,
    const std::string& start,
    const std::string& end) {
  auto [exitCode, output] = PlatformUtils().execCommand(fmt::format(
      "i2cdump -y -r {}-{} {} {}", start, end, std::to_string(bus), addr));

  if (exitCode != 0) {
    throw std::runtime_error(fmt::format(
        "i2cdump command failed with exit code {}: bus={}, addr={}, start={}, end={}",
        exitCode,
        bus,
        addr,
        start,
        end));
  }

  return output;
}

std::string I2CUtils::i2cGet(
    const int bus,
    const std::string& addr,
    const std::string& reg) {
  auto [exitCode, output] = PlatformUtils().execCommand(
      fmt::format("i2cget -y {} {} {}", std::to_string(bus), addr, reg));

  if (exitCode != 0) {
    throw std::runtime_error(fmt::format(
        "i2cget command failed with exit code {}: bus={}, addr={}, reg={}",
        exitCode,
        bus,
        addr,
        reg));
  }

  return output.erase(output.find_last_not_of("\n\r") + 1);
}

void I2CUtils::i2cSet(
    const int bus,
    const std::string& addr,
    const std::string& reg,
    const std::string& data) {
  auto [exitCode, output] = PlatformUtils().execCommand(fmt::format(
      "i2cset -y {} {} {} {}", std::to_string(bus), addr, reg, data));

  if (exitCode != 0) {
    throw std::runtime_error(fmt::format(
        "i2cset command failed with exit code {}: bus={}, addr={}, reg={}, data={}",
        exitCode,
        bus,
        addr,
        reg,
        data));
  }
}

bool I2CUtils::createI2CDevice(const I2CDevice& device, int bus) {
  try {
    // Create the command to create the I2C device
    std::string cmd = fmt::format(
        "echo {} {} > /sys/bus/i2c/devices/i2c-{}/new_device",
        *device.deviceName(),
        *device.address(),
        bus);

    auto [exitCode, output] = PlatformUtils().execCommand(cmd);
    if (exitCode != 0) {
      XLOG(ERR) << "Failed to create I2C device: " << cmd
                << ", error code: " << exitCode;
      return false;
    }

    // Verify the device exists
    std::string devDir = "/sys/bus/i2c/devices";
    std::string busPath = fmt::format("{}/i2c-{}", devDir, bus);

    // Check if the bus directory exists
    if (!std::filesystem::exists(busPath)) {
      XLOG(ERR) << "Device " << *device.address() << " on bus " << bus
                << " not found: bus directory does not exist";
      return false;
    }

    // Get the address suffix (remove "0x" prefix and pad with zeros)
    std::string addrSuffix = (*device.address()).substr(2);
    while (addrSuffix.length() < 4) {
      addrSuffix = "0" + addrSuffix;
    }

    // Check if the device directory exists
    std::string devicePath = fmt::format("{}/{}-{}", devDir, bus, addrSuffix);
    if (!std::filesystem::exists(devicePath)) {
      XLOG(ERR) << "Device " << *device.address() << " on bus " << bus
                << " not found: device directory does not exist";
      return false;
    }

    // Read the name file and verify it is correct
    std::string namePath = fmt::format("{}/name", devicePath);
    std::ifstream nameFile(namePath);
    if (!nameFile.is_open()) {
      XLOG(ERR) << "Failed to open name file for device " << *device.address()
                << " on bus " << bus;
      return false;
    }

    std::string name;
    std::getline(nameFile, name);
    name.erase(name.find_last_not_of("\n\r") + 1); // Trim trailing newlines

    if (name != *device.deviceName()) {
      XLOG(ERR) << "Device " << *device.address() << " on bus " << bus
                << ": wrong device name: expected '" << *device.deviceName()
                << "' got '" << name << "'";
      return false;
    }

    return true;
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to create device " << *device.deviceName()
              << ", error: " << e.what();
    return false;
  }
}

} // namespace facebook::fboss::platform::bsp_tests::cpp
