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

#include <array>
#include <bitset>
#include <cstdint>
#include "fboss/platform/rackmon/rack_firmware_update/FirmwareUpdater.h"
#include "fboss/platform/rackmon/rack_firmware_update/HexFileParser.h"

namespace facebook::fboss::platform::rackmon {

/**
 * Status register for Delta PSU firmware update (32 bits)
 */
class StatusRegister {
 public:
  // Bit field names (from LSB to MSB)
  enum BitField {
    // Byte 0
    WAIT = 0,
    START_PROG_ACCEPTED,
    START_PROG_DECLINED,
    KEY_ACCEPTED,
    KEY_DECLINED,
    ERASE_BUSY,
    ERASE_DONE,
    ERASE_FAIL,
    // Byte 1
    ADD_ACCEPTED,
    ADD_DECLINED,
    SEND_DATA_BUSY,
    SEND_DATA_RDY,
    SEND_DATA_FAIL,
    VERIFY_CRC_BUSY,
    CRC_VERIFIED,
    CRC_WRONG,
    // Byte 2
    FW_IMAGE_ACCEPTED,
    FW_IMAGE_DECLINED,
    RESERVED_18,
    RESERVED_19,
    DEV_UPD_BUSY,
    DEV_UPD_RDY,
    DEV_UPD_FAIL,
    RESERVED_23,
    // Byte 3
    RESERVED_24,
    RESERVED_25,
    RESERVED_26,
    RESERVED_27,
    REV_FLAG,
    COMPATIBILITY_ERROR,
    SEQUENCE_ERROR,
    ERROR_DETECTED,
  };

  StatusRegister() : value_(0) {}
  explicit StatusRegister(uint32_t value) : value_(value) {}
  explicit StatusRegister(const std::array<uint8_t, 4>& bytes);

  bool operator[](BitField field) const {
    return (value_ & (1U << field)) != 0;
  }

  uint32_t getValue() const {
    return value_;
  }

  std::string toString() const;

 private:
  uint32_t value_;
};

/**
 * MEI (Modbus Encapsulated Interface) firmware updater for Delta ORv3 PSUs
 * Uses vendor-specific MEI commands for firmware update with security
 */
class MEIFirmwareUpdater : public FirmwareUpdater {
 public:
  /**
   * Constructor
   * @param client Rackmon client
   * @param deviceAddr Device address
   * @param securityKey 64-bit security key for challenge-response
   */
  MEIFirmwareUpdater(
      std::shared_ptr<RackmonClient> client,
      uint16_t deviceAddr,
      uint64_t securityKey);

  /**
   * Update firmware from Intel HEX file
   * @param firmwareFile Path to .hex or .dflash file
   */
  void updateFirmware(const std::string& firmwareFile) override;

  /**
   * Read firmware version from device
   */
  std::string readVersion() override;

 private:
  uint64_t securityKey_;
  static constexpr size_t CHUNK_SIZE = 128; // Data chunk size in bytes

  /**
   * Get status register from PSU
   */
  StatusRegister getStatusRegister();

  /**
   * Wait for status bit to be set or cleared
   * @param bitSet Bit to wait for being set (or empty)
   * @param bitCleared Bit to wait for being cleared (or empty)
   * @param timeout Timeout in seconds
   * @param delay Delay between checks in seconds
   */
  StatusRegister waitStatus(
      std::optional<StatusRegister::BitField> bitSet,
      std::optional<StatusRegister::BitField> bitCleared,
      double timeout = 100.0,
      double delay = 1.0);

  /**
   * Get challenge/seed from PSU
   */
  std::array<uint8_t, 4> getChallenge();

  /**
   * Send key response to PSU
   */
  void sendKey(const std::array<uint8_t, 4>& key);

  /**
   * Calculate key response from challenge using Delta algorithm
   */
  std::array<uint8_t, 4> calculateKey(const std::array<uint8_t, 4>& challenge);

  /**
   * Perform key handshake (challenge-response authentication)
   */
  void keyHandshake();

  /**
   * Erase flash memory
   */
  void eraseFlash();

  /**
   * Set write address for next data block
   */
  void setWriteAddress(uint32_t address);

  /**
   * Write data block (128 bytes)
   */
  void writeData(const std::vector<uint8_t>& data);

  /**
   * Verify flash CRC
   */
  void verifyFlash();

  /**
   * Activate firmware image
   */
  void activate();

  /**
   * Send entire firmware image
   */
  void sendImage(const HexFile& hexFile);

  /**
   * Build MEI command with device address
   */
  std::vector<uint8_t> buildMEICommand(const std::vector<uint8_t>& cmd) const;
};

} // namespace facebook::fboss::platform::rackmon
