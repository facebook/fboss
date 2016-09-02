/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <cstdint>
#include <mutex>
#include <boost/container/flat_map.hpp>
#include "fboss/agent/Transceiver.h"
#include "fboss/agent/if/gen-cpp2/optic_types.h"

namespace facebook { namespace fboss {
/* This table is populated based on the SFP specification
 * Document: Specification for
 *              Diagnostic Monitoring Interface for Optical Transceivers
 * Document Number: SFF-8472 (Rev 11.3) June 11, 2013.
 */

/* SFP DOM Alarms and Warning Flags */

enum class SfpDomFlag {
  TEMP_ALARM_HIGH,
  TEMP_ALARM_LOW,
  TEMP_WARN_HIGH,
  TEMP_WARN_LOW,
  VCC_ALARM_HIGH,
  VCC_ALARM_LOW,
  VCC_WARN_HIGH,
  VCC_WARN_LOW,
  TX_BIAS_ALARM_HIGH,
  TX_BIAS_ALARM_LOW,
  TX_BIAS_WARN_HIGH,
  TX_BIAS_WARN_LOW,
  TX_PWR_ALARM_HIGH,
  TX_PWR_ALARM_LOW,
  TX_PWR_WARN_HIGH,
  TX_PWR_WARN_LOW,
  RX_PWR_ALARM_HIGH,
  RX_PWR_ALARM_LOW,
  RX_PWR_WARN_HIGH,
  RX_PWR_WARN_LOW,
  LAST,
};

/* SFP DOM values */
enum class SfpDomValue {
  TEMP,
  VCC,
  TX_BIAS,
  TX_PWR,
  RX_PWR,
  LAST,
};

const int MAX_SFP_EEPROM_SIZE = 256;

enum class SffField;
class TransceiverImpl;

/*
 * This is the SFP class which will be storing the SFP EEPROM
 * data from the address 0xA0 which is static data. The class
 * Also contains the presence status of the SFP module for the
 * port.
 *
 * Note: The public functions need to take the lock before calling
 * the private functions.
 */
class SfpModule : public Transceiver {
 public:
  explicit SfpModule(std::unique_ptr<TransceiverImpl> sfpImpl);
  /*
   * Returns if the SFP is present or not
   */
  bool isPresent() const override;
  /*
   * Return a valid type.
   */
  TransceiverType type() const override{
    return TransceiverType::SFP;
  }
  /*
   * This function will check if the SFP is present or not
   */
  void detectTransceiver() override;
  /*
   * This function returns if the SFP supports DOM
   */
  bool isDomSupported() const;
  /*
   * Get the SFP EEPROM Field
   */
  int getFieldValue(SffField fieldName, uint8_t* fieldValue);
  /*
   * This function returns the entire SFP Dom information
   */
  void getSfpDom(SfpDom &dom) override;
  /*
   * This function will update the SFP Dom Fields in the cache
   */
  void updateTransceiverInfoFields() override;
  /*
   * This function returns the entire SFP Dom information
   */
  void getTransceiverInfo(TransceiverInfo &info) override;


 private:
  // no copy or assignment
  SfpModule(SfpModule const &) = delete;
  SfpModule& operator=(SfpModule const &) = delete;

  const int CABLE_MAX_LEN = 255;

  // Cached 0xA0 SFP eeprom value
  uint8_t sfpIdprom_[256];
  // Cached 0xA2 SFP DOM value
  uint8_t sfpDom_[256];
  // SFP Presence status
  bool present_;
  // Denotes if the cache value is valid or stale
  bool dirty_;
  /* Sfp Internal Implementation */
  std::unique_ptr<TransceiverImpl> sfpImpl_;
  // Does the Optic support DOM
  bool domSupport_;

