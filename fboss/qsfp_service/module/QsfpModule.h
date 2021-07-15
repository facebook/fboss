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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/ModuleStateMachine.h"
#include "fboss/qsfp_service/module/Transceiver.h"

#include <folly/Synchronized.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/futures/Future.h>
#include <optional>

namespace facebook {
namespace fboss {

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
      TransceiverManager* transceiverManager,
      std::unique_ptr<TransceiverImpl> qsfpImpl,
      unsigned int portsPerTransceiver);
  virtual ~QsfpModule() override;

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

  virtual void refresh() override;
  folly::Future<folly::Unit> futureRefresh() override;

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

  void transceiverPortsChanged(
      const std::map<uint32_t, PortStatus>& ports) override;

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

  // Module State Machine for this QsfpModule object
  msm::back::state_machine<moduleStateMachine> opticsModuleStateMachine_;

  // Port State Machine for all the ports inside this optics module
  std::vector<msm::back::state_machine<modulePortStateMachine>>
      opticsModulePortStateMachine_;

  // Module state machine function scheduler
  folly::FunctionScheduler opticsMsmFunctionScheduler_;

  /*
   * This is the helper function to create port state machine for all ports in
   * this module.
   */
  void addModulePortStateMachines();
  /*
   * This is the helper function to remove all the port state machine for the
   * module.
   */
  void eraseModulePortStateMachines();
  /*
   * This is the helper function to generate the event "Module Port Up" to the
   * Module State Machine
   */
  void genMsmModPortUpEvent();
  /*
   * This is the helper function to generate the event "Module Port Down" to
   * the Module State Machine
   */
  void genMsmModPortsDownEvent();
  /*
   * In the Discovered state we spawn a timeout to check for Agent port state
   * syncup to qsfp_service
   */
  void scheduleAgentPortSyncupTimeout();
  /*
   * While exiting the Discovered state we need to cancel the agent sync timeout
   * function scheduled earlier.
   */
  void cancelAgentPortSyncupTimeout();
  /*
   * This function spawns a periodic function to bring up the module/port by
   * bring up (first time only) or the remediate.
   */
  void scheduleBringupRemediateFunction();
  /*
   * This function cancels the function scheduled by Module SM Inactive state
   * entry function and stop the scheduled thread
   */
  void exitBringupRemediateFunction();
  /*
   * This function generates the port Up or port Down event to PSM if the Agent
   * has synced up port status info to qsfp_service
   */
  void checkAgentModulePortSyncup();
  /*
   * Function to return module id value for reference
   */
  int getModuleId();
  /*
   * This function will do optics module's port level hardware initialization.
   * If some optics needs the port/lane level init then the inheriting class
   * should override/implement this function.
   */
  virtual void opticsModulePortHwInit(int portId);

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

  using LengthAndGauge = std::pair<double, uint8_t>;

  /*
   * Returns the number of host lanes. Should be overridden by the appropriate
   * module's subclass
   */
  virtual unsigned int numHostLanes() const = 0;
  /*
   * Returns the number of media lanes. Should be overridden by the appropriate
   * module's subclass
   */
  virtual unsigned int numMediaLanes() const = 0;

  virtual void configureModule() {}

  virtual void moduleDiagsCapabilitySet() {}

  bool isVdmSupported() const {
    return diagsCapability_.has_value() && *diagsCapability_.value().vdm_ref();
  }

  std::optional<DiagsCapability> moduleDiagsCapabilityGet() const {
    return diagsCapability_;
  }

 protected:
  // no copy or assignment
  QsfpModule(QsfpModule const&) = delete;
  QsfpModule& operator=(QsfpModule const&) = delete;

  enum : unsigned int {
    EEPROM_DEFAULT = 255,
    MAX_GAUGE = 30,
    DECIMAL_BASE = 10,
    HEX_BASE = 16,
  };

  TransceiverManager* transceiverManager_{nullptr};

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
  /* This counter keeps track of the number of times
   * the optics remediation is performed on a given port
   */
  uint64_t numRemediation_{0};

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
  time_t lastCustomizeTime_{0};
  time_t lastRemediateTime_{0};

