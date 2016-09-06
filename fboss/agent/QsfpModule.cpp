/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "QsfpModule.h"

#include <boost/assign.hpp>
#include <string>
#include <limits>
#include <iomanip>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/TransceiverImpl.h"
#include "fboss/agent/SffFieldInfo.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook { namespace fboss {
  using std::memcpy;
  using std::mutex;
  using std::lock_guard;

// As per SFF-8436, QSFP+ 10 Gbs 4X PLUGGABLE TRANSCEIVER spec

static SffFieldInfo::SffFieldMap qsfpFields = {
  // Base page values, including alarms and sensors
  {SffField::IDENTIFIER, {QsfpPages::LOWER, 0, 1} },
  {SffField::STATUS, {QsfpPages::LOWER, 1, 2} },
  {SffField::TEMPERATURE_ALARMS, {QsfpPages::LOWER, 6, 1} },
  {SffField::VCC_ALARMS, {QsfpPages::LOWER, 7, 1} },
  {SffField::CHANNEL_RX_PWR_ALARMS, {QsfpPages::LOWER, 9, 2} },
  {SffField::CHANNEL_TX_BIAS_ALARMS, {QsfpPages::LOWER, 11, 2} },
  {SffField::TEMPERATURE, {QsfpPages::LOWER, 22, 2} },
  {SffField::VCC, {QsfpPages::LOWER, 26, 2} },
  {SffField::CHANNEL_RX_PWR, {QsfpPages::LOWER, 34, 8} },
  {SffField::CHANNEL_TX_BIAS, {QsfpPages::LOWER, 42, 8} },
  {SffField::RATE_SELECT_RX, {QsfpPages::LOWER, 87, 1} },
  {SffField::RATE_SELECT_TX, {QsfpPages::LOWER, 88, 1} },
  {SffField::POWER_CONTROL, {QsfpPages::LOWER, 93, 1} },
  {SffField::CDR_CONTROL, {QsfpPages::LOWER, 98, 1} },
  {SffField::PAGE_SELECT_BYTE, {QsfpPages::LOWER, 127, 1} },

  // Page 0 values, including vendor info:
  {SffField::EXTENDED_IDENTIFIER, {QsfpPages::PAGE0, 129, 1} },
  {SffField::ETHERNET_COMPLIANCE, {QsfpPages::PAGE0, 131, 1} },
  {SffField::EXTENDED_RATE_COMPLIANCE, {QsfpPages::PAGE0, 141, 1} },
  {SffField::LENGTH_SM_KM, {QsfpPages::PAGE0, 142, 1} },
  {SffField::LENGTH_OM3, {QsfpPages::PAGE0, 143, 1} },
  {SffField::LENGTH_OM2, {QsfpPages::PAGE0, 144, 1} },
  {SffField::LENGTH_OM1, {QsfpPages::PAGE0, 145, 1} },
  {SffField::LENGTH_COPPER, {QsfpPages::PAGE0, 146, 1} },
  {SffField::VENDOR_NAME, {QsfpPages::PAGE0, 148, 16} },
  {SffField::VENDOR_OUI, {QsfpPages::PAGE0, 165, 3} },
  {SffField::PART_NUMBER, {QsfpPages::PAGE0, 168, 16} },
  {SffField::REVISION_NUMBER, {QsfpPages::PAGE0, 184, 2} },
  {SffField::OPTIONS, {QsfpPages::PAGE0, 195, 1} },
  {SffField::VENDOR_SERIAL_NUMBER, {QsfpPages::PAGE0, 196, 16} },
  {SffField::MFG_DATE, {QsfpPages::PAGE0, 212, 8} },
  {SffField::DIAGNOSTIC_MONITORING_TYPE, {QsfpPages::PAGE0, 220, 1} },
  {SffField::ENHANCED_OPTIONS, {QsfpPages::PAGE0, 221, 1} },


  // Page 3 values, including alarm and warning threshold values:
  {SffField::TEMPERATURE_THRESH, {QsfpPages::PAGE3, 128, 8} },
  {SffField::VCC_THRESH, {QsfpPages::PAGE3, 144, 8} },
  {SffField::RX_PWR_THRESH, {QsfpPages::PAGE3, 176, 8} },
  {SffField::TX_BIAS_THRESH, {QsfpPages::PAGE3, 184, 8} },
};

