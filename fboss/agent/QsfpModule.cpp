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
  {SffField::POWER_CONTROL, {QsfpPages::LOWER, 93, 1} },
  {SffField::PAGE_SELECT_BYTE, {QsfpPages::LOWER, 127, 1} },

  // Page 0 values, including vendor info:
  {SffField::EXTENDED_IDENTIFIER, {QsfpPages::PAGE0, 129, 1} },
  {SffField::ETHERNET_COMPLIANCE, {QsfpPages::PAGE0, 131, 1} },
  {SffField::LENGTH_SM_KM, {QsfpPages::PAGE0, 142, 1} },
  {SffField::LENGTH_OM3, {QsfpPages::PAGE0, 143, 1} },
  {SffField::LENGTH_OM2, {QsfpPages::PAGE0, 144, 1} },
  {SffField::LENGTH_OM1, {QsfpPages::PAGE0, 145, 1} },
  {SffField::LENGTH_COPPER, {QsfpPages::PAGE0, 146, 1} },
  {SffField::VENDOR_NAME, {QsfpPages::PAGE0, 148, 16} },
  {SffField::VENDOR_OUI, {QsfpPages::PAGE0, 165, 3} },
  {SffField::PART_NUMBER, {QsfpPages::PAGE0, 168, 16} },
  {SffField::REVISION_NUMBER, {QsfpPages::PAGE0, 184, 2} },
  {SffField::VENDOR_SERIAL_NUMBER, {QsfpPages::PAGE0, 196, 16} },
  {SffField::MFG_DATE, {QsfpPages::PAGE0, 212, 8} },
  {SffField::DIAGNOSTIC_MONITORING_TYPE, {QsfpPages::PAGE0, 220, 1} },

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
      customizeTransceiver();
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

void QsfpModule::customizeTransceiver() {
  /*
   * Determine whether we need to customize any of the QSFP registers.
   * Wedge forces Low Power mode via a pin;  we have to reset this
   * to force High Power mode on LR4s.
   *
   * Note that this function expects to be called with qsfpModuleMutex_
   * held.
   */

  if (dirty_ == true) {
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
  int powerLevel = (*extId & EXT_ID_MASK) >> EXT_ID_SHIFT;

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
