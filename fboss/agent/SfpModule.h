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
#include "fboss/agent/SfpImpl.h"
#include "fboss/agent/if/gen-cpp2/optic_types.h"

namespace facebook { namespace fboss {
/* This table is populated based on the SFP specification
 * Document: Specification for
 *              Diagnostic Monitoring Interface for Optical Transceivers
 * Document Number: SFF-8472 (Rev 11.3) June 11, 2013.
 */
enum class SfpIdpromFields {
  /* 0xA0 Address Fields */
  IDENTIFIER, // Type of Transceiver
  EXT_IDENTIFIER, // Extended type of transceiver
  CONNECTOR_TYPE, // Code for Connector Type
  TRANSCEIVER_CODE, // Code for Electronic or optical capability
  ENCODING_CODE, // High speed Serial encoding algo code
  SIGNALLING_RATE, // nominal signalling rate
  RATE_IDENTIFIER, // type of rate select functionality
  SINGLE_MODE_LENGTH_KM, // link length supported in single mode (unit: km)
  SINGLE_MODE_LENGTH, // link length supported in single mode (unit: 100m)
  SFP_50_UM_MODE_LENGTH, // link length supported in 50um (unit: 10m)
  SFP_62_5_UM_MODE_LENGTH, // link length supported in 62.5 um (unit: 10m)
  CU_OM4_SUPPORTED_LENGTH, // link length supported in cu or om4 mode (unit: m)
  OM3_SUPPORTED_LENGTH, // link length supported in om3 mode (unit: 10m)
  VENDOR_NAME, // SFP Vendor Name (ASCII)
  TRANCEIVER_CAPABILITY, // Code for Electronic or optical capability
  VENDOR_OUI, // SFP Vendor IEEE company ID
  PART_NUMBER, // Part NUmber provided by SFP vendor (ASCII)
  REVISION_NUMBER, // Revision number
  WAVELENGTH, // laser wavelength
  CHECK_CODE_BASEID, // Check code for the above fields
  // Extended options
  ENABLED_OPTIONS, // Indicates the optional transceiver signals enabled
  UPPER_BIT_RATE_MARGIN, // upper bit rate margin
  LOWER_BIT_RATE_MARGIN, // lower but rate margin
  VENDOR_SERIAL_NUMBER, // Vendor Serial Number (ASCII)
  MFG_DATE, // Manufacturing date code
  DIAGNOSTIC_MONITORING_TYPE, // Diagnostic monitoring implemented
  ENHANCED_OPTIONS, // Enhanced options implemented
  SFF_COMPLIANCE, // revision number of SFF compliance
  CHECK_CODE_EXTENDED_OPT, // check code for the extended options
  // Vendor Specific Fields
  VENDOR_EEPROM, // Vendor specific EEPROM
  RESERVED_FIELD, // Reserved

  /* 0xA2 address Fields */
  /* Diagnostics */
  ALARM_THRESHOLD_VALUES, // diagnostic flag alarm and warning thresh values
  EXTERNAL_CALIBRATION, // diagnostic calibration constants
  CHECK_CODE_DMI, // Check code for base Diagnostic Fields
  DIAGNOSTICS, // Diagnostic Monitor Data
  STATUS_CONTROL, // Optional Status and Control bits
  RESERVED, // Reserved for SFF-8079
  ALARM_WARN_FLAGS, // Diagnostic alarm and warning flag
  EXTENDED_STATUS_CONTROL, // Extended status and control bytes
  /* General Purpose */
  VENDOR_MEM_ADDRESS, // Vendor Specific memory address
  USER_EEPROM, // User Writable NVM
  VENDOR_CONTROL, // Vendor Specific Control
};

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
/*
 * This function takes the SfpIDPromField name and returns
 * the dataAddress, offset and the length as per the SFP DOM
 * Document mentioned above.
 */
void getSfpFieldAddress(SfpIdpromFields field, int &dataAddress,
                        int &offset, int &length);

/*
 * This function takes the SfpDomFlag name and returns
 * the bit to be checked for the flag as per the SFP DOM
 * Document mentioned above.
 */
int getSfpDomBit(const SfpDomFlag flag);

const int MAX_SFP_EEPROM_SIZE = 256;

/*
 * This is the SFP class which will be storing the SFP EEPROM
 * data from the address 0xA0 which is static data. The class
 * Also contains the presence status of the SFP module for the
 * port.
 *
 * Note: The public functions need to take the lock before calling
 * the private functions.
 */
class SfpModule {
 public:
  explicit SfpModule(std::unique_ptr<SfpImpl>& sfpImpl);
  /*
   * Returns if the SFP is present or not
   */
  bool isSfpPresent() const;
  /*
   * This function will check if the SFP is present or not
   */
  void detectSfp();
  /*
   * This function returns if the SFP supports DOM
   */
  bool isDomSupported() const;
  /*
   * Get the SFP EEPROM Field
   */
  int getSfpFieldValue(SfpIdpromFields fieldName, uint8_t* fieldValue);
  /*
   * This function will update the SFP Dom Fields in the cache
   */
  void updateSfpDomFields();
  /*
   * This function returns the entire SFP Dom information
   */
  void getSfpDom(SfpDom &dom);

