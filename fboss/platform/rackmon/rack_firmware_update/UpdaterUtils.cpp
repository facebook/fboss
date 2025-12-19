/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/rackmon/rack_firmware_update/UpdaterUtils.h"
#include "fboss/platform/rackmon/rack_firmware_update/RackmonClient.h"
#include "fboss/platform/rackmon/rack_firmware_update/UpdaterExceptions.h"

#include <fmt/format.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <unordered_map>

#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "fboss/platform/rackmon/if/gen-cpp2/RackmonCtrl.h"

namespace facebook::fboss::platform::rackmon {

void printProgress(double percentage, const std::string& message) {
  // Only print if stdout is a TTY
  if (isatty(fileno(stdout))) {
    if (percentage < 100.0) {
      std::cout << fmt::format("\r[{:.2f}%] {}", percentage, message)
                << std::flush;
    } else {
      std::cout << fmt::format("\r[{:.2f}%] {}\n", percentage, message);
    }
  }
}

std::string bytesToHex(const std::vector<uint8_t>& data) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (uint8_t byte : data) {
    oss << std::setw(2) << static_cast<int>(byte);
  }
  return oss.str();
}

int autoInt(const std::string& str) {
  // Support 0x (hex), 0 (octal), and decimal
  size_t pos = 0;
  int value = std::stoi(str, &pos, 0); // Base 0 auto-detects
  return value;
}

std::vector<uint16_t> loadBinaryFirmware(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw FileException("Failed to open firmware file: " + path);
  }

  std::vector<uint16_t> words;
  uint8_t buf[2];

  while (file.read(reinterpret_cast<char*>(buf), 2)) {
    // File already has bytes in 2-byte words in big-endian order
    uint16_t word = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
    words.push_back(word);
  }

  // Check if we read partial word at the end
  if (file.gcount() > 0) {
    throw FileException(
        "Firmware file has odd number of bytes: " +
        std::to_string(file.gcount()) + " trailing bytes");
  }

  return words;
}

uint8_t getPmmAddr(uint16_t addr) {
  // PMM address mapping
  static const std::
      unordered_map<uint8_t, std::vector<std::pair<uint8_t, uint8_t>>>
          pmmAddrs = {
              {0x10, {{0x30, 0x35}}}, {0x11, {{0x3A, 0x3F}}},
              {0x12, {{0x5A, 0x5F}}}, {0x13, {{0x6A, 0x6F}}},
              {0x14, {{0x70, 0x75}}}, {0x15, {{0x7A, 0x7F}}},
              {0x16, {{0x80, 0x85}}}, {0x17, {{0x40, 0x45}}},
              {0x20, {{0x90, 0x95}}}, {0x21, {{0x9A, 0x9F}}},
              {0x22, {{0xAA, 0xAF}}}, {0x23, {{0xBA, 0xBF}}},
              {0x24, {{0xD0, 0xD5}}}, {0x25, {{0xDA, 0xDF}}},
              {0x26, {{0xA0, 0xA5}}}, {0x27, {{0xB0, 0xB5}}},
              {0xF0, {{0x36, 0x38}}}, {0xF1, {{0x46, 0x48}}},
              {0xF2, {{0x56, 0x58}}}, {0xF3, {{0x66, 0x68}}},
              {0xF4, {{0x76, 0x78}}}, {0xF5, {{0x86, 0x88}}},
              {0xF6, {{0x96, 0x98}}}, {0xF7, {{0xA6, 0xA8}}},
          };

  for (const auto& [pmmAddr, ranges] : pmmAddrs) {
    for (const auto& [start, end] : ranges) {
      if (addr >= start && addr <= end) {
        return pmmAddr;
      }
    }
  }
  return 0; // Not found
}

MonitoringGuard::MonitoringGuard(
    std::function<void()> pauseFunc,
    std::function<void()> resumeFunc,
    RackmonClient* client,
    uint16_t addr)
    : resumeFunc_(std::move(resumeFunc)), client_(client), addr_(addr) {
  std::cout << "Pausing monitoring..." << std::endl;
  pauseFunc();

  if (client_ != nullptr && addr_ != 0) {
    controlPmmMonitoring(addr_, 0x1);
  }
}

MonitoringGuard::~MonitoringGuard() {
  try {
    if (client_ != nullptr && addr_ != 0) {
      controlPmmMonitoring(addr_, 0x0);
    }

    std::cout << "Resuming monitoring..." << std::endl;
    resumeFunc_();

    // Sleep to allow monitoring to stabilize
    std::this_thread::sleep_for(std::chrono::seconds(10));
  } catch (const std::exception& e) {
    std::cerr << "Error during monitoring resume: " << e.what() << std::endl;
  }
}

void MonitoringGuard::controlPmmMonitoring(uint16_t addr, uint16_t pause) {
  // PMM pause register address (from Python: pmm_pause_reg = 0x7B)
  constexpr uint16_t PMM_PAUSE_REG = 0x7B;

  // Get PMM address for this device
  uint8_t pmm_addr = getPmmAddr(addr);

  if (pmm_addr == 0) {
    // No PMM for this device
    return;
  }

  try {
    std::cout << fmt::format(
                     "Setting PMM {} Monitoring pause: {}", pmm_addr, pause)
              << std::endl;

    // Write to PMM device's pause register
    // This matches Python: rmd.write(pmm_addr, pmm_pause_reg, pause)
    client_->writeSingleRegister(pmm_addr, PMM_PAUSE_REG, pause);

  } catch (const std::exception& e) {
    std::cerr << fmt::format(
                     "WARNING: Control PMM {} failed: {}",
                     PMM_PAUSE_REG,
                     e.what())
              << std::endl;
  }
}

void listAndPrintDevices() {
  try {
    auto evb = std::make_shared<folly::EventBase>();
    auto socket = folly::AsyncSocket::newSocket(evb.get(), "::1", 5973);
    auto channel =
        apache::thrift::RocketClientChannel::newChannel(std::move(socket));
    channel->setTimeout(5000);
    auto client =
        std::make_shared<apache::thrift::Client<::rackmonsvc::RackmonCtrl>>(
            std::move(channel));

    std::vector<::rackmonsvc::ModbusDeviceInfo> devices;
    client->sync_listModbusDevices(devices);

    std::cout << "Discovered Modbus Devices:" << std::endl;
    std::cout << "============================================================"
              << std::endl;

    for (const auto& dev : devices) {
      std::cout << fmt::format(
                       "Address: {:3d} (0x{:02x})  Type: {}  Mode: {}",
                       *dev.devAddress(),
                       *dev.devAddress(),
                       static_cast<int>(*dev.deviceType()),
                       *dev.mode() == ::rackmonsvc::ModbusDeviceMode::ACTIVE
                           ? "ACTIVE"
                           : "DORMANT")
                << std::endl;
    }

    std::cout << "============================================================"
              << std::endl;
    std::cout << fmt::format("Total: {} device(s)", devices.size())
              << std::endl;

  } catch (const std::exception& e) {
    std::cerr << "Error listing devices: " << e.what() << std::endl;
    throw;
  }
}

} // namespace facebook::fboss::platform::rackmon
