// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/module/QsfpModule.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook {
namespace fboss {

enum class CmisField;

class CmisModule : public QsfpModule {
 public:
  explicit CmisModule(
      TransceiverManager* transceiverManager,
      std::unique_ptr<TransceiverImpl> qsfpImpl,
      unsigned int portsPerTransceiver);
  virtual ~CmisModule() override;

  /*
   * Return a valid type.
   */
  TransceiverType type() const override {
    return TransceiverType::QSFP;
  }

  /*
   * Return the spec this transceiver follows.
   */
   TransceiverManagementInterface managementInterface() const override {
     return TransceiverManagementInterface::CMIS;
   }

  /*
   * Get the QSFP EEPROM Field
   */
  void getFieldValue(CmisField fieldName, uint8_t* fieldValue) const;

  RawDOMData getRawDOMData() override;

  DOMDataUnion getDOMDataUnion() override;

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
  CmisModule(CmisModule const&) = delete;
  CmisModule& operator=(CmisModule const&) = delete;

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
  uint8_t page01_[MAX_QSFP_PAGE_SIZE];
  uint8_t page02_[MAX_QSFP_PAGE_SIZE];
  uint8_t page10_[MAX_QSFP_PAGE_SIZE];
  uint8_t page11_[MAX_QSFP_PAGE_SIZE];
  uint8_t page13_[MAX_QSFP_PAGE_SIZE];
  uint8_t page14_[MAX_QSFP_PAGE_SIZE];

  /*
   * This function returns a pointer to the value in the static cached
   * data after checking the length fits. The thread needs to have the lock
   * before calling this function.
   */
  const uint8_t* getQsfpValuePtr(int dataAddress, int offset, int length) const override;
  /*
   * Perform transceiver customization
   * This must be called with a lock held on qsfpModuleMutex_
   *
   * Default speed is set to DEFAULT - this will prevent any speed specific
   * settings from being applied
   */
  virtual void customizeTransceiverLocked(
      cfg::PortSpeed speed = cfg::PortSpeed::DEFAULT) override;
  /*
   * Based on identifier, sets whether the upper memory of the module is flat or
   * paged.
   */
  void setQsfpFlatMem() override;
  /*
   * Set power mode
   * Wedge forces Low Power mode via a pin;  we have to reset this
   * to force High Power mode on LR4s.
   */
  virtual void setPowerOverrideIfSupported(PowerControlState currentState) override;
  /*
   * Set appropriate application code for PortSpeed, if supported
   */
  void setApplicationCode(cfg::PortSpeed speed);
  /*
   * returns individual sensor values after scaling
   */
  double getQsfpSensor(CmisField field, double (*conversion)(uint16_t value));
  /*
   * returns cable length (negative for "longer than we can represent")
   */
  double getQsfpSMFLength() const;

  double getQsfpOMLength(CmisField field) const;

  /*
   * returns the freeside transceiver technology type
   */
  virtual TransmitterTechnology getQsfpTransmitterTechnology() const override;
  /*
   * Extract sensor flag levels
   */
  FlagLevels getQsfpSensorFlags(CmisField fieldName, int offset);
  /*
   * This function returns various strings from the QSFP EEPROM
   * caller needs to check if DOM is supported or not
   */
  std::string getQsfpString(CmisField flag) const;

  /*
   * Fills in values for alarm and warning thresholds based on field name
   */
  ThresholdLevels getThresholdValues(
      CmisField field,
      double (*conversion)(uint16_t value));
  /*
   * Retreives all alarm and warning thresholds
   */
  std::optional<AlarmThreshold> getThresholdInfo() override;
  /*
   * Gather the sensor info for thrift queries
   */
  GlobalSensors getSensorInfo() override;
  /*
   * Gather per-channel information for thrift queries
   */
  bool getSensorsPerChanInfo(std::vector<Channel>& channels) override;
  /*
   * Gather per-channel flag values from bitfields.
   */
  FlagLevels getChannelFlags(CmisField field, int channel);
  /*
   * Gather the vendor info for thrift queries
   */
  Vendor getVendorInfo() override;
  /*
   * Gather the cable info for thrift queries
   */
  Cable getCableInfo() override;
  /*
   * Retrieves the values of settings based on field name and bit placement
   * Default mask is a noop
   */
  virtual uint8_t getSettingsValue(CmisField field, uint8_t mask = 0xff);
  /*
   * Gather info on what features are enabled and supported
   */
  virtual TransceiverSettings getTransceiverSettingsInfo() override;
  /*
   * Gather supported applications for this module
   */
  void getApplicationCapabilities();
  /*
   * Return which rate select capability is being used, if any
   */
  RateSelectState getRateSelectValue();
  /*
   * Return the rate select optimised bit rates for each channel
   */
  RateSelectSetting getRateSelectSettingValue(RateSelectState state);
  /*
   * Return what power control capability is currently enabled
   */
  PowerControlState getPowerControlValue() override;
  /*
   * Return TransceiverStats
   */
  bool getTransceiverStats(TransceiverStats& stats);
  /*
   * Return SignalFlag which contains Tx/Rx LOS/LOL
   */
  virtual SignalFlags getSignalFlagInfo() override;
  /*
   * Return the extended specification compliance code of the module.
   * This is the field of Byte 192 on page00 and following table 4-4
   * of SFF-8024.
   */
  ExtendedSpecComplianceCode getExtendedSpecificationComplianceCode() override;
  /*
   * Update the cached data with the information from the physical QSFP.
   *
   * The 'allPages' parameter determines which pages we refresh. Data
   * on the first page holds most of the fields that actually change,
   * so unless we have reason to believe the transceiver was unplugged
   * there is not much point in refreshing static data on other pages.
   */
  virtual void updateQsfpData(bool allPages = true) override;

  /*
   * Put logic here that should only be run on ports that have been
   * down for a long time. These are actions that are potentially more
   * disruptive, but have worked in the past to recover a transceiver.
   */
  void remediateFlakyTransceiver() override;

 private:
  void getFieldValueLocked(CmisField fieldName, uint8_t* fieldValue) const;
  /*
   * Helpers to parse DOM data for DAC cables. These incorporate some
   * extra fields that FB has vendors put in the 'Vendor specific'
   * byte range of the SFF spec.
   */
  double getQsfpDACLength() const override;

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

  /*
   * ApplicationCode to ApplicationCodeSel mapping.
   */
  std::map<uint8_t, uint8_t> moduleCapabilities_;
};

} // namespace fboss
} // namespace facebook