static SffFieldMultiplier qsfpMultiplier = {
  {SffField::LENGTH_SM_KM, 1000},
  {SffField::LENGTH_OM3, 2},
  {SffField::LENGTH_OM2, 1},
  {SffField::LENGTH_OM1, 1},
  {SffField::LENGTH_COPPER, 1},
};

void getQsfpFieldAddress(SffField field, int &dataAddress,
                         int &offset, int &length) {
  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields, field);
  dataAddress = info.dataAddress;
  offset = info.offset;
  length = info.length;
}

/*
 * Given a byte, extract bit fields for various alarm flags;
 * note the we might want to use the lower or the upper nibble,
 * so offset is the number of the bit to start at;  this is usually
 * 0 or 4.
 */

FlagLevels QsfpModule::getQsfpFlags(const uint8_t *data,
                                    int offset) {
  FlagLevels flags;

  CHECK_GE(offset, 0);
  CHECK_LE(offset, 4);
  flags.warn.low = (*data & (1 << offset));
  flags.warn.high = (*data & (1 << ++offset));
  flags.alarm.low = (*data & (1 << ++offset));
  flags.alarm.high = (*data & (1 << ++offset));

  return flags;
}

FlagLevels QsfpModule::getQsfpSensorFlags(SffField fieldName) {
  int offset;
  int length;
  int dataAddress;

  /* Determine if QSFP is present */
  getQsfpFieldAddress(fieldName, dataAddress, offset, length);
  const uint8_t *data = getQsfpValuePtr(dataAddress, offset, length);
  return getQsfpFlags(data, 4);
}

bool QsfpModule::getSensorInfo(GlobalSensors& info) {
  info.temp.value = getQsfpSensor(SffField::TEMPERATURE,
                                  SffFieldInfo::getTemp);
  info.temp.flags = getQsfpSensorFlags(SffField::TEMPERATURE_ALARMS);
  info.temp.__isset.flags = true;
  info.vcc.value = getQsfpSensor(SffField::VCC, SffFieldInfo::getVcc);
  info.vcc.flags = getQsfpSensorFlags(SffField::VCC_ALARMS);
  info.vcc.__isset.flags = true;
  return true;
}

bool QsfpModule::getVendorInfo(Vendor &vendor) {
  vendor.name = getQsfpString(SffField::VENDOR_NAME);
  vendor.oui = getQsfpString(SffField::VENDOR_OUI);
  vendor.partNumber = getQsfpString(SffField::PART_NUMBER);
  vendor.rev = getQsfpString(SffField::REVISION_NUMBER);
  vendor.serialNumber = getQsfpString(SffField::VENDOR_SERIAL_NUMBER);
  vendor.dateCode = getQsfpString(SffField::MFG_DATE);
  return true;
}

bool QsfpModule::getCableInfo(Cable &cable) {
  cable.singleMode = getQsfpCableLength(SffField::LENGTH_SM_KM);
  cable.__isset.singleMode = (cable.singleMode != 0);
  cable.om3 = getQsfpCableLength(SffField::LENGTH_OM3);
  cable.__isset.om3 = (cable.om3 != 0);
  cable.om2 = getQsfpCableLength(SffField::LENGTH_OM2);
  cable.__isset.om2 = (cable.om2 != 0);
  cable.om1 = getQsfpCableLength(SffField::LENGTH_OM1);
  cable.__isset.om1 = (cable.om1 != 0);
  cable.copper = getQsfpCableLength(SffField::LENGTH_COPPER);
  cable.__isset.copper = (cable.copper != 0);
  return (cable.__isset.copper || cable.__isset.om1 || cable.__isset.om2 ||
          cable.__isset.om3 || cable.__isset.singleMode);
}

