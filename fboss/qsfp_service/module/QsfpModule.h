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
#include <optional>
#include <set>

#include <folly/Synchronized.h>
#include <folly/executors/FunctionScheduler.h>
#include <folly/futures/Future.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/firmware_storage/FbossFirmware.h"
#include "fboss/lib/link_snapshots/SnapshotManager.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/Transceiver.h"

#define QSFP_LOG(level, tcvr) \
  XLOG(level) << "Transceiver " << tcvr->getNameString() << ": "

#define QSFP_LOG_IF(level, cond, tcvr) \
  XLOG_IF(level, cond) << "Transceiver " << tcvr->getNameString() << ": "

namespace facebook {
namespace fboss {

struct QsfpConfig;
class TransceiverImpl;
class TransceiverManager;

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

using TransceiverOverrides = std::vector<cfg::TransceiverConfigOverride>;

struct TransceiverConfig {
  explicit TransceiverConfig(const TransceiverOverrides& overrides)
      : overridesConfig_(overrides) {}
  TransceiverOverrides overridesConfig_;
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
  static constexpr auto kSnapshotIntervalSeconds = 10;
  // Miniphoton module part number
  static constexpr auto kMiniphotonPartNumber = "LUX1626C4AD";
  using LengthAndGauge = std::pair<double, uint8_t>;

  explicit QsfpModule(
      std::set<std::string> portNames,
      TransceiverImpl* qsfpImpl);
  virtual ~QsfpModule() override;

  /*
   * Determines if the QSFP is present or not.
   */
  TransceiverPresenceDetectionStatus detectPresence() override;

  /*
   * Return a valid type.
   */
  TransceiverType type() const override {
    return TransceiverType::QSFP;
  }

  TransceiverID getID() const override;

  std::string getNameString() const;

  virtual void refresh() override;
  folly::Future<folly::Unit> futureRefresh() override;

  void removeTransceiver() override;

  /*
   * Customize QSPF fields as necessary
   *
   * speed - the speed the port is running at - this will allow setting
   * different qsfp settings based on speed
   */
  void customizeTransceiver(TransceiverPortState& portState) override;

  virtual bool tcvrPortStateSupported(
      TransceiverPortState& /* portState */) const override {
    return false;
  }

  /*
   * Returns the entire QSFP information
   */
  TransceiverInfo getTransceiverInfo() const override;

  /*
   * Returns the Transceiver Part Number
   */
  std::string getPartNumber() const override;

  /*
   * Perform a raw register read on the transceiver
   */
  std::unique_ptr<IOBuf> readTransceiver(
      TransceiverIOParameters param) override;

  /*
   * Perform a raw register write on the transceiver
   */
  bool writeTransceiver(TransceiverIOParameters param, uint8_t data) override;

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

  /*
   * return the cached signal flags and clear it after the read like an clear
   * on read register.
   */
  SignalFlags readAndClearCachedSignalFlags() override;

  /*
   * return the cached media lane signals and clear it after the read like an
   * clear on read register.
   */
  std::map<int, MediaLaneSignals> readAndClearCachedMediaLaneSignals() override;

  /*
   * return the cached module status and clear it after the read like an clear
   * on read register.
   */
  ModuleStatus readAndClearCachedModuleStatus() override;

  /*
   * Returns the total number of media lanes supported by the inserted
   * transceiver
   */
  virtual unsigned int numHostLanes() const;
  /*
   * Returns the total number of media lanes supported by the inserted
   * transceiver
   */
  unsigned int numMediaLanes() const;

  virtual void configureModule(uint8_t /* startHostLane */) {}

  bool isVdmSupported(uint8_t maxGroupRequested = 0) const;

  bool isPrbsSupported(phy::Side side) const {
    return isTransceiverFeatureSupported(TransceiverFeature::PRBS, side);
  }

  bool isSnrSupported(phy::Side side) const {
    auto diagsCapability = diagsCapability_.rlock();
    return (side == phy::Side::LINE)
        ? ((*diagsCapability).has_value() &&
           *(*diagsCapability).value().snrLine())
        : ((*diagsCapability).has_value() &&
           *(*diagsCapability).value().snrSystem());
  }

  std::optional<DiagsCapability> getDiagsCapability() const override {
    // return a copy to avoid needing a lock
    return diagsCapability_.copy();
  }