 private:
  // no copy or assignment
  SfpModule(SfpModule const &) = delete;
  SfpModule& operator=(SfpModule const &) = delete;

  // Cached 0xA0 SFP eeprom value
  uint8_t sfpIdprom_[256];
  // Cached 0xA2 SFP DOM value
  uint8_t sfpDom_[256];
  // SFP Presence status
  bool present_;
  // Denotes if the cache value is valid or stale
  bool dirty_;
  /* Sfp Internal Implementation */
  std::unique_ptr<SfpImpl> sfpImpl_;
  // Does the Optic support DOM
  bool domSupport_;

  /*
   * This function returns a pointer to the value in the static cached
   * data after checking the length fits. The thread needs to have the lock
   * before calling this function.
   */
  const uint8_t* getSfpValuePtr(int dataAddress, int offset,
                                int length) const;
  /*
   * This function returns the values on the offset and length
   * from the static cached data. The thread needs to have the lock
   * before calling this function.
   */
  void getSfpValue(int dataAddress,
                    int offset, int length, uint8_t* data) const;
  /*
   * Sets the IDProm cache data for the port
   * The data should be 256 bytes.
   * The thread needs to have the lock before calling the function.
   */
  void setSfpIdprom(const uint8_t* data);
  /*
   * This is used by the detection thread to set the SFP presence
   * status based on the HW read.
   * The thread needs to have the lock before calling the function.
   */
  void setSfpPresent(bool present);
  /*
   * Sets the Dom Supported bit as per the IDPROM
   * The thread needs to have the lock before calling the function.
   */
  void setDomSupport();
  /* Get the Temperature value in degree Celcius from the raw value */
  float getTemp(const uint16_t temp);
  /* Get the Vcc value in Volts from the raw value */
  float getVcc(const uint16_t temp);
  /* Get the Power value in mW from the raw value */
  float getPwr(const uint16_t temp);
  /* Get the Power value in mA from the raw value */
  float getTxBias(const uint16_t temp);
  /*
   * returns the value of the flag from the raw IDProm data
   * The thread needs to have the lock before calling the function.
   */
  bool getSfpFlagIdProm(const SfpDomFlag flag, const uint8_t* data);
  /* returns the value of for the threshold from the raw data */
  float getValueFromRaw(const SfpDomFlag key, uint16_t value);
  /*
   * This function sets the Dynamic SFP fields
   * Check for DOM support before calling this
   * function.
   * The thread needs to have the lock before calling the function.
   */
  void setSfpDom(const uint8_t* data);
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
  std::string getSfpString(const SfpIdpromFields flag);
  /*
   * This function returns the status of the SFP alarm/warning flag
   * caller needs to check if DOM is supported or not
   */
  bool getSfpThreshFlag(const SfpDomFlag flag);
  /*
   * This function returns the current value of the DOM field
   * caller needs to check if DOM is supported or not
   */
  float getSfpDomValue(const SfpDomValue field);
  /*
   * This function returns the threshold value of the Sfp DOM Flag
   * caller needs to check if DOM is supported or not
   */
  float getSfpThreshValue(const SfpDomFlag flag);
  /*
   * This function returns all the status flags
   * Returns false when no data exists
   */
  bool getDomFlagsMap(SfpDomThreshFlags &domFlags);
  /*
   * This function returns all the values of the DOM
   * Returns false when no data exists
   */
  bool getDomValuesMap(SfpDomReadValue &value);
  /*
   * This function returns all the threshold values of the Sfp DOM
   * returns false when no data exists
   */
  bool getDomThresholdValuesMap(SfpDomThreshValue &domThresh);
  /*
   * This function returns all the vendor values of the Sfp DOM
   * returns false when no data exists
   */
  bool getVendorMap(Vendor &vendor);
  /*
   * This function returns true if both the sfp is present and the
   * cache data is not stale. This should be checked before any
   * function that reads cache data is called
   */
  bool cacheIsValid() const;
};

}} //namespace facebook::fboss
