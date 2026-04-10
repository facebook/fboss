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
#include <map>
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

/**
 * SpeedValidator provides speed validation logic for port configuration.
 * It queries both the platform hardware capabilities and optics (QSFP)
 * to ensure the requested speed is supported.
 *
 * Construct once (which queries Agent and QSFP services via Thrift), then
 * call validateSpeed() for each port without repeated Thrift calls.
 */
class SpeedValidator {
 public:
  /**
   * Construct by querying Agent and QSFP services via Thrift.
   * Build once, then call validateSpeed() for multiple ports.
   *
   * @param hostInfo Connection information for Agent and QSFP services
   */
  explicit SpeedValidator(const HostInfo& hostInfo);

  /**
   * Construct from pre-built data (for unit testing without Thrift).
   *
   * @param platformMapping Platform mapping from the Agent service
   * @param qsfpSupportedProfiles Per-port supported profiles from QSFP service
   */
  SpeedValidator(
      cfg::PlatformMapping platformMapping,
      std::map<std::string, std::vector<cfg::PortProfileID>>
          qsfpSupportedProfiles);

  /**
   * Parse speed string to cfg::PortSpeed enum value.
   * Accepts numeric strings (e.g., "100000"), "auto", or "0" (both return
   * DEFAULT).
   *
   * @param speedStr Speed string to parse
   * @return Corresponding cfg::PortSpeed enum value
   * @throws std::invalid_argument if speedStr is not valid
   */
  static cfg::PortSpeed parseSpeed(const std::string& speedStr);

  /**
   * Validates a speed value for a given port.
   * Use a single SpeedValidator instance to validate multiple ports without
   * repeated Thrift calls.
   *
   * @param portName Port name (e.g., "eth1/1/1")
   * @param speedStr Speed value to validate (e.g., "100000", "auto", "0")
   * @return Vector of matching profile IDs if valid
   * @throws std::runtime_error if speed is invalid with helpful error message
   */
  std::vector<cfg::PortProfileID> validateSpeed(
      const std::string& portName,
      const std::string& speedStr);

 private:
  cfg::PlatformMapping platformMapping_;
  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpSupportedProfiles_;

  std::vector<cfg::PortProfileID> getSupportedProfiles(
      const std::string& portName);

  int64_t getProfileSpeed(PortID portId, cfg::PortProfileID profile);

  std::string formatErrorMessage(
      PortID portId,
      const std::string& portName,
      const std::string& speedStr,
      const std::vector<cfg::PortProfileID>& supportedProfiles);

  static int64_t parseSpeedToMbps(const std::string& speedStr);
  static cfg::PortSpeed speedToPortSpeedEnum(int64_t speedMbps);
};

} // namespace facebook::fboss
