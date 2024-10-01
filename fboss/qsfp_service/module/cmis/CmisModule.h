// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <sys/types.h>
#include "fboss/qsfp_service/module/QsfpModule.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/firmware_storage/FbossFirmware.h"
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
  PAGE22 = 0x22,
  PAGE24 = 0x24,
  PAGE25 = 0x25,
  PAGE26 = 0x26,
  PAGE2F = 0x2F
};

enum VdmConfigType {
  UNSUPPORTED = 0,
  SNR_MEDIA_IN = 5,
  SNR_HOST_IN = 6,
  PAM4_LTP_MEDIA_IN = 7,
  PRE_FEC_BER_MEDIA_IN_MIN = 9,
  PRE_FEC_BER_HOST_IN_MIN = 10,
  PRE_FEC_BER_MEDIA_IN_MAX = 11,
  PRE_FEC_BER_HOST_IN_MAX = 12,
  PRE_FEC_BER_MEDIA_IN_AVG = 13,
  PRE_FEC_BER_HOST_IN_AVG = 14,
  PRE_FEC_BER_MEDIA_IN_CUR = 15,
  PRE_FEC_BER_HOST_IN_CUR = 16,
  ERR_FRAME_MEDIA_IN_MIN = 17,
  ERR_FRAME_HOST_IN_MIN = 18,
  ERR_FRAME_MEDIA_IN_MAX = 19,
  ERR_FRAME_HOST_IN_MAX = 20,
  ERR_FRAME_MEDIA_IN_AVG = 21,
  ERR_FRAME_HOST_IN_AVG = 22,
  ERR_FRAME_MEDIA_IN_CUR = 23,
  ERR_FRAME_HOST_IN_CUR = 24,
  PAM4_LEVEL0_STANDARD_DEVIATION_LINE = 100,
  PAM4_LEVEL1_STANDARD_DEVIATION_LINE = 101,
  PAM4_LEVEL2_STANDARD_DEVIATION_LINE = 102,
  PAM4_LEVEL3_STANDARD_DEVIATION_LINE = 103,
  PAM4_MPI_LINE = 104,
  FEC_TAIL_MEDIA_IN_MAX = 106,
  FEC_TAIL_MEDIA_IN_CURR = 107,
  FEC_TAIL_HOST_IN_MAX = 108,
  FEC_TAIL_HOST_IN_CURR = 109,
};

class CmisModule : public QsfpModule {
 public:
  explicit CmisModule(
      std::set<std::string> portNames,
      TransceiverImpl* qsfpImpl,
      std::shared_ptr<const TransceiverConfig> cfg,
      bool supportRemediate);
  virtual ~CmisModule() override;

  struct ApplicationAdvertisingField {
    uint8_t ApSelCode;
    uint8_t moduleMediaInterface;
    int hostLaneCount;
    int mediaLaneCount;
    std::vector<int> hostStartLanes;
    std::vector<int> mediaStartLanes;
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

  static constexpr int kMaxOsfpNumLanes = 8;

  using LengthAndGauge = std::pair<double, uint8_t>;

  using VdmDiagsLocationStatus = struct VdmDiagsLocationStatus_t {
    bool vdmConfImplementedByModule = false;
    CmisPages vdmValPage;
    int vdmValOffset;
    int vdmValLength;
  };

  void configureModule(uint8_t startHostLane) override;

  /*
   * This function veifies the Module eeprom register checksum for various
   * pages.
   */
  bool verifyEepromChecksums() override;

  /*
   * Returns the current state of prbs (enabled/polynomial)
   */
  prbs::InterfacePrbsState getPortPrbsStateLocked(
      std::optional<const std::string> portName,
      phy::Side side) override;

  VdmDiagsLocationStatus getVdmDiagsValLocation(VdmConfigType vdmConf) const;

  bool tcvrPortStateSupported(TransceiverPortState& portState) const override;

  bool isRequestValidMultiportSpeedConfig(
      cfg::PortSpeed speed,
      uint8_t startHostLane,
      uint8_t numLanes);

  std::optional<std::array<SMFMediaInterfaceCode, kMaxOsfpNumLanes>>
  getValidMultiportSpeedConfig(
      cfg::PortSpeed speed,
      uint8_t startHostLane,
      uint8_t numLanes);

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
  uint8_t page22_[MAX_QSFP_PAGE_SIZE];
  uint8_t page24_[MAX_QSFP_PAGE_SIZE];
  uint8_t page25_[MAX_QSFP_PAGE_SIZE];
  uint8_t page26_[MAX_QSFP_PAGE_SIZE];

  // Some of the pages are static and they need not be read every refresh cycle
  bool staticPagesCached_{false};

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
   */
  void customizeTransceiverLocked(TransceiverPortState& portState) override;

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
  void setApplicationCodeLocked(
      cfg::PortSpeed speed,
      uint8_t startHostLane,
      uint8_t numHostLanesForPort);
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
  PowerControlState getPowerControlValue(bool readFromCache) override;
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
  SMFMediaInterfaceCode getSmfMediaInterface(uint8_t lane = 0) const;

  /*
   * Returns the list of media interfaces supported by the module
   */
  std::vector<MediaInterfaceCode> getSupportedMediaInterfacesLocked()
      const override;

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
   */
  void remediateFlakyTransceiver(
      bool allPortsDown,
      const std::vector<std::string>& ports) override;

  virtual void setDiagsCapability() override;

  virtual std::optional<VdmDiagsStats> getVdmDiagsStatsInfo() override;

  virtual std::optional<VdmPerfMonitorStats> getVdmPerfMonitorStats() override;

  virtual VdmPerfMonitorStatsForOds getVdmPerfMonitorStatsForOds(
      VdmPerfMonitorStats& vdmPerfMonStats) override;