  /*
   * Returns the list of prbs polynomials supported on the given side
   */
  std::vector<prbs::PrbsPolynomial> getPrbsCapabilities(
      phy::Side side) const override {
    auto diagsCapability = diagsCapability_.rlock();
    if (!(*diagsCapability).has_value()) {
      return std::vector<prbs::PrbsPolynomial>();
    }
    if (side == phy::Side::SYSTEM) {
      return (*diagsCapability).value().get_prbsSystemCapabilities();
    }
    return (*diagsCapability).value().get_prbsLineCapabilities();
  }

  void clearTransceiverPrbsStats(const std::string& portName, phy::Side side)
      override;

  SnapshotManager getTransceiverSnapshots() const {
    // return a copy to avoid needing a lock in the caller
    return snapshots_.copy();
  }

  void programTransceiver(
      ProgramTransceiverState& programTcvrState,
      bool needResetDataPath) override;

  bool readyTransceiver() override;

  virtual void triggerVdmStatsCapture() override {}

  void publishSnapshots() override;

  /*
   * Try to remediate such Transceiver if needed.
   * Return true means remediation is needed.
   * When allPortsDown is true, we trigger a full remediation otherwise we just
   * remediate specific datapaths
   */
  bool tryRemediate(
      bool allPortsDown,
      time_t pauseRemediation,
      const std::vector<std::string>& ports) override;

  bool shouldRemediate(time_t pauseRemediation) override;

  void markLastDownTime() override;

  time_t getLastDownTime() const override {
    return lastDownTime_.load();
  }

  std::string getFwStorageHandle() const override;

  phy::PrbsStats getPortPrbsStats(const std::string& portName, phy::Side side)
      override;

  void updatePrbsStats();

  bool setPortPrbs(
      const std::string& /* portName */,
      phy::Side /* side */,
      const prbs::InterfacePrbsState& /* prbs */) override;

  prbs::InterfacePrbsState getPortPrbsState(
      std::optional<const std::string> /* portName */,
      phy::Side /* side */) override;

  void setModulePauseRemediation(int32_t timeout) override;

  time_t getModulePauseRemediationUntil() override;

  static TransceiverManagementInterface getTransceiverManagementInterface(
      const uint8_t moduleId,
      const unsigned int oneBasedPort);

  virtual std::vector<MediaInterfaceCode> getSupportedMediaInterfaces()
      const override;

  virtual std::vector<uint8_t> configuredHostLanes(
      uint8_t hostStartLane) const = 0;

  virtual std::vector<uint8_t> configuredMediaLanes(
      uint8_t hostStartLane) const = 0;

  const std::map<uint8_t, std::string>& getHostLaneToPortName() const {
    return hostLaneToPortName_;
  }

  const std::map<uint8_t, std::string>& getMediaLaneToPortName() const {
    return mediaLaneToPortName_;
  }

  const std::unordered_map<std::string, std::set<uint8_t>>&
  getPortNameToHostLanes() const {
    return portNameToHostLanes_;
  }

  const std::unordered_map<std::string, std::set<uint8_t>>&
  getPortNameToMediaLanes() const {
    return portNameToMediaLanes_;
  }

  virtual bool setTransceiverTx(
      const std::string& portName,
      phy::Side side,
      std::optional<uint8_t> userChannelMask,
      bool enable) override;

  bool isTransceiverFeatureSupported(TransceiverFeature feature) const;

  bool isTransceiverFeatureSupported(TransceiverFeature feature, phy::Side side)
      const;

  void setTransceiverLoopback(
      const std::string& portName,
      phy::Side side,
      bool setLoopback) override;

  std::map<std::string, CdbDatapathSymErrHistogram> getSymbolErrorHistogram()
      override;

 protected:
  /* Qsfp Internal Implementation */
  TransceiverImpl* qsfpImpl_;
  // Flat memory systems don't support paged access to extra data
  bool flatMem_{false};
  /* This counter keeps track of the number of times
   * the optics remediation is performed on a given port
   */
  uint64_t numRemediation_{0};

  folly::Synchronized<SnapshotManager> snapshots_;
  folly::Synchronized<std::optional<TransceiverInfo>> info_;
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
  time_t lastRemediateTime_{0};

  // last time we know that no port was up on this transceiver.
  std::atomic<time_t> lastDownTime_{0};

  // Diagnostic capabilities of the module
  folly::Synchronized<std::optional<DiagsCapability>> diagsCapability_;

  // VDM groups supported
  uint8_t vdmSupportedGroupsMax_ = 0;

  /*
   * Perform transceiver customization
   * This must be called with a lock held on qsfpModuleMutex_
   *
   * Default speed is set to DEFAULT - this will prevent any speed specific
   * settings from being applied
   */
  virtual void customizeTransceiverLocked(TransceiverPortState& portState) = 0;

