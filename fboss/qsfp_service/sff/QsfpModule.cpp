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
#include <iomanip>
#include "fboss/agent/FbossError.h"
#include "fboss/qsfp_service/sff/TransceiverImpl.h"
#include "fboss/qsfp_service/sff/SffFieldInfo.h"
#include "fboss/qsfp_service/StatsPublisher.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>

DEFINE_int32(
    qsfp_data_refresh_interval,
    10,
    "how often to refetch qsfp data that changes frequently");
DEFINE_int32(
    customize_interval,
    30,
    "minimum interval between customizing the same down port twice");
DEFINE_int32(
    tx_enable_interval,
    300,
    "seconds between ensuring tx is enabled on down ports");
DEFINE_int32(
    reset_lpmode_interval,
    300,
    "seconds between flapping lpmode on down ports to try to recover");

using std::memcpy;
using std::mutex;
using std::lock_guard;
using folly::IOBuf;

namespace facebook { namespace fboss {

// As per SFF-8636
static SffFieldInfo::SffFieldMap qsfpFields = {
  // Base page values, including alarms and sensors
  {SffField::IDENTIFIER, {QsfpPages::LOWER, 0, 1} },
  {SffField::STATUS, {QsfpPages::LOWER, 1, 2} },
  {SffField::TEMPERATURE_ALARMS, {QsfpPages::LOWER, 6, 1} },
  {SffField::VCC_ALARMS, {QsfpPages::LOWER, 7, 1} },
  {SffField::CHANNEL_RX_PWR_ALARMS, {QsfpPages::LOWER, 9, 2} },
  {SffField::CHANNEL_TX_BIAS_ALARMS, {QsfpPages::LOWER, 11, 2} },
  {SffField::CHANNEL_TX_PWR_ALARMS, {QsfpPages::LOWER, 13, 2} },
  {SffField::TEMPERATURE, {QsfpPages::LOWER, 22, 2} },
  {SffField::VCC, {QsfpPages::LOWER, 26, 2} },
  {SffField::CHANNEL_RX_PWR, {QsfpPages::LOWER, 34, 8} },
  {SffField::CHANNEL_TX_BIAS, {QsfpPages::LOWER, 42, 8} },
  {SffField::CHANNEL_TX_PWR, {QsfpPages::LOWER, 50, 8} },
  {SffField::TX_DISABLE, {QsfpPages::LOWER, 86, 1} },
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
  {SffField::DEVICE_TECHNOLOGY, {QsfpPages::PAGE0, 147, 1} },
  {SffField::VENDOR_NAME, {QsfpPages::PAGE0, 148, 16} },
  {SffField::VENDOR_OUI, {QsfpPages::PAGE0, 165, 3} },
  {SffField::PART_NUMBER, {QsfpPages::PAGE0, 168, 16} },
  {SffField::REVISION_NUMBER, {QsfpPages::PAGE0, 184, 2} },
  {SffField::OPTIONS, {QsfpPages::PAGE0, 195, 1} },
  {SffField::VENDOR_SERIAL_NUMBER, {QsfpPages::PAGE0, 196, 16} },
  {SffField::MFG_DATE, {QsfpPages::PAGE0, 212, 8} },
  {SffField::DIAGNOSTIC_MONITORING_TYPE, {QsfpPages::PAGE0, 220, 1} },
  {SffField::ENHANCED_OPTIONS, {QsfpPages::PAGE0, 221, 1} },

  // These are custom fields FB gets cable vendors to populate.
  // TODO: add support for turning off these fields via a command-line flag
  {SffField::LENGTH_COPPER_DECIMETERS, {QsfpPages::PAGE0, 236, 1}},
  {SffField::DAC_GAUGE, {QsfpPages::PAGE0, 237, 1}},

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
  {SffField::LENGTH_COPPER_DECIMETERS, 0.1},
};

void getQsfpFieldAddress(SffField field, int &dataAddress,
                         int &offset, int &length) {
  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields, field);
  dataAddress = info.dataAddress;
  offset = info.offset;
  length = info.length;
}

TransceiverID QsfpModule::getID() const {
  return TransceiverID(qsfpImpl_->getNum());
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

double QsfpModule::getQsfpDACLength() const {
  auto base = getQsfpCableLength(SffField::LENGTH_COPPER);
  auto fractional = getQsfpCableLength(SffField::LENGTH_COPPER_DECIMETERS);
  if (fractional < 0) {
    // fractional value not populated, fall back to integer value
    return base;
  } else if (fractional >= 1) {
    // some vendors misunderstood and expressed the full length in terms of dm
    return fractional;
  } else {
    return base + fractional;
  }
}

int QsfpModule::getQsfpDACGauge() const {
  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields, SffField::DAC_GAUGE);
  const uint8_t *val = getQsfpValuePtr(info.dataAddress,
                                       info.offset, info.length);
  //Guard against FF default value
  auto gauge = *val;
  if (gauge == EEPROM_DEFAULT) {
    return 0;
  } else if (gauge > MAX_GAUGE) {
    // HACK: We never use cables with more than 30 (in decimal) gauge
    // However, some vendors put in hex values. For example, some put
    // 0x28 to represent 28 gauge cable (why?!?), which we would
    // incorrectly interpret as 40 if using decimal. This converts
    // values > 30 to the hex value.
    return (gauge / HEX_BASE) * DECIMAL_BASE + gauge % HEX_BASE;
  } else{
    return gauge;
  }
}

bool QsfpModule::getSensorInfo(GlobalSensors& info) {
  info.temp.value = getQsfpSensor(SffField::TEMPERATURE,
                                  SffFieldInfo::getTemp);
  info.temp.flags_ref().value_unchecked() =
      getQsfpSensorFlags(SffField::TEMPERATURE_ALARMS);
  info.temp.__isset.flags = true;
  info.vcc.value = getQsfpSensor(SffField::VCC, SffFieldInfo::getVcc);
  info.vcc.flags_ref().value_unchecked() =
      getQsfpSensorFlags(SffField::VCC_ALARMS);
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

void QsfpModule::getCableInfo(Cable &cable) {
  cable.transmitterTech = getQsfpTransmitterTechnology();
  cable.__isset.transmitterTech = true;

  cable.singleMode_ref().value_unchecked() =
      getQsfpCableLength(SffField::LENGTH_SM_KM);
  cable.__isset.singleMode = (cable.singleMode_ref().value_unchecked() != 0);
  cable.om3_ref().value_unchecked() = getQsfpCableLength(SffField::LENGTH_OM3);
  cable.__isset.om3 = (cable.om3_ref().value_unchecked() != 0);
  cable.om2_ref().value_unchecked() = getQsfpCableLength(SffField::LENGTH_OM2);
  cable.__isset.om2 = (cable.om2_ref().value_unchecked() != 0);
  cable.om1_ref().value_unchecked() = getQsfpCableLength(SffField::LENGTH_OM1);
  cable.__isset.om1 = (cable.om1_ref().value_unchecked() != 0);
  cable.copper_ref().value_unchecked() =
      getQsfpCableLength(SffField::LENGTH_COPPER);
  cable.__isset.copper = (cable.copper_ref().value_unchecked() != 0);

  if (!cable.__isset.copper) {
    // length and gauge fields currently only supported for copper
    // TODO: migrate all cable types
    return;
  }

  auto overrideDacCableInfo = getDACCableOverride();
  if (overrideDacCableInfo) {
    cable.length_ref().value_unchecked() = overrideDacCableInfo->first;
    cable.gauge_ref().value_unchecked() = overrideDacCableInfo->second;
  } else {
    cable.length_ref().value_unchecked() = getQsfpDACLength();
    cable.gauge_ref().value_unchecked() = getQsfpDACGauge();
  }
  cable.__isset.length = (cable.length_ref().value_unchecked() != 0);
  cable.__isset.gauge = (cable.gauge_ref().value_unchecked() != 0);
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
    XLOG(ERR) << "Unable to retrieve rate select setting: rx(" << std::hex
              << rateRx << " and tx(" << rateTx << ") are not equal";
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
  switch (static_cast<PowerControl>(getSettingsValue(
      SffField::POWER_CONTROL, uint8_t(PowerControl::POWER_CONTROL_MASK)))) {
    case PowerControl::POWER_SET_BY_HW:
      return PowerControlState::POWER_SET_BY_HW;
    case PowerControl::HIGH_POWER_OVERRIDE:
      return PowerControlState::HIGH_POWER_OVERRIDE;
    case PowerControl::POWER_OVERRIDE:
      return PowerControlState::POWER_OVERRIDE;
    default:
      return PowerControlState::POWER_LPMODE;
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
    channels[channel].sensors.rxPwr.flags_ref().value_unchecked() =
        getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
    channels[channel].sensors.rxPwr.__isset.flags = true;
  }

  getQsfpFieldAddress(SffField::CHANNEL_TX_BIAS_ALARMS, dataAddress,
                      offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    channels[channel].sensors.txBias.flags_ref().value_unchecked() =
        getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
    channels[channel].sensors.txBias.__isset.flags = true;
  }

  getQsfpFieldAddress(SffField::CHANNEL_TX_PWR_ALARMS, dataAddress,
                      offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    channels[channel].sensors.txPwr.flags_ref().value_unchecked() =
        getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
    channels[channel].sensors.txPwr.__isset.flags = true;
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

  getQsfpFieldAddress(SffField::CHANNEL_TX_PWR, dataAddress,
                      offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    channel.sensors.txPwr.value = SffFieldInfo::getPwr(value);
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  return true;
}

std::string QsfpModule::getQsfpString(SffField field) const {
  int offset;
  int length;
  int dataAddress;

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t *data = getQsfpValuePtr(dataAddress, offset, length);

  while (length > 0 && data[length - 1] == ' ') {
    --length;
  }

  std::string value (reinterpret_cast<const char*>(data), length);
  return validateQsfpString(value) ? value : "UNKNOWN";
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
double QsfpModule::getQsfpCableLength(SffField field) const {
  double length;
  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields, field);
  const uint8_t *data = getQsfpValuePtr(info.dataAddress,
                                        info.offset, info.length);
  auto multiplier = qsfpMultiplier.at(field);
  length = *data * multiplier;
  if (*data == EEPROM_DEFAULT) {
    // TODO: does this really mean the cable is too long?
    length = -(EEPROM_DEFAULT - 1) * multiplier;
  }
  return length;
}

TransmitterTechnology QsfpModule::getQsfpTransmitterTechnology() const {
  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields,
      SffField::DEVICE_TECHNOLOGY);
  const uint8_t* data =
      getQsfpValuePtr(info.dataAddress, info.offset, info.length);

  uint8_t transTech = *data >> DeviceTechnology::TRANSMITTER_TECH_SHIFT;
  if (transTech == DeviceTechnology::UNKNOWN_VALUE) {
    return TransmitterTechnology::UNKNOWN;
  } else if (transTech <= DeviceTechnology::OPTICAL_MAX_VALUE) {
    return TransmitterTechnology::OPTICAL;
  } else {
    return TransmitterTechnology::COPPER;
  }
}

QsfpModule::QsfpModule(
    std::unique_ptr<TransceiverImpl> qsfpImpl,
    unsigned int portsPerTransceiver)
    : qsfpImpl_(std::move(qsfpImpl)),
      portsPerTransceiver_(portsPerTransceiver) {
  CHECK_GT(portsPerTransceiver_, 0);
}

void QsfpModule::setQsfpIdprom() {
  uint8_t status[2];
  int offset;
  int length;
  int dataAddress;

  if (!present_) {
    throw FbossError("Failed setting QSFP IDProm: QSFP is not present");
  }

  // Check if the data is ready
  getQsfpFieldAddress(SffField::STATUS,
                      dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, status);
  if (status[1] & (1 << 0)) {
    StatsPublisher::bumpModuleErrors();
    throw QsfpModuleError("Failed setting QSFP IDProm: QSFP is not ready");
  }
  flatMem_ = status[1] & (1 << 2);
  XLOG(DBG3) << "Detected QSFP " << qsfpImpl_->getName()
             << ", flatMem=" << flatMem_;
}

const uint8_t* QsfpModule::getQsfpValuePtr(int dataAddress, int offset,
                                           int length) const {
  /* if the cached values are not correct */
  if (!cacheIsValid()) {
    throw FbossError("Qsfp is either not present or the data is not read");
  }
  if (dataAddress == QsfpPages::LOWER) {
    CHECK_LE(offset + length, sizeof(lowerPage_));
    /* Copy data from the cache */
    return(lowerPage_ + offset);
  } else {
    offset -= MAX_QSFP_PAGE_SIZE;
    CHECK_GE(offset, 0);
    CHECK_LE(offset, MAX_QSFP_PAGE_SIZE);
    if (dataAddress == QsfpPages::PAGE0) {
      CHECK_LE(offset + length, sizeof(page0_));
      /* Copy data from the cache */
      return(page0_ + offset);
    } else if (dataAddress == QsfpPages::PAGE3 && !flatMem_) {
      CHECK_LE(offset + length, sizeof(page3_));
      /* Copy data from the cache */
      return(page3_ + offset);
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

// Note that this needs to be called while holding the
// qsfpModuleMutex_
bool QsfpModule::cacheIsValid() const {
  return present_ && !dirty_;
}

TransceiverInfo QsfpModule::getTransceiverInfo() {
  auto cachedInfo = info_.rlock();
  if (!cachedInfo->hasValue()) {
    throw QsfpModuleError("Still populating data...");
  }
  return **cachedInfo;
}

bool QsfpModule::detectPresence() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  return detectPresenceLocked();
}

bool QsfpModule::detectPresenceLocked() {
  auto currentQsfpStatus = qsfpImpl_->detectTransceiver();
  if (currentQsfpStatus != present_) {
    XLOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName())
               << " QSFP status changed to " << currentQsfpStatus;
    dirty_ = true;
    present_ = currentQsfpStatus;
  }
  return currentQsfpStatus;
}

TransceiverInfo QsfpModule::parseDataLocked() {
  TransceiverInfo info;
  info.present = present_;
  info.transceiver = type();
  info.port = qsfpImpl_->getNum();
  if (!present_) {
    return info;
  }

  if (getSensorInfo(info.sensor)) {
    info.__isset.sensor = true;
  }
  if (getVendorInfo(info.vendor)) {
    info.__isset.vendor = true;
  }
  getCableInfo(info.cable);
  info.__isset.cable = true;
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

  if (getTransceiverStats(info.stats)) {
    info.__isset.stats = true;
  }

  return info;
}

RawDOMData QsfpModule::getRawDOMData() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  RawDOMData data;
  if (present_) {
    data.lower = IOBuf::wrapBufferAsValue(lowerPage_, MAX_QSFP_PAGE_SIZE);
    data.page0 = IOBuf::wrapBufferAsValue(page0_, MAX_QSFP_PAGE_SIZE);
    if (!flatMem_) {
      data.page3_ref() = IOBuf::wrapBufferAsValue(page3_, MAX_QSFP_PAGE_SIZE);
    }
  }
  return data;
}

bool QsfpModule::safeToCustomize() const {
  if (ports_.size() < portsPerTransceiver_) {
    XLOG(DBG2) << "Not all ports present in transceiver " << getID()
               << " (expected=" << portsPerTransceiver_
               << "). Skip customization";

    return false;
  } else if (ports_.size() > portsPerTransceiver_) {
    throw FbossError(
      ports_.size(), " ports found in transceiver ", getID(),
      " (max=", portsPerTransceiver_, ")");
  }

  bool anyEnabled{false};
  for (const auto& port : ports_) {
    const auto& status = port.second;
    if (status.up) {
      return false;
    }
    anyEnabled = anyEnabled || status.enabled;
  }

  // Only return safe if at least one port is enabled
  return anyEnabled;
}

bool QsfpModule::customizationWanted(time_t cooldown) const {
  if (needsCustomization_) {
    return true;
  }
  if (std::time(nullptr) - lastCustomizeTime_ < cooldown) {
    return false;
  }
  return safeToCustomize();
}

bool QsfpModule::customizationSupported() const {
  // TODO: there may be a better way of determining this rather than
  // looking at transmitter tech.
  auto tech = getQsfpTransmitterTechnology();
  return present_ && tech != TransmitterTechnology::COPPER;
}

bool QsfpModule::shouldRefresh(time_t cooldown) const {
  return std::time(nullptr) - lastRefreshTime_ >= cooldown;
}

void QsfpModule::ensureOutOfReset() const {
  qsfpImpl_->ensureOutOfReset();
  XLOG(DBG3) << "Cleared the reset register of QSFP.";
}

cfg::PortSpeed QsfpModule::getPortSpeed() const {
  cfg::PortSpeed speed = cfg::PortSpeed::DEFAULT;
  for (const auto& port : ports_) {
    const auto& status = port.second;
    auto newSpeed = cfg::PortSpeed(status.speedMbps);
    if (!status.enabled || speed == newSpeed) {
      continue;
    }

    if (speed == cfg::PortSpeed::DEFAULT) {
      speed = newSpeed;
    } else {
      throw FbossError(
        "Multiple speeds found for member ports of transceiver ", getID());
    }
  }
  return speed;
}

void QsfpModule::transceiverPortsChanged(
    const std::vector<std::pair<const int, PortStatus>>& ports) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);

  for (auto& it : ports) {
    CHECK(TransceiverID(it.second.transceiverIdx.transceiverId) == getID());
    ports_[it.first] = std::move(it.second);
  }

  // update the present_ field (and will set dirty_ if presence change detected)
  detectPresenceLocked();

  if (safeToCustomize()) {
    needsCustomization_ = true;
  }

  if (dirty_) {
    // data is stale. This could happen immediately after plugging a
    // port in. Refresh inline in this case in order to not return
    // stale data.
    refreshLocked();
  }
}

void QsfpModule::refresh() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  refreshLocked();
}