  /*
   * This function returns a pointer to the value in the static cached
   * data after checking the length fits. The thread needs to have the lock
   * before calling this function.
   */
  const uint8_t* getSfpValuePtr(std::lock_guard<std::mutex>& lg,
                                int dataAddress, int offset, int length) const;
  /*
   * This function returns the values on the offset and length
   * from the static cached data. The thread needs to have the lock
   * before calling this function.
   */
  void getSfpValue(std::lock_guard<std::mutex>& lg, int dataAddress,
                    int offset, int length, uint8_t* data) const;
  /*
   * Sets the IDProm cache data for the port
   * The data should be 256 bytes.
   * The thread needs to have the lock before calling the function.
   */
  void setSfpIdprom(std::lock_guard<std::mutex>& lg, const uint8_t* data);
  /*
   * This is used by the detection thread to set the SFP presence
   * status based on the HW read.
   * The thread needs to have the lock before calling the function.
   */
  void setPresent(std::lock_guard<std::mutex>& lg, bool present);
  /*
   * Sets the Dom Supported bit as per the IDPROM
   * The thread needs to have the lock before calling the function.
   */
  void setDomSupport(std::lock_guard<std::mutex>& lg);
  /*
   * returns the value of the flag from the raw IDProm data
   * The thread needs to have the lock before calling the function.
   */
  bool getSfpFlagIdProm(std::lock_guard<std::mutex>& lg, const SfpDomFlag flag,
                        const uint8_t* data);
  /* returns the value of for the threshold from the raw data */
  float getValueFromRaw(std::lock_guard<std::mutex>& lg, const SfpDomFlag key,
                        uint16_t value);
  /*
   * This function sets the Dynamic SFP fields
   * Check for DOM support before calling this
   * function.
   * The thread needs to have the lock before calling the function.
   */
  void setSfpDom(std::lock_guard<std::mutex>& lg, const uint8_t* data);
  /*
   * sfpModuleMutex_ is held around all the read and writes to the sfpModule
   *
   * This is used so that we get consistent from the SfpModule and to make
   * sure no other thread tries to write at the time some other is reading
   * the information.
   */
  mutable std::mutex sfpModuleMutex_;
  /*
   * This function returns various strings from the SFP EEPROM
   * caller needs to check if DOM is supported or not
   */
  std::string getSfpString(std::lock_guard<std::mutex>& lg,
                            const SffField flag);
  /*
   * This function returns the status of the SFP alarm/warning flag
   * caller needs to check if DOM is supported or not
   */
  bool getSfpThreshFlag(std::lock_guard<std::mutex>& lg,
                        const SfpDomFlag flag);
  /*
   * This function returns the current value of the DOM field
   * caller needs to check if DOM is supported or not
   */
  float getSfpDomValue(std::lock_guard<std::mutex>& lg,
      const SfpDomValue field);
  /*
   * This function returns the threshold value of the Sfp DOM Flag
   * caller needs to check if DOM is supported or not
   */
  float getSfpThreshValue(std::lock_guard<std::mutex>& lg,
                          const SfpDomFlag flag);
  /*
   * This function returns all the status flags
   * Returns false when no data exists
   */
  bool getDomFlagsMap(std::lock_guard<std::mutex>& lg,
                      SfpDomThreshFlags &domFlags);
  /*
   * This function returns all the values of the DOM
   * Returns false when no data exists
   */
  bool getDomValuesMap(std::lock_guard<std::mutex>& lg,
                      SfpDomReadValue &value);
  /*
   * This function returns all the threshold values of the Sfp DOM
   * returns false when no data exists
   */
  bool getDomThresholdValuesMap(std::lock_guard<std::mutex>& lg,
                                SfpDomThreshValue &domThresh);
  /*
   * This function returns all the vendor values of the SFP
   * returns false when no data exists
   */
  bool getVendorInfo(std::lock_guard<std::mutex>& lg, Vendor &vendor);
  /*
   * Get temperature and Vcc info
   */
  bool getSensorInfo(std::lock_guard<std::mutex>& lg, GlobalSensors& info);
  /*
   * Get cable length info, in meters
   */
  bool getCableInfo(std::lock_guard<std::mutex>& lg, Cable &cable);
  int getSfpCableLength(std::lock_guard<std::mutex>& lg, const SffField field,
                        int multiplier);
  /*
   * Get per-channel data, including sensor value and alarm flags
   */
  bool getSensorsPerChanInfo(std::lock_guard<std::mutex>& lg,
                              std::vector<Channel>& channels);
  /*
   * Get alarm and warning thresholds
   */
  bool getThresholdInfo(std::lock_guard<std::mutex>& lg,
                        AlarmThreshold &threshold);
  /*
   * This function returns true if both the sfp is present and the
   * cache data is not stale. This should be checked before any
   * function that reads cache data is called
   */
  bool cacheIsValid(std::lock_guard<std::mutex>& lg) const;
};

}} //namespace facebook::fboss
