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
#include "fboss/qsfp_service/module/Transceiver.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <optional>
#include <folly/Synchronized.h>
#include <folly/futures/Future.h>

namespace facebook { namespace fboss {

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
  virtual TransceiverInfo parseDataLocked();

  virtual void refresh() override;
  folly::Future<folly::Unit> futureRefresh() override;
  void refreshLocked();

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

  // last time we know transceiver was working because at least one port was up
  time_t lastWorkingTime_{0};

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
  void getQsfpValue(int dataAddress,
                    int offset, int length, uint8_t* data) const;
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
  virtual
      ExtendedSpecComplianceCode getExtendedSpecificationComplianceCode() = 0;
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

  bool shouldRemediate(time_t cooldown) const;

  /*
   * Put logic here that should only be run on ports that have been
   * down for a long time. These are actions that are potentially more
   * disruptive, but have worked in the past to recover a transceiver.
   */
  virtual void remediateFlakyTransceiver(){}

  // make sure that tx_disable bits are clear
  virtual void ensureTxEnabled(){}

  // set to low power and then back to original setting. This has
  // effect of restarting the transceiver state machine
  virtual void resetLowPowerMode(){}

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
  unsigned int moduleResetCounter_{0};
};

}} //namespace facebook::fboss