void QsfpModule::refreshLocked() {
  detectPresenceLocked();

  auto customizeWanted = customizationWanted(FLAGS_customize_interval);
  auto willRefresh = !dirty_ && shouldRefresh(FLAGS_qsfp_data_refresh_interval);
  if (!dirty_ && !customizeWanted && !willRefresh) {
    return;
  }

  if (dirty_) {
    // make sure data is up to date before trying to customize.
    ensureOutOfReset();
    updateQsfpData(true);
  }

  if (customizeWanted) {
    customizeTransceiverLocked(getPortSpeed());
  }

  if (customizeWanted || willRefresh) {
    // update either if data is stale or if we customized this
    // round. We update in the customization because we may have
    // written fields, but only need a partial update because all of
    // these fields are in the LOWER qsfp page. There are a small
    // number of writable fields on other qsfp pages, but we don't
    // currently use them.
    updateQsfpData(false);
  }

  // assign
  info_.wlock()->assign(parseDataLocked());
}

void QsfpModule::getFieldValue(SffField fieldName,
                               uint8_t* fieldValue) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  int offset;
  int length;
  int dataAddress;
  getQsfpFieldAddress(fieldName, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, fieldValue);
}

void QsfpModule::updateQsfpData(bool allPages) {
  // expects the lock to be held
  if (!present_) {
    return;
  }
  try {
    XLOG(DBG2) << "Performing " << ((allPages) ? "full" : "partial")
               << " qsfp data cache refresh for transceiver "
               << folly::to<std::string>(qsfpImpl_->getName());
    qsfpImpl_->readTransceiver(TransceiverI2CApi::ADDR_QSFP, 0,
        sizeof(lowerPage_), lowerPage_);
    lastRefreshTime_ = std::time(nullptr);
    dirty_ = false;
    setQsfpIdprom();

    if (!allPages) {
      // Only the first page has fields that change often so provide
      // an option to only fetch that page. Also the write path is
      // particularly slow due to using an i2c bus, so writing the
      // bytes needed to select later pages on non-flat memories can
      // be quite expensive.
      return;
    }

    // If we have flat memory, we don't have to set the page
    if (!flatMem_) {
      uint8_t page = 0;
      qsfpImpl_->writeTransceiver(TransceiverI2CApi::ADDR_QSFP, 127,
          sizeof(page), &page);
    }
    qsfpImpl_->readTransceiver(TransceiverI2CApi::ADDR_QSFP, 128,
        sizeof(page0_), page0_);
    if (!flatMem_) {
      uint8_t page = 3;
      qsfpImpl_->writeTransceiver(TransceiverI2CApi::ADDR_QSFP, 127,
          sizeof(page), &page);
      qsfpImpl_->readTransceiver(TransceiverI2CApi::ADDR_QSFP, 128,
          sizeof(page3_), page3_);
    }
  } catch (const std::exception& ex) {
    // No matter what kind of exception throws, we need to set the dirty_ flag
    // to true.
    dirty_ = true;
    XLOG(ERR) << "Error update data for transceiver:"
                  << folly::to<std::string>(qsfpImpl_->getName()) << ": "
                  << ex.what();
    throw;
  }
}