/*
 * Threhold values are stored just once;  they aren't per-channel,
 * so in all cases we simple assemble two-byte values and convert
 * them based on the type of the field.
 */

ThresholdLevels QsfpModule::getThresholdValues(SffField field,
                               double (*conversion)(uint16_t value)) {
  int offset;
  int length;
  int dataAddress;
  ThresholdLevels thresh;

  CHECK(!flatMem_);

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t *data = getQsfpValuePtr(dataAddress, offset, length);

  CHECK_GE(length, 8);
  thresh.alarm.high = conversion(data[0] << 8 | data[1]);
  thresh.alarm.low = conversion(data[2] << 8 | data[3]);
  thresh.warn.high = conversion(data[4] << 8 | data[5]);
  thresh.warn.low = conversion(data[6] << 8 | data[7]);

  return thresh;
}

bool QsfpModule::getThresholdInfo(AlarmThreshold &threshold) {
  if (flatMem_) {
    return false;
  }
  threshold.temp = getThresholdValues(SffField::TEMPERATURE_THRESH,
                                      SffFieldInfo::getTemp);
  threshold.vcc = getThresholdValues(SffField::VCC_THRESH,
                                     SffFieldInfo::getVcc);
  threshold.rxPwr= getThresholdValues(SffField::RX_PWR_THRESH,
                                      SffFieldInfo::getPwr);
  threshold.txBias = getThresholdValues(SffField::TX_BIAS_THRESH,
                                        SffFieldInfo::getTxBias);
  return true;
}

uint8_t QsfpModule::getSettingsValue(SffField field, uint8_t mask) {
  int offset;
  int length;
  int dataAddress;

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  return data[0] & mask;
}

bool QsfpModule::getTransceiverSettingsInfo(TransceiverSettings &settings) {
  settings.cdrTx = SffFieldInfo::getFeatureState(
      getSettingsValue(SffField::EXTENDED_IDENTIFIER, EXT_ID_CDR_TX_MASK),
      getSettingsValue(SffField::CDR_CONTROL, CDR_CONTROL_TX_MASK));
  settings.cdrRx = SffFieldInfo::getFeatureState(
      getSettingsValue(SffField::EXTENDED_IDENTIFIER, EXT_ID_CDR_RX_MASK),
      getSettingsValue(SffField::CDR_CONTROL, CDR_CONTROL_RX_MASK));

  settings.powerMeasurement = SffFieldInfo::getFeatureState(
      getSettingsValue(SffField::DIAGNOSTIC_MONITORING_TYPE,
                       POWER_MEASUREMENT_MASK));

  settings.powerControl = getPowerControlValue();
  settings.rateSelect = getRateSelectValue();
  settings.rateSelectSetting = getRateSelectSettingValue(settings.rateSelect);

  return true;
}

RateSelectSetting QsfpModule::getRateSelectSettingValue(RateSelectState state) {
  /* This refers to the optimised bit rate
   * The various values here are listed in the spec:
   * ftp://ftp.seagate.com/sff/SFF-8636.PDF - page 36
   */
  if (state != RateSelectState::EXTENDED_RATE_SELECT_V1 &&
      state != RateSelectState::EXTENDED_RATE_SELECT_V2) {
    return RateSelectSetting::UNSUPPORTED;
  }

  // Each byte has settings for 4 different channels
  // Currently we only support setting them all to the same value
  // We also expect rx and tx to have the same setting
  uint8_t rateRx = getSettingsValue(SffField::RATE_SELECT_RX);
  uint8_t rateTx = getSettingsValue(SffField::RATE_SELECT_TX);
  if (rateRx != rateTx) {
    LOG(ERROR) << "Unable to retrieve rate select setting: rx(" << std::hex <<
      rateRx << " and tx(" << rateTx << ") are not equal";
    return RateSelectSetting::UNSUPPORTED;
  }

  int channelRate = rateRx & 0b11;
  if (state == RateSelectState::EXTENDED_RATE_SELECT_V2) {
    // Offset so that we can correctly index into the enum
    channelRate += 3;
  }
  return (RateSelectSetting) channelRate;
}

