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
#include "fboss/platform/rackmon/rack_firmware_update/RackmonClient.h"

namespace facebook::fboss::platform::rackmon {

/**
 * Abstract base class for firmware updaters
 * Provides common interface for different firmware update protocols
 */
class FirmwareUpdater {
 public:
  /**
   * Constructor
   * @param client Rackmon client for communication
   * @param deviceAddr Device address (Modbus address)
   */
  FirmwareUpdater(std::shared_ptr<RackmonClient> client, uint16_t deviceAddr);

  virtual ~FirmwareUpdater() = default;

  /**
   * Update device firmware from file
   * @param firmwareFile Path to firmware file
   * @throws UpdaterException on failure
   */
  virtual void updateFirmware(const std::string& firmwareFile) = 0;

  /**
   * Read and print firmware version
   * @return Firmware version string
   */
  virtual std::string readVersion() = 0;

  /**
   * Get device address
   */
  uint16_t getDeviceAddress() const {
    return deviceAddr_;
  }

 protected:
  std::shared_ptr<RackmonClient> client_;
  uint16_t deviceAddr_;

  /**
   * Sleep for specified milliseconds
   */
  void sleep(uint32_t milliseconds);

  /**
   * Sleep for specified seconds
   */
  void sleepSeconds(uint32_t seconds);
};

} // namespace facebook::fboss::platform::rackmon