void QsfpModule::customizeTransceiver(cfg::PortSpeed speed) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  if (present_) {
    customizeTransceiverLocked(speed);
  }
}

void QsfpModule::customizeTransceiverLocked(cfg::PortSpeed speed) {
  /*
   * This must be called with a lock held on qsfpModuleMutex_
   */
  if (customizationSupported()) {
    TransceiverSettings settings;
    getTransceiverSettingsInfo(settings);

    // We want this on regardless of speed
    setPowerOverrideIfSupported(settings.powerControl);

    // make sure TX is enabled on the transceiver
    ensureTxEnabled(FLAGS_tx_enable_interval);

    if (speed != cfg::PortSpeed::DEFAULT) {
      setCdrIfSupported(speed, settings.cdrTx, settings.cdrRx);
      setRateSelectIfSupported(
        speed, settings.rateSelect, settings.rateSelectSetting);
    }
  } else {
    XLOG(DBG1) << "Customization not supported on " << qsfpImpl_->getName();
  }

  lastCustomizeTime_ = std::time(nullptr);
  needsCustomization_ = false;
}

void QsfpModule::setCdrIfSupported(cfg::PortSpeed speed,
                                   FeatureState currentStateTx,
                                   FeatureState currentStateRx) {
  /*
   * Note that this function expects to be called with qsfpModuleMutex_
   * held.
   */
  XLOG(INFO) << "Checking if we need to change CDR on "
             << folly::to<std::string>(qsfpImpl_->getName());

  if (currentStateTx == FeatureState::UNSUPPORTED &&
      currentStateRx == FeatureState::UNSUPPORTED) {
    XLOG(INFO) << "CDR is not supported on "
               << folly::to<std::string>(qsfpImpl_->getName());
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
    XLOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName())
               << " Not changing CDR setting, already correctly set";
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
  XLOG(INFO) << folly::to<std::string>(
      "Port: ",
      qsfpImpl_->getName(),
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
    throw FbossError(folly::to<std::string>("Port: ", qsfpImpl_->getName(),
      " Rate select in unknown state, treating as unsupported: ",
      _RateSelectState_VALUES_TO_NAMES.find(currentState)->second));
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
    throw FbossError(folly::to<std::string>("Port: ", qsfpImpl_->getName(),
      " Unable to set rate select for port speed: ",
      cfg::_PortSpeed_VALUES_TO_NAMES.find(speed)->second));
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
  XLOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName())
             << " set rate select to "
             << _RateSelectSetting_VALUES_TO_NAMES.find(newSetting)->second;
}