RateSelectState QsfpModule::getRateSelectValue() {
  /* Rate select can be in one of 3 states:
   * 1. Not supported
   * 2. Rate selection using extended rate select
   * 3. Rate selection with application select tables
   *
   * We currently only support 1 and 2
   * The spec: ftp://ftp.seagate.com/sff/SFF-8636.PDF has a thorough
   * discussion of this on page 36.
   */
  uint8_t enhancedOptions =
    getSettingsValue(SffField::ENHANCED_OPTIONS, ENH_OPT_RATE_SELECT_MASK);
  uint8_t options = getSettingsValue(SffField::OPTIONS, OPT_RATE_SELECT_MASK);

  if (!enhancedOptions && !options) {
    return RateSelectState::UNSUPPORTED;
  }

  uint8_t extendedRateCompliance = getSettingsValue(
      SffField::EXTENDED_RATE_COMPLIANCE);
  if (enhancedOptions == 0b10 && (extendedRateCompliance & 0b01)) {
    return RateSelectState::EXTENDED_RATE_SELECT_V1;
  } else if (enhancedOptions == 0b10 && (extendedRateCompliance & 0b10)) {
    return RateSelectState::EXTENDED_RATE_SELECT_V2;
  } else if (enhancedOptions == 0b01) {
    return RateSelectState::APPLICATION_RATE_SELECT;
  }

  return RateSelectState::UNSUPPORTED;
}

PowerControlState QsfpModule::getPowerControlValue() {
  switch (getSettingsValue(SffField::POWER_CONTROL, POWER_CONTROL_MASK)) {
    case POWER_SET:
      return PowerControlState::POWER_SET;
    case HIGH_POWER_OVERRIDE:
      return PowerControlState::HIGH_POWER_OVERRIDE;
    case POWER_OVERRIDE:
    default:
      return PowerControlState::POWER_OVERRIDE;
  }
}

/*
 * Iterate through the four channels collecting appropriate data;
 * always assumes CHANNEL_COUNT channels is four.
 */

bool QsfpModule::getSensorsPerChanInfo(std::vector<Channel>& channels) {
  int offset;
  int length;
  int dataAddress;

  /*
   * Interestingly enough, the QSFP stores the four alarm flags
   * (alarm high, alarm low, warning high, warning low) in two bytes by
   * channel in order 2, 1, 4, 3;  by using this set of offsets, we
   * should be able to read them in order, by reading the appriopriate
   * bit offsets combined with a byte offset into the data.
   *
   * That is, read bits 4 through 7 of the first byte, then 0 through 3,
   * then 4 through 7 of the second byte, and so on.  Ugh.
   */
  int bitOffset[] = {4, 0, 4, 0};
  int byteOffset[] = {0, 0, 1, 1};

  getQsfpFieldAddress(SffField::CHANNEL_RX_PWR_ALARMS, dataAddress,
                      offset, length);
  const uint8_t *data = getQsfpValuePtr(dataAddress, offset, length);

  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    channels[channel].sensors.rxPwr.flags =
      getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
    channels[channel].sensors.rxPwr.__isset.flags = true;
  }

  getQsfpFieldAddress(SffField::CHANNEL_TX_BIAS_ALARMS, dataAddress,
                      offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    channels[channel].sensors.txBias.flags =
      getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
    channels[channel].sensors.txBias.__isset.flags = true;
  }

  getQsfpFieldAddress(SffField::CHANNEL_RX_PWR, dataAddress,
                      offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    channel.sensors.rxPwr.value = SffFieldInfo::getPwr(value);
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  getQsfpFieldAddress(SffField::CHANNEL_TX_BIAS, dataAddress,
                      offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);
  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    channel.sensors.txBias.value = SffFieldInfo::getTxBias(value);
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);
  // QSFP doesn't report Tx power, so we can't try to report that.
  return true;
}

