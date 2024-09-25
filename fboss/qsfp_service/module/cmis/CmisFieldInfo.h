/* * Copyright (c) 2004-present, Facebook, Inc.  * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once
#include <cstdint>
#include <map>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/QsfpFieldInfo.h"

/*
 * Parse transceiver data fields, as outlined in
 * Common Management Interface Specification by SFF commitee at
 * http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf
 */

namespace facebook {
namespace fboss {

enum class CmisPages;

enum class CmisField {
  // Lower Page
  PAGE_LOWER,
  IDENTIFIER,
  REVISION_COMPLIANCE,
  FLAT_MEM,
  MODULE_STATE,
  BANK0_FLAGS,
  BANK1_FLAGS,
  BANK2_FLAGS,
  BANK3_FLAGS,
  MODULE_FLAG,
  MODULE_ALARMS,
  TEMPERATURE,
  VCC,
  MODULE_CONTROL,
  FIRMWARE_REVISION,
  MEDIA_TYPE_ENCODINGS,
  APPLICATION_ADVERTISING1,
  BANK_SELECT,
  PAGE_SELECT_BYTE,
  // Page 00h
  PAGE_UPPER00H,
  VENDOR_NAME,
  VENDOR_OUI,
  PART_NUMBER,
  REVISION_NUMBER,
  VENDOR_SERIAL_NUMBER,
  MFG_DATE,
  EXTENDED_SPECIFICATION_COMPLIANCE,
  LENGTH_COPPER,
  MEDIA_INTERFACE_TECHNOLOGY,
  PAGE0_CSUM,
  // Page 01h
  PAGE_UPPER01H,
  LENGTH_SMF,
  LENGTH_OM5,
  LENGTH_OM4,
  LENGTH_OM3,
  LENGTH_OM2,
  VDM_DIAG_SUPPORT,
  TX_CONTROL_SUPPORT,
  RX_CONTROL_SUPPORT,
  TX_BIAS_MULTIPLIER,
  TX_SIG_INT_CONT_AD,
  RX_SIG_INT_CONT_AD,
  CDB_SUPPORT,
  MEDIA_LANE_ASSIGNMENT,
  DSP_FW_VERSION,
  BUILD_REVISION,
  APPLICATION_ADVERTISING2,
  PAGE1_CSUM,
  // Page 02h
  PAGE_UPPER02H,
  TEMPERATURE_THRESH,
  VCC_THRESH,
  TX_PWR_THRESH,
  TX_BIAS_THRESH,
  RX_PWR_THRESH,
  PAGE2_CSUM,
  // Page 10h
  PAGE_UPPER10H,
  DATA_PATH_DEINIT,
  TX_POLARITY_FLIP,
  TX_DISABLE,
  TX_SQUELCH_DISABLE,
  TX_FORCE_SQUELCH,
  TX_ADAPTATION_FREEZE,
  TX_ADAPTATION_STORE,
  RX_POLARITY_FLIP,
  RX_DISABLE,
  RX_SQUELCH_DISABLE,
  STAGE_CTRL_SET_0,
  STAGE_CTRL_SET0_IMMEDIATE,
  APP_SEL_ALL_LANES,
  APP_SEL_LANE_1,
  APP_SEL_LANE_2,
  APP_SEL_LANE_3,
  APP_SEL_LANE_4,
  APP_SEL_LANE_5,
  APP_SEL_LANE_6,
  APP_SEL_LANE_7,
  APP_SEL_LANE_8,
  RX_CONTROL_PRE_CURSOR,
  RX_CONTROL_POST_CURSOR,
  RX_CONTROL_MAIN,
  // Page 11h
  PAGE_UPPER11H,
  DATA_PATH_STATE,
  TX_FAULT_FLAG,
  TX_LOS_FLAG,
  TX_LOL_FLAG,
  TX_EQ_FLAG,
  TX_PWR_FLAG,
  TX_BIAS_FLAG,
  RX_LOS_FLAG,
  RX_LOL_FLAG,
  RX_PWR_FLAG,
  CHANNEL_TX_PWR,
  CHANNEL_TX_BIAS,
  CHANNEL_RX_PWR,
  CONFIG_ERROR_LANES,
  ACTIVE_CTRL_ALL_LANES,
  ACTIVE_CTRL_LANE_1,
  ACTIVE_CTRL_LANE_2,
  ACTIVE_CTRL_LANE_3,
  ACTIVE_CTRL_LANE_4,
  ACTIVE_CTRL_LANE_5,
  ACTIVE_CTRL_LANE_6,
  ACTIVE_CTRL_LANE_7,
  ACTIVE_CTRL_LANE_8,
  TX_CDR_CONTROL,
  RX_CDR_CONTROL,
  RX_OUT_PRE_CURSOR,
  RX_OUT_POST_CURSOR,
  RX_OUT_MAIN,
  // Page 13h
  PAGE_UPPER13H,
  // Page 14h
  PAGE_UPPER14H,
  // Page 1Ch
  PAGE_UPPER1CH,
  LOOPBACK_CAPABILITY,
  PATTERN_CAPABILITY,
  DIAGNOSTIC_CAPABILITY,
  PATTERN_CHECKER_CAPABILITY,
  HOST_SUPPORTED_GENERATOR_PATTERNS,
  MEDIA_SUPPORTED_GENERATOR_PATTERNS,
  HOST_SUPPORTED_CHECKER_PATTERNS,
  MEDIA_SUPPORTED_CHECKER_PATTERNS,
  HOST_GEN_ENABLE,
  HOST_GEN_INV,
  HOST_GEN_PRE_FEC,
  HOST_PATTERN_SELECT_LANE_2_1,
  HOST_PATTERN_SELECT_LANE_4_3,
  HOST_PATTERN_SELECT_LANE_6_5,
  HOST_PATTERN_SELECT_LANE_8_7,
  MEDIA_GEN_ENABLE,
  MEDIA_GEN_INV,
  MEDIA_GEN_PRE_FEC,
  MEDIA_PATTERN_SELECT_LANE_2_1,
  MEDIA_PATTERN_SELECT_LANE_4_3,
  MEDIA_PATTERN_SELECT_LANE_6_5,
  MEDIA_PATTERN_SELECT_LANE_8_7,
  HOST_CHECKER_ENABLE,
  HOST_CHECKER_INV,
  HOST_CHECKER_POST_FEC,
  HOST_CHECKER_PATTERN_SELECT_LANE_2_1,
  HOST_CHECKER_PATTERN_SELECT_LANE_4_3,
  HOST_CHECKER_PATTERN_SELECT_LANE_6_5,
  HOST_CHECKER_PATTERN_SELECT_LANE_8_7,
  MEDIA_CHECKER_ENABLE,
  MEDIA_CHECKER_INV,
  MEDIA_CHECKER_POST_FEC,
  MEDIA_CHECKER_PATTERN_SELECT_LANE_2_1,
  MEDIA_CHECKER_PATTERN_SELECT_LANE_4_3,
  MEDIA_CHECKER_PATTERN_SELECT_LANE_6_5,
  MEDIA_CHECKER_PATTERN_SELECT_LANE_8_7,
  REF_CLK_CTRL,
  BER_CTRL,
  HOST_NEAR_LB_EN,
  MEDIA_NEAR_LB_EN,
  HOST_FAR_LB_EN,
  MEDIA_FAR_LB_EN,
  REF_CLK_LOSS,
  HOST_CHECKER_GATING_COMPLETE,
  MEDIA_CHECKER_GATING_COMPLETE,
  HOST_PPG_LOL,
  MEDIA_PPG_LOL,
  HOST_BERT_LOL,
  MEDIA_BERT_LOL,
  // Page 1Dh
  PAGE_UPPER1DH,
  DIAG_SEL,
  HOST_LANE_GENERATOR_LOL_LATCH,
  MEDIA_LANE_GENERATOR_LOL_LATCH,
  HOST_LANE_CHECKER_LOL_LATCH,
  MEDIA_LANE_CHECKER_LOL_LATCH,
  HOST_BER,
  MEDIA_BER_HOST_SNR,
  MEDIA_SNR,
  // Page 20h
  PAGE_UPPER20H,
  // Page 21h
  PAGE_UPPER21H,
  // Page 22h
  PAGE_UPPER22H,
  // Page 24h
  PAGE_UPPER24H,
  // Page 25h
  PAGE_UPPER25H,
  // Page 26h
  PAGE_UPPER26H,
  // Page 2Fh
  PAGE_UPPER2FH,
  VDM_GROUPS_SUPPORT,
  VDM_LATCH_REQUEST,
  VDM_LATCH_DONE,
};

enum FieldMasks : uint8_t {
  CABLE_LENGTH_MASK = 0x3f, // Highest two bits are multipliers.
  CDR_IMPL_MASK = 0x01,