void QsfpModule::setPowerOverrideIfSupported(PowerControlState currentState) {
  /* Wedge forces Low Power mode via a pin;  we have to reset this
   * to force High Power mode on all transceivers except SR4-40G.
   *
   * Note that this function expects to be called with qsfpModuleMutex_
   * held.
   */

  int offset;
  int length;
  int dataAddress;
  getQsfpFieldAddress(SffField::ETHERNET_COMPLIANCE, dataAddress,
                      offset, length);
  const uint8_t *ether = getQsfpValuePtr(dataAddress, offset, length);

  getQsfpFieldAddress(SffField::EXTENDED_IDENTIFIER, dataAddress,
                      offset, length);
  const uint8_t *extId = getQsfpValuePtr(dataAddress, offset, length);

  auto desiredSetting = PowerControlState::POWER_OVERRIDE;

  // SR4-40G is represented by a value of 2 - SFF-8636
  // This is the only transceiver that should use LP mode
  if (*ether == EthernetCompliance::SR4_40GBASE) {
    desiredSetting = PowerControlState::POWER_LPMODE;
  } else {
    uint8_t highPowerLevel = (*extId & EXT_ID_HI_POWER_MASK);

    if (highPowerLevel > 0) {
      desiredSetting = PowerControlState::HIGH_POWER_OVERRIDE;
    }
  }

  auto portStr = folly::to<std::string>(qsfpImpl_->getName());
  XLOG(DBG1) << "Port " << portStr << ": Power control "
             << _PowerControlState_VALUES_TO_NAMES.find(currentState)->second
             << " Ext ID " << std::hex << (int)*extId << " Ethernet compliance "
             << std::hex << (int)*ether << " Desired power control "
             << _PowerControlState_VALUES_TO_NAMES.find(desiredSetting)->second;

  if (currentState == desiredSetting &&
      std::time(nullptr) - lastPowerClassReset_ < FLAGS_reset_lpmode_interval) {
    XLOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName())
               << " Power override already correctly set, doing nothing";
    return;
  }

  uint8_t lowPower = uint8_t(PowerControl::POWER_LPMODE);
  uint8_t power = uint8_t(PowerControl::POWER_OVERRIDE);
  if (desiredSetting == PowerControlState::HIGH_POWER_OVERRIDE) {
    power = uint8_t(PowerControl::HIGH_POWER_OVERRIDE);
  } else if (desiredSetting == PowerControlState::POWER_LPMODE) {
    power = uint8_t(PowerControl::POWER_LPMODE);
  }

  getQsfpFieldAddress(SffField::POWER_CONTROL, dataAddress, offset, length);

  // first set to low power
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, offset, sizeof(lowPower), &lowPower);

  // then enable target power class
  if (lowPower != power) {
    qsfpImpl_->writeTransceiver(
        TransceiverI2CApi::ADDR_QSFP, offset, sizeof(power), &power);
  }

  // update last time we reset the power class
  lastPowerClassReset_ = std::time(nullptr);

  XLOG(INFO) << "Port " << portStr << ": QSFP set to power setting "
             << _PowerControlState_VALUES_TO_NAMES.find(desiredSetting)->second
             << " (" << int(power) << ")";
}

