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
#include "fboss/qsfp_service/sff/Transceiver.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <folly/Optional.h>
#include <folly/Synchronized.h>

namespace facebook { namespace fboss {

// As per SFF-8436, QSFP+ 10 Gbs 4X PLUGGABLE TRANSCEIVER spec

enum QsfpPages {
  LOWER,
  PAGE0,
  PAGE3,
};

enum class SffField;
class TransceiverImpl;

/*
 * This function takes the QsfpIDPromField name and returns
 * the dataAddress, offset and the length as per the QSFP DOM
 * Document mentioned above.
 */
void getQsfpFieldAddress(SffField field, int &dataAddress,
                        int &offset, int &length);

/**
 * This is the QSFP module error which should be throw only if it's module
 * related issue.
 */
class QsfpModuleError : public std::exception {
public:
  explicit QsfpModuleError(const std::string& what) : what_(what) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

private:
  std::string what_;
};

/*
 * This is the QSFP class which will be storing the QSFP EEPROM
 * data from the address 0xA0 which is static data. The class
 * Also contains the presence status of the QSFP module for the
 * port.
 *
 * Note: The public functions need to take the lock before calling
 * the private functions.
 */
class QsfpModule : public Transceiver {
 public:
  explicit QsfpModule(
      std::unique_ptr<TransceiverImpl> qsfpImpl,
      unsigned int portsPerTransceiver);

  /*
   * Determines if the QSFP is present or not.
   */
  bool detectPresence() override;

  /*
   * Return a valid type.
   */
  TransceiverType type() const override {
    return TransceiverType::QSFP;
  }

  TransceiverID getID() const override;

  bool detectPresenceLocked();
  TransceiverInfo parseDataLocked();

  virtual void refresh() override;
  void refreshLocked();

  /*
   * Get the QSFP EEPROM Field
   */
  void getFieldValue(SffField fieldName, uint8_t* fieldValue);

  /*
   * Customize QSPF fields as necessary
   *
   * speed - the speed the port is running at - this will allow setting
   * different qsfp settings based on speed
   */
  void customizeTransceiver(cfg::PortSpeed speed) override;

  /*
   * Returns the entire QSFP information
   */
  TransceiverInfo getTransceiverInfo() override;

  RawDOMData getRawDOMData() override;

  void transceiverPortsChanged(
    const std::vector<std::pair<const int, PortStatus>>& ports) override;

  /*
   * The size of the pages used by QSFP.  See below for an explanation of
   * how they are laid out.  This needs to be publicly accessible for
   * testing.
   */
  enum : unsigned int {
    // Size of page read from QSFP via I2C
    MAX_QSFP_PAGE_SIZE = 128,
    // Number of channels per module
    CHANNEL_COUNT = 4,
  };

  using LengthAndGauge = std::pair<double, uint8_t>;

 protected:
  // no copy or assignment
  QsfpModule(QsfpModule const &) = delete;
  QsfpModule& operator=(QsfpModule const &) = delete;

  enum : unsigned int {
    EEPROM_DEFAULT = 255,
    MAX_GAUGE = 30,
    DECIMAL_BASE = 10,
    HEX_BASE = 16,
  };
  // QSFP+ requires a bottom 128 byte page describing important monitoring
  // information, and then an upper 128 byte page with less frequently
  // referenced information, including vendor identifiers.  There are
  // three other optional pages;  the third provides a bunch of
  // alarm and warning thresholds which we are interested in.
  uint8_t lowerPage_[MAX_QSFP_PAGE_SIZE];
  uint8_t page0_[MAX_QSFP_PAGE_SIZE];
  uint8_t page3_[MAX_QSFP_PAGE_SIZE];

  /* Qsfp Internal Implementation */
  std::unique_ptr<TransceiverImpl> qsfpImpl_;
  // QSFP Presence status
  bool present_{false};
  // Denotes if the cache value is valid or stale
  bool dirty_{true};
  // Flat memory systems don't support paged access to extra data
  bool flatMem_{false};
  // This transceiver needs customization
  bool needsCustomization_{false};

  folly::Synchronized<folly::Optional<TransceiverInfo>> info_;
  /*
   * qsfpModuleMutex_ is held around all the read and writes to the qsfpModule
   *
   * This is used so that we get consistent from the QsfpModule and to make
   * sure no other thread tries to write at the time some other is reading
   * the information.
   */
  mutable std::mutex qsfpModuleMutex_;

  /*
   * Used to track last time key actions were taken so we don't retry
   * too frequently. These MUST be accessed holding qsfpModuleMutex_.
   */
  time_t lastRefreshTime_{0};
  time_t lastCustomizeTime_{0};
  time_t lastTxEnable_{0};
  time_t lastPowerClassReset_{0};

  /*
   * Perform transceiver customization
   * This must be called with a lock held on qsfpModuleMutex_
   *
   * Default speed is set to DEFAULT - this will prevent any speed specific
   * settings from being applied
   */
  void customizeTransceiverLocked(
      cfg::PortSpeed speed=cfg::PortSpeed::DEFAULT);

