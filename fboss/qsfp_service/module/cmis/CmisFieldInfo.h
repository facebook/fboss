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

/*
 * Parse transceiver data fields, as outlined in
 * Common Management Interface Specification by SFF commitee at
 * http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf
 */

namespace facebook {
namespace fboss {

enum class CmisField {
  // Lower Page
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
  APPLICATION_ADVERTISING1,
  BANK_SELECT,
  PAGE_SELECT_BYTE,
  // Page 00h
  VENDOR_NAME,
  VENDOR_OUI,
  PART_NUMBER,
  REVISION_NUMBER,
  VENDOR_SERIAL_NUMBER,
  MFG_DATE,
  EXTENDED_SPECIFICATION_COMPLIANCE,
  LENGTH_COPPER,
  MEDIA_INTERFACE_TECHNOLOGY,
  // Page 01h
  LENGTH_SMF,
  LENGTH_OM5,
  LENGTH_OM4,
  LENGTH_OM3,
  LENGTH_OM2,
  TX_SIG_INT_CONT_AD,
  RX_SIG_INT_CONT_AD,
  // Page 02h
  TEMPERATURE_THRESH,
  VCC_THRESH,
  TX_PWR_THRESH,
  TX_BIAS_THRESH,
  RX_PWR_THRESH,
  // Page 10h
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
  APP_SEL_LANE_1,
  APP_SEL_LANE_2,
  APP_SEL_LANE_3,
  APP_SEL_LANE_4,
  // Page 11h
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
  ACTIVE_CTRL_LANE_1,
  ACTIVE_CTRL_LANE_2,
  ACTIVE_CTRL_LANE_3,
  ACTIVE_CTRL_LANE_4,
  TX_CDR_CONTROL,
  RX_CDR_CONTROL,
  // Page 1Ch
  LOOPBACK_CAPABILITY,
  PATTERN_CAPABILITY,
  DIAGNOSTIC_CAPABILITY,
  PATTERN_CHECKER_CAPABILITY,
  HOST_GEN_ENABLE,
  HOST_GEN_INV,
  HOST_GEN_PRE_FEC,
  HOST_PATTERN_SELECT_LANE_2_1,
  HOST_PATTERN_SELECT_LANE_4_3,
  MEDIA_GEN_ENABLE,
  MEDIA_GEN_INV,
  MEDIA_GEN_PRE_FEC,
  MEDIA_PATTERN_SELECT_LANE_2_1,
  MEDIA_PATTERN_SELECT_LANE_4_3,
  HOST_CHECKER_ENABLE,
  HOST_CHECKER_INV,
  HOST_CHECKER_POST_FEC,
  HOST_CHECKER_PATTERN_SELECT_LANE_2_1,
  HOST_CHECKER_PATTERN_SELECT_LANE_4_3,
  MEDIA_CHECKER_ENABLE,
  MEDIA_CHECKER_INV,
  MEDIA_CHECKER_POST_FEC,
  MEDIA_CHECKER_PATTERN_SELECT_LANE_2_1,
  MEDIA_CHECKER_PATTERN_SELECT_LANE_4_3,
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
  DIAG_SEL,
  HOST_LANE_CHECKER_LOL,
  HOST_BER,
  MEDIA_BER_HOST_SNR,
  MEDIA_SNR,
};

enum FieldMasks : uint8_t {
  CABLE_LENGTH_MASK = 0x3f, // Highest two bits are multipliers.
  CDR_IMPL_MASK = 0x01,
  POWER_CONTROL_MASK = 0x40,
  APP_SEL_MASK = 0xf0,
};

enum DeviceTechnology : uint8_t {
  OPTICAL_MAX_VALUE = 0b1001,
  UNKNOWN_VALUE = 0b1000,
};

// We use the code from Table4-7 SM media interface codes in Sff-8024 here.
enum ApplicationCode : uint8_t {
  CWDM4_100G = 0x10,
  FR4_200GBASE = 0x18,
};

class CmisFieldInfo {
 public:
  int dataAddress;
  std::uint32_t offset;
  std::uint32_t length;

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

  // Render result as a member of FeatureState enum
  static FeatureState getFeatureState(uint8_t support, uint8_t enabled = 1);

  typedef std::map<CmisField, CmisFieldInfo> CmisFieldMap;

  /*
   * This function takes the CmisField name and returns
   * the dataAddress, offset and the length as per the CMIS DOM
   * Document mentioned above.
   */
  static CmisFieldInfo getCmisFieldAddress(
      const CmisFieldMap& map,
      CmisField field);
};

// Store multipliers for various conversion functions:

typedef std::map<CmisField, std::uint32_t> CmisFieldMultiplier;

// Store the mapping between port speed and ApplicationCode:

typedef std::map<cfg::PortSpeed, ApplicationCode> SpeedApplicationMapping;

} // namespace fboss
} // namespace facebook