  // last time we know that no port was up on this transceiver.
  time_t lastDownTime_{0};

  // This is a map of system level port id to the local port id inside the
  // module. The local port id is used to identify the Port State Machine
  // instance within the module
  std::map<uint32_t, uint32_t> systemPortToModulePortIdMap_;

  // Diagnostic capabilities of the module
  std::optional<DiagsCapability> diagsCapability_;

  /*
   * This function will return the local module port id for the given system
   * port id. The local module port id is used to index into PSM instance
   */
  uint32_t getSystemPortToModulePortIdLocked(uint32_t sysPortId);

  /*
   * Perform transceiver customization
   * This must be called with a lock held on qsfpModuleMutex_
   *
   * Default speed is set to DEFAULT - this will prevent any speed specific
   * settings from being applied
   */
  virtual void customizeTransceiverLocked(
      cfg::PortSpeed speed = cfg::PortSpeed::DEFAULT) = 0;

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
  virtual void setPowerOverrideIfSupported(PowerControlState currentState) = 0;
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
  virtual PowerControlState getPowerControlValue() = 0;
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
   * of SFF-8024.
   */
  virtual ExtendedSpecComplianceCode getExtendedSpecificationComplianceCode()
      const = 0;

  double mwToDb(double value);

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

  virtual bool shouldRemediate(time_t cooldown);

  /*
   * Put logic here that should only be run on ports that have been
   * down for a long time. These are actions that are potentially more
   * disruptive, but have worked in the past to recover a transceiver.
   */
  virtual void remediateFlakyTransceiver() {}

  // make sure that tx_disable bits are clear
  virtual void ensureTxEnabled() {}

  // set to low power and then back to original setting. This has
  // effect of restarting the transceiver state machine
  virtual void resetLowPowerMode() {}

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

  /*
   * Perform logic OR operation to signal flags in order to cache them
   * for ODS to report.
   */
  void cacheSignalFlags(const SignalFlags& signalflag);

  /*
   * We found that some CMIS module did not enable Rx output squelch by
   * default, which introduced some difficulty to bring link back up when
   * flapped. This function is to ensure that Rx output squelch is always
   * enabled.
   */
  virtual void ensureRxOutputSquelchEnabled(
      const std::vector<HostLaneSettings>& /*hostLaneSettings*/) const {}

  virtual std::optional<VdmDiagsStats> getVdmDiagsStatsInfo() {
    return std::nullopt;
  }

  std::map<uint32_t, PortStatus> ports_;
  unsigned int portsPerTransceiver_{0};
  unsigned int moduleResetCounter_{0};

  unsigned int numHostLanes_{4};
  unsigned int numMediaLanes_{4};

  // Due to the mismatch of ODS reporting frequency and the interval of us
  // reading transceiver data, some of the clear on read information may
  // be lost in this process and not being captured in the ODS time series.
  // This would bring difficulty in root cause link issues. Thus here we
  // have a cache to store the collected data in ODS reporting frequency.
  SignalFlags signalFlagCache_;
  std::map<int, MediaLaneSignals> mediaSignalsCache_;

 private:
  void refreshLocked();
  virtual TransceiverInfo parseDataLocked();
  /*
   * Perform a raw register read on the transceiver
   * This must be called with a lock held on qsfpModuleMutex_
   */
  std::unique_ptr<IOBuf> readTransceiverLocked(TransceiverIOParameters param);

  /*
   * Perform a raw register write on the transceiver
   * This must be called with a lock held on qsfpModuleMutex_
   */
  bool writeTransceiverLocked(TransceiverIOParameters param, uint8_t data);

  /*
   * Perform logic OR operation to media lane signals in order to cache them
   * for ODS to report.
   */
  void cacheMediaLaneSignals(const std::vector<MediaLaneSignals>& mediaSignals);
};
} // namespace fboss
} // namespace facebook
