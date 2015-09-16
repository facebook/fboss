/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "SfpModule.h"

#include "fboss/agent/FbossError.h"
#include <boost/assign.hpp>
#include <string>

namespace facebook { namespace fboss {
  using std::memcpy;
  using std::mutex;
  using std::lock_guard;

struct SfpIdpromFieldInfo {
  std::uint32_t dataAddress;
  std::uint32_t offset;
  std::uint32_t length;
};

static std::map<SfpIdpromFields, SfpIdpromFieldInfo> sfpFields = {
  /* 0xA0 EEPROM Field Values */
  {SfpIdpromFields::IDENTIFIER, {0x50, 0x0, 1} },
  {SfpIdpromFields::EXT_IDENTIFIER, {0x50, 0x1, 1} },
  {SfpIdpromFields::CONNECTOR_TYPE, {0x50, 0x2, 1} },
  {SfpIdpromFields::TRANSCEIVER_CODE, {0x50, 0x3, 8} },
  {SfpIdpromFields::ENCODING_CODE, {0x50, 0xB, 1} },
  {SfpIdpromFields::SIGNALLING_RATE, {0x50, 0xC, 1} },
  {SfpIdpromFields::RATE_IDENTIFIER, {0x50, 0xD, 1} },
  {SfpIdpromFields::LENGTH_SM_KM, {0x50, 0xE, 1} },
  {SfpIdpromFields::LENGTH_SM, {0x50, 0xF, 1} },
  {SfpIdpromFields::LENGTH_OM2, {0x50, 0x10, 1} },
  {SfpIdpromFields::LENGTH_OM1, {0x50, 0x11, 1} },
  {SfpIdpromFields::LENGTH_COPPER, {0x50, 0x12, 1} },
  {SfpIdpromFields::LENGTH_OM3, {0x50, 0x13, 1} },
  {SfpIdpromFields::VENDOR_NAME, {0x50, 0x14, 16} },
  {SfpIdpromFields::TRANCEIVER_CAPABILITY, {0x50, 0x24, 1} },
  {SfpIdpromFields::VENDOR_OUI, {0x50, 0x25, 3} },
  {SfpIdpromFields::PART_NUMBER, {0x50, 0x28, 16} },
  {SfpIdpromFields::REVISION_NUMBER, {0x50, 0x38, 4} },
  {SfpIdpromFields::WAVELENGTH, {0x50, 0x3C, 2} },
  {SfpIdpromFields::CHECK_CODE_BASEID, {0x50, 0x3F, 1} },
  {SfpIdpromFields::ENABLED_OPTIONS, {0x50, 0x40, 2} },
  {SfpIdpromFields::UPPER_BIT_RATE_MARGIN, {0x50, 0x42, 1} },
  {SfpIdpromFields::LOWER_BIT_RATE_MARGIN, {0x50, 0x43, 1} },
  {SfpIdpromFields::VENDOR_SERIAL_NUMBER, {0x50, 0x44, 16} },
  {SfpIdpromFields::MFG_DATE, {0x50, 0x54, 8} },
  {SfpIdpromFields::DIAGNOSTIC_MONITORING_TYPE, {0x50, 0x5C, 1} },
  {SfpIdpromFields::ENHANCED_OPTIONS, {0x50, 0x5D, 1} },
  {SfpIdpromFields::SFF_COMPLIANCE, {0x50, 0x5E, 1} },
  {SfpIdpromFields::CHECK_CODE_EXTENDED_OPT, {0x50, 0x5F, 1} },
  {SfpIdpromFields::VENDOR_EEPROM, {0x50, 0x60, 32} },
  {SfpIdpromFields::RESERVED_FIELD, {0x50, 0x80, 128} },
   /* 0xA2 EEPROM Field Values */
  {SfpIdpromFields::ALARM_THRESHOLD_VALUES, {0x51, 0x0, 40} },
  {SfpIdpromFields::EXTERNAL_CALIBRATION, {0x51, 0x38, 36} },
  {SfpIdpromFields::CHECK_CODE_DMI, {0x51, 0x5F, 1} },
  {SfpIdpromFields::DIAGNOSTICS, {0x51, 0x60, 10} },
  {SfpIdpromFields::STATUS_CONTROL, {0x51, 0x6E, 1} },
  {SfpIdpromFields::RESERVED, {0x51, 0x6F, 1} },
  {SfpIdpromFields::ALARM_WARN_FLAGS, {0x51, 0x70, 6} },
  {SfpIdpromFields::EXTENDED_STATUS_CONTROL, {0x51, 0x76, 2} },
  {SfpIdpromFields::VENDOR_MEM_ADDRESS, {0x51, 0x78, 8} },
  {SfpIdpromFields::USER_EEPROM, {0x51, 0x80, 120} },
  {SfpIdpromFields::VENDOR_CONTROL, {0x51, 0xF8, 8} },
};

static std::map<SfpDomFlag, int> sfpDomFlag = {
  {SfpDomFlag::TEMP_ALARM_HIGH,    47},
  {SfpDomFlag::TEMP_ALARM_LOW,     46},
  {SfpDomFlag::VCC_ALARM_HIGH,     45},
  {SfpDomFlag::VCC_ALARM_LOW,      44},
  {SfpDomFlag::TX_BIAS_ALARM_HIGH, 43},
  {SfpDomFlag::TX_BIAS_ALARM_LOW,  42},
  {SfpDomFlag::TX_PWR_ALARM_HIGH,  41},
  {SfpDomFlag::TX_PWR_ALARM_LOW,   40},
  {SfpDomFlag::RX_PWR_ALARM_HIGH,  39},
  {SfpDomFlag::RX_PWR_ALARM_LOW,   38},
  {SfpDomFlag::TEMP_WARN_HIGH,     15},
  {SfpDomFlag::TEMP_WARN_LOW,      14},
  {SfpDomFlag::VCC_WARN_HIGH,      13},
  {SfpDomFlag::VCC_WARN_LOW,       12},
  {SfpDomFlag::TX_BIAS_WARN_HIGH,  11},
  {SfpDomFlag::TX_BIAS_WARN_LOW,   10},
  {SfpDomFlag::TX_PWR_WARN_HIGH,    9},
  {SfpDomFlag::TX_PWR_WARN_LOW,     8},
  {SfpDomFlag::RX_PWR_WARN_HIGH,    7},
  {SfpDomFlag::RX_PWR_WARN_LOW,     6},
};

void getSfpFieldAddress(SfpIdpromFields field, int &dataAddress,
                                    int &offset, int &length) {
  auto sfpFieldInfo = sfpFields.find(field);
  if (sfpFieldInfo == sfpFields.end()) {
    throw FbossError("Invalid SFP Field ID");
  }
  dataAddress = sfpFieldInfo->second.dataAddress;
  offset = sfpFieldInfo->second.offset;
  length = sfpFieldInfo->second.length;
}

int getSfpDomBit(const SfpDomFlag flag) {
  auto domFlag = sfpDomFlag.find(flag);
  if (domFlag == sfpDomFlag.end()) {
    throw FbossError("Invalid SFP Field ID");
  }
  return domFlag->second;
}

bool SfpModule::getDomFlagsMap(SfpDomThreshFlags &domFlags) {
  if (cacheIsValid() && domSupport_) {
    domFlags.tempAlarmHigh = getSfpThreshFlag(SfpDomFlag::TEMP_ALARM_HIGH);
    domFlags.tempAlarmLow = getSfpThreshFlag(SfpDomFlag::TEMP_ALARM_LOW);
    domFlags.tempWarnHigh = getSfpThreshFlag(SfpDomFlag::TEMP_WARN_HIGH);
    domFlags.tempWarnLow = getSfpThreshFlag(SfpDomFlag::TEMP_WARN_LOW);
    domFlags.vccAlarmHigh = getSfpThreshFlag(SfpDomFlag::VCC_ALARM_HIGH);
    domFlags.vccAlarmLow = getSfpThreshFlag(SfpDomFlag::VCC_ALARM_LOW);
    domFlags.vccWarnHigh = getSfpThreshFlag(SfpDomFlag::VCC_WARN_HIGH);
    domFlags.vccWarnLow = getSfpThreshFlag(SfpDomFlag::VCC_WARN_LOW);
    domFlags.txBiasAlarmHigh = getSfpThreshFlag(SfpDomFlag::TX_BIAS_ALARM_HIGH);
    domFlags.txBiasAlarmLow = getSfpThreshFlag(SfpDomFlag::TX_BIAS_ALARM_LOW);
    domFlags.txBiasWarnHigh = getSfpThreshFlag(SfpDomFlag::TX_BIAS_WARN_HIGH);
    domFlags.txBiasWarnLow = getSfpThreshFlag(SfpDomFlag::TX_BIAS_WARN_LOW);
    domFlags.txPwrAlarmHigh = getSfpThreshFlag(SfpDomFlag::TX_PWR_ALARM_HIGH);
    domFlags.txPwrAlarmLow = getSfpThreshFlag(SfpDomFlag::TX_PWR_ALARM_LOW);
    domFlags.txPwrWarnHigh = getSfpThreshFlag(SfpDomFlag::TX_PWR_WARN_HIGH);
    domFlags.txPwrWarnLow = getSfpThreshFlag(SfpDomFlag::TX_PWR_WARN_LOW);
    domFlags.rxPwrAlarmHigh = getSfpThreshFlag(SfpDomFlag::RX_PWR_ALARM_HIGH);
    domFlags.rxPwrAlarmLow = getSfpThreshFlag(SfpDomFlag::RX_PWR_ALARM_LOW);
    domFlags.rxPwrWarnHigh = getSfpThreshFlag(SfpDomFlag::RX_PWR_WARN_HIGH);
    domFlags.rxPwrWarnLow = getSfpThreshFlag(SfpDomFlag::RX_PWR_WARN_LOW);
    return true;
  }
  return false;
}

bool SfpModule::getDomThresholdValuesMap(SfpDomThreshValue &domThresh) {
  if (cacheIsValid() && domSupport_) {
    domThresh.tempAlarmHigh = getSfpThreshValue(SfpDomFlag::TEMP_ALARM_HIGH);
    domThresh.tempAlarmLow = getSfpThreshValue(SfpDomFlag::TEMP_ALARM_LOW);
    domThresh.tempWarnHigh = getSfpThreshValue(SfpDomFlag::TEMP_WARN_HIGH);
    domThresh.tempWarnLow = getSfpThreshValue(SfpDomFlag::TEMP_WARN_LOW);
    domThresh.vccAlarmHigh = getSfpThreshValue(SfpDomFlag::VCC_ALARM_HIGH);
    domThresh.vccAlarmLow = getSfpThreshValue(SfpDomFlag::VCC_ALARM_LOW);
    domThresh.vccWarnHigh = getSfpThreshValue(SfpDomFlag::VCC_WARN_HIGH);
    domThresh.vccWarnLow = getSfpThreshValue(SfpDomFlag::VCC_WARN_LOW);
    domThresh.txBiasAlarmHigh = getSfpThreshValue(
                                                SfpDomFlag::TX_BIAS_ALARM_HIGH);
    domThresh.txBiasAlarmLow = getSfpThreshValue(SfpDomFlag::TX_BIAS_ALARM_LOW);
    domThresh.txBiasWarnHigh = getSfpThreshValue(SfpDomFlag::TX_BIAS_WARN_HIGH);
    domThresh.txBiasWarnLow = getSfpThreshValue(SfpDomFlag::TX_BIAS_WARN_LOW);
    domThresh.txPwrAlarmHigh = getSfpThreshValue(SfpDomFlag::TX_PWR_ALARM_HIGH);
    domThresh.txPwrAlarmLow = getSfpThreshValue(SfpDomFlag::TX_PWR_ALARM_LOW);
    domThresh.txPwrWarnHigh = getSfpThreshValue(SfpDomFlag::TX_PWR_WARN_HIGH);
    domThresh.txPwrWarnLow = getSfpThreshValue(SfpDomFlag::TX_PWR_WARN_LOW);
    domThresh.rxPwrAlarmHigh = getSfpThreshValue(SfpDomFlag::RX_PWR_ALARM_HIGH);
    domThresh.rxPwrAlarmLow = getSfpThreshValue(SfpDomFlag::RX_PWR_ALARM_LOW);
    domThresh.rxPwrWarnHigh = getSfpThreshValue(SfpDomFlag::RX_PWR_WARN_HIGH);
    domThresh.rxPwrWarnLow = getSfpThreshValue(SfpDomFlag::RX_PWR_WARN_LOW);
    return true;
  }
  return false;
}

// Note that this is doing the same thing as getDomThresholdValuesMap()
// above, but putting the results into different data structures.

bool SfpModule::getThresholdInfo(AlarmThreshold &threshold) {
  if (!domSupport_) {
    return false;
  }
  threshold.temp.alarm.high = getSfpThreshValue(SfpDomFlag::TEMP_ALARM_HIGH);
  threshold.temp.alarm.low = getSfpThreshValue(SfpDomFlag::TEMP_ALARM_LOW);
  threshold.temp.warn.high = getSfpThreshValue(SfpDomFlag::TEMP_WARN_HIGH);
  threshold.temp.warn.low = getSfpThreshValue(SfpDomFlag::TEMP_WARN_LOW);
  threshold.vcc.alarm.high = getSfpThreshValue(SfpDomFlag::VCC_ALARM_HIGH);
  threshold.vcc.alarm.low = getSfpThreshValue(SfpDomFlag::VCC_ALARM_LOW);
  threshold.vcc.warn.high = getSfpThreshValue(SfpDomFlag::VCC_WARN_HIGH);
  threshold.vcc.warn.low = getSfpThreshValue(SfpDomFlag::VCC_WARN_LOW);
  threshold.txBias.alarm.high =
    getSfpThreshValue( SfpDomFlag::TX_BIAS_ALARM_HIGH);
  threshold.txBias.alarm.low =
    getSfpThreshValue(SfpDomFlag::TX_BIAS_ALARM_LOW);
  threshold.txBias.warn.high =
    getSfpThreshValue(SfpDomFlag::TX_BIAS_WARN_HIGH);
  threshold.txBias.warn.low =
    getSfpThreshValue(SfpDomFlag::TX_BIAS_WARN_LOW);
  threshold.txPwr.alarm.high =
    getSfpThreshValue(SfpDomFlag::TX_PWR_ALARM_HIGH);
  threshold.txPwr.alarm.low =
    getSfpThreshValue(SfpDomFlag::TX_PWR_ALARM_LOW);
  threshold.txPwr.warn.high =
    getSfpThreshValue(SfpDomFlag::TX_PWR_WARN_HIGH);
  threshold.txPwr.warn.low =
    getSfpThreshValue(SfpDomFlag::TX_PWR_WARN_LOW);
  threshold.__isset.txPwr = true;
  threshold.rxPwr.alarm.high =
    getSfpThreshValue(SfpDomFlag::RX_PWR_ALARM_HIGH);
  threshold.rxPwr.alarm.low =
    getSfpThreshValue(SfpDomFlag::RX_PWR_ALARM_LOW);
  threshold.rxPwr.warn.high =
    getSfpThreshValue(SfpDomFlag::RX_PWR_WARN_HIGH);
  threshold.rxPwr.warn.low =
    getSfpThreshValue(SfpDomFlag::RX_PWR_WARN_LOW);
  return true;
}

bool SfpModule::getDomValuesMap(SfpDomReadValue &value) {
  if (cacheIsValid() && domSupport_) {
    value.temp = getSfpDomValue(SfpDomValue::TEMP);
    value.vcc = getSfpDomValue(SfpDomValue::VCC);
    value.txBias = getSfpDomValue(SfpDomValue::TX_BIAS);
    value.txPwr = getSfpDomValue(SfpDomValue::TX_PWR);
    value.rxPwr = getSfpDomValue(SfpDomValue::RX_PWR);
    return true;
  }
  return false;
}

bool SfpModule::getSensorInfo(GlobalSensors& info) {
  if (!domSupport_) {
    return false;
  }
  info.temp.value = getSfpDomValue(SfpDomValue::TEMP);
  info.temp.flags.alarm.high = getSfpThreshFlag(SfpDomFlag::TEMP_ALARM_HIGH);
  info.temp.flags.alarm.low = getSfpThreshFlag(SfpDomFlag::TEMP_ALARM_LOW);
  info.temp.flags.warn.high = getSfpThreshFlag(SfpDomFlag::TEMP_WARN_HIGH);
  info.temp.flags.warn.low = getSfpThreshFlag(SfpDomFlag::TEMP_WARN_LOW);
  info.temp.__isset.flags = true;
  info.vcc.value = getSfpDomValue(SfpDomValue::VCC);
  info.vcc.flags.alarm.high = getSfpThreshFlag(SfpDomFlag::VCC_ALARM_HIGH);
  info.vcc.flags.alarm.low = getSfpThreshFlag(SfpDomFlag::VCC_ALARM_LOW);
  info.vcc.flags.warn.high = getSfpThreshFlag(SfpDomFlag::VCC_WARN_HIGH);
  info.vcc.flags.warn.low = getSfpThreshFlag(SfpDomFlag::VCC_WARN_LOW);
  info.vcc.__isset.flags = true;
  return true;
}

bool SfpModule::getSensorsPerChanInfo(std::vector<Channel>& channels) {
  if (!domSupport_) {
    return false;
  }

  CHECK(channels.size() == 1);
  channels[0].sensors.txBias.value = getSfpDomValue(SfpDomValue::TX_BIAS);
  channels[0].sensors.txPwr.value = getSfpDomValue(SfpDomValue::TX_PWR);
  channels[0].sensors.__isset.txPwr = true;
  channels[0].sensors.rxPwr.value = getSfpDomValue(SfpDomValue::RX_PWR);

  channels[0].sensors.txBias.flags.alarm.high =
      getSfpThreshFlag(SfpDomFlag::TX_BIAS_ALARM_HIGH);
  channels[0].sensors.txBias.flags.alarm.low =
      getSfpThreshFlag(SfpDomFlag::TX_BIAS_ALARM_LOW);
  channels[0].sensors.txBias.flags.warn.high =
      getSfpThreshFlag(SfpDomFlag::TX_BIAS_WARN_HIGH);
  channels[0].sensors.txBias.flags.warn.low =
      getSfpThreshFlag(SfpDomFlag::TX_BIAS_WARN_LOW);
  channels[0].sensors.txBias.__isset.flags = true;
  channels[0].sensors.txPwr.flags.alarm.high =
      getSfpThreshFlag(SfpDomFlag::TX_PWR_ALARM_HIGH);
  channels[0].sensors.txPwr.flags.alarm.low =
      getSfpThreshFlag(SfpDomFlag::TX_PWR_ALARM_LOW);
  channels[0].sensors.txPwr.flags.warn.high =
      getSfpThreshFlag(SfpDomFlag::TX_PWR_WARN_HIGH);
  channels[0].sensors.txPwr.flags.warn.low =
      getSfpThreshFlag(SfpDomFlag::TX_PWR_WARN_LOW);
  channels[0].sensors.txPwr.__isset.flags = true;
  channels[0].sensors.rxPwr.flags.alarm.high =
      getSfpThreshFlag(SfpDomFlag::RX_PWR_ALARM_HIGH);
  channels[0].sensors.rxPwr.flags.alarm.low =
      getSfpThreshFlag(SfpDomFlag::RX_PWR_ALARM_LOW);
  channels[0].sensors.rxPwr.flags.warn.high =
      getSfpThreshFlag(SfpDomFlag::RX_PWR_WARN_HIGH);
  channels[0].sensors.rxPwr.flags.warn.low =
      getSfpThreshFlag(SfpDomFlag::RX_PWR_WARN_LOW);
  channels[0].sensors.rxPwr.__isset.flags = true;
  return true;
}


bool SfpModule::getVendorInfo(Vendor &vendor) {
  if (cacheIsValid()) {
    vendor.name = getSfpString(SfpIdpromFields::VENDOR_NAME);
    vendor.oui = getSfpString(SfpIdpromFields::VENDOR_OUI);
    vendor.partNumber = getSfpString(SfpIdpromFields::PART_NUMBER);
    vendor.rev = getSfpString(SfpIdpromFields::REVISION_NUMBER);
    vendor.serialNumber = getSfpString(SfpIdpromFields::VENDOR_SERIAL_NUMBER);
    vendor.dateCode = getSfpString(SfpIdpromFields::MFG_DATE);
    return true;
  }
  return false;
}

bool SfpModule::getCableInfo(Cable &cable) {
  cable.singleModeKm = getSfpCableLength(SfpIdpromFields::LENGTH_SM_KM, 1000);
  cable.__isset.singleMode = (cable.singleModeKm != 0);
  cable.singleMode = getSfpCableLength(SfpIdpromFields::LENGTH_SM, 100);
  cable.__isset.singleMode = (cable.singleMode != 0);
  cable.om3 = getSfpCableLength(SfpIdpromFields::LENGTH_OM3, 10);
  cable.__isset.om3 = (cable.om3 != 0);
  cable.om2 = getSfpCableLength(SfpIdpromFields::LENGTH_OM2, 10);
  cable.__isset.om2 = (cable.om2 != 0);
  cable.om1 = getSfpCableLength(SfpIdpromFields::LENGTH_OM1, 10);
  cable.__isset.om1 = (cable.om1 != 0);
  // XXX:  Note that copper and OM4 use different multipliers, but
  //       it isn't clear how we distinguish them.  Need to dive further
  //       into the SFP spec.
  cable.copper = getSfpCableLength(SfpIdpromFields::LENGTH_COPPER, 1);
  cable.__isset.copper = (cable.copper != 0);
  return (cable.__isset.copper || cable.__isset.om1 || cable.__isset.om2 ||
          cable.__isset.om3 || cable.__isset.singleMode ||
          cable.__isset.singleModeKm);
}

/*
 * Cable length is report as a single byte;  each field has a
 * specific multiplier to use to get the true length.  For instance,
 * single mode fiber length is specified in km, so the multiplier
 * is 1000.  In addition, the raw value of 255 indicates that the
 * cable is longer than can be represented.  We use a negative
 * value of the appropriate magnitude to communicate that to thrift
 * clients.
 */

int SfpModule::getSfpCableLength(const SfpIdpromFields field,
                                  int multiplier) {
  int length;
  auto sfpFieldInfo = sfpFields.find(field);
  if (sfpFieldInfo == sfpFields.end()) {
    throw FbossError("Invalid SFP Field ID");
  }
  auto info = sfpFieldInfo->second;
  const uint8_t *data = getSfpValuePtr(info.dataAddress,
                                       info.offset, info.length);
  length = *data * multiplier;
  if (*data == CABLE_MAX_LEN) {
    length = -(CABLE_MAX_LEN - 1) * multiplier;
  }
  return length;
}

std::string SfpModule::getSfpString(const SfpIdpromFields flag) {
  int offset, dataAddress, length;
  getSfpFieldAddress(flag, dataAddress,
                      offset, length);
  const uint8_t *data = getSfpValuePtr(dataAddress, offset, length);

  while (length > 0 && data[length - 1] == ' ') {
    --length;
  }
  return std::string(reinterpret_cast<const char*>(data), length);
}

bool SfpModule::getSfpThreshFlag(const SfpDomFlag flag) {
  uint8_t data[10];
  int offset, dataAddress, length;
  getSfpFieldAddress(SfpIdpromFields::ALARM_WARN_FLAGS, dataAddress,
                      offset, length);
  getSfpValue(dataAddress, offset, length, data);
  return getSfpFlagIdProm(flag, data);
}

float SfpModule::getSfpThreshValue(const SfpDomFlag flag) {
  uint8_t data[40];
  int offset, dataAddress, length;
  uint8_t msb, lsb;
  int idx = (static_cast<int>(flag) -
                            static_cast<int>(SfpDomFlag::TEMP_ALARM_HIGH)) * 2;
  uint16_t rawValue;
  getSfpFieldAddress(SfpIdpromFields::ALARM_THRESHOLD_VALUES, dataAddress,
                      offset, length);
  getSfpValue(dataAddress, offset, length, data);
  msb = data[idx];
  lsb = data[idx + 1];
  rawValue = (msb << 8) | lsb;
  return getValueFromRaw(flag, rawValue);
}

float SfpModule::getSfpDomValue(const SfpDomValue field) {
  uint16_t temp;
  uint8_t data[10];
  int offset, dataAddress, length;
  getSfpFieldAddress(SfpIdpromFields::DIAGNOSTICS, dataAddress,
                      offset, length);
  getSfpValue(dataAddress, offset, length, data);
  switch (field) {
    case SfpDomValue::TEMP:
      temp = (data[0] << 8) | data[1];
      return getTemp(temp);
    case SfpDomValue::VCC:
      temp = (data[2] << 8) | data[3];
      return getVcc(temp);
    case SfpDomValue::TX_BIAS:
      temp = (data[4] << 8) | data[5];
      return getTxBias(temp);
    case SfpDomValue::TX_PWR:
      temp = (data[6] << 8) | data[7];
      return getPwr(temp);
    case SfpDomValue::RX_PWR:
      temp = (data[8] << 8) | data[9];
      return getPwr(temp);
    default:
      throw FbossError("Unknown DOM value field");
  }
}

SfpModule::SfpModule(std::unique_ptr<TransceiverImpl> sfpImpl)
  : sfpImpl_(std::move(sfpImpl)) {
  present_ = false;
  dirty_ = true;
  domSupport_ = false;
}

void SfpModule::setSfpIdprom(const uint8_t* data) {
  if (!present_) {
    throw FbossError("Sfp IDProm set failed as SFP is not present");
  }
  memcpy(sfpIdprom_, data, sizeof(sfpIdprom_));
  /* set the DOM supported flag */
  setDomSupport();
}

void SfpModule::setDomSupport() {
  uint8_t data;
  int offset, dataAddress, length;
  getSfpFieldAddress(SfpIdpromFields::DIAGNOSTIC_MONITORING_TYPE,
                                 dataAddress, offset, length);
  getSfpValue(dataAddress, offset, length, &data);
  /* bit 7 and 6 needs to be checked for DOM */
  if ((data & (1 << 7)) || (data & (1 << 6)) == 0) {
    domSupport_ = false;
  } else if (data & (1 << 2)) {
    /* TODO(klahey):  bit 2 indicates that we need an address change
     * operation, which we have not yet implemented.
     */
    domSupport_ = false;
    LOG(ERROR) << "Port: " << folly::to<std::string>(sfpImpl_->getName()) <<
      " could not read SFP DOM info, need address change support";
  } else {
    domSupport_ = true;
  }
}

bool SfpModule::isDomSupported() const {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  return domSupport_;
}

const uint8_t* SfpModule::getSfpValuePtr(int dataAddress, int offset,
                                         int length) const {
  /* if the cached values are not correct */
  if (!cacheIsValid()) {
    throw FbossError("Sfp is either not present or the data is not read");
  }
  if (dataAddress == 0x50) {
    CHECK_LE(offset + length, sizeof(sfpIdprom_));
    /* Copy data from the cache */
    return(sfpIdprom_ + offset);
  } else if (dataAddress == 0x51) {
    CHECK_LE(offset + length, sizeof(sfpDom_));
    /* Copy data from the cache */
    return(sfpDom_ + offset);
  } else {
    throw FbossError("Invalid Data Address 0x%d", dataAddress);
  }
}

void SfpModule::getSfpValue(int dataAddress,
                            int offset, int length, uint8_t* data) const {
  const uint8_t *ptr = getSfpValuePtr(dataAddress, offset, length);

  memcpy(data, ptr, length);
}

// Note that this needs to be called while holding the
// sfpModuleMutex_
bool SfpModule::cacheIsValid() const {
  return present_ && !dirty_;
}

bool SfpModule::isPresent() const {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  return present_;
}

void SfpModule::setPresent(bool present) {
  present_ = present;
  /* Set the dirty bit as the SFP was removed and
   * the cached data is no longer valid until next
   * set IDProm is called
   */
  if (present_ == false) {
    dirty_ = true;
    domSupport_ = false;
  }
}

void SfpModule::setSfpDom(const uint8_t* data) {
  if (!domSupport_) {
    throw FbossError("Dom not supported cannot set the values");
  }
  memcpy(sfpDom_, data, sizeof(sfpDom_));
}

bool SfpModule::getSfpFlagIdProm(const SfpDomFlag flag, const uint8_t* data) {
  auto alarmBit = getSfpDomBit(flag);
  int offset = 5 - (alarmBit / 8);
  int bit = (alarmBit % 8);
  if (data[offset] & (1 << bit)) {
    return true;
  }
  return false;
}

void SfpModule::getSfpDom(SfpDom &dom) {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  dom.name = folly::to<std::string>(sfpImpl_->getName());
  dom.sfpPresent = present_;
  dom.domSupported = domSupport_;
  if (getDomFlagsMap(dom.flags)) {
    dom.__isset.flags = true;
  }
  if (getDomThresholdValuesMap(dom.threshValue)) {
    dom.__isset.threshValue = true;
  }
  if (getDomValuesMap(dom.value)) {
    dom.__isset.value = true;
  }
  if (getVendorInfo(dom.vendor)) {
    dom.__isset.vendor = true;
  }
}

void SfpModule::getTransceiverInfo(TransceiverInfo &info) {
  lock_guard<std::mutex> g(sfpModuleMutex_);

  info.present = present_;
  info.transceiver = type();
  info.port = sfpImpl_->getNum();
  if (!cacheIsValid()) {
    return;
  }
  if (getSensorInfo(info.sensor)) {
    info.__isset.sensor = true;
  }
  if (getVendorInfo(info.vendor)) {
    info.__isset.vendor = true;
  }
  if (getCableInfo(info.cable)) {
    info.__isset.cable = true;
  }
  if (getThresholdInfo(info.thresholds)) {
    info.__isset.thresholds = true;
  }

  Channel chan;
  chan.channel = 0;
  info.channels.push_back(chan);
  if (!getSensorsPerChanInfo(info.channels)) {
    info.channels.clear();
  }
}

float SfpModule::getValueFromRaw(const SfpDomFlag key, uint16_t value) {
  switch (key) {
    case SfpDomFlag::TEMP_ALARM_HIGH:
    case SfpDomFlag::TEMP_ALARM_LOW:
    case SfpDomFlag::TEMP_WARN_HIGH:
    case SfpDomFlag::TEMP_WARN_LOW:
      return getTemp(value);
    case SfpDomFlag::VCC_ALARM_HIGH:
    case SfpDomFlag::VCC_ALARM_LOW:
    case SfpDomFlag::VCC_WARN_HIGH:
    case SfpDomFlag::VCC_WARN_LOW:
      return getVcc(value);
    case SfpDomFlag::TX_BIAS_ALARM_HIGH:
    case SfpDomFlag::TX_BIAS_ALARM_LOW:
    case SfpDomFlag::TX_BIAS_WARN_HIGH:
    case SfpDomFlag::TX_BIAS_WARN_LOW:
      return getTxBias(value);
    case SfpDomFlag::TX_PWR_ALARM_HIGH:
    case SfpDomFlag::TX_PWR_ALARM_LOW:
    case SfpDomFlag::TX_PWR_WARN_HIGH:
    case SfpDomFlag::TX_PWR_WARN_LOW:
    case SfpDomFlag::RX_PWR_ALARM_HIGH:
    case SfpDomFlag::RX_PWR_ALARM_LOW:
    case SfpDomFlag::RX_PWR_WARN_HIGH:
    case SfpDomFlag::RX_PWR_WARN_LOW:
      return getPwr(value);
    default:
      return 0.0;
  }
}

float SfpModule::getTemp(const uint16_t temp) {
  float data;
  data = temp / 256;
  if (data > 128) {
    data = data - 256;
  }
  return data;
}

float SfpModule::getVcc(const uint16_t temp) {
  float data;
  data = temp / 10000;
  return data;
}

float SfpModule::getTxBias(const uint16_t temp) {
  float data;
  data = temp * 2 / 1000;
  return data;
}

float SfpModule::getPwr(const uint16_t temp) {
  float data;
  data = temp * 0.1 / 1000;
  return data;
}

void SfpModule::detectTransceiver() {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  uint8_t value[MAX_SFP_EEPROM_SIZE];
  auto currentSfpStatus = sfpImpl_->detectTransceiver();
  if (currentSfpStatus != present_) {
    LOG(INFO) << "Port: " << folly::to<std::string>(sfpImpl_->getName()) <<
                  " SFP status changed to " << currentSfpStatus;
    setPresent(currentSfpStatus);
    if (currentSfpStatus) {
      try {
        sfpImpl_->readTransceiver(0x50, 0x0, MAX_SFP_EEPROM_SIZE, value);
        dirty_ = false;
        setSfpIdprom(value);
        if (domSupport_) {
          sfpImpl_->readTransceiver(0x51, 0x0, MAX_SFP_EEPROM_SIZE, value);
          setSfpDom(value);
        }
      } catch (const std::exception& ex) {
        dirty_ = true;
        LOG(ERROR) << "Error parsing or reading SFP data for port: " <<
             folly::to<std::string>(sfpImpl_->getName());
      }
    }
  }
}

int SfpModule::getFieldValue(SfpIdpromFields fieldName,
                             uint8_t* fieldValue) {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  int dataAddress, offset, length, rc;
  /* Determine if SFP is present */
  if (cacheIsValid()) {
    try {
      getSfpFieldAddress(fieldName, dataAddress, offset, length);
      getSfpValue(dataAddress, offset, length, fieldValue);
    } catch (const std::exception& ex) {
      LOG(ERROR) << "Error reading field value for port: " <<
                folly::to<std::string>(sfpImpl_->getName()) << " " << ex.what();
    }
  }
  return -1;
}

void SfpModule::updateTransceiverInfoFields() {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  uint8_t value[MAX_SFP_EEPROM_SIZE];
  if (cacheIsValid() && domSupport_) {
    sfpImpl_->readTransceiver(0x51, 0x0, MAX_SFP_EEPROM_SIZE, value);
    setSfpDom(value);
  }
}

}} //namespace facebook::fboss
