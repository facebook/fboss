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

#include <boost/assign.hpp>
#include <string>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/TransceiverImpl.h"
#include "fboss/agent/SffFieldInfo.h"

namespace facebook { namespace fboss {
  using std::memcpy;
  using std::mutex;
  using std::lock_guard;

static SffFieldInfo::SffFieldMap sfpFields = {
  /* 0xA0 EEPROM Field Values */
  {SffField::IDENTIFIER, {0x50, 0x0, 1} },
  {SffField::EXT_IDENTIFIER, {0x50, 0x1, 1} },
  {SffField::CONNECTOR_TYPE, {0x50, 0x2, 1} },
  {SffField::TRANSCEIVER_CODE, {0x50, 0x3, 8} },
  {SffField::ENCODING_CODE, {0x50, 0xB, 1} },
  {SffField::SIGNALLING_RATE, {0x50, 0xC, 1} },
  {SffField::RATE_IDENTIFIER, {0x50, 0xD, 1} },
  {SffField::LENGTH_SM_KM, {0x50, 0xE, 1} },
  {SffField::LENGTH_SM, {0x50, 0xF, 1} },
  {SffField::LENGTH_OM2, {0x50, 0x10, 1} },
  {SffField::LENGTH_OM1, {0x50, 0x11, 1} },
  {SffField::LENGTH_COPPER, {0x50, 0x12, 1} },
  {SffField::LENGTH_OM3, {0x50, 0x13, 1} },
  {SffField::VENDOR_NAME, {0x50, 0x14, 16} },
  {SffField::TRANCEIVER_CAPABILITY, {0x50, 0x24, 1} },
  {SffField::VENDOR_OUI, {0x50, 0x25, 3} },
  {SffField::PART_NUMBER, {0x50, 0x28, 16} },
  {SffField::REVISION_NUMBER, {0x50, 0x38, 4} },
  {SffField::WAVELENGTH, {0x50, 0x3C, 2} },
  {SffField::CHECK_CODE_BASEID, {0x50, 0x3F, 1} },
  {SffField::ENABLED_OPTIONS, {0x50, 0x40, 2} },
  {SffField::UPPER_BIT_RATE_MARGIN, {0x50, 0x42, 1} },
  {SffField::LOWER_BIT_RATE_MARGIN, {0x50, 0x43, 1} },
  {SffField::VENDOR_SERIAL_NUMBER, {0x50, 0x44, 16} },
  {SffField::MFG_DATE, {0x50, 0x54, 8} },
  {SffField::DIAGNOSTIC_MONITORING_TYPE, {0x50, 0x5C, 1} },
  {SffField::ENHANCED_OPTIONS, {0x50, 0x5D, 1} },
  {SffField::SFF_COMPLIANCE, {0x50, 0x5E, 1} },
  {SffField::CHECK_CODE_EXTENDED_OPT, {0x50, 0x5F, 1} },
  {SffField::VENDOR_EEPROM, {0x50, 0x60, 32} },
   /* 0xA2 EEPROM Field Values */
  {SffField::ALARM_THRESHOLD_VALUES, {0x51, 0x0, 40} },
  {SffField::EXTERNAL_CALIBRATION, {0x51, 0x38, 36} },
  {SffField::CHECK_CODE_DMI, {0x51, 0x5F, 1} },
  {SffField::DIAGNOSTICS, {0x51, 0x60, 10} },
  {SffField::STATUS_CONTROL, {0x51, 0x6E, 1} },
  {SffField::ALARM_WARN_FLAGS, {0x51, 0x70, 6} },
  {SffField::EXTENDED_STATUS_CONTROL, {0x51, 0x76, 2} },
  {SffField::VENDOR_MEM_ADDRESS, {0x51, 0x78, 8} },
  {SffField::USER_EEPROM, {0x51, 0x80, 120} },
  {SffField::VENDOR_CONTROL, {0x51, 0xF8, 8} },
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

static void getSfpFieldAddress(SffField field, int &dataAddress,
                               int &offset, int &length) {
  auto sfpFieldInfo = sfpFields.find(field);
  if (sfpFieldInfo == sfpFields.end()) {
    throw FbossError("Invalid SFP Field ID");
  }
  dataAddress = sfpFieldInfo->second.dataAddress;
  offset = sfpFieldInfo->second.offset;
  length = sfpFieldInfo->second.length;
}

static int getSfpDomBit(const SfpDomFlag flag) {
  auto domFlag = sfpDomFlag.find(flag);
  if (domFlag == sfpDomFlag.end()) {
    throw FbossError("Invalid SFP Field ID");
  }
  return domFlag->second;
}

bool SfpModule::getDomFlagsMap(lock_guard<std::mutex>& lg,
                                SfpDomThreshFlags &domFlags) {
  if (cacheIsValid(lg) && domSupport_) {
    domFlags.tempAlarmHigh = getSfpThreshFlag(lg, SfpDomFlag::TEMP_ALARM_HIGH);
    domFlags.tempAlarmLow = getSfpThreshFlag(lg, SfpDomFlag::TEMP_ALARM_LOW);
    domFlags.tempWarnHigh = getSfpThreshFlag(lg, SfpDomFlag::TEMP_WARN_HIGH);
    domFlags.tempWarnLow = getSfpThreshFlag(lg, SfpDomFlag::TEMP_WARN_LOW);
    domFlags.vccAlarmHigh = getSfpThreshFlag(lg, SfpDomFlag::VCC_ALARM_HIGH);
    domFlags.vccAlarmLow = getSfpThreshFlag(lg, SfpDomFlag::VCC_ALARM_LOW);
    domFlags.vccWarnHigh = getSfpThreshFlag(lg, SfpDomFlag::VCC_WARN_HIGH);
    domFlags.vccWarnLow = getSfpThreshFlag(lg, SfpDomFlag::VCC_WARN_LOW);
    domFlags.txBiasAlarmHigh = getSfpThreshFlag(lg,
                                              SfpDomFlag::TX_BIAS_ALARM_HIGH);
    domFlags.txBiasAlarmLow = getSfpThreshFlag(lg,
                                              SfpDomFlag::TX_BIAS_ALARM_LOW);
    domFlags.txBiasWarnHigh = getSfpThreshFlag(lg,
                                              SfpDomFlag::TX_BIAS_WARN_HIGH);
    domFlags.txBiasWarnLow = getSfpThreshFlag(lg,
                                              SfpDomFlag::TX_BIAS_WARN_LOW);
    domFlags.txPwrAlarmHigh = getSfpThreshFlag(lg,
                                              SfpDomFlag::TX_PWR_ALARM_HIGH);
    domFlags.txPwrAlarmLow = getSfpThreshFlag(lg, SfpDomFlag::TX_PWR_ALARM_LOW);
    domFlags.txPwrWarnHigh = getSfpThreshFlag(lg, SfpDomFlag::TX_PWR_WARN_HIGH);
    domFlags.txPwrWarnLow = getSfpThreshFlag(lg, SfpDomFlag::TX_PWR_WARN_LOW);
    domFlags.rxPwrAlarmHigh = getSfpThreshFlag(lg,
                                                SfpDomFlag::RX_PWR_ALARM_HIGH);
    domFlags.rxPwrAlarmLow = getSfpThreshFlag(lg,
                                              SfpDomFlag::RX_PWR_ALARM_LOW);
    domFlags.rxPwrWarnHigh = getSfpThreshFlag(lg,
                                              SfpDomFlag::RX_PWR_WARN_HIGH);
    domFlags.rxPwrWarnLow = getSfpThreshFlag(lg, SfpDomFlag::RX_PWR_WARN_LOW);
    return true;
  }
  return false;
}

bool SfpModule::getDomThresholdValuesMap(lock_guard<std::mutex>& lg,
                                          SfpDomThreshValue &domThresh) {
  if (cacheIsValid(lg) && domSupport_) {
    domThresh.tempAlarmHigh = getSfpThreshValue(lg,
                                                SfpDomFlag::TEMP_ALARM_HIGH);
    domThresh.tempAlarmLow = getSfpThreshValue(lg, SfpDomFlag::TEMP_ALARM_LOW);
    domThresh.tempWarnHigh = getSfpThreshValue(lg, SfpDomFlag::TEMP_WARN_HIGH);
    domThresh.tempWarnLow = getSfpThreshValue(lg, SfpDomFlag::TEMP_WARN_LOW);
    domThresh.vccAlarmHigh = getSfpThreshValue(lg, SfpDomFlag::VCC_ALARM_HIGH);
    domThresh.vccAlarmLow = getSfpThreshValue(lg, SfpDomFlag::VCC_ALARM_LOW);
    domThresh.vccWarnHigh = getSfpThreshValue(lg, SfpDomFlag::VCC_WARN_HIGH);
    domThresh.vccWarnLow = getSfpThreshValue(lg, SfpDomFlag::VCC_WARN_LOW);
    domThresh.txBiasAlarmHigh = getSfpThreshValue(lg,
                                                SfpDomFlag::TX_BIAS_ALARM_HIGH);
    domThresh.txBiasAlarmLow = getSfpThreshValue(lg,
                                                SfpDomFlag::TX_BIAS_ALARM_LOW);
    domThresh.txBiasWarnHigh = getSfpThreshValue(lg,
                                                SfpDomFlag::TX_BIAS_WARN_HIGH);
    domThresh.txBiasWarnLow = getSfpThreshValue(lg,
                                                SfpDomFlag::TX_BIAS_WARN_LOW);
    domThresh.txPwrAlarmHigh = getSfpThreshValue(lg,
                                                SfpDomFlag::TX_PWR_ALARM_HIGH);
    domThresh.txPwrAlarmLow = getSfpThreshValue(lg,
                                                SfpDomFlag::TX_PWR_ALARM_LOW);
    domThresh.txPwrWarnHigh = getSfpThreshValue(lg,
                                                SfpDomFlag::TX_PWR_WARN_HIGH);
    domThresh.txPwrWarnLow = getSfpThreshValue(lg,
                                                  SfpDomFlag::TX_PWR_WARN_LOW);
    domThresh.rxPwrAlarmHigh = getSfpThreshValue(lg,
                                                SfpDomFlag::RX_PWR_ALARM_HIGH);
    domThresh.rxPwrAlarmLow = getSfpThreshValue(lg,
                                                SfpDomFlag::RX_PWR_ALARM_LOW);
    domThresh.rxPwrWarnHigh = getSfpThreshValue(lg,
                                                SfpDomFlag::RX_PWR_WARN_HIGH);
    domThresh.rxPwrWarnLow = getSfpThreshValue(lg, SfpDomFlag::RX_PWR_WARN_LOW);
    return true;
  }
  return false;
}

// Note that this is doing the same thing as getDomThresholdValuesMap()
// above, but putting the results into different data structures.

bool SfpModule::getThresholdInfo(lock_guard<std::mutex>& lg,
                                  AlarmThreshold &threshold) {
  if (!domSupport_) {
    return false;
  }
  threshold.temp.alarm.high = getSfpThreshValue(lg,
                                                SfpDomFlag::TEMP_ALARM_HIGH);
  threshold.temp.alarm.low = getSfpThreshValue(lg, SfpDomFlag::TEMP_ALARM_LOW);
  threshold.temp.warn.high = getSfpThreshValue(lg, SfpDomFlag::TEMP_WARN_HIGH);
  threshold.temp.warn.low = getSfpThreshValue(lg, SfpDomFlag::TEMP_WARN_LOW);
  threshold.vcc.alarm.high = getSfpThreshValue(lg, SfpDomFlag::VCC_ALARM_HIGH);
  threshold.vcc.alarm.low = getSfpThreshValue(lg, SfpDomFlag::VCC_ALARM_LOW);
  threshold.vcc.warn.high = getSfpThreshValue(lg, SfpDomFlag::VCC_WARN_HIGH);
  threshold.vcc.warn.low = getSfpThreshValue(lg, SfpDomFlag::VCC_WARN_LOW);
  threshold.txBias.alarm.high =
    getSfpThreshValue(lg, SfpDomFlag::TX_BIAS_ALARM_HIGH);
  threshold.txBias.alarm.low =
    getSfpThreshValue(lg, SfpDomFlag::TX_BIAS_ALARM_LOW);
  threshold.txBias.warn.high =
    getSfpThreshValue(lg, SfpDomFlag::TX_BIAS_WARN_HIGH);
  threshold.txBias.warn.low =
    getSfpThreshValue(lg, SfpDomFlag::TX_BIAS_WARN_LOW);
  threshold.txPwr.alarm.high =
    getSfpThreshValue(lg, SfpDomFlag::TX_PWR_ALARM_HIGH);
  threshold.txPwr.alarm.low =
    getSfpThreshValue(lg, SfpDomFlag::TX_PWR_ALARM_LOW);
  threshold.txPwr.warn.high =
    getSfpThreshValue(lg, SfpDomFlag::TX_PWR_WARN_HIGH);
  threshold.txPwr.warn.low =
    getSfpThreshValue(lg, SfpDomFlag::TX_PWR_WARN_LOW);
  threshold.__isset.txPwr = true;
  threshold.rxPwr.alarm.high =
    getSfpThreshValue(lg, SfpDomFlag::RX_PWR_ALARM_HIGH);
  threshold.rxPwr.alarm.low =
    getSfpThreshValue(lg, SfpDomFlag::RX_PWR_ALARM_LOW);
  threshold.rxPwr.warn.high =
    getSfpThreshValue(lg, SfpDomFlag::RX_PWR_WARN_HIGH);
  threshold.rxPwr.warn.low =
    getSfpThreshValue(lg, SfpDomFlag::RX_PWR_WARN_LOW);
  return true;
}

bool SfpModule::getDomValuesMap(lock_guard<std::mutex>& lg,
                                SfpDomReadValue &value) {
  if (cacheIsValid(lg) && domSupport_) {
    value.temp = getSfpDomValue(lg, SfpDomValue::TEMP);
    value.vcc = getSfpDomValue(lg, SfpDomValue::VCC);
    value.txBias = getSfpDomValue(lg, SfpDomValue::TX_BIAS);
    value.txPwr = getSfpDomValue(lg, SfpDomValue::TX_PWR);
    value.rxPwr = getSfpDomValue(lg, SfpDomValue::RX_PWR);
    return true;
  }
  return false;
}

bool SfpModule::getSensorInfo(lock_guard<std::mutex>& lg,
                              GlobalSensors& info) {
  if (!domSupport_) {
    return false;
  }
  info.temp.value = getSfpDomValue(lg, SfpDomValue::TEMP);
  info.temp.flags.alarm.high = getSfpThreshFlag(lg,
                                                SfpDomFlag::TEMP_ALARM_HIGH);
  info.temp.flags.alarm.low = getSfpThreshFlag(lg, SfpDomFlag::TEMP_ALARM_LOW);
  info.temp.flags.warn.high = getSfpThreshFlag(lg, SfpDomFlag::TEMP_WARN_HIGH);
  info.temp.flags.warn.low = getSfpThreshFlag(lg, SfpDomFlag::TEMP_WARN_LOW);
  info.temp.__isset.flags = true;
  info.vcc.value = getSfpDomValue(lg, SfpDomValue::VCC);
  info.vcc.flags.alarm.high = getSfpThreshFlag(lg, SfpDomFlag::VCC_ALARM_HIGH);
  info.vcc.flags.alarm.low = getSfpThreshFlag(lg, SfpDomFlag::VCC_ALARM_LOW);
  info.vcc.flags.warn.high = getSfpThreshFlag(lg, SfpDomFlag::VCC_WARN_HIGH);
  info.vcc.flags.warn.low = getSfpThreshFlag(lg, SfpDomFlag::VCC_WARN_LOW);
  info.vcc.__isset.flags = true;
  return true;
}

bool SfpModule::getSensorsPerChanInfo(lock_guard<std::mutex>& lg,
                                      std::vector<Channel>& channels) {
  if (!domSupport_) {
    return false;
  }

  CHECK(channels.size() == 1);
  channels[0].sensors.txBias.value = getSfpDomValue(lg, SfpDomValue::TX_BIAS);
  channels[0].sensors.txPwr.value = getSfpDomValue(lg, SfpDomValue::TX_PWR);
  channels[0].sensors.__isset.txPwr = true;
  channels[0].sensors.rxPwr.value = getSfpDomValue(lg, SfpDomValue::RX_PWR);

  channels[0].sensors.txBias.flags.alarm.high =
      getSfpThreshFlag(lg, SfpDomFlag::TX_BIAS_ALARM_HIGH);
  channels[0].sensors.txBias.flags.alarm.low =
      getSfpThreshFlag(lg, SfpDomFlag::TX_BIAS_ALARM_LOW);
  channels[0].sensors.txBias.flags.warn.high =
      getSfpThreshFlag(lg, SfpDomFlag::TX_BIAS_WARN_HIGH);
  channels[0].sensors.txBias.flags.warn.low =
      getSfpThreshFlag(lg, SfpDomFlag::TX_BIAS_WARN_LOW);
  channels[0].sensors.txBias.__isset.flags = true;
  channels[0].sensors.txPwr.flags.alarm.high =
      getSfpThreshFlag(lg, SfpDomFlag::TX_PWR_ALARM_HIGH);
  channels[0].sensors.txPwr.flags.alarm.low =
      getSfpThreshFlag(lg, SfpDomFlag::TX_PWR_ALARM_LOW);
  channels[0].sensors.txPwr.flags.warn.high =
      getSfpThreshFlag(lg, SfpDomFlag::TX_PWR_WARN_HIGH);
  channels[0].sensors.txPwr.flags.warn.low =
      getSfpThreshFlag(lg, SfpDomFlag::TX_PWR_WARN_LOW);
  channels[0].sensors.txPwr.__isset.flags = true;
  channels[0].sensors.rxPwr.flags.alarm.high =
      getSfpThreshFlag(lg, SfpDomFlag::RX_PWR_ALARM_HIGH);
  channels[0].sensors.rxPwr.flags.alarm.low =
      getSfpThreshFlag(lg, SfpDomFlag::RX_PWR_ALARM_LOW);
  channels[0].sensors.rxPwr.flags.warn.high =
      getSfpThreshFlag(lg, SfpDomFlag::RX_PWR_WARN_HIGH);
  channels[0].sensors.rxPwr.flags.warn.low =
      getSfpThreshFlag(lg, SfpDomFlag::RX_PWR_WARN_LOW);
  channels[0].sensors.rxPwr.__isset.flags = true;
  return true;
}


bool SfpModule::getVendorInfo(lock_guard<std::mutex>& lg, Vendor &vendor) {
  if (cacheIsValid(lg)) {
    vendor.name = getSfpString(lg, SffField::VENDOR_NAME);
    vendor.oui = getSfpString(lg, SffField::VENDOR_OUI);
    vendor.partNumber = getSfpString(lg, SffField::PART_NUMBER);
    vendor.rev = getSfpString(lg, SffField::REVISION_NUMBER);
    vendor.serialNumber = getSfpString(lg, SffField::VENDOR_SERIAL_NUMBER);
    vendor.dateCode = getSfpString(lg, SffField::MFG_DATE);
    return true;
  }
  return false;
}

bool SfpModule::getCableInfo(lock_guard<std::mutex>& lg, Cable &cable) {
  cable.singleModeKm = getSfpCableLength(lg, SffField::LENGTH_SM_KM, 1000);
  cable.__isset.singleMode = (cable.singleModeKm != 0);
  cable.singleMode = getSfpCableLength(lg, SffField::LENGTH_SM, 100);
  cable.__isset.singleMode = (cable.singleMode != 0);
  cable.om3 = getSfpCableLength(lg, SffField::LENGTH_OM3, 10);
  cable.__isset.om3 = (cable.om3 != 0);
  cable.om2 = getSfpCableLength(lg, SffField::LENGTH_OM2, 10);
  cable.__isset.om2 = (cable.om2 != 0);
  cable.om1 = getSfpCableLength(lg, SffField::LENGTH_OM1, 10);
  cable.__isset.om1 = (cable.om1 != 0);
  // XXX:  Note that copper and OM4 use different multipliers, but
  //       it isn't clear how we distinguish them.  Need to dive further
  //       into the SFP spec.
  cable.copper = getSfpCableLength(lg, SffField::LENGTH_COPPER, 1);
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

int SfpModule::getSfpCableLength(lock_guard<std::mutex>& lg,
                                  const SffField field, int multiplier) {
  int length;
  auto sfpFieldInfo = sfpFields.find(field);
  if (sfpFieldInfo == sfpFields.end()) {
    throw FbossError("Invalid SFP Field ID");
  }
  auto info = sfpFieldInfo->second;
  const uint8_t *data = getSfpValuePtr(lg, info.dataAddress,
                                       info.offset, info.length);
  length = *data * multiplier;
  if (*data == CABLE_MAX_LEN) {
    length = -(CABLE_MAX_LEN - 1) * multiplier;
  }
  return length;
}

std::string SfpModule::getSfpString(lock_guard<std::mutex>& lg,
                                    const SffField flag) {
  int offset, dataAddress, length;
  getSfpFieldAddress(flag, dataAddress,
                      offset, length);
  const uint8_t *data = getSfpValuePtr(lg, dataAddress, offset, length);

  while (length > 0 && data[length - 1] == ' ') {
    --length;
  }
  return std::string(reinterpret_cast<const char*>(data), length);
}

bool SfpModule::getSfpThreshFlag(lock_guard<std::mutex>& lg,
                                  const SfpDomFlag flag) {
  uint8_t data[10];
  int offset, dataAddress, length;
  getSfpFieldAddress(SffField::ALARM_WARN_FLAGS, dataAddress,
                      offset, length);
  getSfpValue(lg, dataAddress, offset, length, data);
  return getSfpFlagIdProm(lg, flag, data);
}

float SfpModule::getSfpThreshValue(lock_guard<std::mutex>& lg,
                                    const SfpDomFlag flag) {
  uint8_t data[40];
  int offset, dataAddress, length;
  uint8_t msb, lsb;
  int idx = (static_cast<int>(flag) -
                            static_cast<int>(SfpDomFlag::TEMP_ALARM_HIGH)) * 2;
  uint16_t rawValue;
  getSfpFieldAddress(SffField::ALARM_THRESHOLD_VALUES, dataAddress,
                      offset, length);
  getSfpValue(lg, dataAddress, offset, length, data);
  msb = data[idx];
  lsb = data[idx + 1];
  rawValue = (msb << 8) | lsb;
  return getValueFromRaw(lg, flag, rawValue);
}

float SfpModule::getSfpDomValue(lock_guard<std::mutex>& lg,
                                const SfpDomValue field) {
  uint16_t temp;
  uint8_t data[10];
  int offset, dataAddress, length;
  getSfpFieldAddress(SffField::DIAGNOSTICS, dataAddress,
                      offset, length);
  getSfpValue(lg, dataAddress, offset, length, data);
  switch (field) {
    case SfpDomValue::TEMP:
      temp = (data[0] << 8) | data[1];
      return SffFieldInfo::getTemp(temp);
    case SfpDomValue::VCC:
      temp = (data[2] << 8) | data[3];
      return SffFieldInfo::getVcc(temp);
    case SfpDomValue::TX_BIAS:
      temp = (data[4] << 8) | data[5];
      return SffFieldInfo::getTxBias(temp);
    case SfpDomValue::TX_PWR:
      temp = (data[6] << 8) | data[7];
      return SffFieldInfo::getPwr(temp);
    case SfpDomValue::RX_PWR:
      temp = (data[8] << 8) | data[9];
      return SffFieldInfo::getPwr(temp);
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

void SfpModule::setSfpIdprom(lock_guard<std::mutex>& lg, const uint8_t* data) {
  if (!present_) {
    throw FbossError("Sfp IDProm set failed as SFP is not present");
  }
  memcpy(sfpIdprom_, data, sizeof(sfpIdprom_));
  /* set the DOM supported flag */
  setDomSupport(lg);
}

void SfpModule::setDomSupport(lock_guard<std::mutex>& lg) {
  uint8_t data;
  int offset, dataAddress, length;
  getSfpFieldAddress(SffField::DIAGNOSTIC_MONITORING_TYPE,
                                 dataAddress, offset, length);
  getSfpValue(lg, dataAddress, offset, length, &data);
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

const uint8_t* SfpModule::getSfpValuePtr(lock_guard<std::mutex>& lg,
                                          int dataAddress, int offset,
                                          int length) const {
  /* if the cached values are not correct */
  if (!cacheIsValid(lg)) {
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

void SfpModule::getSfpValue(lock_guard<std::mutex>& lg, int dataAddress,
                            int offset, int length, uint8_t* data) const {
  const uint8_t *ptr = getSfpValuePtr(lg, dataAddress, offset, length);

  memcpy(data, ptr, length);
}

// Note that this needs to be called while holding the
// sfpModuleMutex_
bool SfpModule::cacheIsValid(lock_guard<std::mutex>& lg) const {
  return present_ && !dirty_;
}

bool SfpModule::isPresent() const {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  return present_;
}

void SfpModule::setPresent(lock_guard<std::mutex>& lg, bool present) {
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

void SfpModule::setSfpDom(lock_guard<std::mutex>& lg, const uint8_t* data) {
  if (!domSupport_) {
    throw FbossError("Dom not supported cannot set the values");
  }
  memcpy(sfpDom_, data, sizeof(sfpDom_));
}

bool SfpModule::getSfpFlagIdProm(lock_guard<std::mutex>& lg,
                                  const SfpDomFlag flag, const uint8_t* data) {
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
  if (getDomFlagsMap(g, dom.flags)) {
    dom.__isset.flags = true;
  }
  if (getDomThresholdValuesMap(g, dom.threshValue)) {
    dom.__isset.threshValue = true;
  }
  if (getDomValuesMap(g, dom.value)) {
    dom.__isset.value = true;
  }
  if (getVendorInfo(g, dom.vendor)) {
    dom.__isset.vendor = true;
  }
}

void SfpModule::getTransceiverInfo(TransceiverInfo &info) {
  lock_guard<std::mutex> g(sfpModuleMutex_);

  info.present = present_;
  info.transceiver = type();
  info.port = sfpImpl_->getNum();
  if (!cacheIsValid(g)) {
    return;
  }
  if (getSensorInfo(g, info.sensor)) {
    info.__isset.sensor = true;
  }
  if (getVendorInfo(g, info.vendor)) {
    info.__isset.vendor = true;
  }
  if (getCableInfo(g, info.cable)) {
    info.__isset.cable = true;
  }
  if (getThresholdInfo(g, info.thresholds)) {
    info.__isset.thresholds = true;
  }

  Channel chan;
  chan.channel = 0;
  info.channels.push_back(chan);
  if (!getSensorsPerChanInfo(g, info.channels)) {
    info.channels.clear();
  }
}

float SfpModule::getValueFromRaw(lock_guard<std::mutex>& lg,
                                  const SfpDomFlag key, uint16_t value) {
  switch (key) {
    case SfpDomFlag::TEMP_ALARM_HIGH:
    case SfpDomFlag::TEMP_ALARM_LOW:
    case SfpDomFlag::TEMP_WARN_HIGH:
    case SfpDomFlag::TEMP_WARN_LOW:
      return SffFieldInfo::getTemp(value);
    case SfpDomFlag::VCC_ALARM_HIGH:
    case SfpDomFlag::VCC_ALARM_LOW:
    case SfpDomFlag::VCC_WARN_HIGH:
    case SfpDomFlag::VCC_WARN_LOW:
      return SffFieldInfo::getVcc(value);
    case SfpDomFlag::TX_BIAS_ALARM_HIGH:
    case SfpDomFlag::TX_BIAS_ALARM_LOW:
    case SfpDomFlag::TX_BIAS_WARN_HIGH:
    case SfpDomFlag::TX_BIAS_WARN_LOW:
      return SffFieldInfo::getTxBias(value);
    case SfpDomFlag::TX_PWR_ALARM_HIGH:
    case SfpDomFlag::TX_PWR_ALARM_LOW:
    case SfpDomFlag::TX_PWR_WARN_HIGH:
    case SfpDomFlag::TX_PWR_WARN_LOW:
    case SfpDomFlag::RX_PWR_ALARM_HIGH:
    case SfpDomFlag::RX_PWR_ALARM_LOW:
    case SfpDomFlag::RX_PWR_WARN_HIGH:
    case SfpDomFlag::RX_PWR_WARN_LOW:
      return SffFieldInfo::getPwr(value);
    default:
      return 0.0;
  }
}

void SfpModule::detectTransceiver() {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  uint8_t value[MAX_SFP_EEPROM_SIZE];
  auto currentSfpStatus = sfpImpl_->detectTransceiver();
  if (currentSfpStatus != present_) {
    LOG(INFO) << "Port: " << folly::to<std::string>(sfpImpl_->getName()) <<
                  " SFP status changed to " << currentSfpStatus;
    setPresent(g, currentSfpStatus);
    if (currentSfpStatus) {
      try {
        sfpImpl_->readTransceiver(0x50, 0x0, MAX_SFP_EEPROM_SIZE, value);
        dirty_ = false;
        setSfpIdprom(g, value);
        if (domSupport_) {
          sfpImpl_->readTransceiver(0x51, 0x0, MAX_SFP_EEPROM_SIZE, value);
          setSfpDom(g, value);
        }
      } catch (const std::exception& ex) {
        dirty_ = true;
        LOG(ERROR) << "Error parsing or reading SFP data for port: " <<
             folly::to<std::string>(sfpImpl_->getName());
      }
    }
  }
}

int SfpModule::getFieldValue(SffField fieldName,
                             uint8_t* fieldValue) {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  int dataAddress, offset, length, rc;
  /* Determine if SFP is present */
  if (cacheIsValid(g)) {
    try {
      getSfpFieldAddress(fieldName, dataAddress, offset, length);
      getSfpValue(g, dataAddress, offset, length, fieldValue);
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
  if (cacheIsValid(g) && domSupport_) {
    sfpImpl_->readTransceiver(0x51, 0x0, MAX_SFP_EEPROM_SIZE, value);
    setSfpDom(g, value);
  }
}

}} //namespace facebook::fboss
