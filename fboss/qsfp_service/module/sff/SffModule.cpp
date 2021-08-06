// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/sff/SffModule.h"

#include <assert.h>
#include <boost/assign.hpp>
#include <iomanip>
#include <string>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMode.h"
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

// TODO: Since this is an extended experiment, we resue the aoi_override flag
// to mark the ones that will apply the settings overwrite. Will rename when
// override become official across DC.
DEFINE_bool(
    aoi_override,
    false,
    "To override channel control settings on optic modules");

namespace {

constexpr int kUsecBetweenPowerModeFlap = 100000;

}

namespace facebook {
namespace fboss {

// As per SFF-8436, QSFP+ 10 Gbs 4X PLUGGABLE TRANSCEIVER spec

enum SffPages {
  LOWER,
  PAGE0,
  PAGE3,
};

// As per SFF-8636
static SffFieldInfo::SffFieldMap qsfpFields = {
    // Base page values, including alarms and sensors
    {SffField::IDENTIFIER, {SffPages::LOWER, 0, 1}},
    {SffField::STATUS, {SffPages::LOWER, 1, 2}},
    {SffField::LOS, {SffPages::LOWER, 3, 1}},
    {SffField::FAULT, {SffPages::LOWER, 4, 1}},
    {SffField::LOL, {SffPages::LOWER, 5, 1}},
    {SffField::TEMPERATURE_ALARMS, {SffPages::LOWER, 6, 1}},
    {SffField::VCC_ALARMS, {SffPages::LOWER, 7, 1}},
    {SffField::CHANNEL_RX_PWR_ALARMS, {SffPages::LOWER, 9, 2}},
    {SffField::CHANNEL_TX_BIAS_ALARMS, {SffPages::LOWER, 11, 2}},
    {SffField::CHANNEL_TX_PWR_ALARMS, {SffPages::LOWER, 13, 2}},
    {SffField::TEMPERATURE, {SffPages::LOWER, 22, 2}},
    {SffField::VCC, {SffPages::LOWER, 26, 2}},
    {SffField::CHANNEL_RX_PWR, {SffPages::LOWER, 34, 8}},
    {SffField::CHANNEL_TX_BIAS, {SffPages::LOWER, 42, 8}},
    {SffField::CHANNEL_TX_PWR, {SffPages::LOWER, 50, 8}},
    {SffField::TX_DISABLE, {SffPages::LOWER, 86, 1}},
    {SffField::RATE_SELECT_RX, {SffPages::LOWER, 87, 1}},
    {SffField::RATE_SELECT_TX, {SffPages::LOWER, 88, 1}},
    {SffField::POWER_CONTROL, {SffPages::LOWER, 93, 1}},
    {SffField::CDR_CONTROL, {SffPages::LOWER, 98, 1}},
    {SffField::PAGE_SELECT_BYTE, {SffPages::LOWER, 127, 1}},

    // Page 0 values, including vendor info:
    {SffField::EXTENDED_IDENTIFIER, {SffPages::PAGE0, 129, 1}},
    {SffField::ETHERNET_COMPLIANCE, {SffPages::PAGE0, 131, 1}},
    {SffField::EXTENDED_RATE_COMPLIANCE, {SffPages::PAGE0, 141, 1}},
    {SffField::LENGTH_SM_KM, {SffPages::PAGE0, 142, 1}},
    {SffField::LENGTH_OM3, {SffPages::PAGE0, 143, 1}},
    {SffField::LENGTH_OM2, {SffPages::PAGE0, 144, 1}},
    {SffField::LENGTH_OM1, {SffPages::PAGE0, 145, 1}},
    {SffField::LENGTH_COPPER, {SffPages::PAGE0, 146, 1}},
    {SffField::DEVICE_TECHNOLOGY, {SffPages::PAGE0, 147, 1}},
    {SffField::VENDOR_NAME, {SffPages::PAGE0, 148, 16}},
    {SffField::VENDOR_OUI, {SffPages::PAGE0, 165, 3}},
    {SffField::PART_NUMBER, {SffPages::PAGE0, 168, 16}},
    {SffField::REVISION_NUMBER, {SffPages::PAGE0, 184, 2}},
    {SffField::PAGE0_CSUM, {SffPages::PAGE0, 191, 1}},
    {SffField::EXTENDED_SPECIFICATION_COMPLIANCE, {SffPages::PAGE0, 192, 1}},
    {SffField::OPTIONS, {SffPages::PAGE0, 195, 1}},
    {SffField::VENDOR_SERIAL_NUMBER, {SffPages::PAGE0, 196, 16}},
    {SffField::MFG_DATE, {SffPages::PAGE0, 212, 8}},
    {SffField::DIAGNOSTIC_MONITORING_TYPE, {SffPages::PAGE0, 220, 1}},
    {SffField::ENHANCED_OPTIONS, {SffPages::PAGE0, 221, 1}},
    {SffField::PAGE0_EXTCSUM, {SffPages::PAGE0, 223, 1}},

    // These are custom fields FB gets cable vendors to populate.
    // TODO: add support for turning off these fields via a command-line flag
    {SffField::LENGTH_COPPER_DECIMETERS, {SffPages::PAGE0, 236, 1}},
    {SffField::DAC_GAUGE, {SffPages::PAGE0, 237, 1}},

    // Page 3 values, including alarm and warning threshold values:
    {SffField::TEMPERATURE_THRESH, {SffPages::PAGE3, 128, 8}},
    {SffField::VCC_THRESH, {SffPages::PAGE3, 144, 8}},
    {SffField::RX_PWR_THRESH, {SffPages::PAGE3, 176, 8}},
    {SffField::TX_BIAS_THRESH, {SffPages::PAGE3, 184, 8}},
    {SffField::TX_PWR_THRESH, {SffPages::PAGE3, 192, 8}},
    {SffField::TX_EQUALIZATION, {SffPages::PAGE3, 234, 2}},
    {SffField::RX_EMPHASIS, {SffPages::PAGE3, 236, 2}},
    {SffField::RX_AMPLITUDE, {SffPages::PAGE3, 238, 2}},
    {SffField::SQUELCH_CONTROL, {SffPages::PAGE3, 240, 1}},
    {SffField::TXRX_OUTPUT_CONTROL, {SffPages::PAGE3, 241, 1}},
};

static SffFieldMultiplier qsfpMultiplier = {
    {SffField::LENGTH_SM_KM, 1000},
    {SffField::LENGTH_OM3, 2},
    {SffField::LENGTH_OM2, 1},
    {SffField::LENGTH_OM1, 1},
    {SffField::LENGTH_COPPER, 1},
    {SffField::LENGTH_COPPER_DECIMETERS, 0.1},
};

constexpr uint8_t kPage0CsumRangeStart = 128;
constexpr uint8_t kPage0CsumRangeLength = 63;
constexpr uint8_t kPage0ExtCsumRangeStart = 192;
constexpr uint8_t kPage0ExtCsumRangeLength = 31;

struct checksumInfoStruct {
  uint8_t checksumRangeStartOffset;
  uint8_t checksumRangeStartLength;
  SffField checksumValOffset;
};

static std::map<int, std::vector<checksumInfoStruct>> checksumInfoSff = {
    {
        SffPages::PAGE0,
        {
            {kPage0CsumRangeStart, kPage0CsumRangeLength, SffField::PAGE0_CSUM},
            {kPage0ExtCsumRangeStart,
             kPage0ExtCsumRangeLength,
             SffField::PAGE0_EXTCSUM},
        },
    },
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
    TransceiverManager* transceiverManager,
    std::unique_ptr<TransceiverImpl> qsfpImpl,
    unsigned int portsPerTransceiver)
    : QsfpModule(transceiverManager, std::move(qsfpImpl), portsPerTransceiver) {
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

GlobalSensors SffModule::getSensorInfo() {
  GlobalSensors info = GlobalSensors();
  info.temp_ref()->value_ref() =
      getQsfpSensor(SffField::TEMPERATURE, SffFieldInfo::getTemp);
  info.temp_ref()->flags_ref() =
      getQsfpSensorFlags(SffField::TEMPERATURE_ALARMS);
  info.vcc_ref()->value_ref() =
      getQsfpSensor(SffField::VCC, SffFieldInfo::getVcc);
  info.vcc_ref()->flags_ref() = getQsfpSensorFlags(SffField::VCC_ALARMS);
  return info;
}

Vendor SffModule::getVendorInfo() {
  Vendor vendor = Vendor();
  *vendor.name_ref() = getQsfpString(SffField::VENDOR_NAME);
  *vendor.oui_ref() = getQsfpString(SffField::VENDOR_OUI);
  *vendor.partNumber_ref() = getQsfpString(SffField::PART_NUMBER);
  *vendor.rev_ref() = getQsfpString(SffField::REVISION_NUMBER);
  *vendor.serialNumber_ref() = getQsfpString(SffField::VENDOR_SERIAL_NUMBER);
  *vendor.dateCode_ref() = getQsfpString(SffField::MFG_DATE);
  return vendor;
}

Cable SffModule::getCableInfo() {
  Cable cable = Cable();
  cable.transmitterTech_ref() = getQsfpTransmitterTechnology();

  cable.singleMode_ref() = getQsfpCableLength(SffField::LENGTH_SM_KM);
  if (cable.singleMode_ref().value_or({}) == 0) {
    cable.singleMode_ref().reset();
  }
  cable.om3_ref() = getQsfpCableLength(SffField::LENGTH_OM3);
  if (cable.om3_ref().value_or({}) == 0) {
    cable.om3_ref().reset();
  }
  cable.om2_ref() = getQsfpCableLength(SffField::LENGTH_OM2);
  if (cable.om2_ref().value_or({}) == 0) {
    cable.om2_ref().reset();
  }
  cable.om1_ref() = getQsfpCableLength(SffField::LENGTH_OM1);
  if (cable.om1_ref().value_or({}) == 0) {
    cable.om1_ref().reset();
  }
  cable.copper_ref() = getQsfpCableLength(SffField::LENGTH_COPPER);
  if (cable.copper_ref().value_or({}) == 0) {
    cable.copper_ref().reset();
  }
  if (!cable.copper_ref()) {
    // length and gauge fields currently only supported for copper
    // TODO: migrate all cable types
    return cable;
  }

  auto overrideDacCableInfo = getDACCableOverride();
  if (overrideDacCableInfo) {
    cable.length_ref() = overrideDacCableInfo->first;
    cable.gauge_ref() = overrideDacCableInfo->second;
  } else {
    cable.length_ref() = getQsfpDACLength();
    cable.gauge_ref() = getQsfpDACGauge();
  }
  if (cable.length_ref().value_or({}) == 0) {
    cable.length_ref().reset();
  }
  if (cable.gauge_ref().value_or({}) == 0) {
    cable.gauge_ref().reset();
  }
  return cable;
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
  thresh.alarm_ref()->high_ref() = conversion(data[0] << 8 | data[1]);
  thresh.alarm_ref()->low_ref() = conversion(data[2] << 8 | data[3]);
  thresh.warn_ref()->high_ref() = conversion(data[4] << 8 | data[5]);
  thresh.warn_ref()->low_ref() = conversion(data[6] << 8 | data[7]);

  return thresh;
}

std::optional<AlarmThreshold> SffModule::getThresholdInfo() {
  if (flatMem_) {
    return {};
  }
  AlarmThreshold threshold = AlarmThreshold();
  threshold.temp_ref() =
      getThresholdValues(SffField::TEMPERATURE_THRESH, SffFieldInfo::getTemp);
  threshold.vcc_ref() =
      getThresholdValues(SffField::VCC_THRESH, SffFieldInfo::getVcc);
  threshold.rxPwr_ref() =
      getThresholdValues(SffField::RX_PWR_THRESH, SffFieldInfo::getPwr);
  threshold.txBias_ref() =
      getThresholdValues(SffField::TX_BIAS_THRESH, SffFieldInfo::getTxBias);
  threshold.txPwr_ref() =
      getThresholdValues(SffField::TX_PWR_THRESH, SffFieldInfo::getPwr);
  return threshold;
}

uint8_t SffModule::getSettingsValue(SffField field, uint8_t mask) const {
  int offset;
  int length;
  int dataAddress;

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  return data[0] & mask;
}

TransceiverModuleIdentifier SffModule::getIdentifier() {
  return (TransceiverModuleIdentifier)getSettingsValue(SffField::IDENTIFIER);
}

TransceiverSettings SffModule::getTransceiverSettingsInfo() {
  TransceiverSettings settings = TransceiverSettings();
  settings.cdrTx_ref() = SffFieldInfo::getFeatureState(
      getSettingsValue(SffField::EXTENDED_IDENTIFIER, EXT_ID_CDR_TX_MASK),
      getSettingsValue(SffField::CDR_CONTROL, UPPER_BITS_MASK));
  settings.cdrRx_ref() = SffFieldInfo::getFeatureState(
      getSettingsValue(SffField::EXTENDED_IDENTIFIER, EXT_ID_CDR_RX_MASK),
      getSettingsValue(SffField::CDR_CONTROL, LOWER_BITS_MASK));
  settings.powerMeasurement_ref() =
      SffFieldInfo::getFeatureState(getSettingsValue(
          SffField::DIAGNOSTIC_MONITORING_TYPE, POWER_MEASUREMENT_MASK));
  settings.powerControl_ref() = getPowerControlValue();
  settings.rateSelect_ref() = getRateSelectValue();
  settings.rateSelectSetting_ref() =
      getRateSelectSettingValue(*settings.rateSelect_ref());

  settings.mediaInterface_ref() =
      std::vector<MediaInterfaceId>(numMediaLanes());
  if (!getMediaInterfaceId(*(settings.mediaInterface_ref()))) {
    settings.mediaInterface_ref().reset();
  }

  settings.mediaLaneSettings_ref() =
      std::vector<MediaLaneSettings>(numMediaLanes());
  settings.hostLaneSettings_ref() =
      std::vector<HostLaneSettings>(numHostLanes());
  if (!flatMem_) {
    if (!getMediaLaneSettings(*(settings.mediaLaneSettings_ref()))) {
      settings.mediaLaneSettings_ref().reset();
    }
    if (!getHostLaneSettings(*(settings.hostLaneSettings_ref()))) {
      settings.hostLaneSettings_ref().reset();
    }
  }
  return settings;
}

unsigned int SffModule::numHostLanes() const {
  // Always returns 4 for now. This should return the number of
  // lanes based on the media type instead
  return 4;
}

unsigned int SffModule::numMediaLanes() const {
  // This should return the number of lanes based on the media type
  // Return 4 for all media types except FR1. When other media type support
  // is added that have channel count different than 4, we need to
  // change this function to reflect that.

  ExtendedSpecComplianceCode ext_comp_code =
      getExtendedSpecificationComplianceCode();

  if (ext_comp_code == ExtendedSpecComplianceCode::FR1_100G) {
    return 1;
  } else {
    return 4;
  }
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
    XLOG(ERR) << folly::sformat(
        "Unable to retrieve rate select setting: rx({:#x}) and tx({:#x}) are not equal",
        rateRx,
        rateTx);

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
 * Iterate through the host lanes collecting appropriate data;
 */

bool SffModule::getSignalsPerHostLane(std::vector<HostLaneSignals>& signals) {
  assert(signals.size() == numHostLanes());
  // Currently no signals to report on host lanes
  return false;
}

/*
 * Iterate through the media channels collecting appropriate data;
 */
bool SffModule::getSignalsPerMediaLane(std::vector<MediaLaneSignals>& signals) {
  assert(signals.size() == numMediaLanes());

  auto txLos = getSettingsValue(SffField::LOS, UPPER_BITS_MASK) >> 4;
  auto rxLos = getSettingsValue(SffField::LOS, LOWER_BITS_MASK);
  auto txLol = getSettingsValue(SffField::LOL, UPPER_BITS_MASK) >> 4;
  auto rxLol = getSettingsValue(SffField::LOL, LOWER_BITS_MASK);
  auto txFault = getSettingsValue(SffField::FAULT, LOWER_BITS_MASK);
  auto txAdaptEqFault = getSettingsValue(SffField::FAULT, UPPER_BITS_MASK) >> 4;

  for (int lane = 0; lane < signals.size(); lane++) {
    auto laneMask = (1 << lane);
    signals[lane].lane_ref() = lane;
    signals[lane].txLos_ref() = txLos & laneMask;
    signals[lane].rxLos_ref() = rxLos & laneMask;
    signals[lane].txLol_ref() = txLol & laneMask;
    signals[lane].rxLol_ref() = rxLol & laneMask;
    signals[lane].txFault_ref() = txFault & laneMask;
    signals[lane].txAdaptEqFault_ref() = txAdaptEqFault & laneMask;
  }

  return true;
}

/*
 * Iterate through the four channels collecting appropriate data;
 */

bool SffModule::getSensorsPerChanInfo(std::vector<Channel>& channels) {
  int offset;
  int length;
  int dataAddress;
  auto channel_count = numMediaLanes();

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

  assert(channels.size() == channel_count);

  getQsfpFieldAddress(
      SffField::CHANNEL_RX_PWR_ALARMS, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  for (int channel = 0; channel < channel_count; channel++) {
    channels[channel].sensors_ref()->rxPwr_ref()->flags_ref() =
        getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
  }

  getQsfpFieldAddress(
      SffField::CHANNEL_TX_BIAS_ALARMS, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (int channel = 0; channel < channel_count; channel++) {
    channels[channel].sensors_ref()->txBias_ref()->flags_ref() =
        getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
  }

  getQsfpFieldAddress(
      SffField::CHANNEL_TX_PWR_ALARMS, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (int channel = 0; channel < channel_count; channel++) {
    channels[channel].sensors_ref()->txPwr_ref()->flags_ref() =
        getQsfpFlags(data + byteOffset[channel], bitOffset[channel]);
  }

  getQsfpFieldAddress(SffField::CHANNEL_RX_PWR, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    auto pwr = SffFieldInfo::getPwr(value);
    channel.sensors_ref()->rxPwr_ref()->value_ref() = pwr;
    Sensor rxDbm;
    rxDbm.value_ref() = mwToDb(pwr);
    channel.sensors_ref()->rxPwrdBm_ref() = rxDbm;
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  getQsfpFieldAddress(SffField::CHANNEL_TX_BIAS, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);
  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    channel.sensors_ref()->txBias_ref()->value_ref() =
        SffFieldInfo::getTxBias(value);
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  getQsfpFieldAddress(SffField::CHANNEL_TX_PWR, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    auto pwr = SffFieldInfo::getPwr(value);
    channel.sensors_ref()->txPwr_ref()->value_ref() = pwr;
    Sensor txDbm;
    txDbm.value_ref() = mwToDb(pwr);
    channel.sensors_ref()->txPwrdBm_ref() = txDbm;
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

SignalFlags SffModule::getSignalFlagInfo() {
  SignalFlags signalFlags = SignalFlags();

  signalFlags.txLos_ref() = getSettingsValue(SffField::LOS, UPPER_BITS_MASK);
  *signalFlags.txLos_ref() >>= 4;
  signalFlags.rxLos_ref() = getSettingsValue(SffField::LOS, LOWER_BITS_MASK);
  signalFlags.txLol_ref() = getSettingsValue(SffField::LOL, UPPER_BITS_MASK);
  *signalFlags.txLol_ref() >>= 4;
  signalFlags.rxLol_ref() = getSettingsValue(SffField::LOL, LOWER_BITS_MASK);

  return signalFlags;
}

ExtendedSpecComplianceCode SffModule::getExtendedSpecificationComplianceCode()
    const {
  return (ExtendedSpecComplianceCode)getSettingsValue(
      SffField::EXTENDED_SPECIFICATION_COMPLIANCE);
}

bool SffModule::getMediaInterfaceId(
    std::vector<MediaInterfaceId>& mediaInterface) {
  assert(mediaInterface.size() == numMediaLanes());

  // Currently setting the same media interface for all media lanes
  auto extSpecCompliance = getExtendedSpecificationComplianceCode();
  for (int lane = 0; lane < mediaInterface.size(); lane++) {
    mediaInterface[lane].lane_ref() = lane;
    MediaInterfaceUnion media;
    media.extendedSpecificationComplianceCode_ref() = extSpecCompliance;
    mediaInterface[lane].media_ref() = media;
  }

  return true;
}

ModuleStatus SffModule::getModuleStatus() {
  ModuleStatus moduleStatus;
  std::array<uint8_t, 2> status = {{0, 0}};
  int offset;
  int length;
  int dataAddress;

  getQsfpFieldAddress(SffField::STATUS, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, status.data());
  moduleStatus.dataNotReady_ref() = status[1] & (1 << 0);
  moduleStatus.interruptL_ref() = status[1] & (1 << 1);

  return moduleStatus;
}

void SffModule::setQsfpFlatMem() {
  std::array<uint8_t, 2> status = {{0, 0}};
  int offset;
  int length;
  int dataAddress;

  if (!present_) {
    throw FbossError("Failed setting QSFP Flat Mem: QSFP is not present");
  }

  // Check if the data is ready
  getQsfpFieldAddress(SffField::STATUS, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, status.data());
  if (status[1] & (1 << 0)) {
    StatsPublisher::bumpModuleErrors();
    throw QsfpModuleError("Failed setting QSFP Flat Mem: QSFP is not ready");
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
  if (dataAddress == SffPages::LOWER) {
    CHECK_LE(offset + length, sizeof(lowerPage_));
    /* Copy data from the cache */
    return (lowerPage_ + offset);
  } else {
    offset -= MAX_QSFP_PAGE_SIZE;
    CHECK_GE(offset, 0);
    CHECK_LE(offset, MAX_QSFP_PAGE_SIZE);
    if (dataAddress == SffPages::PAGE0) {
      CHECK_LE(offset + length, sizeof(page0_));
      /* Copy data from the cache */
      return (page0_ + offset);
    } else if (dataAddress == SffPages::PAGE3 && !flatMem_) {
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
    *data.lower_ref() =
        IOBuf::wrapBufferAsValue(lowerPage_, MAX_QSFP_PAGE_SIZE);
    *data.page0_ref() = IOBuf::wrapBufferAsValue(page0_, MAX_QSFP_PAGE_SIZE);
    if (!flatMem_) {
      data.page3_ref() = IOBuf::wrapBufferAsValue(page3_, MAX_QSFP_PAGE_SIZE);
    }
  }
  return data;
}

DOMDataUnion SffModule::getDOMDataUnion() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  Sff8636Data sffData;
  if (present_) {
    *sffData.lower_ref() =
        IOBuf::wrapBufferAsValue(lowerPage_, MAX_QSFP_PAGE_SIZE);
    *sffData.page0_ref() = IOBuf::wrapBufferAsValue(page0_, MAX_QSFP_PAGE_SIZE);
    if (!flatMem_) {
      sffData.page3_ref() =
          IOBuf::wrapBufferAsValue(page3_, MAX_QSFP_PAGE_SIZE);
    }
  }
  sffData.timeCollected_ref() = lastRefreshTime_;
  DOMDataUnion data;
  data.sff8636_ref() = sffData;
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
    setQsfpFlatMem();

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
             << apache::thrift::util::enumNameSafe(currentState) << " Ext ID "
             << std::hex << (int)*extId << " Ethernet compliance " << std::hex
             << (int)*ether << " Desired power control "
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

  getQsfpFieldAddress(SffField::POWER_CONTROL, dataAddress, offset, length);

  // enable target power class
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, offset, sizeof(power), &power);

  XLOG(INFO) << folly::sformat(
      "Port {:s}: QSFP set to power setting {:s} ({:d})",
      portStr,
      apache::thrift::util::enumNameSafe(desiredSetting),
      power);
}

/*
 * Put logic here that should only be run on ports that have been
 * down for a long time. These are actions that are potentially more
 * disruptive, but have worked in the past to recover a transceiver.
 */
void SffModule::remediateFlakyTransceiver() {
  XLOG(INFO) << "Performing potentially disruptive remediations on "
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

bool SffModule::getMediaLaneSettings(
    std::vector<MediaLaneSettings>& laneSettings) {
  assert(laneSettings.size() == numMediaLanes());
  auto txSquelch = getSettingsValue(SffField::SQUELCH_CONTROL, LOWER_BITS_MASK);
  auto txAdaptEq =
      getSettingsValue(SffField::TXRX_OUTPUT_CONTROL, LOWER_BITS_MASK);
  auto txDisable = getSettingsValue(SffField::TX_DISABLE);

  for (int lane = 0; lane < laneSettings.size(); lane++) {
    auto laneMask = (1 << lane);
    laneSettings[lane].lane_ref() = lane;
    laneSettings[lane].txSquelch_ref() = txSquelch & laneMask;
    laneSettings[lane].txAdaptiveEqControl_ref() = txAdaptEq & laneMask;
    laneSettings[lane].txDisable_ref() = txDisable & laneMask;
  }
  return true;
}

bool SffModule::getHostLaneSettings(
    std::vector<HostLaneSettings>& laneSettings) {
  assert(laneSettings.size() == numHostLanes());
  int offset;
  int length;
  int dataAddress;

  std::array<uint8_t, 2> txEq = {{0, 0}};
  getQsfpFieldAddress(SffField::TX_EQUALIZATION, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, txEq.data());

  std::array<uint8_t, 2> rxEmp = {{0, 0}};
  getQsfpFieldAddress(SffField::RX_EMPHASIS, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, rxEmp.data());

  std::array<uint8_t, 2> rxAmp = {{0, 0}};
  getQsfpFieldAddress(SffField::RX_AMPLITUDE, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, rxAmp.data());

  auto rxSquelch = getSettingsValue(SffField::SQUELCH_CONTROL) >> 4;
  auto rxOutput = getSettingsValue(SffField::TXRX_OUTPUT_CONTROL) >> 4;

  for (int lane = 0; lane < laneSettings.size(); lane++) {
    bool evenLane = (lane % 2 == 0);
    auto laneMask = (1 << lane);
    laneSettings[lane].lane_ref() = lane;
    laneSettings[lane].txInputEqualization_ref() =
        evenLane ? txEq[lane / 2] >> 4 : txEq[lane / 2] & 0xf;
    laneSettings[lane].rxOutputEmphasis_ref() =
        evenLane ? rxEmp[lane / 2] >> 4 : rxEmp[lane / 2] & 0xf;
    laneSettings[lane].rxOutputAmplitude_ref() =
        evenLane ? rxAmp[lane / 2] >> 4 : rxAmp[lane / 2] & 0xf;
    laneSettings[lane].rxSquelch_ref() = rxSquelch & laneMask;
    laneSettings[lane].rxOutput_ref() = rxOutput & laneMask;
  }
  return true;
}

void SffModule::overwriteChannelControlSettings() {
  // Most transceiver we use have their RX emphasis control set to 0.
  // However modules from AOI set those as 2dB which causes lower
  // signal quality when working with credo xphy on yamp. Thus as part
  // of the redmediation, we set that value to 0.
  auto cachedInfo = info_.rlock();

  if (!cachedInfo->has_value()) {
    return;
  }

  uint8_t page = 3;
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, 127, sizeof(page), &page);
  int offset;
  int length;
  int dataAddress;

  std::array<uint8_t, 2> buf = {{0}};
  getQsfpFieldAddress(SffField::TX_EQUALIZATION, dataAddress, offset, length);
  CHECK_EQ(length, 2);
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, offset, length, buf.data());

  getQsfpFieldAddress(SffField::RX_EMPHASIS, dataAddress, offset, length);
  CHECK_EQ(length, 2);
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, offset, length, buf.data());

  buf.fill(0x22);
  getQsfpFieldAddress(SffField::RX_AMPLITUDE, dataAddress, offset, length);
  CHECK_EQ(length, 2);
  qsfpImpl_->writeTransceiver(
      TransceiverI2CApi::ADDR_QSFP, offset, length, buf.data());

  // Bump up the ODS counter.
  StatsPublisher::bumpAOIOverride();
}

void SffModule::customizeTransceiverLocked(cfg::PortSpeed speed) {
  /*
   * This must be called with a lock held on qsfpModuleMutex_
   */
  if (customizationSupported()) {
    TransceiverSettings settings = getTransceiverSettingsInfo();

    // Before turning up power, check whether we want to override
    // channel control settings or no.
    // TODO(clin82) Remove aoi_override flag once this solution has been
    // deployed.
    if (transceiverManager_->getPlatformMode() == PlatformMode::YAMP) {
      overwriteChannelControlSettings();
    }

    // We want this on regardless of speed
    setPowerOverrideIfSupported(*settings.powerControl_ref());

    if (speed != cfg::PortSpeed::DEFAULT) {
      setCdrIfSupported(speed, *settings.cdrTx_ref(), *settings.cdrRx_ref());
      setRateSelectIfSupported(
          speed, *settings.rateSelect_ref(), *settings.rateSelectSetting_ref());
    }
  } else {
    XLOG(DBG1) << "Customization not supported on " << qsfpImpl_->getName();
  }

  lastCustomizeTime_ = std::time(nullptr);
  needsCustomization_ = false;
}

/*
 * verifyEepromChecksums
 *
 * This function verifies the module's eeprom register checksum in various
 * pages. For SFF8636 module the checksums are kept in 2 pages:
 *   Page 0: Register 191 contains checksum for values in register 128 to 190
 *   Page 0: Register 223 contains checksum for values in register 192 to 222
 *   Page 1: Register 128 contains checksum for values in register 129 to 255
 * These checksums are 8 bit sum of all the 8 bit values
 */
bool SffModule::verifyEepromChecksums() {
  bool rc = true;
  // Verify checksum for all pages
  for (auto& csumInfoIt : checksumInfoSff) {
    // For flat memory module, check for page 0 only
    if (flatMem_ && csumInfoIt.first != SffPages::PAGE0) {
      continue;
    }
    rc |= verifyEepromChecksum(csumInfoIt.first);
  }
  XLOG(INFO) << folly::sformat(
      "Module {} EEPROM Checksum {:s}",
      qsfpImpl_->getName(),
      rc ? "Passed" : "Failed");
  return rc;
}

/*
 * verifyEepromChecksums
 *
 * This function verifies the module's eeprom register checksum for a given
 * page. The checksum is 8 bit sum of all the 8 bit values in range of
 * registers
 */
bool SffModule::verifyEepromChecksum(int pageId) {
  int offset;
  int length;
  int dataAddress;
  const uint8_t* data;
  uint8_t checkSum, expectedChecksum;

  // Return false if the registers are not cached yet (this is not expected)
  if (!cacheIsValid()) {
    XLOG(INFO) << folly::sformat(
        "Module {} can't do eeprom checksum as the register cache is not populated",
        qsfpImpl_->getName());
    return false;
  }
  // Return false if we don't know range of registers to validate the checksum
  // on this page
  if (checksumInfoSff.find(pageId) == checksumInfoSff.end()) {
    XLOG(INFO) << folly::sformat(
        "Module {} can't do eeprom checksum for page {:d}",
        qsfpImpl_->getName(),
        pageId);
    return false;
  }

  // Get the range of registers, compute checksum and compare
  for (auto& csumRange : checksumInfoSff[pageId]) {
    dataAddress = pageId;
    offset = csumRange.checksumRangeStartOffset;
    length = csumRange.checksumRangeStartLength;
    data = getQsfpValuePtr(dataAddress, offset, length);

    checkSum = 0;
    for (int i = 0; i < length; i++) {
      checkSum += data[i];
    }

    getQsfpFieldAddress(
        csumRange.checksumValOffset, dataAddress, offset, length);
    data = getQsfpValuePtr(dataAddress, offset, length);
    expectedChecksum = data[0];

    if (checkSum != expectedChecksum) {
      XLOG(ERR) << folly::sformat(
          "Module {}: Page {:d}: expected checksum {:#x}, actual {:#x}",
          qsfpImpl_->getName(),
          pageId,
          expectedChecksum,
          checkSum);
      return false;
    } else {
      XLOG(INFO) << folly::sformat(
          "Module {}: Page {:d}: checksum verified successfully {:#x}",
          qsfpImpl_->getName(),
          pageId,
          checkSum);
    }
  }
  return true;
}

} // namespace fboss
} // namespace facebook
