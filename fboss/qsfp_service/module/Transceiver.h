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
#include <vector>

#include <folly/futures/Future.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook {
namespace fboss {

enum TransceiverStateMachineEvent {
  TCVR_EV_EVENT_DETECT_TRANSCEIVER = 0,
  TCVR_EV_RESET_TRANSCEIVER = 1,
  TCVR_EV_REMOVE_TRANSCEIVER = 2,
  TCVR_EV_READ_EEPROM = 3,
  TCVR_EV_ALL_PORTS_DOWN = 4,
  TCVR_EV_PORT_UP = 5,
  // NOTE: Such event is never invoked in our code yet
  TCVR_EV_TRIGGER_UPGRADE = 6,
  // NOTE: Such event is never invoked in our code yet
  TCVR_EV_FORCED_UPGRADE = 7,
  TCVR_EV_AGENT_SYNC_TIMEOUT = 8,
  TCVR_EV_BRINGUP_DONE = 9,
  TCVR_EV_REMEDIATE_DONE = 10,
  TCVR_EV_PROGRAM_IPHY = 11,
  TCVR_EV_PROGRAM_XPHY = 12,
  TCVR_EV_PROGRAM_TRANSCEIVER = 13,
  TCVR_EV_RESET_TO_DISCOVERED = 14,
  TCVR_EV_RESET_TO_NOT_PRESENT = 15,
  TCVR_EV_REMEDIATE_TRANSCEIVER = 16,
  TCVR_EV_PREPARE_TRANSCEIVER = 17,
  TCVR_EV_UPGRADE_FIRMWARE = 18,
};

struct TransceiverPortState {
  std::string portName;
  uint8_t startHostLane;
  cfg::PortSpeed speed = cfg::PortSpeed::DEFAULT;
  uint8_t numHostLanes;
  TransmitterTechnology transmitterTech = TransmitterTechnology::UNKNOWN;

  bool operator==(const TransceiverPortState& other) const {
    return speed == other.speed && portName == other.portName &&
        startHostLane == other.startHostLane &&
        transmitterTech == other.transmitterTech;
  }
};

struct ProgramTransceiverState {
  std::map<std::string, TransceiverPortState> ports;

  bool operator==(const ProgramTransceiverState& other) const {
    return ports == other.ports;
  }
};

/* Virtual class to handle the different transceivers our equipment is likely
 * to support.  This supports, for now, QSFP and SFP.
 */

struct TransceiverID;
class TransceiverManager;
class FbossFirmware;

class Transceiver {
 public:
  explicit Transceiver() {}
  virtual ~Transceiver() {}

  /*
   * Transceiver type (SFP, QSFP)
   */
  virtual TransceiverType type() const = 0;

  virtual TransceiverID getID() const = 0;

  virtual const folly::EventBase* getEvb() const = 0;

  /*
   * Return the spec this transceiver follows.
   */
  virtual TransceiverManagementInterface managementInterface() const = 0;

  /*
   * For a transceiver, Returns [IsPresent, PresenceStatusChanged]
   */
  struct TransceiverPresenceDetectionStatus {
    bool present;
    bool statusChanged;
  };

  virtual TransceiverPresenceDetectionStatus detectPresence() = 0;

  // Return cached present state without asking from hardware
  bool isPresent() const {
    return present_;
  }

  /*
   * Check if the transceiver is present or not and refresh data.
   */
  virtual void refresh() = 0;
  virtual folly::Future<folly::Unit> futureRefresh() = 0;

  virtual void removeTransceiver() = 0;

  /*
   * Return all of the transceiver information
   */
  virtual TransceiverInfo getTransceiverInfo() const = 0;

  /*
   * Get the Transceiver Part Number
   */
  virtual std::string getPartNumber() const = 0;

  /*
   * Return raw page data from the qsfp DOM
   */
  virtual RawDOMData getRawDOMData() = 0;

  /*
   * Return a union of two spec formated data from the qsfp DOM
   */
  virtual DOMDataUnion getDOMDataUnion() = 0;

  /*
   * Program Transceiver based on speed
   * This new function will combine current customizeTransceiver() and
   * configureModule() from QsfpModule.
   * TODO(joseph5wu) Will deprecate configureModule() and customizeTransceiver()
   * once we switch to use the new state machine.
   */
  virtual void programTransceiver(
      ProgramTransceiverState& programTcvrState,
      bool needResetDataPath) = 0;

  /*
   * Check if the Transceiver is in ready state for further programming
   */
  virtual bool readyTransceiver() = 0;

  /*
   * Set speed specific settings for the transceiver
   */
  virtual void customizeTransceiver(TransceiverPortState& portState) = 0;

  /*
   * Perform raw register read on a specific transceiver
   */
  virtual std::unique_ptr<IOBuf> readTransceiver(
      TransceiverIOParameters param) = 0;
  /*
   * Future version of readTransceiver()
   */
  virtual folly::Future<std::pair<int32_t, std::unique_ptr<IOBuf>>>
  futureReadTransceiver(TransceiverIOParameters param) = 0;

