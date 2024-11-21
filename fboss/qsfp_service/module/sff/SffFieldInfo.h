/* * Copyright (c) 2004-present, Facebook, Inc.  * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once
#include <cstdint>
#include <map>

#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/QsfpFieldInfo.h"
#include "fboss/qsfp_service/module/sff/gen-cpp2/sff_types.h"

/*
 * Parse transceiver data fields, as outlined in various documents
 * by the Small Form Factor (SFF) committee, including SFP and QSFP+
 * modules.
 */

namespace facebook {
namespace fboss {

enum class SffPages;

enum DeviceTechnologySff : uint8_t {
  TRANSMITTER_TECH_SHIFT = 4,
  OPTICAL_MAX_VALUE_SFF = 0b1001,
  UNKNOWN_VALUE_SFF = 0b1000,
};

enum class PowerControl : uint8_t {
  // Use lpmode set by pin
  // We always want the first bit set to 1 so it's set in sw
  POWER_SET_BY_HW = 0b0,
  // Use low power mode
  POWER_LPMODE = 0b11,
  // Disable low power mode
  // High power mode, class 1 to 4(<3.5W)
  POWER_OVERRIDE = 0b01,
  // Disable low power mode
  // Super high power mode, class 5 to 7(>3.5W)
  HIGH_POWER_OVERRIDE = 0b101,
  // Only interested in first 3 bits
  POWER_CONTROL_MASK = 0b111,
};

// This should be called ExtendedIdentifier
enum ExternalIdentifer : uint8_t {
  EXT_ID_POWER_SHIFT = 6,
  EXT_ID_POWER_MASK = 0xc0,
  EXT_ID_HI_POWER_MASK = 0x03,
  EXT_ID_CDR_RX_MASK = 0x04,
  EXT_ID_CDR_TX_MASK = 0x08,
};

enum EthernetCompliance : uint8_t {
  ACTIVE_CABLE = 1 << 0,
  LR4_40GBASE = 1 << 1,
  SR4_40GBASE = 1 << 2,
  CR4_40GBASE = 1 << 3,
  SR_10GBASE = 1 << 4,
  LR_10GBASE = 1 << 5,
  LRM_40GBASE = 1 << 6,
};

enum DiagnosticMonitoringType {
  POWER_MEASUREMENT_MASK = 0x04,
  RX_POWER_MEASUREMENT_TYPE_MASK = 0x08,
};

enum EnhancedOptions : uint8_t {
  ENH_OPT_RATE_SELECT_MASK = 0x0c,
};

enum Options : uint8_t {
  OPT_RATE_SELECT_MASK = 1 << 5,
};

enum HalfByteMasks : uint8_t {
  UPPER_BITS_MASK = 0xf0,
  LOWER_BITS_MASK = 0x0f,
};

enum FieldMasks : uint8_t {
  DATA_NOT_READY_MASK = 0x01,
};

enum DiagsCapabilityMask : uint8_t {
  TX_DISABLE_SUPPORT_MASK = 0x10,
  RX_DISABLE_SUPPORT_MASK = 0x04,
};

enum MiniphotonLoopbackRegVal : uint8_t {
  MINIPHOTON_LPBK_LINE_MASK = 0b10101010,
  MINIPHOTON_LPBK_SYSTEM_MASK = 0b01010101,
};

class SffFieldInfo : public QsfpFieldInfo<SffField, SffPages> {
 public:
  // Conversion routines used for both SFP and QSFP:

  // Render degrees Celcius from fix-point integer value
  static double getTemp(uint16_t temp);

  // Render Vcc in volts from fixed-point value
  static double getVcc(uint16_t temp);

  // Render TxBias in mA from fixed-point value
  static double getTxBias(uint16_t temp);

  // Render power in mW from fixed-point value
  static double getPwr(uint16_t temp);

  // Render result as a member of FeatureState enum
  static FeatureState getFeatureState(uint8_t support, uint8_t enabled = 1);
};

// Store multipliers for various conversion functions:

typedef std::map<SffField, std::uint32_t> SffFieldMultiplier;

} // namespace fboss
} // namespace facebook