  /*
   * If the current power state is not same as desired one then change it and
   * return true when module is in ready state
   */
  virtual bool ensureTransceiverReadyLocked() = 0;

  /*
   * This function returns a pointer to the value in the static cached
   * data after checking the length fits. The thread needs to have the lock
   * before calling this function.
   */
  virtual const uint8_t*
  getQsfpValuePtr(int dataAddress, int offset, int length) const = 0;
  /*
   * This function returns the values on the offset and length
   * from the static cached data. The thread needs to have the lock
   * before calling this function.
   */
  void getQsfpValue(int dataAddress, int offset, int length, uint8_t* data)
      const;
  /*
   * Based on identifier, sets whether the upper memory of the module is flat or
   * paged.
   */
  virtual void setQsfpFlatMem() = 0;
  /*
   * Set power mode
   * Wedge forces Low Power mode via a pin;  we have to reset this
   * to force High Power mode on LR4s.
   */
  virtual void setPowerOverrideIfSupportedLocked(
      PowerControlState currentState) = 0;
  /*
   * Enable or disable CDR
   * Which action to take is determined by the port speed
   */
  virtual void setCdrIfSupported(
      cfg::PortSpeed /*speed*/,
      FeatureState /*currentStateTx*/,
      FeatureState /*currentStateRx*/) {}
  /*
   * Set appropriate rate select value for PortSpeed, if supported
   */
  virtual void setRateSelectIfSupported(
      cfg::PortSpeed /*speed*/,
      RateSelectState /*currentState*/,
      RateSelectSetting /*currentSetting*/) {}

  /*
   * returns the freeside transceiver technology type
   */
  virtual TransmitterTechnology getQsfpTransmitterTechnology() const = 0;
  /*
   * Parse out flag values from bitfields
   */
  static FlagLevels getQsfpFlags(const uint8_t* data, int offset);

  bool validateQsfpString(const std::string& value) const;

  /*
   * Retreives all alarm and warning thresholds
   */
  virtual std::optional<AlarmThreshold> getThresholdInfo() = 0;
  /*
   * Gather the sensor info for thrift queries
   */
  virtual GlobalSensors getSensorInfo() = 0;
  /*
   * Gather per-channel information for thrift queries
   */
  virtual bool getSensorsPerChanInfo(std::vector<Channel>& channels) = 0;
  /*
   * Gather per-media-lane signal information for thrift queries
   */
  virtual bool getSignalsPerMediaLane(
      std::vector<MediaLaneSignals>& signals) = 0;
  /*
   * Gather per-host-lane signal information for thrift queries
   */
  virtual bool getSignalsPerHostLane(std::vector<HostLaneSignals>& signals) = 0;
  /*
   * Gather the vendor info for thrift queries
   */
  virtual Vendor getVendorInfo() = 0;
  /*
   * Gather the cable info for thrift queries
   */
  virtual Cable getCableInfo() = 0;

  /*
   * Gather info on what features are enabled and supported
   */
  virtual TransceiverSettings getTransceiverSettingsInfo() = 0;
  /*
   * Return what power control capability is currently enabled
   */
  virtual PowerControlState getPowerControlValue(bool readFromCache) = 0;
  /*
   * Return TransceiverStats
   */
  std::optional<TransceiverStats> getTransceiverStats();
  /*
   * Return SignalFlag which contains Tx/Rx LOS/LOL
   */
  virtual SignalFlags getSignalFlagInfo() = 0;
  /*
   * Return the extended specification compliance code of the module.
   * This is the field of Byte 192 on page00 and following table 4-4
   * of SFF-8024. Applicable only for sff-8636
   */
  virtual std::optional<ExtendedSpecComplianceCode>
  getExtendedSpecificationComplianceCode() const {
    return std::nullopt;
  }

  virtual std::vector<MediaInterfaceCode> getSupportedMediaInterfacesLocked()
      const {
    return std::vector<MediaInterfaceCode>();
  }

  double mwToDb(double value);

  /*
   * A utility function to return BER value in double format
   */
  static double getBerFloatValue(uint8_t lsb, uint8_t msb);

  virtual TransceiverModuleIdentifier getIdentifier() = 0;
  virtual ModuleStatus getModuleStatus() = 0;
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
  virtual void updateQsfpData(bool allPages = true) = 0;

  /*
   * Helpers to parse DOM data for DAC cables. These incorporate some
   * extra fields that FB has vendors put in the 'Vendor specific'
   * byte range of the SFF spec.
   */
  virtual double getQsfpDACLength() const = 0;