std::string QsfpModule::getQsfpString(SffField field) {
  int offset;
  int length;
  int dataAddress;

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t *data = getQsfpValuePtr(dataAddress, offset, length);

  while (length > 0 && data[length - 1] == ' ') {
    --length;
  }
  return std::string(reinterpret_cast<const char*>(data), length);
}

double QsfpModule::getQsfpSensor(SffField field,
    double (*conversion)(uint16_t value)) {

  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields, field);
  const uint8_t *data = getQsfpValuePtr(info.dataAddress,
                                        info.offset, info.length);
  return conversion(data[0] << 8 | data[1]);
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

int QsfpModule::getQsfpCableLength(SffField field) {
  int length;
  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields, field);
  const uint8_t *data = getQsfpValuePtr(info.dataAddress,
                                        info.offset, info.length);
  auto multiplier = qsfpMultiplier.at(field);
  length = *data * multiplier;
  if (*data == MAX_CABLE_LEN) {
    length = -(MAX_CABLE_LEN - 1) * multiplier;
  }
  return length;
}

QsfpModule::QsfpModule(std::unique_ptr<TransceiverImpl> qsfpImpl)
  : qsfpImpl_(std::move(qsfpImpl)) {
  present_ = false;
  dirty_ = true;
}

void QsfpModule::setQsfpIdprom() {
  uint8_t status[2];
  int offset;
  int length;
  int dataAddress;

  if (!present_) {
    throw FbossError("QSFP IDProm set failed as QSFP is not present");
  }

  // Check if the data is ready
  getQsfpFieldAddress(SffField::STATUS,
                      dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, status);
  if (status[1] & (1 << 0)) {
    dirty_ = true;
    throw FbossError("QSFP IDProm failed as QSFP is not read");
  }
  flatMem_ = status[1] & (1 << 2);
  dirty_ = false;
}

const uint8_t* QsfpModule::getQsfpValuePtr(int dataAddress, int offset,
                                           int length) const {
  /* if the cached values are not correct */
  if (!cacheIsValid()) {
    throw FbossError("Qsfp is either not present or the data is not read");
  }
  if (dataAddress == QsfpPages::LOWER) {
    CHECK_LE(offset + length, sizeof(qsfpIdprom_));
    /* Copy data from the cache */
    return(qsfpIdprom_ + offset);
  } else {
    offset -= MAX_QSFP_PAGE_SIZE;
    CHECK_GE(offset, 0);
    CHECK_LE(offset, MAX_QSFP_PAGE_SIZE);
    if (dataAddress == QsfpPages::PAGE0) {
      CHECK_LE(offset + length, sizeof(qsfpPage0_));
      /* Copy data from the cache */
      return(qsfpPage0_ + offset);
    } else if (dataAddress == QsfpPages::PAGE3 && !flatMem_) {
      CHECK_LE(offset + length, sizeof(qsfpPage3_));
      /* Copy data from the cache */
      return(qsfpPage3_ + offset);
    } else {
      throw FbossError("Invalid Data Address 0x%d", dataAddress);
    }
  }
}

void QsfpModule::getQsfpValue(int dataAddress, int offset, int length,
                              uint8_t* data) const {
  const uint8_t *ptr = getQsfpValuePtr(dataAddress, offset, length);

  memcpy(data, ptr, length);
}

bool QsfpModule::isPresent() const {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  return present_;
}

void QsfpModule::setPresent(bool present) {
  present_ = present;
  /* Set the dirty bit as the QSFP was removed and
   * the cached data is no longer valid until next
   * set IDProm is called
   */
  if (present_ == false) {
    dirty_ = true;
  }
}

