/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace facebook::fboss::platform::rackmon {

/**
 * Retry a function up to 'times' attempts with delay between attempts
 * @param func Function to retry
 * @param times Maximum number of attempts
 * @param delay Delay between attempts in milliseconds
 * @param verbose Verbosity level (0=silent, 1=errors, 2=all attempts)
 * @return Result of successful function call
 * @throws Last exception if all attempts fail
 */
template <typename Func, typename... Args>
auto retry(
    Func&& func,
    int times,
    std::chrono::milliseconds delay = std::chrono::milliseconds(0),
    int verbose = 1) {
  int attempt = 0;
  while (attempt < times) {
    try {
      if (verbose >= 2) {
        std::cout << "Attempt " << attempt + 1 << " of " << times << std::endl;
      }
      return func();
    } catch (const std::exception& e) {
      if (verbose >= 1) {
        std::cerr << "Exception on attempt " << (attempt + 1) << " of " << times
                  << ": " << e.what() << std::endl;
      }
      attempt++;
      if (attempt < times) {
        std::this_thread::sleep_for(delay);
      } else {
        throw; // Re-throw the last exception
      }
    }
  }
  // Should not reach here
  return func();
}

/**
 * Print progress percentage to stdout if connected to a TTY
 * @param percentage Progress percentage (0-100)
 * @param message Optional message to display
 */
void printProgress(double percentage, const std::string& message = "");

/**
 * Convert bytes to hex string representation
 * @param data Vector of bytes
 * @return Hex string (e.g., "0a1b2c")
 */
std::string bytesToHex(const std::vector<uint8_t>& data);

/**
 * Convert string to integer, supporting hex (0x prefix) and octal (0 prefix)
 * @param str String to convert
 * @return Parsed integer value
 */
int autoInt(const std::string& str);

/**
 * Load binary firmware file as 16-bit words in big-endian format
 * File contains 2-byte words already in big-endian order
 * @param path Path to firmware file
 * @return Vector of 16-bit words
 */
std::vector<uint16_t> loadBinaryFirmware(const std::string& path);

/**
 * Get PMM address for a given device address
 * Used for PMM monitoring control
 * @param addr Device address
 * @return PMM address, or 0 if not found
 */
uint8_t getPmmAddr(uint16_t addr);

// Forward declaration
class RackmonClient;

/**
 * RAII guard to pause and resume monitoring
 * Automatically resumes on destruction (even on exception)
 */
class MonitoringGuard {
 public:
  /**
   * Constructor - pauses monitoring
   * @param pauseFunc Function to call to pause monitoring
   * @param resumeFunc Function to call to resume monitoring
   * @param client Optional RackmonClient pointer for PMM control
   * @param addr Optional device address for PMM control
   */
  MonitoringGuard(
      std::function<void()> pauseFunc,
      std::function<void()> resumeFunc,
      RackmonClient* client = nullptr,
      uint16_t addr = 0);

  // No copy
  MonitoringGuard(const MonitoringGuard&) = delete;
  MonitoringGuard& operator=(const MonitoringGuard&) = delete;

  // Destructor - resumes monitoring
  ~MonitoringGuard();

 private:
  std::function<void()> resumeFunc_;
  RackmonClient* client_;
  uint16_t addr_;
  void controlPmmMonitoring(uint16_t addr, uint16_t pause);
};

// Device listing function
void listAndPrintDevices();

} // namespace facebook::fboss::platform::rackmon
