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
  {SfpIdpromFields::SINGLE_MODE_LENGTH_KM, {0x50, 0xE, 1} },
  {SfpIdpromFields::SINGLE_MODE_LENGTH, {0x50, 0xF, 1} },
  {SfpIdpromFields::SFP_50_UM_MODE_LENGTH, {0x50, 0x10, 1} },
  {SfpIdpromFields::SFP_62_5_UM_MODE_LENGTH, {0x50, 0x11, 1} },
  {SfpIdpromFields::CU_OM4_SUPPORTED_LENGTH, {0x50, 0x12, 1} },
  {SfpIdpromFields::OM3_SUPPORTED_LENGTH, {0x50, 0x13, 1} },
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
  if (present_ && domSupport_) {
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
  if (present_ && domSupport_) {
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

bool SfpModule::getDomValuesMap(SfpDomReadValue &value) {
  if (present_ && domSupport_) {
    value.temp = getSfpDomValue(SfpDomValue::TEMP);
    value.vcc = getSfpDomValue(SfpDomValue::VCC);
    value.txBias = getSfpDomValue(SfpDomValue::TX_BIAS);
    value.txPwr = getSfpDomValue(SfpDomValue::TX_PWR);
    value.rxPwr = getSfpDomValue(SfpDomValue::RX_PWR);
    return true;
  }
  return false;
}

bool SfpModule::getVendorMap(SfpVendor &vendor) {
  if (present_) {
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

SfpModule::SfpModule(std::unique_ptr<SfpImpl>& sfpImpl)
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
  dirty_ = false;
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
  if (dirty_ || (!present_)) {
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

bool SfpModule::isSfpPresent() const {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  return present_;
}

void SfpModule::setSfpPresent(bool present) {
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
  if (getVendorMap(dom.vendor)) {
    dom.__isset.vendor = true;
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

void SfpModule::detectSfp() {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  uint8_t value[MAX_SFP_EEPROM_SIZE];
  auto currentSfpStatus = sfpImpl_->detectSfp();
  if (currentSfpStatus != present_) {
    LOG(INFO) << "Port: " << folly::to<std::string>(sfpImpl_->getName()) <<
                  " SFP status changed to " << currentSfpStatus;
    setSfpPresent(currentSfpStatus);
    if (currentSfpStatus) {
      sfpImpl_->readSfpEeprom(0x50, 0x0, MAX_SFP_EEPROM_SIZE, value);
      setSfpIdprom(value);
      if (domSupport_) {
        sfpImpl_->readSfpEeprom(0x51, 0x0, MAX_SFP_EEPROM_SIZE, value);
        setSfpDom(value);
      }
    }
  }
}

int SfpModule::getSfpFieldValue(SfpIdpromFields fieldName,
                                                uint8_t* fieldValue) {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  int dataAddress, offset, length, rc;
  /* Determine if SFP is present */
  if (present_) {
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

void SfpModule::updateSfpDomFields() {
  lock_guard<std::mutex> g(sfpModuleMutex_);
  uint8_t value[MAX_SFP_EEPROM_SIZE];
  if (present_ && domSupport_) {
    sfpImpl_->readSfpEeprom(0x51, 0x0, MAX_SFP_EEPROM_SIZE, value);
    setSfpDom(value);
  }
}

}} //namespace facebook::fboss
