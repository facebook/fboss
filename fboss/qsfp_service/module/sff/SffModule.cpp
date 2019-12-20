// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/sff/SffModule.h"

#include <assert.h>
#include <boost/assign.hpp>
#include <iomanip>
#include <string>
#include "fboss/agent/FbossError.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/qsfp_service/StatsPublisher.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"
#include "fboss/qsfp_service/module/sff/SffFieldInfo.h"

#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>

#include <thrift/lib/cpp/util/EnumUtils.h>

using folly::IOBuf;
using std::lock_guard;
using std::memcpy;
using std::mutex;

using namespace apache::thrift;

namespace {

constexpr int kUsecBetweenPowerModeFlap = 100000;

}

namespace facebook {
namespace fboss {

// As per SFF-8636
static SffFieldInfo::SffFieldMap qsfpFields = {
    // Base page values, including alarms and sensors
    {SffField::IDENTIFIER, {QsfpPages::LOWER, 0, 1}},
    {SffField::STATUS, {QsfpPages::LOWER, 1, 2}},
    {SffField::TEMPERATURE_ALARMS, {QsfpPages::LOWER, 6, 1}},
    {SffField::VCC_ALARMS, {QsfpPages::LOWER, 7, 1}},
    {SffField::CHANNEL_RX_PWR_ALARMS, {QsfpPages::LOWER, 9, 2}},
    {SffField::CHANNEL_TX_BIAS_ALARMS, {QsfpPages::LOWER, 11, 2}},
    {SffField::CHANNEL_TX_PWR_ALARMS, {QsfpPages::LOWER, 13, 2}},
    {SffField::TEMPERATURE, {QsfpPages::LOWER, 22, 2}},
    {SffField::VCC, {QsfpPages::LOWER, 26, 2}},
    {SffField::CHANNEL_RX_PWR, {QsfpPages::LOWER, 34, 8}},
    {SffField::CHANNEL_TX_BIAS, {QsfpPages::LOWER, 42, 8}},
    {SffField::CHANNEL_TX_PWR, {QsfpPages::LOWER, 50, 8}},
    {SffField::TX_DISABLE, {QsfpPages::LOWER, 86, 1}},
    {SffField::RATE_SELECT_RX, {QsfpPages::LOWER, 87, 1}},
    {SffField::RATE_SELECT_TX, {QsfpPages::LOWER, 88, 1}},
    {SffField::POWER_CONTROL, {QsfpPages::LOWER, 93, 1}},
    {SffField::CDR_CONTROL, {QsfpPages::LOWER, 98, 1}},
    {SffField::PAGE_SELECT_BYTE, {QsfpPages::LOWER, 127, 1}},

    // Page 0 values, including vendor info:
    {SffField::EXTENDED_IDENTIFIER, {QsfpPages::PAGE0, 129, 1}},
    {SffField::ETHERNET_COMPLIANCE, {QsfpPages::PAGE0, 131, 1}},
    {SffField::EXTENDED_RATE_COMPLIANCE, {QsfpPages::PAGE0, 141, 1}},
    {SffField::LENGTH_SM_KM, {QsfpPages::PAGE0, 142, 1}},
    {SffField::LENGTH_OM3, {QsfpPages::PAGE0, 143, 1}},
    {SffField::LENGTH_OM2, {QsfpPages::PAGE0, 144, 1}},
    {SffField::LENGTH_OM1, {QsfpPages::PAGE0, 145, 1}},
    {SffField::LENGTH_COPPER, {QsfpPages::PAGE0, 146, 1}},
    {SffField::DEVICE_TECHNOLOGY, {QsfpPages::PAGE0, 147, 1}},
    {SffField::VENDOR_NAME, {QsfpPages::PAGE0, 148, 16}},
    {SffField::VENDOR_OUI, {QsfpPages::PAGE0, 165, 3}},
    {SffField::PART_NUMBER, {QsfpPages::PAGE0, 168, 16}},
    {SffField::REVISION_NUMBER, {QsfpPages::PAGE0, 184, 2}},
    {SffField::OPTIONS, {QsfpPages::PAGE0, 195, 1}},
    {SffField::VENDOR_SERIAL_NUMBER, {QsfpPages::PAGE0, 196, 16}},
    {SffField::MFG_DATE, {QsfpPages::PAGE0, 212, 8}},
    {SffField::DIAGNOSTIC_MONITORING_TYPE, {QsfpPages::PAGE0, 220, 1}},
    {SffField::ENHANCED_OPTIONS, {QsfpPages::PAGE0, 221, 1}},

    // These are custom fields FB gets cable vendors to populate.
    // TODO: add support for turning off these fields via a command-line flag
    {SffField::LENGTH_COPPER_DECIMETERS, {QsfpPages::PAGE0, 236, 1}},
    {SffField::DAC_GAUGE, {QsfpPages::PAGE0, 237, 1}},

    // Page 3 values, including alarm and warning threshold values:
    {SffField::TEMPERATURE_THRESH, {QsfpPages::PAGE3, 128, 8}},
    {SffField::VCC_THRESH, {QsfpPages::PAGE3, 144, 8}},
    {SffField::RX_PWR_THRESH, {QsfpPages::PAGE3, 176, 8}},
    {SffField::TX_BIAS_THRESH, {QsfpPages::PAGE3, 184, 8}},
};

static SffFieldMultiplier qsfpMultiplier = {
    {SffField::LENGTH_SM_KM, 1000},
    {SffField::LENGTH_OM3, 2},
    {SffField::LENGTH_OM2, 1},
    {SffField::LENGTH_OM1, 1},
    {SffField::LENGTH_COPPER, 1},
    {SffField::LENGTH_COPPER_DECIMETERS, 0.1},
};

void getQsfpFieldAddress(
    SffField field,
    int& dataAddress,
    int& offset,
    int& length) {
  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields, field);
  dataAddress = info.dataAddress;
  offset = info.offset;
  length = info.length;
}

SffModule::SffModule(
    std::unique_ptr<TransceiverImpl> qsfpImpl,
    unsigned int portsPerTransceiver)
    : QsfpModule(std::move(qsfpImpl), portsPerTransceiver) {
  CHECK_GT(portsPerTransceiver, 0);
}

SffModule::~SffModule() {}

FlagLevels SffModule::getQsfpSensorFlags(SffField fieldName) {
  int offset;
  int length;
  int dataAddress;

  /* Determine if QSFP is present */
  getQsfpFieldAddress(fieldName, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);
  return getQsfpFlags(data, 4);
}

double SffModule::getQsfpDACLength() const {
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

int SffModule::getQsfpDACGauge() const {
  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields, SffField::DAC_GAUGE);
  const uint8_t* val =
      getQsfpValuePtr(info.dataAddress, info.offset, info.length);
  // Guard against FF default value
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
  } else {
    return gauge;
  }
}