  /*
   * Put logic here that should only be run on ports that have been
   * down for a long time. These are actions that are potentially more
   * disruptive, but have worked in the past to recover a transceiver.
   */
  virtual void remediateFlakyTransceiver(
      bool allPortsDown,
      const std::vector<std::string>& ports) = 0;

  // make sure that tx_disable bits are clear
  virtual void ensureTxEnabled() {}

  // set to low power and then back to original setting. This has
  // effect of restarting the transceiver state machine
  virtual void resetLowPowerMode() {}

  /*
   * Gets the module media interface. This is the intended media interface
   * application for this module. The module may be able to run in a different
   * application (with lesser bandwidth). For example if a 200G-FR4 module is
   * configured for 100G-CWDM4 application, then getModuleMediaInterface will
   * return 200G-FR4
   */
  virtual MediaInterfaceCode getModuleMediaInterface() const = 0;

  /*
   * Returns true if getting the mediaInterfaceId is successful, false otherwise
   * Populates the passed in vector with the mediaInterfaceId per lane
   */
  virtual bool getMediaInterfaceId(
      std::vector<MediaInterfaceId>& mediaInterface) = 0;

  /*
   * Returns whether customization is supported at all. Basically
   * checks if something is plugged in and checks if copper.
   */
  virtual bool customizationSupported() const;

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
  virtual void ensureOutOfReset() const;

  /*
   * Perform logic OR operation to flags in order to cache them
   * for ODS to report.
   */
  void cacheSignalFlags(const SignalFlags& signalflag);
  void cacheStatusFlags(const ModuleStatus& status);

  virtual void latchAndReadVdmDataLocked() override {}

  /*
   * We found that some CMIS module did not enable Rx output squelch by
   * default, which introduced some difficulty to bring link back up when
   * flapped. This function is to ensure that Rx output squelch is always
   * enabled.
   */
  virtual void ensureRxOutputSquelchEnabled(
      const std::vector<HostLaneSettings>& /*hostLaneSettings*/) {}

  virtual std::optional<VdmDiagsStats> getVdmDiagsStatsInfo() {
    return std::nullopt;
  }

  virtual std::optional<VdmPerfMonitorStats> getVdmPerfMonitorStats() {
    return std::nullopt;
  }

  virtual VdmPerfMonitorStatsForOds getVdmPerfMonitorStatsForOds(
      VdmPerfMonitorStats& /* vdmPerfMonStats */) {
    return VdmPerfMonitorStatsForOds{};
  }

  virtual std::map<std::string, CdbDatapathSymErrHistogram>
  getCdbSymbolErrorHistogramLocked() {
    return {};
  }

  virtual bool setTransceiverTxLocked(
      const std::string& /* portName */,
      phy::Side /* side */,
      std::optional<uint8_t> /* userChannelMask */,
      bool /* enable */) {
    return false;
  }

  virtual bool setTransceiverTxImplLocked(
      const std::set<uint8_t>& /* tcvrLanes */,
      phy::Side /* side */,
      std::optional<uint8_t> /* userChannelMask */,
      bool /* enable */) {
    return false;
  }

  virtual void setTransceiverLoopbackLocked(
      const std::string& /* portName */,
      phy::Side /* side */,
      bool /* setLoopback */) {}

  /*
   * Returns a set of Transceiver lanes for a given SW port for a given side
   */
  std::set<uint8_t> getTcvrLanesForPort(
      const std::string& portName,
      phy::Side side) const;

  // Due to the mismatch of ODS reporting frequency and the interval of us
  // reading transceiver data, some of the clear on read information may
  // be lost in this process and not being captured in the ODS time series.
  // This would bring difficulty in root cause link issues. Thus here we
  // have a cache to store the collected data in ODS reporting frequency.
  SignalFlags signalFlagCache_;
  std::map<int, MediaLaneSignals> mediaSignalsCache_;
  ModuleStatus moduleStatusCache_;

  std::atomic_bool captureVdmStats_{false};

  folly::Synchronized<phy::PrbsStats> systemPrbsStats_;
  folly::Synchronized<phy::PrbsStats> linePrbsStats_;

  bool shouldRemediateLocked(time_t pauseRemidiation) override;

  virtual bool upgradeFirmwareLockedImpl(
      std::unique_ptr<FbossFirmware> /* fbossFw */) const {
    return false;
  }

  void triggerModuleReset();

  // Map key = laneId, value = last datapath reset time for that lane
  std::unordered_map<int, std::time_t> lastDatapathResetTimes_;

  uint8_t datapathResetPendingMask_{0};

