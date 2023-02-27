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

/*
 * Parse transceiver data fields, as outlined in various documents
 * by the Small Form Factor (SFF) committee, including SFP and QSFP+
 * modules.
 */

namespace facebook {
namespace fboss {

enum class SffPages;

enum class SffField {
  // Fields for entire page reads
  PAGE_LOWER,
  PAGE_UPPER0,
  PAGE_UPPER3,
  // Shared QSFP and SFP fields:
  IDENTIFIER, // Type of Transceiver
  STATUS, // Support flags for upper pages
  LOS, // Loss of Signal
  FAULT, // TX Faults
  LOL, // Loss of Lock
  TEMPERATURE_ALARMS,
  VCC_ALARMS, // Voltage
  CHANNEL_RX_PWR_ALARMS,
  CHANNEL_TX_BIAS_ALARMS,
  CHANNEL_TX_PWR_ALARMS,
  TEMPERATURE,
  VCC, // Voltage
  CHANNEL_RX_PWR,
  CHANNEL_TX_BIAS,
  CHANNEL_TX_PWR,
  TX_DISABLE,
  RATE_SELECT_RX,
  RATE_SELECT_TX,
  POWER_CONTROL,
  CDR_CONTROL, // Whether CDR is enabled
  ETHERNET_COMPLIANCE,
  EXTENDED_IDENTIFIER,
  PAGE_SELECT_BYTE,
  LENGTH_SM_KM, // Single mode, in km
  LENGTH_SM, // Single mode in 100m (not in QSFP)
  LENGTH_OM3,
  LENGTH_OM2,
  LENGTH_OM1,
  LENGTH_COPPER,
  LENGTH_COPPER_DECIMETERS,
  DAC_GAUGE,

  DEVICE_TECHNOLOGY, // Device or cable technology of free side device
  OPTIONS, // Variety of options, including rate select support
  VENDOR_NAME, // QSFP Vendor Name (ASCII)
  VENDOR_OUI, // QSFP Vendor IEEE company ID
  PART_NUMBER, // Part NUmber provided by QSFP vendor (ASCII)
  REVISION_NUMBER, // Revision number
  VENDOR_SERIAL_NUMBER, // Vendor Serial Number (ASCII)
  MFG_DATE, // Manufacturing date code
  DIAGNOSTIC_MONITORING_TYPE, // Diagnostic monitoring implemented
  TEMPERATURE_THRESH,
  VCC_THRESH,
  RX_PWR_THRESH,
  TX_BIAS_THRESH,
  TX_PWR_THRESH,
  EXTENDED_RATE_COMPLIANCE,
  TX_EQUALIZATION,
  RX_EMPHASIS,
  RX_AMPLITUDE,
  SQUELCH_CONTROL,
  TXRX_OUTPUT_CONTROL,
  EXTENDED_SPECIFICATION_COMPLIANCE,
  FR1_PRBS_SUPPORT,

  PAGE0_CSUM,
  PAGE0_EXTCSUM,
  PAGE1_CSUM,

  // SFP-specific Fields
  /* 0xA0 Address Fields */
  EXT_IDENTIFIER, // Extended type of transceiver
  CONNECTOR_TYPE, // Code for Connector Type
  TRANSCEIVER_CODE, // Code for Electronic or optical capability
  ENCODING_CODE, // High speed Serial encoding algo code
  SIGNALLING_RATE, // nominal signalling rate
  RATE_IDENTIFIER, // type of rate select functionality
  TRANCEIVER_CAPABILITY, // Code for Electronic or optical capability
  WAVELENGTH, // laser wavelength
  CHECK_CODE_BASEID, // Check code for the above fields
  // Extended options
  ENABLED_OPTIONS, // Indicates the optional transceiver signals enabled
  UPPER_BIT_RATE_MARGIN, // upper bit rate margin
  LOWER_BIT_RATE_MARGIN, // lower but rate margin
  ENHANCED_OPTIONS, // Enhanced options implemented
  SFF_COMPLIANCE, // revision number of SFF compliance
  CHECK_CODE_EXTENDED_OPT, // check code for the extended options
  VENDOR_EEPROM,

  /* 0xA2 address Fields */
  /* Diagnostics */
  ALARM_THRESHOLD_VALUES, // diagnostic flag alarm and warning thresh values
  EXTERNAL_CALIBRATION, // diagnostic calibration constants
  CHECK_CODE_DMI, // Check code for base Diagnostic Fields
  DIAGNOSTICS, // Diagnostic Monitor Data
  STATUS_CONTROL, // Optional Status and Control bits
  ALARM_WARN_FLAGS, // Diagnostic alarm and warning flag
  EXTENDED_STATUS_CONTROL, // Extended status and control bytes
  /* General Purpose */
  VENDOR_MEM_ADDRESS, // Vendor Specific memory address
  USER_EEPROM, // User Writable NVM
  VENDOR_CONTROL, // Vendor Specific Control
};

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
