// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/module/QsfpModule.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <optional>

namespace facebook {
namespace fboss {

enum class CmisField;

enum class CmisPages : int {
  LOWER = -1,
  PAGE00 = 0,
  PAGE01 = 1,
  PAGE02 = 2,
  PAGE10 = 0x10,
  PAGE11 = 0x11,
  PAGE13 = 0x13,
  PAGE14 = 0x14,
  PAGE20 = 0x20,
  PAGE21 = 0x21,
  PAGE24 = 0x24,
  PAGE25 = 0x25,
  PAGE2F = 0x2F
};

class CmisModule : public QsfpModule {
 public:
  explicit CmisModule(
      TransceiverManager* transceiverManager,
      std::unique_ptr<TransceiverImpl> qsfpImpl);
  virtual ~CmisModule() override;

  struct ApplicationAdvertisingField {
    uint8_t ApSelCode;
    uint8_t moduleMediaInterface;
    int hostLaneCount;
    int mediaLaneCount;
  };

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
  };

  using LengthAndGauge = std::pair<double, uint8_t>;

  /*
   * Returns the number of lanes on the host side
   */
  unsigned int numHostLanes() const override;
  /*
   * Returns the number of lanes on the media side
   */
  unsigned int numMediaLanes() const override;

  void configureModule() override;

  /*
   * This function veifies the Module eeprom register checksum for various
   * pages.
   */
  bool verifyEepromChecksums() override;

  /*
   * Returns the current state of prbs (enabled/polynomial)
   */
  prbs::InterfacePrbsState getPortPrbsStateLocked(Side side) override;

 protected:
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
  uint8_t page20_[MAX_QSFP_PAGE_SIZE];
  uint8_t page21_[MAX_QSFP_PAGE_SIZE];
  uint8_t page24_[MAX_QSFP_PAGE_SIZE];
  uint8_t page25_[MAX_QSFP_PAGE_SIZE];

  /*
   * This function returns a pointer to the value in the static cached
   * data after checking the length fits. The thread needs to have the lock
   * before calling this function.
   */
  const uint8_t* getQsfpValuePtr(int dataAddress, int offset, int length)
      const override;
  /*
   * Perform transceiver customization
   * This must be called with a lock held on qsfpModuleMutex_
   *
   * Default speed is set to DEFAULT - this will prevent any speed specific
   * settings from being applied
   */
  void customizeTransceiverLocked(
      cfg::PortSpeed speed = cfg::PortSpeed::DEFAULT) override;

  /*
   * If the current power state is not same as desired one then change it and
   * return true when module is in ready state
   */
  virtual bool ensureTransceiverReadyLocked() override;

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
  virtual void setPowerOverrideIfSupportedLocked(
      PowerControlState currentState) override;
  /*
   * Set appropriate application code for PortSpeed, if supported
   */
  void setApplicationCodeLocked(cfg::PortSpeed speed);
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
   * Gather per-media-lane signal information for thrift queries
   */
  bool getSignalsPerMediaLane(std::vector<MediaLaneSignals>& signals) override;
  /*
   * Gather per-host-lane signal information for thrift queries
   */
  bool getSignalsPerHostLane(std::vector<HostLaneSignals>& signals) override;
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
  virtual uint8_t getSettingsValue(CmisField field, uint8_t mask = 0xff) const;
  /*
   * Gather info on what features are enabled and supported
   */
  virtual TransceiverSettings getTransceiverSettingsInfo() override;
  /*
   * Gather supported applications for this module, and store them in
   * moduleCapabilities_
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
   * Returns the identifier in byte 0
   */
  TransceiverModuleIdentifier getIdentifier() override;
  /*
   * Returns the module status in byte 3
   */
  ModuleStatus getModuleStatus() override;
  /*
   * Fetches the media interface ids per media lane and returns false if it
   * fails
   */
  bool getMediaInterfaceId(
      std::vector<MediaInterfaceId>& mediaInterface) override;
  /*
   * Gets the Media Type encoding (byte 85 in CMIS)
   */
  MediaTypeEncodings getMediaTypeEncoding() const;
  /*
   * Gets the Single Mode Fiber Interface codes from SFF-8024
   */
  SMFMediaInterfaceCode getSmfMediaInterface() const;
  /*
   * Returns the firmware version
   * <Module firmware version, DSP version, Build revision>
   */
  std::array<std::string, 3> getFwRevisions();
  FirmwareStatus getFwStatus();