 private:
  // no copy or assignment
  QsfpModule(QsfpModule const&) = delete;
  QsfpModule& operator=(QsfpModule const&) = delete;

  void refreshLocked();
  virtual void updateCachedTransceiverInfoLocked(ModuleStatus moduleStatus);

  void removeTransceiverLocked();

  TransceiverPresenceDetectionStatus detectPresenceLocked();

  /*
   * Try to remediate such Transceiver if needed.
   * Return true means remediation is needed.
   * When allPortsDown is true, we trigger a full remediation otherwise we just
   * remediate specific datapaths
   */
  bool tryRemediateLocked(
      bool allPortsDown,
      time_t pauseRemdiation,
      const std::vector<std::string>& ports);
  /*
   * Perform a raw register read on the transceiver
   * This must be called with a lock held on qsfpModuleMutex_
   */
  std::unique_ptr<IOBuf> readTransceiverLocked(TransceiverIOParameters param);
  /*
   * Future version of readTransceiver()
   */
  folly::Future<std::pair<int32_t, std::unique_ptr<IOBuf>>>
  futureReadTransceiver(TransceiverIOParameters param) override;

  /*
   * Perform a raw register write on the transceiver
   * This must be called with a lock held on qsfpModuleMutex_
   */
  bool writeTransceiverLocked(TransceiverIOParameters param, uint8_t data);
  /*
   * Future version of writeTransceiver()
   */
  folly::Future<std::pair<int32_t, bool>> futureWriteTransceiver(
      TransceiverIOParameters param,
      uint8_t data) override;

  bool upgradeFirmware(
      std::vector<std::unique_ptr<FbossFirmware>>& fwList) override;

  bool upgradeFirmwareLocked(
      std::vector<std::unique_ptr<FbossFirmware>>& fwList);

  /*
   * Perform logic OR operation to media lane signals in order to cache them
   * for ODS to report.
   */
  void cacheMediaLaneSignals(const std::vector<MediaLaneSignals>& mediaSignals);

  /*
   * Returns the current state of prbs (enabled/polynomial)
   */
  virtual prbs::InterfacePrbsState getPortPrbsStateLocked(
      std::optional<const std::string> /* portName */,
      phy::Side /* side */) {
    return prbs::InterfacePrbsState();
  }

  /*
   * Sets the prbs state on a port. Expects the caller to hold the qsfp module
   * level lock. Returns true if setting the prbs was successful
   */
  virtual bool setPortPrbsLocked(
      const std::string& /* portName */,
      phy::Side /* side */,
      const prbs::InterfacePrbsState& /* prbs */) {
    return false;
  }

  /*
   * Returns the port prbs stats for a given side
   * Expects the caller to hold the qsfp module level lock
   */
  virtual phy::PrbsStats getPortPrbsStatsSideLocked(
      phy::Side /* side */,
      bool /* checkerEnabled */,
      const phy::PrbsStats& /* lastStats */) {
    return phy::PrbsStats{};
  }

  /*
   * Update cmisStateChanged field of `moduleStatus` by comparing whether
   * `curModuleStatus` has the `true` value of cmisStateChanged
   * Only CMIS needs this
   */
  virtual void updateCmisStateChanged(
      ModuleStatus& /* moduleStatus */,
      std::optional<ModuleStatus> /* curModuleStatus */ = std::nullopt) {}

  friend class TransceiverStateMachineTest;

  std::map<uint8_t, std::string> hostLaneToPortName_;
  std::map<uint8_t, std::string> mediaLaneToPortName_;

  void updateLaneToPortNameMapping(
      const std::string& portName,
      uint8_t startHostLane);

  std::unordered_map<std::string, std::set<uint8_t>> portNameToHostLanes_;
  std::unordered_map<std::string, std::set<uint8_t>> portNameToMediaLanes_;

  time_t lastFwUpgradeStartTime_{0};
  time_t lastFwUpgradeEndTime_{0};

  std::string getFwStorageHandle(const std::string& tcvrPartNumber) const;

  const folly::EventBase* getEvb() const override;

  // Function to update the fwUpgradeInProgress field in
  // transceiverInfo.tcvrState and return the new transceiverInfo
  TransceiverInfo updateFwUpgradeStatusInTcvrInfoLocked(
      bool upgradeInProgress) override;

  std::string primaryPortName_;

  std::time_t getLastDatapathResetTime(int lane) {
    if (lastDatapathResetTimes_.find(lane) == lastDatapathResetTimes_.end()) {
      return 0;
    }
    return lastDatapathResetTimes_[lane];
  }
};
} // namespace fboss
} // namespace facebook
