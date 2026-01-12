/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/rackmon/rack_firmware_update/MailboxFirmwareUpdater.h"
#include "fboss/platform/rackmon/rack_firmware_update/UpdaterExceptions.h"
#include "fboss/platform/rackmon/rack_firmware_update/UpdaterUtils.h"

#include <fmt/format.h>
#include <iostream>

namespace facebook::fboss::platform::rackmon {

// Vendor parameter definitions
VendorParams VendorParams::panasonic() {
  return {
      "panasonic",
      96, // blockSize
      0xAA55, // bootMode
      false, // blockWait
      "FW_Revision",
      56, // versionRegAddr (register 56)
      4, // versionRegLen (4 registers)
      {} // no workarounds
  };
}

VendorParams VendorParams::delta() {
  return {
      "delta",
      64, // blockSize
      0xA5A5, // bootMode
      true, // blockWait
      "FW_Revision",
      56, // versionRegAddr
      4, // versionRegLen
      {} // no workarounds
  };
}

VendorParams VendorParams::deltaPowerTether() {
  return {
      "delta_power_tether",
      16, // blockSize
      0xAA55, // bootMode
      true, // blockWait
      "PSU_FW_Revision",
      48, // versionRegAddr (register 48 for PSU)
      4, // versionRegLen
      {Workaround::WRITE_BLOCK_CRC_EXPECTED}};
}

VendorParams VendorParams::hprPanasonic() {
  return {
      "hpr_panasonic",
      96, // blockSize
      0xAA55, // bootMode
      false, // blockWait
      "FW_Revision",
      56, // versionRegAddr
      4, // versionRegLen
      {Workaround::FORCE_EXIT_BOOT_MODE, Workaround::FORCE_CLEAR_VERIFY}};
}

VendorParams VendorParams::hprPmmPanasonic() {
  return {
      "hpr_pmm_panasonic",
      68, // blockSize
      0xAA55, // bootMode
      true, // blockWait
      "PMM_FW_Revision",
      56, // versionRegAddr
      4, // versionRegLen
      {} // no workarounds
  };
}

VendorParams VendorParams::hprPmmDelta() {
  return {
      "hpr_pmm_delta",
      68, // blockSize
      0xAA55, // bootMode
      true, // blockWait
      "PMM_FW_Revision",
      56, // versionRegAddr
      4, // versionRegLen
      {Workaround::FORCE_EXIT_BOOT_MODE}};
}

VendorParams VendorParams::hprPmmAei() {
  return {
      "hpr_pmm_aei",
      68, // blockSize
      0xAA55, // bootMode
      true, // blockWait
      "PMM_FW_Revision",
      56, // versionRegAddr
      4, // versionRegLen
      {Workaround::FORCE_EXIT_BOOT_MODE}};
}

VendorParams VendorParams::getByName(const std::string& vendor) {
  if (vendor == "panasonic") {
    return panasonic();
  }
  if (vendor == "delta") {
    return delta();
  }
  if (vendor == "delta_power_tether") {
    return deltaPowerTether();
  }
  if (vendor == "hpr_panasonic") {
    return hprPanasonic();
  }
  if (vendor == "hpr_pmm_panasonic") {
    return hprPmmPanasonic();
  }
  if (vendor == "hpr_pmm_delta") {
    return hprPmmDelta();
  }
  if (vendor == "hpr_pmm_aei") {
    return hprPmmAei();
  }

  throw UpdaterException("Unknown vendor: " + vendor);
}

// MailboxFirmwareUpdater implementation
MailboxFirmwareUpdater::MailboxFirmwareUpdater(
    std::shared_ptr<RackmonClient> client,
    uint16_t deviceAddr,
    const VendorParams& params)
    : FirmwareUpdater(std::move(client), deviceAddr),
      params_(params),
      activeWorkarounds_(params.workarounds) {}

void MailboxFirmwareUpdater::unlockFirmware() {
  retry(
      [this]() {
        client_->writeSingleRegister(deviceAddr_, REG_UNLOCK, 0x55AA, 1000);
      },
      5,
      std::chrono::milliseconds(500));
}

void MailboxFirmwareUpdater::enterBootMode() {
  std::cout << "Entering Boot Mode..." << std::endl;

  retry(
      [this]() {
        client_->writeSingleRegister(deviceAddr_, REG_ENTER_BOOT, 0xAA55, 5000);
        sleepSeconds(15); // Allow time for device to erase and prepare
        verifyFirmwareStatus(
            static_cast<uint16_t>(FirmwareStatus::ENTERED_BOOT_MODE));
      },
      5,
      std::chrono::milliseconds(500));
}

void MailboxFirmwareUpdater::exitBootMode() {
  std::cout << "Exiting Boot Mode..." << std::endl;

  retry(
      [this]() {
        try {
          client_->writeSingleRegister(
              deviceAddr_, REG_EXIT_BOOT, 0x55AA, 10000);
        } catch (const ModbusTimeoutException&) {
          std::cout
              << "Exit boot mode timed out... Checking if we are in correct status"
              << std::endl;
          verifyFirmwareStatus(
              static_cast<uint16_t>(FirmwareStatus::NORMAL_OPERATION));
        }
      },
      5,
      std::chrono::milliseconds(1000));
}

uint16_t MailboxFirmwareUpdater::getFirmwareStatus() {
  auto result = client_->readHoldingRegisters(deviceAddr_, REG_STATUS, 1, 1000);
  return result[0];
}

void MailboxFirmwareUpdater::verifyFirmwareStatus(uint16_t expected) {
  auto status = getFirmwareStatus();
  if (status != expected) {
    throw FirmwareStatusException(status, expected);
  }
}

void MailboxFirmwareUpdater::verifyFirmwareStatusWithRetry(
    uint16_t expected,
    int retries) {
  retry(
      [this, expected]() { verifyFirmwareStatus(expected); },
      retries,
      std::chrono::milliseconds(1000));
}

void MailboxFirmwareUpdater::writeBlock(const std::vector<uint16_t>& data) {
  size_t blockSizeWords = params_.blockSize / 2;

  if (data.size() < blockSizeWords) {
    // Workaround for bad .bin files with spurious extra bytes
    return;
  }

  if (data.size() != blockSizeWords) {
    throw UpdaterException(
        fmt::format(
            "Invalid block size: {} words (expected {})",
            data.size(),
            blockSizeWords));
  }

  bool expectCrcError =
      activeWorkarounds_.count(Workaround::WRITE_BLOCK_CRC_EXPECTED) > 0;

  retry(
      [this, &data, expectCrcError]() {
        try {
          client_->presetMultipleRegisters(
              deviceAddr_, REG_WRITE_BLOCK, data, 1000);
        } catch (const ModbusCRCException&) {
          if (expectCrcError) {
            // Suppress CRC error for early boot-loaders with incorrect CRC16
            static bool printed = false;
            if (!printed) {
              std::cout << "WARNING: CRCError suppressed" << std::endl;
              printed = true;
            }
          } else {
            throw;
          }
        }
      },
      5,
      std::chrono::milliseconds(1000));
}

void MailboxFirmwareUpdater::waitWriteBlock() {
  // Wait for FIRMWARE_PACKET_CORRECT status with many retries
  retry(
      [this]() {
        verifyFirmwareStatus(
            static_cast<uint16_t>(FirmwareStatus::FIRMWARE_PACKET_CORRECT));
      },
      500,
      std::chrono::milliseconds(10),
      0); // verbose=0
}

void MailboxFirmwareUpdater::transferImage(const std::vector<uint16_t>& image) {
  size_t blockSizeWords = params_.blockSize / 2;
  size_t numWords = image.size();
  size_t totalBlocks = (numWords + blockSizeWords - 1) / blockSizeWords;
  size_t sentBlocks = 0;

  for (size_t i = 0; i < numWords; i += blockSizeWords) {
    // Extract block
    std::vector<uint16_t> block;
    size_t remaining = numWords - i;
    if (remaining >= blockSizeWords) {
      block.assign(image.begin() + i, image.begin() + i + blockSizeWords);
    } else {
      block.assign(image.begin() + i, image.end());
    }

    writeBlock(block);

    if (params_.blockWait) {
      waitWriteBlock();
    } else {
      sleep(100);
    }

    printProgress(
        sentBlocks * 100.0 / totalBlocks,
        fmt::format("Sending block {} of {}...", sentBlocks, totalBlocks));
    sentBlocks++;
  }

  printProgress(
      100.0, fmt::format("Sending block {} of {}...", sentBlocks, totalBlocks));
  std::cout << std::endl;
}

void MailboxFirmwareUpdater::verifyFirmware() {
  sleepSeconds(10);
  client_->writeSingleRegister(deviceAddr_, REG_VERIFY, 0x55AA, 10000);
}

void MailboxFirmwareUpdater::workaroundForceExitBootMode() {
  auto currMode = getFirmwareStatus();
  if (currMode == static_cast<uint16_t>(FirmwareStatus::NORMAL_OPERATION)) {
    return;
  }

  std::cout
      << fmt::format(
             "WARNING: Current firmware in status 0x{:02x}. Expected 0x{:02x}",
             currMode,
             static_cast<uint16_t>(FirmwareStatus::NORMAL_OPERATION))
      << std::endl;
  std::cout << "Initiating remediation" << std::endl;

  // Assume previous aborted upgrade. Force a verify to walk it through abort.
  verifyFirmware();
  sleepSeconds(10);

  // Some devices need us to force clear the verify register
  if (activeWorkarounds_.count(Workaround::FORCE_CLEAR_VERIFY) > 0) {
    client_->writeSingleRegister(deviceAddr_, REG_VERIFY, 0, 1000);
  }

  exitBootMode();
  sleepSeconds(10);

  currMode = getFirmwareStatus();
  if (currMode != static_cast<uint16_t>(FirmwareStatus::NORMAL_OPERATION)) {
    std::cout << "ERROR: Workaround to recover firmware from mode failed."
              << std::endl;
    std::cout << fmt::format("Current status: 0x{:02x}", currMode) << std::endl;
    std::cout << "Continuing upgrade hoping for the best" << std::endl;
  }

  unlockFirmware();
}

// BootModeGuard implementation
MailboxFirmwareUpdater::BootModeGuard::BootModeGuard(
    MailboxFirmwareUpdater* updater)
    : updater_(updater) {
  updater_->enterBootMode();
}

MailboxFirmwareUpdater::BootModeGuard::~BootModeGuard() {
  try {
    updater_->sleepSeconds(10);
    updater_->exitBootMode();
    updater_->sleepSeconds(16);
    updater_->verifyFirmwareStatus(
        static_cast<uint16_t>(FirmwareStatus::NORMAL_OPERATION));
  } catch (const std::exception& e) {
    std::cerr << "Error during boot mode exit: " << e.what() << std::endl;
  }
}

void MailboxFirmwareUpdater::updateFirmware(const std::string& firmwareFile) {
  std::cout << "Parsing Firmware..." << std::endl;
  auto binImage = loadBinaryFirmware(firmwareFile);

  std::cout << "Unlock Engineering Mode" << std::endl;
  unlockFirmware();

  if (activeWorkarounds_.count(Workaround::FORCE_EXIT_BOOT_MODE) > 0) {
    workaroundForceExitBootMode();
  }

  {
    BootModeGuard guard(this);

    std::cout << "Transferring image" << std::endl;
    sleepSeconds(5);
    transferImage(binImage);

    std::cout << "Request Verify Firmware" << std::endl;
    verifyFirmware();

    std::cout
        << "Waiting for verification to complete (this may take 60+ seconds)"
        << std::endl;
    sleepSeconds(30);

    std::cout << "Checking firmware status (with retry up to 60 seconds)"
              << std::endl;
    verifyFirmwareStatusWithRetry(
        static_cast<uint16_t>(FirmwareStatus::FIRMWARE_UPGRADE_SUCCESS), 60);
  }

  std::cout << "done" << std::endl;
}

std::string MailboxFirmwareUpdater::readVersion() {
  try {
    // Read version registers
    auto registers = client_->readHoldingRegisters(
        deviceAddr_, params_.versionRegAddr, params_.versionRegLen, 1000);

    // Convert registers to string
    // Each register is 2 bytes (16 bits), stored as ASCII characters
    std::string version;
    for (auto reg : registers) {
      // Each register contains 2 ASCII characters (big-endian)
      char highByte = static_cast<char>((reg >> 8) & 0xFF);
      char lowByte = static_cast<char>(reg & 0xFF);

      // Add non-null characters
      if (highByte != 0) {
        version += highByte;
      }
      if (lowByte != 0) {
        version += lowByte;
      }
    }

    return version;
  } catch (const std::exception& e) {
    throw UpdaterException(std::string("Failed to read version: ") + e.what());
  }
}

} // namespace facebook::fboss::platform::rackmon