  // Byte 26 (Module Global Controls) bitmask:
  // Bit 6: LowPwrAllowRequestHW in CMIS 5.0, LowPwr in 4.0
  // Bit 5: Squelch control: 0 = Squelch reduces OMA, 1 = Squelch reduces Pave
  // Bit 4: LowPwrRequestSW in CMIS 5.0, ForceLowPwr in 4.0
  LOW_PWR_BIT = (1 << 6),
  SQUELCH_CONTROL = (1 << 5),
  FORCE_LOW_PWR_BIT = (1 << 4),
  // When clearing LP mode, we'll clear both bits. When setting LP mode,
  // we'll only set bit 6
  POWER_CONTROL_MASK = LOW_PWR_BIT | FORCE_LOW_PWR_BIT,

  APP_SEL_BITSHIFT = 4,
  APP_SEL_MASK = 0xf0,
  DATA_PATH_ID_MASK = 0xe,
  DATA_PATH_ID_BITSHIFT = 0x1,
  FWFAULT_MASK = 0x06,
  MODULE_STATE_CHANGED_MASK = 0x01,
  UPPER_FOUR_BITS_MASK = 0xf0,
  LOWER_FOUR_BITS_MASK = 0x0f,
  VDM_SUPPORT_MASK = 0x40,
  VDM_LATCH_REQUEST_MASK = 0x80,
  VDM_LATCH_DONE_MASK = 0x80,
  DIAGS_SUPPORT_MASK = 0x20,
  CDB_SUPPORT_MASK = 0xc0,
  LOOPBACK_SYS_SUPPOR_MASK = 0x09,
  LOOPBACK_LINE_SUPPORT_MASK = 0x06,
  PRBS_SYS_SUPPRT_MASK = 0x0f,
  PRBS_LINE_SUPPRT_MASK = 0xf0,
  TX_BIAS_MULTIPLIER_MASK = 0x18,
  MODULE_STATUS_MASK = 0x0E,
  TX_DISABLE_SUPPORT_MASK = 0x02,
  RX_DISABLE_SUPPORT_MASK = 0x02,
  SNR_LINE_SUPPORT_MASK = 0x20,
  SNR_SYS_SUPPORT_MASK = 0x10,
  VDM_GROUPS_SUPPORT_MASK = 0x03,
  VDM_DESCRIPTOR_RESOURCE_MASK = 0x0f,
  CDB_FW_DOWNLOAD_EPL_SUPPORTED = 0x10,
  CDB_FW_DOWNLOAD_LPL_EPL_SUPPORTED = 0x11,
  BER_CTRL_RESET_STAT_MASK = 0x20,
};

enum FieldBitShift : uint8_t {
  MODULE_STATUS_BITSHIFT = 1,
};

enum DeviceTechnologyCmis : uint8_t {
  OPTICAL_MAX_VALUE_CMIS = 0b1001,
  UNKNOWN_VALUE_CMIS = 0b1000,
};

class CmisFieldInfo : public QsfpFieldInfo<CmisField, CmisPages> {
 public:
  // Render degrees Celcius from fix-point integer value
  static double getTemp(uint16_t temp);