// Note that this needs to be called while holding the
// qsfpModuleMutex_
bool QsfpModule::cacheIsValid() const {
  return present_ && !dirty_;
}

void QsfpModule::getSfpDom(SfpDom &) {
}

void QsfpModule::getTransceiverInfo(TransceiverInfo &info) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  info.present = present_;
  info.transceiver = type();
  info.port = qsfpImpl_->getNum();
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
  if (getTransceiverSettingsInfo(info.settings)) {
    info.__isset.settings = true;
  }
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    Channel chan;
    chan.channel = i;
    info.channels.push_back(chan);
  }

  if (getSensorsPerChanInfo(info.channels)) {
    info.__isset.channels = true;
  } else {
    info.channels.clear();
  }
}

void QsfpModule::detectTransceiver() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  auto currentQsfpStatus = qsfpImpl_->detectTransceiver();
  if (currentQsfpStatus != present_) {
    LOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName()) <<
                  " QSFP status changed to " << currentQsfpStatus;
    setPresent(currentQsfpStatus);
    if (currentQsfpStatus) {
      updateQsfpData();
      customizeTransceiverLocked();
    }
  }
}

int QsfpModule::getFieldValue(SffField fieldName,
                             uint8_t* fieldValue) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  int offset;
  int length;
  int dataAddress;

  /* Determine if QSFP is present */
  if (cacheIsValid()) {
    try {
      getQsfpFieldAddress(fieldName, dataAddress, offset, length);
      getQsfpValue(dataAddress, offset, length, fieldValue);
    } catch (const std::exception& ex) {
      LOG(ERROR) << "Error reading field value for transceiver:" <<
             folly::to<std::string>(qsfpImpl_->getName()) << " " << ex.what();
    }
  }
  return -1;
}

void QsfpModule::updateTransceiverInfoFields() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  updateQsfpData();
}

void QsfpModule::updateQsfpData() {
  if (present_) {
    try {
      qsfpImpl_->readTransceiver(0x50, 0, sizeof(qsfpIdprom_), qsfpIdprom_);
      dirty_ = false;
      setQsfpIdprom();
      /*
       * XXX:  Should we bother to read this other data every time?
       *       Surely, it isn't changing?  Is there some other utility to
       *       update this stuff?  Perhaps just reading the base 128 bytes
       *       is enough after we initially determine that somethings there?
       */

      // If we have flat memory, we don't have to set the page
      if (!flatMem_) {
        uint8_t page = 0;
        qsfpImpl_->writeTransceiver(0x50, 127, sizeof(page), &page);
      }
      qsfpImpl_->readTransceiver(0x50, 128, sizeof(qsfpPage0_), qsfpPage0_);
      if (!flatMem_) {
        uint8_t page = 3;
        qsfpImpl_->writeTransceiver(0x50, 127, sizeof(page), &page);
        qsfpImpl_->readTransceiver(0x50, 128, sizeof(qsfpPage3_), qsfpPage3_);
      }
    } catch (const std::exception& ex) {
      dirty_ = true;
      LOG(WARNING) << "Error reading data for transceiver:" <<
           folly::to<std::string>(qsfpImpl_->getName()) << " " << ex.what();
    }
  }
}

void QsfpModule::customizeTransceiver(const cfg::PortSpeed& speed) {
  if (!present_) {
    // Detect the transceiver if not already present
    detectTransceiver();
  } else {
    lock_guard<std::mutex> g(qsfpModuleMutex_);
    customizeTransceiverLocked(speed);
  }
}

void QsfpModule::customizeTransceiverLocked(const cfg::PortSpeed& speed) {
  /*
   * This must be called with a lock held on qsfpModuleMutex_
   */
  if (dirty_) {
    return;
  }
  TransceiverSettings settings;
  getTransceiverSettingsInfo(settings);

  // We want this on regardless of speed
  setPowerOverrideIfSupported(settings.powerControl);
  if (speed == cfg::PortSpeed::DEFAULT) {
    return;
  }

  setCdrIfSupported(speed, settings.cdrTx, settings.cdrRx);
  setRateSelectIfSupported(speed, settings.rateSelect,
      settings.rateSelectSetting);
}