  /*
   * This function returns a pointer to the value in the static cached
   * data after checking the length fits. The thread needs to have the lock
   * before calling this function.
   */
  const uint8_t* getQsfpValuePtr(int dataAddress, int offset,
                                 int length) const;
  /*
   * This function returns the values on the offset and length
   * from the static cached data. The thread needs to have the lock
   * before calling this function.
   */
  void getQsfpValue(int dataAddress,
                    int offset, int length, uint8_t* data) const;
  /*
   * Sets the IDProm cache data for the port
   * The data should be 256 bytes.
   * The thread needs to have the lock before calling the function.
   */
  void setQsfpIdprom();
  /*
   * Set power mode
   * Wedge forces Low Power mode via a pin;  we have to reset this
   * to force High Power mode on LR4s.
   */
  virtual void setPowerOverrideIfSupported(PowerControlState currentState);
  /*
   * Enable or disable CDR
   * Which action to take is determined by the port speed
   */
  virtual void setCdrIfSupported(cfg::PortSpeed speed,
      FeatureState currentStateTx, FeatureState currentStateRx);
  /*
   * Set appropriate rate select value for PortSpeed, if supported
   */
  virtual void setRateSelectIfSupported(cfg::PortSpeed speed,
      RateSelectState currentState, RateSelectSetting currentSetting);
  /*
   * returns individual sensor values after scaling
   */
  double getQsfpSensor(SffField field,
    double (*conversion)(uint16_t value));
  /*
   * returns cable length (negative for "longer than we can represent")
   */
  double getQsfpCableLength(SffField field) const;

  /*
   * returns the freeside transceiver technology type
   */
  virtual TransmitterTechnology getQsfpTransmitterTechnology() const;
  /*
   * Parse out flag values from bitfields
   */
  static FlagLevels getQsfpFlags(const uint8_t* data, int offset);
  /*
   * Extract sensor flag levels
   */
  FlagLevels getQsfpSensorFlags(SffField fieldName);
  /*
   * This function returns various strings from the QSFP EEPROM
   * caller needs to check if DOM is supported or not
   */
  std::string getQsfpString(SffField flag) const;

  bool validateQsfpString(const std::string& value) const;

  /*
   * Fills in values for alarm and warning thresholds based on field name
   */
  ThresholdLevels getThresholdValues(SffField field,
                            double (*conversion)(uint16_t value));
  /*
   * Retreives all alarm and warning thresholds
   */
  bool getThresholdInfo(AlarmThreshold& threshold);
  /*
   * Gather the sensor info for thrift queries
   */
  bool getSensorInfo(GlobalSensors &sensor);
  /*
   * Gather per-channel information for thrift queries
   */
  bool getSensorsPerChanInfo(std::vector<Channel>& channels);
  /*
   * Gather the vendor info for thrift queries
   */
  bool getVendorInfo(Vendor &vendor);
  /*
   * Gather the cable info for thrift queries
   */
  void getCableInfo(Cable &cable);
  /*
   * Retrieves the values of settings based on field name and bit placement
   * Default mask is a noop
   */
  virtual uint8_t getSettingsValue(SffField field, uint8_t mask=0xff);
  /*
   * Gather info on what features are enabled and supported
   */
  virtual bool getTransceiverSettingsInfo(TransceiverSettings &settings);
  /*
   * Return which rate select capability is being used, if any
   */
  RateSelectState getRateSelectValue();
  /*
   * Return the rate select optimised bit rates for each channel
   */
  RateSelectSetting getRateSelectSettingValue(
      RateSelectState state);
  /*
   * Return what power control capability is currently enabled
   */
  PowerControlState getPowerControlValue();
  /*
   * Return TransceiverStats
   */
  bool getTransceiverStats(TransceiverStats& stats);
  /*
   * This function returns true if both the sfp is present and the
   * cache data is not stale. This should be checked before any
   * function that reads cache data is called
   */
  virtual bool cacheIsValid() const;
  /*
   * Update the cached data with the information from the physical QSFP.
   *
   * The 'allPages' parameter determines which pages we refresh. Data
   * on the first page holds most of the fields that actually change,
   * so unless we have reason to believe the transceiver was unplugged
   * there is not much point in refreshing static data on other pages.
   */
  virtual void updateQsfpData(bool allPages = true);

 private:
  /*
   * Helpers to parse DOM data for DAC cables. These incorporate some
   * extra fields that FB has vendors put in the 'Vendor specific'
   * byte range of the SFF spec.
   */
  double getQsfpDACLength() const;
  int getQsfpDACGauge() const;
  /*
   * Provides the option to override the length/gauge values read from
   * the DOM for certain transceivers. This is useful when vendors
   * input incorrect data and the accuracy of these fields is
   * important for proper tuning.
   */
  const folly::Optional<LengthAndGauge> getDACCableOverride() const;

  // make sure that tx_disable bits are clear
  void ensureTxEnabled(time_t cooldown);

  /*
   * Determine if it is safe to customize the ports based on the
   * status of our member ports.
   */
  bool safeToCustomize() const;

  /*
   * Similar to safeToCustomize, but also factors in whether we think
   * we need customization (needsCustomization_) and also makes sure
   * we haven't customized too recently via the cooldown param.
   */
  bool customizationWanted(time_t cooldown) const;

  /*
   * Returns whether customization is supported at all. Basically
   * checks if something is plugged in and checks if copper.
   */
  bool customizationSupported() const;

  /*
   * Whether enough time has passed that we should refresh our data.
   * Cooldown parameter indicates how much time must have elapsed
   * since last time we refreshed the DOM data.
   */
  bool shouldRefresh(time_t cooldown) const;

  /*
   * In the case of Minipack using Facebook FPGA, we need to clear the reset
   * register of QSFP whenever it is newly inserted.
   */
  void ensureOutOfReset() const;

  /*
   * Determine set speed of enabled member ports.
   */
  cfg::PortSpeed getPortSpeed() const;

  std::map<uint32_t, PortStatus> ports_;
  unsigned int portsPerTransceiver_{0};
};

}} //namespace facebook::fboss