  // Render Vcc in volts from fixed-point value
  static double getVcc(uint16_t temp);

  // Render TxBias in mA from fixed-point value
  static double getTxBias(uint16_t temp);

  // Render power in mW from fixed-point value
  static double getPwr(uint16_t temp);

  // Render SNR from 16 bits fixed-point value
  static double getSnr(const uint16_t data);

  // Render Precursor from 4 bits fixed-point value
  static double getPreCursor(const uint8_t data);

  // Render Postcursor from 4 bits fixed-point value
  static double getPostCursor(const uint8_t data);

  // Render Amplitude from 4 bits fixed-point value
  static std::pair<uint32_t, uint32_t> getAmplitude(const uint8_t data);

  // Render result as a member of FeatureState enum
  static FeatureState getFeatureState(uint8_t support, uint8_t enabled = 1);

  // Get Tx Bias current multiplier
  static uint8_t getTxBiasMultiplier(const uint8_t data);
};

// Store multipliers for various conversion functions:

typedef std::map<CmisField, std::uint32_t> CmisFieldMultiplier;

// Store the mapping between port speed and ApplicationCode.
// We use the module Media Interface ID, which is located at the second byte
// of the APPLICATION_ADVERTISING field, as Application ID here, which is the
// code from Table4-7 SM media interface codes in Sff-8024.

using SpeedApplicationMapping =
    std::map<cfg::PortSpeed, std::vector<SMFMediaInterfaceCode>>;

} // namespace fboss
} // namespace facebook