  virtual std::map<std::string, CdbDatapathSymErrHistogram>
  getCdbSymbolErrorHistogramLocked() override;

  /*
   * Trigger next VDM stats capture
   */
  void triggerVdmStatsCapture() override;

  /*
   * Latch and read VDM data
   */
  void latchAndReadVdmDataLocked() override;

  bool supportRemediate() override;

  void resetDataPath() override;

  /*
   * Returns the ApplicationAdvertisingField corresponding to the application or
   * nullopt if it doesn't exist
   */
  std::optional<ApplicationAdvertisingField> getApplicationField(
      uint8_t application,
      uint8_t startHostLane) const;

  SMFMediaInterfaceCode getApplicationFromApSelCode(uint8_t apSelCode) const;

  // Returns the list of host lanes configured in the same datapath as the
  // provided startHostLane
  std::vector<uint8_t> configuredHostLanes(
      uint8_t startHostLane) const override;

  // Returns the list of media lanes configured in the same datapath as the
  // provided startHostLane
  std::vector<uint8_t> configuredMediaLanes(
      uint8_t startHostLane) const override;

  /*
   * Set the Transceiver Tx channel enable/disable
   */
  virtual bool setTransceiverTxLocked(
      const std::string& portName,
      phy::Side side,
      std::optional<uint8_t> userChannelMask,
      bool enable) override;

  virtual bool setTransceiverTxImplLocked(
      const std::set<uint8_t>& tcvrLanes,
      phy::Side side,
      std::optional<uint8_t> userChannelMask,
      bool enable) override;

  /*
   * Set the Transceiver loopback system side
   */
  virtual void setTransceiverLoopbackLocked(
      const std::string& portName,
      phy::Side side,
      bool setLoopback) override;

 private:
  // no copy or assignment
  CmisModule(CmisModule const&) = delete;
  CmisModule& operator=(CmisModule const&) = delete;

  // VDM data location of each VDM config types
  std::map<VdmConfigType, VdmDiagsLocationStatus> vdmConfigDataLocations_;

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
  void setModuleRxEqualizerLocked(
      RxEqualizerSettings rxEqualizer,
      uint8_t startHostLane,
      uint8_t numLanes);

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
  bool checkLaneConfigError(uint8_t startHostLane, uint8_t hostLaneCount);

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
   * Application advertising fields.
   */
  std::vector<ApplicationAdvertisingField> moduleCapabilities_;

  /*
   * Gets the module media interface. This is the intended media interface
   * application for this module. The module may be able to run in a different
   * application (with lesser bandwidth). For example if a 200G-FR4 module is
   * configured for 100G-CWDM4 application, then getModuleMediaInterface will
   * return 200G-FR4
   */
  MediaInterfaceCode getModuleMediaInterface() const override;

  void resetDataPathWithFunc(
      std::optional<std::function<void()>> afterDataPathDeinitFunc =
          std::nullopt,
      uint8_t hostLaneMask = 0xFF);

  /*
   * Set the PRBS Generator and Checker on a module for the desired side (Line
   * or System side)
   */
  bool setPortPrbsLocked(
      const std::string& portName,
      phy::Side side,
      const prbs::InterfacePrbsState& prbs) override;

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

  // Returns the currently configured mediaInterfaceCode on a host lane
  uint8_t currentConfiguredMediaInterfaceCode(uint8_t hostLane) const;

  CmisLaneState getDatapathLaneStateLocked(
      uint8_t lane,
      bool readFromCache = true);

  bool upgradeFirmwareLockedImpl(
      std::unique_ptr<FbossFirmware> fbossFw) const override;

  void readFromCacheOrHw(
      CmisField field,
      uint8_t* data,
      bool forcedReadFromHw = false);

  void updateVdmDiagsValLocation();

  double f16ToDouble(uint8_t byte0, uint8_t byte1);
  std::pair<std::optional<const uint8_t*>, int> getVdmDataValPtr(
      VdmConfigType vdmConf);

  SMFMediaInterfaceCode getMediaIntfCodeFromSpeed(
      cfg::PortSpeed speed,
      uint8_t numLanes);

  bool isMultiPortOptics() {
    return getIdentifier() == TransceiverModuleIdentifier::OSFP;
  }

  // Private functions to extract and fill in VDM performance monitoring stats
  bool fillVdmPerfMonitorSnr(VdmPerfMonitorStats& vdmStats);
  bool fillVdmPerfMonitorBer(VdmPerfMonitorStats& vdmStats);
  bool fillVdmPerfMonitorFecErr(VdmPerfMonitorStats& vdmStats);
  bool fillVdmPerfMonitorFecTail(VdmPerfMonitorStats& vdmStats);
  bool fillVdmPerfMonitorLtp(VdmPerfMonitorStats& vdmStats);
  bool fillVdmPerfMonitorPam4Data(VdmPerfMonitorStats& vdmStats);

  void setApplicationSelectCode(
      uint8_t apSelCode,
      uint8_t mediaInterfaceCode,
      uint8_t startHostLane,
      uint8_t numHostLanes,
      uint8_t hostLaneMask);
  void setApplicationSelectCodeAllPorts(
      cfg::PortSpeed speed,
      uint8_t startHostLane,
      uint8_t numHostLanes,
      uint8_t hostLaneMask);

  const std::shared_ptr<const TransceiverConfig> tcvrConfig_;

  bool supportRemediate_;
  std::map<int32_t, SymErrHistogramBin> getCdbSymbolErrorHistogramLocked(
      uint8_t datapathId,
      bool mediaSide);

  void clearTransceiverPrbsStats(const std::string& portName, phy::Side side)
      override;

  std::time_t vdmIntervalStartTime_{0};
};

} // namespace fboss
} // namespace facebook
