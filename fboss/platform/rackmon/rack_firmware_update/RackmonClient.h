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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "fboss/platform/rackmon/if/gen-cpp2/rackmonsvc_types.h"

// Forward declarations for header-only types
namespace folly {
class EventBase;
} // namespace folly

namespace apache {
namespace thrift {
class RocketClientChannel;
template <typename T>
class Client;
} // namespace thrift
} // namespace apache

namespace rackmonsvc {
class RackmonCtrl;
} // namespace rackmonsvc

namespace facebook::fboss::platform::rackmon {

/**
 * C++ client wrapper for Rackmon thrift service
 * Provides firmware update operations via thrift
 */
class RackmonClient {
 public:
  /**
   * Constructor - Creates persistent connection to rackmon service
   * @param host Rackmon service host (default: ::1 for IPv6 localhost)
   * @param port Rackmon service port (default: 5973)
   */
  explicit RackmonClient(const std::string& host = "::1", uint16_t port = 5973);

  /**
   * Destructor - Cleans up persistent connection
   */
  ~RackmonClient();

  /**
   * Read holding registers from a Modbus device
   * @param devAddress Device address
   * @param regAddress Register address
   * @param numRegisters Number of registers to read
   * @param timeout Timeout in milliseconds (0 = default)
   * @return Vector of register values
   * @throws UpdaterException on error
   */
  std::vector<uint16_t> readHoldingRegisters(
      uint16_t devAddress,
      uint16_t regAddress,
      uint16_t numRegisters,
      uint32_t timeout = 0);

  /**
   * Write a single register to a Modbus device
   * @param devAddress Device address
   * @param regAddress Register address
   * @param value Value to write
   * @param timeout Timeout in milliseconds (0 = default)
   * @throws UpdaterException on error
   */
  void writeSingleRegister(
      uint16_t devAddress,
      uint16_t regAddress,
      uint16_t value,
      uint32_t timeout = 0);

  /**
   * Write multiple registers to a Modbus device
   * @param devAddress Device address
   * @param regAddress Starting register address
   * @param values Vector of values to write
   * @param timeout Timeout in milliseconds (0 = default)
   * @throws UpdaterException on error
   */
  void presetMultipleRegisters(
      uint16_t devAddress,
      uint16_t regAddress,
      const std::vector<uint16_t>& values,
      uint32_t timeout = 0);

  /**
   * Send a raw Modbus command
   * @param cmd Raw command bytes (without CRC)
   * @param expectedLength Expected response length (including CRC)
   * @param timeout Timeout in milliseconds (0 = default)
   * @param uniqueDevAddr Optional unique device address
   * @return Response bytes (excluding CRC)
   * @throws UpdaterException on error
   */
  std::vector<uint8_t> sendRawCommand(
      const std::vector<uint8_t>& cmd,
      uint32_t expectedLength,
      uint32_t timeout = 0,
      uint16_t uniqueDevAddr = 0);

  /**
   * Pause rackmon monitoring
   * @throws UpdaterException on error
   */
  void pauseMonitoring();

  /**
   * Resume rackmon monitoring
   * @throws UpdaterException on error
   */
  void resumeMonitoring();

 private:
  std::string host_;
  uint16_t port_;

  // Persistent connection members
  std::unique_ptr<folly::EventBase> evb_;
  std::shared_ptr<apache::thrift::RocketClientChannel> channel_;
  std::unique_ptr<apache::thrift::Client<::rackmonsvc::RackmonCtrl>> client_;

  /**
   * Ensure connection is established and valid
   * Creates new connection if needed
   */
  void ensureConnected();

  /**
   * Convert thrift status code to exception
   * @param status Thrift status code
   * @throws UpdaterException based on status code
   */
  void checkStatus(::rackmonsvc::RackmonStatusCode status);
};

} // namespace facebook::fboss::platform::rackmon