void QsfpModule::ensureTxEnabled(time_t cooldown) {
  // Sometimes transceivers lock up and disable TX. When we customize
  // the transceiver let's also ensure that tx is enabled. We have
  // even seen transceivers report to have tx enabled in the DOM, but
  // no traffic was flowing. When we forcibly set the tx_enable bits
  // again, traffic began flowing.  Because of this, we ALWAYS set the
  // bits (even if they report enabled).
  //
  // This is likely a very rare occurence, so we make sure we only write
  // to the transceiver at most every cooldown seconds.
  int offset;
  int length;
  int dataAddress;
  getQsfpFieldAddress(SffField::TX_DISABLE, dataAddress, offset, length);
  const uint8_t *disabled = getQsfpValuePtr(dataAddress, offset, length);

  if (*disabled || std::time(nullptr) - lastTxEnable_ >= cooldown) {
    // If tx is disabled or it has been > cooldown secs, force writ
    std::array<uint8_t, 1> buf = {{0}};
    qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, offset, 1, buf.data());
    lastTxEnable_ = std::time(nullptr);
  }
}

bool QsfpModule::getTransceiverStats(TransceiverStats& stats) {
  auto transceiverStats = qsfpImpl_->getTransceiverStats();
  if (!transceiverStats.hasValue()) {
    return false;
  }
  stats = transceiverStats.value();
  return true;
}

}} //namespace facebook::fboss