void QsfpModule::setCdrIfSupported(cfg::PortSpeed speed,
                                   FeatureState currentStateTx,
                                   FeatureState currentStateRx) {
  /*
   * Note that this function expects to be called with qsfpModuleMutex_
   * held.
   */

  if (currentStateTx == FeatureState::UNSUPPORTED &&
      currentStateRx == FeatureState::UNSUPPORTED) {
    return;
  }

  // If only one of Rx or Tx is supported, it doesn't matter what
  // we set the value to, so in that case, treat is as if
  // no change is needed
  auto toChange = [speed](FeatureState state) {
    return
      state != FeatureState::UNSUPPORTED &&
      ((speed == cfg::PortSpeed::HUNDREDG && state != FeatureState::ENABLED) ||
      (speed != cfg::PortSpeed::HUNDREDG && state != FeatureState::DISABLED));
  };

  bool changeRx = toChange(currentStateRx);
  bool changeTx = toChange(currentStateTx);
  if (!changeRx && !changeTx) {
    LOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName()) <<
      " Not changing CDR setting, already correctly set";
    return;
  }

  // If one of rx and tx need a change, set the whole byte - whichever
  // isn't supported will be ignored anyway
  FeatureState newState = FeatureState::DISABLED;
  uint8_t value = 0x0;
  if (speed == cfg::PortSpeed::HUNDREDG) {
    value = 0xFF;
    newState = FeatureState::ENABLED;
  }
  int dataLength, dataAddress, dataOffset;
  getQsfpFieldAddress(SffField::CDR_CONTROL, dataAddress,
                      dataOffset, dataLength);

  qsfpImpl_->writeTransceiver(TransceiverI2CApi::ADDR_QSFP, dataOffset,
      sizeof(value), &value);
  LOG(INFO) << folly::to<std::string>("Port: ", qsfpImpl_->getName(),
     " Setting CDR to state: ",
     _FeatureState_VALUES_TO_NAMES.find(newState)->second);
}

void QsfpModule::setRateSelectIfSupported(cfg::PortSpeed speed,
    RateSelectState currentState, RateSelectSetting currentSetting) {
  if (currentState == RateSelectState::UNSUPPORTED) {
    return;
  } else if (currentState == RateSelectState::APPLICATION_RATE_SELECT) {
    // Currently only support extended rate select, so treat application
    // rate select as an invalid option
    LOG(ERROR) << "Port: " << folly::to<std::string>(qsfpImpl_->getName()) <<
      " Rate select in unknown state, treating as unsupported: " <<
      _RateSelectState_VALUES_TO_NAMES.find(currentState)->second;
    return;
  }

  uint8_t value;
  RateSelectSetting newSetting;
  bool alreadySet = false;
  auto translateEnum = [currentSetting, &value, &newSetting] (
      RateSelectSetting desired,
      uint8_t newValue) {
    if (currentSetting == desired) {
      return true;
    }
    newSetting = desired;
    value = newValue;
    return false;
  };

  if (currentState == RateSelectState::EXTENDED_RATE_SELECT_V1) {
    // Use the highest possible speed in this version
    alreadySet = translateEnum(RateSelectSetting::FROM_6_6GB_AND_ABOVE,
        0b10101010);
  } else if (speed == cfg::PortSpeed::FORTYG) {
    // Optimised for 10G channels
    alreadySet = translateEnum(RateSelectSetting::LESS_THAN_12GB,
      0b00000000);
  } else if (speed == cfg::PortSpeed::HUNDREDG) {
    // Optimised for 25GB channels
    alreadySet = translateEnum(RateSelectSetting::FROM_24GB_to_26GB,
        0b10101010);
  } else {
    LOG(ERROR) << "Port: " << folly::to<std::string>(qsfpImpl_->getName()) <<
      " Unable to set rate select for port speed: " <<
      cfg::_PortSpeed_VALUES_TO_NAMES.find(speed)->second;
    return;
  }

  if (alreadySet) {
    return;
  }

  int dataLength, dataAddress, dataOffset;

  getQsfpFieldAddress(SffField::RATE_SELECT_RX, dataAddress,
                      dataOffset, dataLength);
  qsfpImpl_->writeTransceiver(TransceiverI2CApi::ADDR_QSFP,
      dataOffset, sizeof(value), &value);

  getQsfpFieldAddress(SffField::RATE_SELECT_RX, dataAddress,
                      dataOffset, dataLength);
  qsfpImpl_->writeTransceiver(TransceiverI2CApi::ADDR_QSFP,
      dataOffset, sizeof(value), &value);
  LOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName()) <<
    " set rate select to " <<
    _RateSelectSetting_VALUES_TO_NAMES.find(newSetting)->second;
}