  /*
   * Gather host side per lane configuration settings and return false when it
   * fails
   */
  bool getHostLaneSettings(std::vector<HostLaneSettings>& laneSettings);
  /*
   * Gather media side per lane configuration settings and return false when it
   * fails
   */
  bool getMediaLaneSettings(std::vector<MediaLaneSettings>& laneSettings);
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
   * Only return true if there's an actual remediation happened
   */
  bool remediateFlakyTransceiver() override;

  virtual void setDiagsCapability() override;

  virtual std::optional<VdmDiagsStats> getVdmDiagsStatsInfo() override;

  /*
   * Trigger next VDM stats capture
   */
  void triggerVdmStatsCapture() override;

  /*
   * Latch and read VDM data
   */
  void latchAndReadVdmDataLocked() override;

  bool supportRemediate() override {
    return true;
  }

  void resetDataPath() override;

 private:
  // no copy or assignment
  CmisModule(CmisModule const&) = delete;
  CmisModule& operator=(CmisModule const&) = delete;

  /* Helper function to read/write a CmisField. The function will extract the
   * page number, offset and length information from the CmisField and then make
   * the corresponding qsfpImpl->readTransceiver and qsfpImpl->writeTransceiver
   * calls. The user should avoid making direct calls to
   * qsfpImpl->read/writeTransceiver and instead do register IO using
   * readCmisField/writeCmisField helper functions. The helper function will
   * also change the page when it's supported by the transceiver and when not
   * specifically asked to skip page change (for batch operations). */
  void
  readCmisField(CmisField field, uint8_t* data, bool skipPageChange = false);
  void
  writeCmisField(CmisField field, uint8_t* data, bool skipPageChange = false);

  void getFieldValueLocked(CmisField fieldName, uint8_t* fieldValue) const;
  /*
   * Helpers to parse DOM data for DAC cables. These incorporate some
   * extra fields that FB has vendors put in the 'Vendor specific'
   * byte range of the SFF spec.
   */
  double getQsfpDACLength() const override;

  /*
   * Set the optics Rx euqlizer pre/post/main values
   */
  void setModuleRxEqualizerLocked(RxEqualizerSettings rxEqualizer);

  /*
   * We found that some module did not enable Rx output squelch by default,
   * which introduced some difficulty to bring link back up when flapped.
   * This function is to ensure that Rx output squelch is always enabled.
   */
  void ensureRxOutputSquelchEnabled(
      const std::vector<HostLaneSettings>& hostLaneSettings) override;

  /*
   * Check if the module has accepted the lane configuration specified by
   * ApSel or other settings like RxEqualizer setting. In case of config
   * rejection the function returns false
   */
  bool checkLaneConfigError();

  /*
   * This function veifies the Module eeprom register checksum for a given page
   */
  bool verifyEepromChecksum(CmisPages pageId);

  /*
   * Reads the CMIS standard L-Module State Changed latched register
   * (Will be 1 on first read after a state change, and will read 0 on
   * subsequent reads)
   */
  virtual bool getModuleStateChanged();

  /*
   * ApplicationCode to ApplicationCodeSel mapping.
   */
  std::map<uint8_t, ApplicationAdvertisingField> moduleCapabilities_;

  /*
   * Gets the module media interface. This is the intended media interface
   * application for this module. The module may be able to run in a different
   * application (with lesser bandwidth). For example if a 200G-FR4 module is
   * configured for 100G-CWDM4 application, then getModuleMediaInterface will
   * return 200G-FR4
   */
  MediaInterfaceCode getModuleMediaInterface() override;

  void resetDataPathWithFunc(
      std::optional<std::function<void()>> afterDataPathDeinitFunc =
          std::nullopt);

  /*
   * Set the PRBS Generator and Checker on a module for the desired side (Line
   * or System side)
   */
  bool setPortPrbsLocked(phy::Side side, const prbs::InterfacePrbsState& prbs)
      override;

  /*
   * Get the PRBS stats for a module
   */
  phy::PrbsStats getPortPrbsStatsSideLocked(
      phy::Side side,
      bool checkerEnabled,
      const phy::PrbsStats& lastStats) override;

  void updateVdmCacheLocked();

  void updateCmisStateChanged(
      ModuleStatus& moduleStatus,
      std::optional<ModuleStatus> curModuleStatus = std::nullopt) override;
};

} // namespace fboss
} // namespace facebook
