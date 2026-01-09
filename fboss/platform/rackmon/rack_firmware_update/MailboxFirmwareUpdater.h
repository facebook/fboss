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
#include <set>
#include <string>
#include <vector>
#include "fboss/platform/rackmon/rack_firmware_update/FirmwareUpdater.h"

namespace facebook::fboss::platform::rackmon {

// Hardware workarounds
enum class Workaround {
  WRITE_BLOCK_CRC_EXPECTED, // Expect CRC errors on write
  FORCE_EXIT_BOOT_MODE, // Force exit boot mode on start
  FORCE_CLEAR_VERIFY, // Clear verify register
};

/**
 * Vendor-specific parameters for mailbox firmware updates
 */
struct VendorParams {
  std::string name;
  uint16_t blockSize; // Block size in bytes
  uint16_t bootMode; // Boot mode value (e.g., 0xAA55)
  bool blockWait; // Wait for block acknowledgment
  std::string versionReg; // Register name for version
  uint16_t versionRegAddr; // Register address for version
  uint16_t versionRegLen; // Number of registers for version (usually 4)
  std::set<Workaround> workarounds; // Hardware workarounds

  // Predefined vendor configurations
  static VendorParams panasonic();
  static VendorParams delta();
  static VendorParams deltaPowerTether();
  static VendorParams hprPanasonic();
  static VendorParams hprPmmPanasonic();
  static VendorParams hprPmmDelta();
  static VendorParams hprPmmAei();

  // Get vendor params by name
  static VendorParams getByName(const std::string& vendor);
};

/**
 * Firmware status values for mailbox protocol
 */
enum class FirmwareStatus : uint16_t {
  NORMAL_OPERATION = 0x0000,
  ENTERED_BOOT_MODE = 0x0001,
  FIRMWARE_PACKET_CORRECT = 0x0006,
  WAIT_STATUS = 0x0018,
  FIRMWARE_UPGRADE_FAILED = 0x0055,
  FIRMWARE_UPGRADE_SUCCESS = 0x00AA,
};

/**
 * Mailbox-based firmware updater for PSU/BBU devices
 * Uses register-based protocol (Panasonic, Delta, HPR variants)
 */
class MailboxFirmwareUpdater : public FirmwareUpdater {
 public:
  /**
   * Constructor
   * @param client Rackmon client
   * @param deviceAddr Device address
   * @param params Vendor parameters
   */
  MailboxFirmwareUpdater(
      std::shared_ptr<RackmonClient> client,
      uint16_t deviceAddr,
      const VendorParams& params);

  /**
   * Update firmware from binary file
   * @param firmwareFile Path to .bin firmware file
   */
  void updateFirmware(const std::string& firmwareFile) override;

  /**
   * Read firmware version from device
   */
  std::string readVersion() override;

 private:
  VendorParams params_;
  std::set<Workaround> activeWorkarounds_;

  // Register addresses
  static constexpr uint16_t REG_UNLOCK = 0x300;
  static constexpr uint16_t REG_ENTER_BOOT = 0x301;
  static constexpr uint16_t REG_STATUS = 0x302;
  static constexpr uint16_t REG_VERIFY = 0x303;
  static constexpr uint16_t REG_EXIT_BOOT = 0x304;
  static constexpr uint16_t REG_WRITE_BLOCK = 0x310;

  /**
   * Unlock firmware engineering mode
   */
  void unlockFirmware();

  /**
   * Enter boot mode
   */
  void enterBootMode();

  /**
   * Exit boot mode
   */
  void exitBootMode();

  /**
   * Get firmware status
   */
  uint16_t getFirmwareStatus();

  /**
   * Verify firmware status matches expected
   */
  void verifyFirmwareStatus(uint16_t expected);

  /**
   * Verify firmware status with retry
   * @param expected Expected status value
   * @param retries Number of retry attempts (default 10 for verification)
   */
  void verifyFirmwareStatusWithRetry(uint16_t expected, int retries = 10);

  /**
   * Write firmware block
   */
  void writeBlock(const std::vector<uint16_t>& data);

  /**
   * Wait for write block completion
   */
  void waitWriteBlock();

  /**
   * Transfer firmware image
   */
  void transferImage(const std::vector<uint16_t>& image);

  /**
   * Verify firmware
   */
  void verifyFirmware();

  /**
   * Workaround: Force exit boot mode if stuck
   */
  void workaroundForceExitBootMode();

  /**
   * RAII class to ensure boot mode is always exited
   */
  class BootModeGuard {
   public:
    BootModeGuard(MailboxFirmwareUpdater* updater);
    ~BootModeGuard();

    BootModeGuard(const BootModeGuard&) = delete;
    BootModeGuard& operator=(const BootModeGuard&) = delete;

   private:
    MailboxFirmwareUpdater* updater_;
  };
};

} // namespace facebook::fboss::platform::rackmon
