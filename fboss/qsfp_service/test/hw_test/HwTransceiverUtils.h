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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss::utility {

class HwTransceiverUtils {
 public:
  static void verifyTransceiverSettings(
      const TransceiverInfo& transceiver,
      cfg::PortProfileID profile);

  // T114627923 Because some old firmware might not enable all capabilities
  // use `skipCheckingIndividualCapability` to skip checking individual
  // capability until we can make sure all modules have the latest firmware
  static void verifyDiagsCapability(
      const TransceiverInfo& transceiver,
      std::optional<DiagsCapability> diagsCapability,
      bool skipCheckingIndividualCapability = true);

 private:
  static void verifyOpticsSettings(
      const TransceiverInfo& transceiver,
      cfg::PortProfileID profile);
  static void verifyMediaInterfaceCompliance(
      const TransceiverInfo& transceiver,
      cfg::PortProfileID profile);
  static void verify10gProfile(
      const TransceiverInfo& transceiver,
      const TransceiverManagementInterface mgmtInterface,
      const std::vector<MediaInterfaceId>& mediaInterfaces);
  static void verify100gProfile(
      const TransceiverManagementInterface mgmtInterface,
      const std::vector<MediaInterfaceId>& mediaInterfaces);
  static void verify200gProfile(
      const TransceiverManagementInterface mgmtInterface,
      const std::vector<MediaInterfaceId>& mediaInterfaces);
  static void verify400gProfile(
      const TransceiverManagementInterface mgmtInterface,
      const std::vector<MediaInterfaceId>& mediaInterfaces);
  static void verifyCopper100gProfile(
      const TransceiverInfo& transceiver,
      const std::vector<MediaInterfaceId>& mediaInterfaces);
  static void verifyCopper200gProfile(
      const TransceiverInfo& transceiver,
      const std::vector<MediaInterfaceId>& mediaInterfaces);
  static void verifyCopper53gProfile(
      const TransceiverInfo& transceiver,
      const std::vector<MediaInterfaceId>& mediaInterfaces);

  static void verifyDataPathEnabled(const TransceiverInfo& transceiver);
};

} // namespace facebook::fboss::utility