bool SffModule::getSensorInfo(GlobalSensors& info) {
  info.temp.value = getQsfpSensor(SffField::TEMPERATURE, SffFieldInfo::getTemp);
  info.temp.flags_ref().value_unchecked() =
      getQsfpSensorFlags(SffField::TEMPERATURE_ALARMS);
  info.temp.__isset.flags = true;
  info.vcc.value = getQsfpSensor(SffField::VCC, SffFieldInfo::getVcc);
  info.vcc.flags_ref().value_unchecked() =
      getQsfpSensorFlags(SffField::VCC_ALARMS);
  info.vcc.__isset.flags = true;
  return true;
}

bool SffModule::getVendorInfo(Vendor& vendor) {
  vendor.name = getQsfpString(SffField::VENDOR_NAME);
  vendor.oui = getQsfpString(SffField::VENDOR_OUI);
  vendor.partNumber = getQsfpString(SffField::PART_NUMBER);
  vendor.rev = getQsfpString(SffField::REVISION_NUMBER);
  vendor.serialNumber = getQsfpString(SffField::VENDOR_SERIAL_NUMBER);
  vendor.dateCode = getQsfpString(SffField::MFG_DATE);
  return true;
}

void SffModule::getCableInfo(Cable& cable) {
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
ThresholdLevels SffModule::getThresholdValues(
    SffField field,
    double (*conversion)(uint16_t value)) {
  int offset;
  int length;
  int dataAddress;
  ThresholdLevels thresh;

  CHECK(!flatMem_);

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  CHECK_GE(length, 8);
  thresh.alarm.high = conversion(data[0] << 8 | data[1]);
  thresh.alarm.low = conversion(data[2] << 8 | data[3]);
  thresh.warn.high = conversion(data[4] << 8 | data[5]);
  thresh.warn.low = conversion(data[6] << 8 | data[7]);

  return thresh;
}

bool SffModule::getThresholdInfo(AlarmThreshold& threshold) {
  if (flatMem_) {
    return false;
  }
  threshold.temp =
      getThresholdValues(SffField::TEMPERATURE_THRESH, SffFieldInfo::getTemp);
  threshold.vcc =
      getThresholdValues(SffField::VCC_THRESH, SffFieldInfo::getVcc);
  threshold.rxPwr =
      getThresholdValues(SffField::RX_PWR_THRESH, SffFieldInfo::getPwr);
  threshold.txBias =
      getThresholdValues(SffField::TX_BIAS_THRESH, SffFieldInfo::getTxBias);
  return true;
}

uint8_t SffModule::getSettingsValue(SffField field, uint8_t mask) {
  int offset;
  int length;
  int dataAddress;

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  return data[0] & mask;
}

bool SffModule::getTransceiverSettingsInfo(TransceiverSettings& settings) {
  settings.cdrTx = SffFieldInfo::getFeatureState(
      getSettingsValue(SffField::EXTENDED_IDENTIFIER, EXT_ID_CDR_TX_MASK),
      getSettingsValue(SffField::CDR_CONTROL, CDR_CONTROL_TX_MASK));
  settings.cdrRx = SffFieldInfo::getFeatureState(
      getSettingsValue(SffField::EXTENDED_IDENTIFIER, EXT_ID_CDR_RX_MASK),
      getSettingsValue(SffField::CDR_CONTROL, CDR_CONTROL_RX_MASK));

  settings.powerMeasurement = SffFieldInfo::getFeatureState(getSettingsValue(
      SffField::DIAGNOSTIC_MONITORING_TYPE, POWER_MEASUREMENT_MASK));

  settings.powerControl = getPowerControlValue();
  settings.rateSelect = getRateSelectValue();
  settings.rateSelectSetting = getRateSelectSettingValue(settings.rateSelect);

  return true;
}

RateSelectSetting SffModule::getRateSelectSettingValue(RateSelectState state) {
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
  return (RateSelectSetting)channelRate;
}

RateSelectState SffModule::getRateSelectValue() {
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

  uint8_t extendedRateCompliance =
      getSettingsValue(SffField::EXTENDED_RATE_COMPLIANCE);
  if (enhancedOptions == 0b10 && (extendedRateCompliance & 0b01)) {
    return RateSelectState::EXTENDED_RATE_SELECT_V1;
  } else if (enhancedOptions == 0b10 && (extendedRateCompliance & 0b10)) {
    return RateSelectState::EXTENDED_RATE_SELECT_V2;
  } else if (enhancedOptions == 0b01) {
    return RateSelectState::APPLICATION_RATE_SELECT;
  }

  return RateSelectState::UNSUPPORTED;
}

PowerControlState SffModule::getPowerControlValue() {
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

bool SffModule::getSensorsPerChanInfo(std::vector<Channel>& channels) {
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
  std::array<uint8_t, 4> bitOffset{{4, 0, 4, 0}};
  std::array<uint8_t, 4> byteOffset{{0, 0, 1, 1}};

  assert (channels.size() == CHANNEL_COUNT);

  getQsfpFieldAddress(
      SffField::CHANNEL_RX_PWR_ALARMS, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    channels[channel].sensors.rxPwr.flags_ref().value_unchecked() =
        getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
    channels[channel].sensors.rxPwr.__isset.flags = true;
  }

  getQsfpFieldAddress(
      SffField::CHANNEL_TX_BIAS_ALARMS, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    channels[channel].sensors.txBias.flags_ref().value_unchecked() =
        getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
    channels[channel].sensors.txBias.__isset.flags = true;
  }

  getQsfpFieldAddress(
      SffField::CHANNEL_TX_PWR_ALARMS, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    channels[channel].sensors.txPwr.flags_ref().value_unchecked() =
        getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
    channels[channel].sensors.txPwr.__isset.flags = true;
  }

  getQsfpFieldAddress(SffField::CHANNEL_RX_PWR, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    channel.sensors.rxPwr.value = SffFieldInfo::getPwr(value);
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  getQsfpFieldAddress(SffField::CHANNEL_TX_BIAS, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);
  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    channel.sensors.txBias.value = SffFieldInfo::getTxBias(value);
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  getQsfpFieldAddress(SffField::CHANNEL_TX_PWR, dataAddress, offset, length);
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

std::string SffModule::getQsfpString(SffField field) const {
  int offset;
  int length;
  int dataAddress;

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  while (length > 0 && data[length - 1] == ' ') {
    --length;
  }

  std::string value(reinterpret_cast<const char*>(data), length);
  return validateQsfpString(value) ? value : "UNKNOWN";
}

double SffModule::getQsfpSensor(
    SffField field,
    double (*conversion)(uint16_t value)) {
  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields, field);
  const uint8_t* data =
      getQsfpValuePtr(info.dataAddress, info.offset, info.length);
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
double SffModule::getQsfpCableLength(SffField field) const {
  double length;
  auto info = SffFieldInfo::getSffFieldAddress(qsfpFields, field);
  const uint8_t* data =
      getQsfpValuePtr(info.dataAddress, info.offset, info.length);
  auto multiplier = qsfpMultiplier.at(field);
  length = *data * multiplier;
  if (*data == EEPROM_DEFAULT) {
    // TODO: does this really mean the cable is too long?
    length = -(EEPROM_DEFAULT - 1) * multiplier;
  }
  return length;
}

TransmitterTechnology SffModule::getQsfpTransmitterTechnology() const {
  auto info =
      SffFieldInfo::getSffFieldAddress(qsfpFields, SffField::DEVICE_TECHNOLOGY);
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

void SffModule::setQsfpIdprom() {
  std::array<uint8_t, 2> status = {{0, 0}};
  int offset;
  int length;
  int dataAddress;

  if (!present_) {
    throw FbossError("Failed setting QSFP IDProm: QSFP is not present");
  }

  // Check if the data is ready
  getQsfpFieldAddress(SffField::STATUS, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, status.data());
  if (status[1] & (1 << 0)) {
    StatsPublisher::bumpModuleErrors();
    throw QsfpModuleError("Failed setting QSFP IDProm: QSFP is not ready");
  }
  flatMem_ = status[1] & (1 << 2);
  XLOG(DBG3) << "Detected QSFP " << qsfpImpl_->getName()
             << ", flatMem=" << flatMem_;
}

const uint8_t*
SffModule::getQsfpValuePtr(int dataAddress, int offset, int length) const {
  /* if the cached values are not correct */
  if (!cacheIsValid()) {
    throw FbossError("Qsfp is either not present or the data is not read");
  }
  if (dataAddress == QsfpPages::LOWER) {
    CHECK_LE(offset + length, sizeof(lowerPage_));
    /* Copy data from the cache */
    return (lowerPage_ + offset);
  } else {
    offset -= MAX_QSFP_PAGE_SIZE;
    CHECK_GE(offset, 0);
    CHECK_LE(offset, MAX_QSFP_PAGE_SIZE);
    if (dataAddress == QsfpPages::PAGE0) {
      CHECK_LE(offset + length, sizeof(page0_));
      /* Copy data from the cache */
      return (page0_ + offset);
    } else if (dataAddress == QsfpPages::PAGE3 && !flatMem_) {
      CHECK_LE(offset + length, sizeof(page3_));
      /* Copy data from the cache */
      return (page3_ + offset);
    } else {
      throw FbossError("Invalid Data Address 0x%d", dataAddress);
    }
  }
}

RawDOMData SffModule::getRawDOMData() {
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

void SffModule::getFieldValue(SffField fieldName, uint8_t* fieldValue) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  int offset;
  int length;
  int dataAddress;
  getQsfpFieldAddress(fieldName, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, fieldValue);
}

void SffModule::updateQsfpData(bool allPages) {
  // expects the lock to be held
  if (!present_) {
    return;
  }
  try {
    XLOG(DBG2) << "Performing " << ((allPages) ? "full" : "partial")
               << " qsfp data cache refresh for transceiver "
               << folly::to<std::string>(qsfpImpl_->getName());
    qsfpImpl_->readTransceiver(
        TransceiverI2CApi::ADDR_QSFP, 0, sizeof(lowerPage_), lowerPage_);
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
      qsfpImpl_->writeTransceiver(
          TransceiverI2CApi::ADDR_QSFP, 127, sizeof(page), &page);
    }
    qsfpImpl_->readTransceiver(
        TransceiverI2CApi::ADDR_QSFP, 128, sizeof(page0_), page0_);
    if (!flatMem_) {
      uint8_t page = 3;
      qsfpImpl_->writeTransceiver(
          TransceiverI2CApi::ADDR_QSFP, 127, sizeof(page), &page);
      qsfpImpl_->readTransceiver(
          TransceiverI2CApi::ADDR_QSFP, 128, sizeof(page3_), page3_);
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

void SffModule::setCdrIfSupported(
    cfg::PortSpeed speed,
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
    return state != FeatureState::UNSUPPORTED &&
        ((speed == cfg::PortSpeed::HUNDREDG &&
          state != FeatureState::ENABLED) ||
         (speed != cfg::PortSpeed::HUNDREDG &&
          state != FeatureState::DISABLED));
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
  getQsfpFieldAddress(
      SffField::CDR_CONTROL, dataAddress, dataOffset, dataLength);

  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, dataOffset, sizeof(value), &value);
  XLOG(INFO) << folly::to<std::string>(
      "Port: ",
      qsfpImpl_->getName(),
      " Setting CDR to state: ",
      apache::thrift::util::enumNameSafe(newState));
}

void SffModule::setRateSelectIfSupported(
    cfg::PortSpeed speed,
    RateSelectState currentState,
    RateSelectSetting currentSetting) {
  if (currentState == RateSelectState::UNSUPPORTED) {
    return;
  } else if (currentState == RateSelectState::APPLICATION_RATE_SELECT) {
    // Currently only support extended rate select, so treat application
    // rate select as an invalid option
    throw FbossError(folly::to<std::string>(
        "Port: ",
        qsfpImpl_->getName(),
        " Rate select in unknown state, treating as unsupported: ",
        apache::thrift::util::enumNameSafe(currentState)));
  }

  uint8_t value;
  RateSelectSetting newSetting;
  bool alreadySet = false;
  auto translateEnum = [currentSetting, &value, &newSetting](
                           RateSelectSetting desired, uint8_t newValue) {
    if (currentSetting == desired) {
      return true;
    }
    newSetting = desired;
    value = newValue;
    return false;
  };

  if (currentState == RateSelectState::EXTENDED_RATE_SELECT_V1) {
    // Use the highest possible speed in this version
    alreadySet =
        translateEnum(RateSelectSetting::FROM_6_6GB_AND_ABOVE, 0b10101010);
  } else if (speed == cfg::PortSpeed::FORTYG) {
    // Optimised for 10G channels
    alreadySet = translateEnum(RateSelectSetting::LESS_THAN_12GB, 0b00000000);
  } else if (speed == cfg::PortSpeed::HUNDREDG) {
    // Optimised for 25GB channels
    alreadySet =
        translateEnum(RateSelectSetting::FROM_24GB_to_26GB, 0b10101010);
  } else {
    throw FbossError(folly::to<std::string>(
        "Port: ",
        qsfpImpl_->getName(),
        " Unable to set rate select for port speed: ",
        apache::thrift::util::enumNameSafe(speed)));
  }

  if (alreadySet) {
    return;
  }

  int dataLength, dataAddress, dataOffset;

  getQsfpFieldAddress(
      SffField::RATE_SELECT_RX, dataAddress, dataOffset, dataLength);
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, dataOffset, sizeof(value), &value);

  getQsfpFieldAddress(
      SffField::RATE_SELECT_RX, dataAddress, dataOffset, dataLength);
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, dataOffset, sizeof(value), &value);
  XLOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName())
             << " set rate select to "
             << apache::thrift::util::enumNameSafe(newSetting);
}

void SffModule::setPowerOverrideIfSupported(PowerControlState currentState) {
  /* Wedge forces Low Power mode via a pin;  we have to reset this
   * to force High Power mode on all transceivers except SR4-40G.
   *
   * Note that this function expects to be called with qsfpModuleMutex_
   * held.
   */

  int offset;
  int length;
  int dataAddress;
  getQsfpFieldAddress(
      SffField::ETHERNET_COMPLIANCE, dataAddress, offset, length);
  const uint8_t* ether = getQsfpValuePtr(dataAddress, offset, length);

  getQsfpFieldAddress(
      SffField::EXTENDED_IDENTIFIER, dataAddress, offset, length);
  const uint8_t* extId = getQsfpValuePtr(dataAddress, offset, length);

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
             << apache::thrift::util::enumNameSafe(currentState)
             << " Ext ID " << std::hex << (int)*extId << " Ethernet compliance "
             << std::hex << (int)*ether << " Desired power control "
             << apache::thrift::util::enumNameSafe(desiredSetting);

  if (currentState == desiredSetting) {
    XLOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName())
               << " Power override already correctly set, doing nothing";
    return;
  }

  uint8_t power = uint8_t(PowerControl::POWER_OVERRIDE);
  if (desiredSetting == PowerControlState::HIGH_POWER_OVERRIDE) {
    power = uint8_t(PowerControl::HIGH_POWER_OVERRIDE);
  } else if (desiredSetting == PowerControlState::POWER_LPMODE) {
    power = uint8_t(PowerControl::POWER_LPMODE);
  }

  // enable target power class
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, offset, sizeof(power), &power);

  getQsfpFieldAddress(SffField::POWER_CONTROL, dataAddress, offset, length);

  XLOG(INFO) << "Port " << portStr << ": QSFP set to power setting "
             << apache::thrift::util::enumNameSafe(desiredSetting)
             << " (" << int(power) << ")";
}

/*
 * Put logic here that should only be run on ports that have been
 * down for a long time. These are actions that are potentially more
 * disruptive, but have worked in the past to recover a transceiver.
 */
void SffModule::remediateFlakyTransceiver() {
  XLOG(DBG0) << "Performing potentially disruptive remediations on "
             << qsfpImpl_->getName();

  ensureTxEnabled();
  resetLowPowerMode();
  lastRemediateTime_ = std::time(nullptr);
}
void SffModule::resetLowPowerMode() {
  // Newer transceivers will have a auto-clearing reset bit that is
  // probably better to use than this.

  int offset, length, dataAddress;
  getQsfpFieldAddress(SffField::POWER_CONTROL, dataAddress, offset, length);

  uint8_t oldPower;
  getQsfpValue(dataAddress, offset, length, &oldPower);

  uint8_t lowPower = uint8_t(PowerControl::POWER_LPMODE);

  if (oldPower != lowPower) {
    // first set to low power
    qsfpImpl_->writeTransceiver(
        TransceiverI2CApi::ADDR_QSFP, offset, sizeof(lowPower), &lowPower);

    // Transceivers need a bit of time to handle the low power setting
    // we just sent. We should be able to use the status register to be
    // smarter about this, but just sleeping 0.1s for now.
    usleep(kUsecBetweenPowerModeFlap);
  }

  // set back to previous value
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, offset, sizeof(oldPower), &oldPower);
}

void SffModule::ensureTxEnabled() {
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

  // Force enable
  std::array<uint8_t, 1> buf = {{0}};
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, offset, 1, buf.data());
}

} // namespace fboss
} // namespace facebook
