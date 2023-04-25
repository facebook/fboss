// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/sff/Sff8472Module.h"

#include <string>
#include "fboss/agent/FbossError.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/module/QsfpFieldInfo.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"
#include "fboss/qsfp_service/module/sff/Sff8472FieldInfo.h"

#include <folly/logging/xlog.h>

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace {}

namespace facebook {
namespace fboss {

static std::map<Ethernet10GComplianceCode, MediaInterfaceCode>
    mediaInterfaceMapping = {
        {Ethernet10GComplianceCode::LR_10G, MediaInterfaceCode::LR_10G},
        {Ethernet10GComplianceCode::SR_10G, MediaInterfaceCode::SR_10G},
};

// As per SFF-8472
static QsfpFieldInfo<Sff8472Field, Sff8472Pages>::QsfpFieldMap sfpFields = {
    // Fields for reading an entire page
    {Sff8472Field::PAGE_LOWER_A0,
     {TransceiverI2CApi::ADDR_QSFP, Sff8472Pages::LOWER, 0, 128}},
    {Sff8472Field::PAGE_LOWER_A2,
     {TransceiverI2CApi::ADDR_QSFP_A2, Sff8472Pages::LOWER, 0, 128}},
    // Address A0h fields
    {Sff8472Field::IDENTIFIER,
     {TransceiverI2CApi::ADDR_QSFP, Sff8472Pages::LOWER, 0, 1}},
    {Sff8472Field::ETHERNET_10G_COMPLIANCE_CODE,
     {TransceiverI2CApi::ADDR_QSFP, Sff8472Pages::LOWER, 3, 1}},
    {Sff8472Field::VENDOR_NAME,
     {TransceiverI2CApi::ADDR_QSFP, Sff8472Pages::LOWER, 20, 16}},
    {Sff8472Field::VENDOR_OUI,
     {TransceiverI2CApi::ADDR_QSFP, Sff8472Pages::LOWER, 37, 3}},
    {Sff8472Field::VENDOR_PART_NUMBER,
     {TransceiverI2CApi::ADDR_QSFP, Sff8472Pages::LOWER, 40, 16}},
    {Sff8472Field::VENDOR_REVISION_NUMBER,
     {TransceiverI2CApi::ADDR_QSFP, Sff8472Pages::LOWER, 56, 4}},
    {Sff8472Field::VENDOR_SERIAL_NUMBER,
     {TransceiverI2CApi::ADDR_QSFP, Sff8472Pages::LOWER, 68, 16}},
    {Sff8472Field::VENDOR_MFG_DATE,
     {TransceiverI2CApi::ADDR_QSFP, Sff8472Pages::LOWER, 84, 8}},

    // Address A2h fields
    {Sff8472Field::ALARM_WARNING_THRESHOLDS,
     {TransceiverI2CApi::ADDR_QSFP_A2, Sff8472Pages::LOWER, 0, 40}},
    {Sff8472Field::TEMPERATURE,
     {TransceiverI2CApi::ADDR_QSFP_A2, Sff8472Pages::LOWER, 96, 2}},
    {Sff8472Field::VCC,
     {TransceiverI2CApi::ADDR_QSFP_A2, Sff8472Pages::LOWER, 98, 2}},
    {Sff8472Field::CHANNEL_TX_BIAS,
     {TransceiverI2CApi::ADDR_QSFP_A2, Sff8472Pages::LOWER, 100, 2}},
    {Sff8472Field::CHANNEL_TX_PWR,
     {TransceiverI2CApi::ADDR_QSFP_A2, Sff8472Pages::LOWER, 102, 2}},
    {Sff8472Field::CHANNEL_RX_PWR,
     {TransceiverI2CApi::ADDR_QSFP_A2, Sff8472Pages::LOWER, 104, 2}},
    {Sff8472Field::STATUS_AND_CONTROL_BITS,
     {TransceiverI2CApi::ADDR_QSFP_A2, Sff8472Pages::LOWER, 110, 1}},
    {Sff8472Field::ALARM_WARNING_FLAGS,
     {TransceiverI2CApi::ADDR_QSFP_A2, Sff8472Pages::LOWER, 112, 6}},
};

void getSfpFieldAddress(
    Sff8472Field field,
    int& dataAddress,
    int& offset,
    int& length,
    uint8_t& transceiverI2CAddress) {
  auto info = QsfpFieldInfo<Sff8472Field, Sff8472Pages>::getQsfpFieldAddress(
      sfpFields, field);
  dataAddress = info.dataAddress;
  offset = info.offset;
  length = info.length;
  transceiverI2CAddress = info.transceiverI2CAddress;
}

const uint8_t* Sff8472Module::getSfpValuePtr(
    int dataAddress,
    int offset,
    int length,
    uint8_t transceiverI2CAddress) const {
  /* if the cached values are not correct */
  if (!cacheIsValid()) {
    throw FbossError("Sfp is either not present or the data is not read");
  }
  if (dataAddress == static_cast<int>(Sff8472Pages::LOWER)) {
    if (transceiverI2CAddress == TransceiverI2CApi::ADDR_QSFP) {
      CHECK_LE(offset + length, sizeof(a0LowerPage_));
      return (a0LowerPage_ + offset);
    } else if (transceiverI2CAddress == TransceiverI2CApi::ADDR_QSFP_A2) {
      CHECK_LE(offset + length, sizeof(a2LowerPage_));
      return (a2LowerPage_ + offset);
    }
  }
  throw FbossError(
      "Invalid Data Address 0x%d, transceiverI2CAddress 0x%d",
      dataAddress,
      transceiverI2CAddress);
}

void Sff8472Module::readSff8472Field(Sff8472Field field, uint8_t* data) {
  int dataLength, dataPage, dataOffset;
  uint8_t transceiverI2CAddress;
  getSfpFieldAddress(
      field, dataPage, dataOffset, dataLength, transceiverI2CAddress);
  qsfpImpl_->readTransceiver(
      {transceiverI2CAddress, dataOffset, dataLength}, data);
}

void Sff8472Module::writeSff8472Field(Sff8472Field field, uint8_t* data) {
  int dataLength, dataPage, dataOffset;
  uint8_t transceiverI2CAddress;
  getSfpFieldAddress(
      field, dataPage, dataOffset, dataLength, transceiverI2CAddress);
  qsfpImpl_->writeTransceiver(
      {transceiverI2CAddress, dataOffset, dataLength}, data);
}

void Sff8472Module::getSfpValue(
    int dataAddress,
    int offset,
    int length,
    uint8_t transceiverI2CAddress,
    uint8_t* data) const {
  const uint8_t* ptr =
      getSfpValuePtr(dataAddress, offset, length, transceiverI2CAddress);

  memcpy(data, ptr, length);
}

uint8_t Sff8472Module::getSettingsValue(Sff8472Field field, uint8_t mask)
    const {
  int offset;
  int length;
  int dataAddress;
  uint8_t transceiverI2CAddress;

  getSfpFieldAddress(field, dataAddress, offset, length, transceiverI2CAddress);
  const uint8_t* data =
      getSfpValuePtr(dataAddress, offset, length, transceiverI2CAddress);

  return data[0] & mask;
}

Sff8472Module::Sff8472Module(
    TransceiverManager* transceiverManager,
    std::unique_ptr<TransceiverImpl> qsfpImpl)
    : QsfpModule(transceiverManager, std::move(qsfpImpl)) {}

Sff8472Module::~Sff8472Module() {}

void Sff8472Module::updateQsfpData(bool allPages) {
  // expects the lock to be held
  if (!present_) {
    return;
  }
  try {
    QSFP_LOG(DBG2, this) << "Performing " << ((allPages) ? "full" : "partial")
                         << " sfp data cache refresh for transceiver";
    readSff8472Field(Sff8472Field::PAGE_LOWER_A2, a2LowerPage_);
    lastRefreshTime_ = std::time(nullptr);
    dirty_ = false;

    if (!allPages) {
      // Only address A2h lower page0 has diagnostic data that changes very
      // often. Avoid reading the static data frequently.
      return;
    }

    readSff8472Field(Sff8472Field::PAGE_LOWER_A0, a0LowerPage_);
  } catch (const std::exception& ex) {
    // No matter what kind of exception throws, we need to set the dirty_ flag
    // to true.
    dirty_ = true;
    QSFP_LOG(ERR, this) << "Error update data: " << ex.what();
    throw;
  }
}

TransceiverSettings Sff8472Module::getTransceiverSettingsInfo() {
  TransceiverSettings settings = TransceiverSettings();
  settings.powerControl() = getPowerControlValue();
  settings.mediaInterface() = std::vector<MediaInterfaceId>(numMediaLanes());
  if (!getMediaInterfaceId(*(settings.mediaInterface()))) {
    settings.mediaInterface().reset();
  }

  settings.mediaLaneSettings() =
      std::vector<MediaLaneSettings>(numMediaLanes());
  if (!getMediaLaneSettings(*(settings.mediaLaneSettings()))) {
    settings.mediaLaneSettings().reset();
  }
  return settings;
}

bool Sff8472Module::getMediaInterfaceId(
    std::vector<MediaInterfaceId>& mediaInterface) {
  CHECK_EQ(mediaInterface.size(), numMediaLanes());

  auto ethernet10GCompliance = getEthernet10GComplianceCode();
  for (int lane = 0; lane < mediaInterface.size(); lane++) {
    mediaInterface[lane].lane() = lane;
    MediaInterfaceUnion media;
    media.ethernet10GComplianceCode_ref() = ethernet10GCompliance;
    if (auto it = mediaInterfaceMapping.find(ethernet10GCompliance);
        it != mediaInterfaceMapping.end()) {
      mediaInterface[lane].code() = it->second;
    } else {
      QSFP_LOG(ERR, this) << "Unable to find MediaInterfaceCode for "
                          << apache::thrift::util::enumNameSafe(
                                 ethernet10GCompliance);
      mediaInterface[lane].code() = MediaInterfaceCode::UNKNOWN;
    }
    mediaInterface[lane].media() = media;
  }

  return true;
}

MediaInterfaceCode Sff8472Module::getModuleMediaInterface() const {
  auto ethernet10GCompliance = getEthernet10GComplianceCode();

  if (auto it = mediaInterfaceMapping.find(ethernet10GCompliance);
      it != mediaInterfaceMapping.end()) {
    return it->second;
  }
  QSFP_LOG(ERR, this) << "Cannot find "
                      << apache::thrift::util::enumNameSafe(
                             ethernet10GCompliance)
                      << " in mediaInterfaceMapping";
  return MediaInterfaceCode::UNKNOWN;
}

Ethernet10GComplianceCode Sff8472Module::getEthernet10GComplianceCode() const {
  return Ethernet10GComplianceCode(
      getSettingsValue(Sff8472Field::ETHERNET_10G_COMPLIANCE_CODE));
}

double Sff8472Module::getSfpSensor(
    Sff8472Field field,
    double (*conversion)(uint16_t value)) {
  auto info = QsfpFieldInfo<Sff8472Field, Sff8472Pages>::getQsfpFieldAddress(
      sfpFields, field);
  const uint8_t* data = getSfpValuePtr(
      info.dataAddress, info.offset, info.length, info.transceiverI2CAddress);
  CHECK_EQ(info.length, 2);
  return conversion(data[0] << 8 | data[1]);
}

GlobalSensors Sff8472Module::getSensorInfo() {
  GlobalSensors info = GlobalSensors();
  info.temp()->value() =
      getSfpSensor(Sff8472Field::TEMPERATURE, Sff8472FieldInfo::getTemp);
  info.vcc()->value() =
      getSfpSensor(Sff8472Field::VCC, Sff8472FieldInfo::getVcc);
  return info;
}

bool Sff8472Module::getSensorsPerChanInfo(std::vector<Channel>& channels) {
  int offset;
  int length;
  int dataAddress;
  uint8_t transceiverI2CAddress;

  // TX Bias
  getSfpFieldAddress(
      Sff8472Field::CHANNEL_TX_BIAS,
      dataAddress,
      offset,
      length,
      transceiverI2CAddress);
  const uint8_t* data =
      getSfpValuePtr(dataAddress, offset, length, transceiverI2CAddress);
  uint16_t value = data[0] << 8 | data[1];
  CHECK_EQ(channels.size(), 1);
  CHECK_EQ(numMediaLanes(), 1);
  auto firstChannel = channels.begin();
  firstChannel->sensors()->txBias()->value() =
      Sff8472FieldInfo::getTxBias(value);

  // TX Power
  getSfpFieldAddress(
      Sff8472Field::CHANNEL_TX_PWR,
      dataAddress,
      offset,
      length,
      transceiverI2CAddress);
  data = getSfpValuePtr(dataAddress, offset, length, transceiverI2CAddress);
  value = data[0] << 8 | data[1];
  auto pwr = Sff8472FieldInfo::getPwr(value);
  firstChannel->sensors()->txPwr()->value() = pwr;
  Sensor txDbm;
  txDbm.value() = mwToDb(pwr);
  firstChannel->sensors()->txPwrdBm() = txDbm;

  // RX Power
  getSfpFieldAddress(
      Sff8472Field::CHANNEL_RX_PWR,
      dataAddress,
      offset,
      length,
      transceiverI2CAddress);
  data = getSfpValuePtr(dataAddress, offset, length, transceiverI2CAddress);
  value = data[0] << 8 | data[1];
  pwr = Sff8472FieldInfo::getPwr(value);
  firstChannel->sensors()->rxPwr()->value() = pwr;
  Sensor rxDbm;
  rxDbm.value() = mwToDb(pwr);
  firstChannel->sensors()->rxPwrdBm() = rxDbm;

  return true;
}

SignalFlags Sff8472Module::getSignalFlagInfo() {
  SignalFlags signalFlags = SignalFlags();

  signalFlags.rxLos() = getSettingsValue(
      Sff8472Field::STATUS_AND_CONTROL_BITS, FieldMasks::RX_LOS_MASK);

  return signalFlags;
}

/*
 * Iterate through the media channels collecting appropriate data;
 */
bool Sff8472Module::getSignalsPerMediaLane(
    std::vector<MediaLaneSignals>& signals) {
  auto rxLos = getSettingsValue(
      Sff8472Field::STATUS_AND_CONTROL_BITS, FieldMasks::RX_LOS_MASK);
  auto txFault = getSettingsValue(
      Sff8472Field::STATUS_AND_CONTROL_BITS, FieldMasks::TX_FAULT_MASK);

  CHECK_EQ(signals.size(), 1);
  CHECK_EQ(numMediaLanes(), 1);
  auto firstSignal = signals.begin();
  firstSignal->lane() = 0;
  firstSignal->rxLos() = rxLos;
  firstSignal->txFault() = txFault;

  return true;
}

bool Sff8472Module::getMediaLaneSettings(
    std::vector<MediaLaneSettings>& laneSettings) {
  auto txDisable = getSettingsValue(
      Sff8472Field::STATUS_AND_CONTROL_BITS, FieldMasks::TX_DISABLE_STATE_MASK);

  CHECK_EQ(laneSettings.size(), 1);
  auto firstLane = laneSettings.begin();
  firstLane->lane() = 0;
  firstLane->txDisable() = txDisable;
  return true;
}

std::string Sff8472Module::getSfpString(Sff8472Field field) const {
  int offset;
  int length;
  int dataAddress;
  uint8_t transceiverI2CAddress;

  getSfpFieldAddress(field, dataAddress, offset, length, transceiverI2CAddress);
  const uint8_t* data =
      getSfpValuePtr(dataAddress, offset, length, transceiverI2CAddress);

  while (length > 0 && data[length - 1] == ' ') {
    --length;
  }

  std::string value(reinterpret_cast<const char*>(data), length);
  return validateQsfpString(value) ? value : "UNKNOWN";
}

Vendor Sff8472Module::getVendorInfo() {
  Vendor vendor = Vendor();
  vendor.name() = getSfpString(Sff8472Field::VENDOR_NAME);
  vendor.oui() = getSfpString(Sff8472Field::VENDOR_OUI);
  vendor.partNumber() = getSfpString(Sff8472Field::VENDOR_PART_NUMBER);
  vendor.rev() = getSfpString(Sff8472Field::VENDOR_REVISION_NUMBER);
  vendor.serialNumber() = getSfpString(Sff8472Field::VENDOR_SERIAL_NUMBER);
  vendor.dateCode() = getSfpString(Sff8472Field::VENDOR_MFG_DATE);
  return vendor;
}

bool Sff8472Module::remediateFlakyTransceiver(
    bool /* allPortsDown */,
    const std::vector<std::string>& /* ports */) {
  QSFP_LOG(INFO, this) << "Performing potentially disruptive remediations";

  // This api accept 1 based module id however the module id in WedgeManager
  // is 0 based.
  // Note that triggering a hard reset on a SFP module used with a QSFP-to-SFP
  // adapter actually toggles the txDisable line
  getTransceiverManager()->getQsfpPlatformApi()->triggerQsfpHardReset(
      static_cast<unsigned int>(getID()) + 1);

  // Even though remediation might not trigger, we still need to update the
  // lastRemediateTime_ so that we can use cool down before next remediation
  lastRemediateTime_ = std::time(nullptr);
  return true;
}

} // namespace fboss
} // namespace facebook
