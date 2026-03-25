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

#include <stdexcept>
#include <string>

namespace facebook::fboss::platform::rackmon {

/**
 * Base exception class for firmware update operations
 */
class UpdaterException : public std::runtime_error {
 public:
  explicit UpdaterException(const std::string& msg) : std::runtime_error(msg) {}
};

/**
 * Exception thrown when a Modbus operation times out
 */
class ModbusTimeoutException : public UpdaterException {
 public:
  ModbusTimeoutException() : UpdaterException("Modbus operation timed out") {}
  explicit ModbusTimeoutException(const std::string& msg)
      : UpdaterException(msg) {}
};

/**
 * Exception thrown when a CRC check fails
 */
class ModbusCRCException : public UpdaterException {
 public:
  ModbusCRCException() : UpdaterException("CRC check failed") {}
  explicit ModbusCRCException(const std::string& msg) : UpdaterException(msg) {}
};

/**
 * Exception thrown when firmware status is unexpected
 */
class FirmwareStatusException : public UpdaterException {
 public:
  FirmwareStatusException(uint16_t actual, uint16_t expected)
      : UpdaterException(
            "Bad firmware state: 0x" + toHex(actual) + " expected: 0x" +
            toHex(expected)),
        actual_(actual),
        expected_(expected) {}

  uint16_t getActual() const {
    return actual_;
  }
  uint16_t getExpected() const {
    return expected_;
  }

 private:
  uint16_t actual_;
  uint16_t expected_;

  static std::string toHex(uint16_t val);
};

/**
 * Exception thrown when an MEI response is malformed
 */
class BadMEIResponseException : public UpdaterException {
 public:
  BadMEIResponseException() : UpdaterException("Bad MEI response") {}
  explicit BadMEIResponseException(const std::string& msg)
      : UpdaterException(msg) {}
};

/**
 * Exception thrown when file operations fail
 */
class FileException : public UpdaterException {
 public:
  explicit FileException(const std::string& msg) : UpdaterException(msg) {}
};

/**
 * Exception thrown when firmware verification fails
 */
class VerificationException : public UpdaterException {
 public:
  explicit VerificationException(const std::string& msg)
      : UpdaterException(msg) {}
};

} // namespace facebook::fboss::platform::rackmon