  /*
   * Perform raw register write on a specific transceiver
   */
  virtual bool writeTransceiver(
      TransceiverIOParameters param,
      uint8_t data) = 0;
  /*
   * Future version of writeTransceiver()
   */
  virtual folly::Future<std::pair<int32_t, bool>> futureWriteTransceiver(
      TransceiverIOParameters param,
      uint8_t data) = 0;

  /*
   * return the cached signal flags and clear it after the read like an clear
   * on read register.
   */
  virtual SignalFlags readAndClearCachedSignalFlags() = 0;

  /*
   * return the cached Module Status and clear it after the read like an clear
   * on read register.
   */
  virtual ModuleStatus readAndClearCachedModuleStatus() = 0;

  /*
   * return the cached media lane signals and clear it after the read like an
   * clear on read register.
   */
  virtual std::map<int, MediaLaneSignals>
  readAndClearCachedMediaLaneSignals() = 0;

  virtual void triggerVdmStatsCapture() = 0;

  virtual void publishSnapshots() = 0;

  /*
   * Try to remediate such Transceiver if needed.
   * Return true means remediation is needed.
   * When allPortsDown is true, we trigger a full remediation otherwise we just
   * remediate specific datapaths
   */
  virtual bool tryRemediate(
      bool allPortsDown,
      time_t pauseRemediation,
      const std::vector<std::string>& ports) = 0;

  virtual bool shouldRemediate(time_t pauseRemediation) = 0;

  /*
   * Returns the list of prbs polynomials supported on the given side
   */
  virtual std::vector<prbs::PrbsPolynomial> getPrbsCapabilities(
      phy::Side side) const = 0;

  virtual bool setPortPrbs(
      const std::string& /* portName */,
      phy::Side /* side */,
      const prbs::InterfacePrbsState& /* prbs */) = 0;

  virtual prbs::InterfacePrbsState getPortPrbsState(
      std::optional<const std::string> /* portName */,
      phy::Side /* side */) = 0;

  virtual phy::PrbsStats getPortPrbsStats(
      const std::string& /* portName */,
      phy::Side /* side */) = 0;

  // Clear the PRBS stats for the given port and side
  virtual void clearTransceiverPrbsStats(
      const std::string& portName,
      phy::Side side) = 0;

  /*
   * Return true if such Transceiver can support remediation.
   * Different from shouldRemediate(), which will also check whether there's
   * a recent remediation and will returl false if the remediation interval
   * time is not up yet.
   * While this function is based on whether the Transceiver can support
   * remediation from HW perspective.
   */
  virtual bool supportRemediate() = 0;

  virtual void markLastDownTime() = 0;

  virtual time_t getLastDownTime() const = 0;

  /*
   * Verifies the Transceiver register checksum from EEPROM
   */
  virtual bool verifyEepromChecksums() {
    return true;
  }

  virtual void resetDataPath() = 0;

  virtual std::optional<DiagsCapability> getDiagsCapability() const = 0;

  virtual void setDiagsCapability() {}

  virtual bool setTransceiverTx(
      const std::string& /* portName */,
      phy::Side /* side */,
      std::optional<uint8_t> /* userChannelMask */,
      bool /* enable */) = 0;

  virtual void setTransceiverLoopback(
      const std::string& /* portName */,
      phy::Side /* side */,
      bool /* setLoopback */) = 0;

  time_t modulePauseRemediationUntil_{0};
  virtual void setModulePauseRemediation(int32_t timeout) = 0;
  virtual time_t getModulePauseRemediationUntil() = 0;
  bool getDirty_() const {
    return dirty_;
  }

  // Blocking call to upgrade the firmware on the transceiver.
  // Returns true if successful, false otherwise.
  virtual bool upgradeFirmware(
      std::vector<std::unique_ptr<FbossFirmware>>& fwList) = 0;

  virtual TransceiverInfo updateFwUpgradeStatusInTcvrInfoLocked(
      bool upgradeInProgress) = 0;

  virtual std::string getFwStorageHandle() const = 0;

  virtual std::map<std::string, CdbDatapathSymErrHistogram>
  getSymbolErrorHistogram() = 0;

  virtual std::vector<MediaInterfaceCode> getSupportedMediaInterfaces()
      const = 0;

  virtual bool tcvrPortStateSupported(
      TransceiverPortState& portState) const = 0;

 protected:
  virtual void latchAndReadVdmDataLocked() = 0;
  virtual bool shouldRemediateLocked(time_t pauseRemidiation) = 0;

  TransceiverManager* getTransceiverManager() const {
    return transceiverManager_;
  }

  // QSFP Presence status
  bool present_{false};
  // Denotes if the cache value is valid or stale
  bool dirty_{true};

 private:
  // no copy or assignment
  Transceiver(Transceiver const&) = delete;
  Transceiver& operator=(Transceiver const&) = delete;

  TransceiverManager* transceiverManager_{nullptr};
};

} // namespace fboss
} // namespace facebook