void QsfpModule::setPowerOverrideIfSupported(PowerControlState currentState) {
  /* Wedge forces Low Power mode via a pin;  we have to reset this
   * to force High Power mode on LR4s.
   *
   * Note that this function expects to be called with qsfpModuleMutex_
   * held.
   */
  if (currentState != PowerControlState::POWER_SET) {
    LOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName()) <<
      "Power override already set, doing nothing";
    return;
  }

  int offset;
  int length;
  int dataAddress;

  getQsfpFieldAddress(SffField::EXTENDED_IDENTIFIER, dataAddress,
                      offset, length);
  const uint8_t *extId = getQsfpValuePtr(dataAddress, offset, length);
  getQsfpFieldAddress(SffField::ETHERNET_COMPLIANCE, dataAddress,
                      offset, length);
  const uint8_t *ethCompliance = getQsfpValuePtr(dataAddress, offset, length);

  int pwrCtrlAddress;
  int pwrCtrlOffset;
  int pwrCtrlLength;
  getQsfpFieldAddress(SffField::POWER_CONTROL, pwrCtrlAddress,
                      pwrCtrlOffset, pwrCtrlLength);
  const uint8_t *pwrCtrl = getQsfpValuePtr(pwrCtrlAddress,
                                           pwrCtrlOffset, pwrCtrlLength);

  /*
   * It is not clear whether we'll have to use some of these values
   * in future to determine whether or not to set the high power override.
   * Leave the logging in until this is fully debugged -- this should
   * only trigger on QSFP insertion.
   */

  VLOG(1) << "Port: " << folly::to<std::string>(qsfpImpl_->getName()) <<
             " QSFP Ext ID " << std::hex << (int) *extId <<
             " Ether Compliance " << std::hex << (int) *ethCompliance <<
             " Power Control " << std::hex << (int) *pwrCtrl;

  int highPowerLevel = (*extId & EXT_ID_HI_POWER_MASK);
  int powerLevel = (*extId & EXT_ID_POWER_MASK) >> EXT_ID_POWER_SHIFT;

  if (highPowerLevel > 0 || powerLevel > 0) {
      uint8_t power = POWER_OVERRIDE;
      if (highPowerLevel > 0) {
        power |= HIGH_POWER_OVERRIDE;
      }

      // Note that we don't have to set the page here, but there should
      // probably be a setQsfpValue() function to handle pages, etc.

      if (pwrCtrlAddress != QsfpPages::LOWER) {
         throw FbossError("QSFP failed to set POWER_CONTROL for LR4 "
                          "due to incorrect page number");
      }
      if (pwrCtrlLength != sizeof(power)) {
         throw FbossError("QSFP failed to set POWER_CONTROL for LR4 "
                          "due to incorrect length");
      }

      qsfpImpl_->writeTransceiver(0x50, pwrCtrlOffset, sizeof(power), &power);
      LOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName()) <<
                " QSFP set to override low power";
  }
}

}} //namespace facebook::fboss
