/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/module/QsfpModule.h"

#include <boost/assign.hpp>
#include <fboss/lib/phy/gen-cpp2/phy_types.h>
#include <iomanip>
#include <string>
#include "common/time/Time.h"
#include "fboss/agent/FbossError.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/qsfp_service/StatsPublisher.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"

#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>

DEFINE_int32(
    qsfp_data_refresh_interval,
    10,
    "how often to refetch qsfp data that changes frequently");
DEFINE_int32(
    customize_interval,
    30,
    "minimum interval between customizing the same down port twice");
DEFINE_int32(
    remediate_interval,
    360,
    "seconds between running more destructive remediations on down ports");
DEFINE_int32(
    initial_remediate_interval,
    120,
    "seconds to wait before running first destructive remediations on down ports after bootup");

using folly::IOBuf;
using std::lock_guard;
using std::memcpy;
using std::mutex;

namespace facebook {
namespace fboss {

// Module state machine Timeout (seconds) for Agent to qsfp_service port status
// sync up first time
static constexpr int kStateMachineAgentPortSyncupTimeout = 120;
// Module State machine optics remediation/bringup interval (seconds)
static constexpr int kStateMachineOpticsRemediateInterval = 30;

TransceiverID QsfpModule::getID() const {
  return TransceiverID(qsfpImpl_->getNum());
}

// Converts power from milliwatts to decibel-milliwatts
double QsfpModule::mwToDb(double value) {
  if (value <= 0.01) {
    return -40;
  }
  return 10 * log10(value);
};

/*
 * Given a byte, extract bit fields for various alarm flags;
 * note the we might want to use the lower or the upper nibble,
 * so offset is the number of the bit to start at;  this is usually
 * 0 or 4.
 */

FlagLevels QsfpModule::getQsfpFlags(const uint8_t* data, int offset) {
  FlagLevels flags;

  CHECK_GE(offset, 0);
  CHECK_LE(offset, 4);
  flags.warn()->low() = (*data & (1 << offset));
  flags.warn()->high() = (*data & (1 << ++offset));
  flags.alarm()->low() = (*data & (1 << ++offset));
  flags.alarm()->high() = (*data & (1 << ++offset));

  return flags;
}

QsfpModule::QsfpModule(
    TransceiverManager* transceiverManager,
    std::unique_ptr<TransceiverImpl> qsfpImpl,
    unsigned int portsPerTransceiver)
    : transceiverManager_(transceiverManager),
      qsfpImpl_(std::move(qsfpImpl)),
      snapshots_(TransceiverSnapshotCache(
          // allowing a null transceiverManager here seems
          // kinda sketchy, but QsfpModule tests rely on it at the moment
          transceiverManager_ == nullptr
              ? std::set<std::string>()
              : transceiverManager_->getPortNames(getID()))),
      portsPerTransceiver_(portsPerTransceiver) {
  markLastDownTime();

  // Keeping the QsfpModule object raw pointer inside the Module State Machine
  // as an FSM attribute. This will be used when FSM invokes the state
  // transition or event handling function and in the callback we need something
  // from QsfpModule object
  setLegacyModuleStateMachineModulePointer(this);
}

QsfpModule::~QsfpModule() {
  // The transceiver has been removed
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  stateUpdateLocked(TransceiverStateMachineEvent::REMOVE_TRANSCEIVER);
}

/*
 * Function to return module id for reference in state machine logging
 */
int QsfpModule::getModuleId() {
  return qsfpImpl_->getNum();
}

/*
 * getSystemPortToModulePortIdLocked
 *
 * This function will return the local module port id for the given system
 * port id. The local module port id is used to index into PSM instance. It
 * also adds the system port Id to  the port mapping list if that does not
 * exist.
 */
uint32_t QsfpModule::getSystemPortToModulePortIdLocked(uint32_t sysPortId) {
  // If the system port id exist in the list then return the module port id
  // corresponding to it
  if (systemPortToModulePortIdMap_.find(sysPortId) !=
      systemPortToModulePortIdMap_.end()) {
    return systemPortToModulePortIdMap_.find(sysPortId)->second;
  }

  // If the system port id does not exist in the list then add it to the
  // end of the list and return that index
  uint32_t modPortId = systemPortToModulePortIdMap_.size();
  systemPortToModulePortIdMap_[sysPortId] = modPortId;

  return modPortId;
}

void QsfpModule::getQsfpValue(
    int dataAddress,
    int offset,
    int length,
    uint8_t* data) const {
  const uint8_t* ptr = getQsfpValuePtr(dataAddress, offset, length);

  memcpy(data, ptr, length);
}

// Note that this needs to be called while holding the
// qsfpModuleMutex_
bool QsfpModule::cacheIsValid() const {
  return present_ && !dirty_;
}

TransceiverInfo QsfpModule::getTransceiverInfo() {
  auto cachedInfo = info_.rlock();
  if (!cachedInfo->has_value()) {
    throw QsfpModuleError("Still populating data...");
  }
  return **cachedInfo;
}

bool QsfpModule::detectPresence() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  return detectPresenceLocked();
}

bool QsfpModule::detectPresenceLocked() {
  auto currentQsfpStatus = qsfpImpl_->detectTransceiver();
  if (currentQsfpStatus != present_) {
    XLOG(DBG1) << "Port: " << folly::to<std::string>(qsfpImpl_->getName())
               << " QSFP status changed from "
               << (present_ ? "PRESENT" : "NOT PRESENT") << " to "
               << (currentQsfpStatus ? "PRESENT" : "NOT PRESENT");
    dirty_ = true;
    present_ = currentQsfpStatus;
    moduleResetCounter_ = 0;

    // If a transceiver went from present to missing, clear the cached data.
    if (!present_) {
      info_.wlock()->reset();
    }
    // In the case of an OBO module or an inaccessable present module,
    // we need to fill in the essential info before parsing the DOM data
    // which may not be available.
    TransceiverInfo info;
    info.present() = present_;
    info.transceiver() = type();
    info.port() = qsfpImpl_->getNum();
    *info_.wlock() = info;
  }
  return currentQsfpStatus;
}

void QsfpModule::updateCachedTransceiverInfoLocked(ModuleStatus moduleStatus) {
  TransceiverInfo info;
  info.present() = present_;
  info.transceiver() = type();
  info.port() = qsfpImpl_->getNum();
  if (present_) {
    auto nMediaLanes = numMediaLanes();
    info.mediaLaneSignals() = std::vector<MediaLaneSignals>(nMediaLanes);
    if (!getSignalsPerMediaLane(*info.mediaLaneSignals())) {
      info.mediaLaneSignals()->clear();
    } else {
      cacheMediaLaneSignals(*info.mediaLaneSignals());
    }

    info.sensor() = getSensorInfo();
    info.vendor() = getVendorInfo();
    info.cable() = getCableInfo();
    if (auto threshold = getThresholdInfo()) {
      info.thresholds() = *threshold;
    }
    info.settings() = getTransceiverSettingsInfo();

    for (int i = 0; i < nMediaLanes; i++) {
      Channel chan;
      chan.channel() = i;
      info.channels()->push_back(chan);
    }
    if (!getSensorsPerChanInfo(*info.channels())) {
      info.channels()->clear();
    }

    info.hostLaneSignals() = std::vector<HostLaneSignals>(numHostLanes());
    if (!getSignalsPerHostLane(*info.hostLaneSignals())) {
      info.hostLaneSignals()->clear();
    }

    if (auto transceiverStats = getTransceiverStats()) {
      info.stats() = *transceiverStats;
    }

    info.signalFlag() = getSignalFlagInfo();
    cacheSignalFlags(*info.signalFlag());

    if (auto extSpecCompliance = getExtendedSpecificationComplianceCode()) {
      info.extendedSpecificationComplianceCode() = *extSpecCompliance;
    }
    info.transceiverManagementInterface() = managementInterface();

    info.identifier() = getIdentifier();
    auto currentStatus = getModuleStatus();
    // Use the input `moduleStatus` as the reference to update the
    // `cmisStateChanged` for currentStatus, which will be used in the
    // TransceiverInfo
    updateCmisStateChanged(currentStatus, moduleStatus);
    info.status() = currentStatus;
    cacheStatusFlags(currentStatus);

    // If the StatsPublisher thread has triggered the VDM data capture then
    // latch, read data (page 24 and 25), release latch
    if (captureVdmStats_) {
      latchAndReadVdmDataLocked();
    }

    if (auto vdmStats = getVdmDiagsStatsInfo()) {
      info.vdmDiagsStats() = *vdmStats;

      // If the StatsPublisher thread has triggered the VDM data capture then
      // capure this data into transceiverInfo cache
      if (captureVdmStats_) {
        info.vdmDiagsStatsForOds() = *vdmStats;
      } else {
        // If the VDM is not updated in this cycle then retain older values
        auto cachedTcvrInfo = getTransceiverInfo();
        if (cachedTcvrInfo.vdmDiagsStatsForOds()) {
          info.vdmDiagsStatsForOds() =
              cachedTcvrInfo.vdmDiagsStatsForOds().value();
        }
      }
      captureVdmStats_ = false;
    }

    info.timeCollected() = lastRefreshTime_;
    info.remediationCounter() = numRemediation_;
    info.eepromCsumValid() = verifyEepromChecksums();

    info.moduleMediaInterface() = getModuleMediaInterface();
  }

  phy::LinkSnapshot snapshot;
  snapshot.transceiverInfo_ref() = info;
  snapshots_.wlock()->addSnapshot(snapshot);
  *info_.wlock() = info;
}

bool QsfpModule::safeToCustomize() const {
  // This function is no longer needed with the new state machine implementation
  if (FLAGS_use_new_state_machine) {
    return false;
  }

  if (ports_.size() < portsPerTransceiver_) {
    XLOG(DBG1) << "Not all ports present in transceiver " << getID()
               << " (expected=" << portsPerTransceiver_
               << "). Skip customization";

    return false;
  } else if (ports_.size() > portsPerTransceiver_) {
    throw FbossError(
        ports_.size(),
        " ports found in transceiver ",
        getID(),
        " (max=",
        portsPerTransceiver_,
        ")");
  }

  bool anyEnabled{false};
  for (const auto& port : ports_) {
    const auto& status = port.second;
    if (*status.up()) {
      return false;
    }
    anyEnabled = anyEnabled || *status.enabled();
  }

  // Only return safe if at least one port is enabled
  return anyEnabled;
}

bool QsfpModule::customizationWanted(time_t cooldown) const {
  // This function is no longer needed with the new state machine implementation
  if (FLAGS_use_new_state_machine) {
    return false;
  }
  if (needsCustomization_) {
    return true;
  }
  if (std::time(nullptr) - lastCustomizeTime_ < cooldown) {
    return false;
  }
  return safeToCustomize();
}

bool QsfpModule::customizationSupported() const {
  // TODO: there may be a better way of determining this rather than
  // looking at transmitter tech.
  auto tech = getQsfpTransmitterTechnology();
  return present_ && tech != TransmitterTechnology::COPPER;
}

bool QsfpModule::shouldRefresh(time_t cooldown) const {
  return std::time(nullptr) - lastRefreshTime_ >= cooldown;
}

void QsfpModule::ensureOutOfReset() const {
  qsfpImpl_->ensureOutOfReset();
  XLOG(DBG3) << "Cleared the reset register of QSFP.";
}

cfg::PortSpeed QsfpModule::getPortSpeed() const {
  cfg::PortSpeed speed = cfg::PortSpeed::DEFAULT;
  for (const auto& port : ports_) {
    const auto& status = port.second;
    auto newSpeed = cfg::PortSpeed(*status.speedMbps());
    if (!(*status.enabled()) || speed == newSpeed) {
      continue;
    }

    if (speed == cfg::PortSpeed::DEFAULT) {
      speed = newSpeed;
    } else {
      throw FbossError(
          "Multiple speeds found for member ports of transceiver ", getID());
    }
  }
  return speed;
}

void QsfpModule::cacheSignalFlags(const SignalFlags& signalflag) {
  signalFlagCache_.txLos() = *signalflag.txLos() | *signalFlagCache_.txLos();
  signalFlagCache_.rxLos() = *signalflag.rxLos() | *signalFlagCache_.rxLos();
  signalFlagCache_.txLol() = *signalflag.txLol() | *signalFlagCache_.txLol();
  signalFlagCache_.rxLol() = *signalflag.rxLol() | *signalFlagCache_.rxLol();
}

void QsfpModule::cacheStatusFlags(const ModuleStatus& status) {
  if (moduleStatusCache_.cmisStateChanged() && status.cmisStateChanged()) {
    moduleStatusCache_.cmisStateChanged() =
        *status.cmisStateChanged() | *moduleStatusCache_.cmisStateChanged();
  } else {
    moduleStatusCache_.cmisStateChanged().copy_from(status.cmisStateChanged());
  }
}

void QsfpModule::cacheMediaLaneSignals(
    const std::vector<MediaLaneSignals>& mediaSignals) {
  for (const auto& signal : mediaSignals) {
    if (mediaSignalsCache_.find(*signal.lane()) == mediaSignalsCache_.end()) {
      // Initialize all lanes to false if an entry in the cache doesn't exist
      // yet
      mediaSignalsCache_[*signal.lane()].lane() = *signal.lane();
      mediaSignalsCache_[*signal.lane()].txFault() = false;
    }
    if (auto txFault = signal.txFault()) {
      if (*txFault) {
        mediaSignalsCache_[*signal.lane()].txFault() = true;
      }
    }
  }
}

bool QsfpModule::setPortPrbs(phy::Side side, const phy::PortPrbsState& prbs) {
  bool status = false;
  auto setPrbsLambda = [&status, side, prbs, this]() {
    lock_guard<std::mutex> g(qsfpModuleMutex_);
    status = setPortPrbsLocked(side, prbs);
  };
  auto i2cEvb = qsfpImpl_->getI2cEventBase();
  if (!i2cEvb) {
    // Certain platforms cannot execute multiple I2C transactions in parallel
    // and therefore don't have an I2C evb thread
    setPrbsLambda();
  } else {
    via(i2cEvb)
        .thenValue([setPrbsLambda](auto&&) mutable { setPrbsLambda(); })
        .get();
  }
  return status;
}

prbs::InterfacePrbsState QsfpModule::getPortPrbsState(phy::Side side) {
  prbs::InterfacePrbsState state;
  auto getPrbsStateLambda = [&state, side, this]() {
    lock_guard<std::mutex> g(qsfpModuleMutex_);
    state = getPortPrbsStateLocked(side);
  };
  auto i2cEvb = qsfpImpl_->getI2cEventBase();
  if (!i2cEvb) {
    // Certain platforms cannot execute multiple I2C transactions in parallel
    // and therefore don't have an I2C evb thread
    getPrbsStateLambda();
  } else {
    via(i2cEvb)
        .thenValue(
            [getPrbsStateLambda](auto&&) mutable { getPrbsStateLambda(); })
        .get();
  }
  return state;
}

void QsfpModule::transceiverPortsChanged(
    const std::map<uint32_t, PortStatus>& ports) {
  // Always use i2cEvb to program transceivers if there's an i2cEvb
  auto transceiverPortsChangedHandler = [&ports, this]() {
    lock_guard<std::mutex> g(qsfpModuleMutex_);
    // List of ports inside this module whose operation status has changed
    std::vector<uint32_t> changedPortList;
    auto anyStateChanged = false;

    for (auto& it : ports) {
      CHECK(
          TransceiverID(
              *it.second.transceiverIdx().value_or({}).transceiverId()) ==
          getID());

      // Record this port in the changed port list if:
      //  - The existing port status is empty (first time sync from agent)
      //  - The new port status synced from Agent is different from existing
      //  port status
      if (ports_[it.first].profileID()->empty() ||
          (*ports_[it.first].up() != *it.second.up())) {
        if (*ports_[it.first].up() != *it.second.up()) {
          anyStateChanged = true;
        }
        changedPortList.push_back(it.first);
      }

      ports_[it.first] = std::move(it.second);
    }

    if (anyStateChanged) {
      publishSnapshots();
    }

    // update the present_ field (and will set dirty_ if presence change
    // detected)
    detectPresenceLocked();

    if (safeToCustomize()) {
      needsCustomization_ = true;
      // safetocustomize helped confirmed that no port was up for this
      // transceiver. Record the time for future references.
      markLastDownTime();
    }

    if (dirty_) {
      // data is stale. This could happen immediately after plugging a
      // port in. Refresh inline in this case in order to not return
      // stale data.
      refreshLocked();
    }

    // For the ports in the Changed Port List, generate the Port Up or Port Down
    // event to the corresponding Port State Machine
    for (auto& it : changedPortList) {
      // Get the module port id so that we can index into port state machine
      // instance
      uint32_t modulePortId = getSystemPortToModulePortIdLocked(it);

      if (*ports_[it].up()) {
        // Generate Port up event
        if (modulePortId < portStateMachines_.size()) {
          portStateMachines_[modulePortId].process_event(
              MODULE_PORT_EVENT_AGENT_PORT_UP);
        }
      } else {
        // Generate Port Down event
        if (modulePortId < portStateMachines_.size()) {
          portStateMachines_[modulePortId].process_event(
              MODULE_PORT_EVENT_AGENT_PORT_DOWN);
        }
      }
    }
  };

  auto i2cEvb = qsfpImpl_->getI2cEventBase();
  if (!i2cEvb) {
    // Certain platforms cannot execute multiple I2C transactions in parallel
    // and therefore don't have an I2C evb thread
    transceiverPortsChangedHandler();
  } else {
    via(i2cEvb)
        .thenValue([transceiverPortsChangedHandler](auto&&) mutable {
          transceiverPortsChangedHandler();
        })
        .get();
  }
}

void QsfpModule::refresh() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  refreshLocked();
}

folly::Future<folly::Unit> QsfpModule::futureRefresh() {
  // Always use i2cEvb to program transceivers if there's an i2cEvb
  auto i2cEvb = qsfpImpl_->getI2cEventBase();
  if (!i2cEvb) {
    try {
      refresh();
    } catch (const std::exception& ex) {
      XLOG(DBG2) << "Transceiver " << static_cast<int>(this->getID())
                 << ": Error calling refresh(): " << ex.what();
    }
    return folly::makeFuture();
  }

  return via(i2cEvb).thenValue([&](auto&&) mutable {
    try {
      this->refresh();
    } catch (const std::exception& ex) {
      XLOG(DBG2) << "Transceiver " << static_cast<int>(this->getID())
                 << ": Error calling refresh(): " << ex.what();
    }
  });
}

void QsfpModule::refreshLocked() {
  ModuleStatus moduleStatus;
  detectPresenceLocked();

  bool newTransceiverDetected = false;
  auto customizeWanted = customizationWanted(FLAGS_customize_interval);
  auto willRefresh = !dirty_ && shouldRefresh(FLAGS_qsfp_data_refresh_interval);
  if (!dirty_ && !customizeWanted && !willRefresh) {
    return;
  }

  if (dirty_ && present_) {
    // A new transceiver has been detected
    stateUpdateLocked(TransceiverStateMachineEvent::DETECT_TRANSCEIVER);
    newTransceiverDetected = true;
  } else if (dirty_ && !present_) {
    // The transceiver has been removed
    stateUpdateLocked(TransceiverStateMachineEvent::REMOVE_TRANSCEIVER);
  }

  // Each of the reset functions need to check whether the transceiver is
  // present or not, and then handle its own logic differently. Even though
  // the transceiver might be absent here, we'll still go through all of the
  // rest functions
  if (dirty_) {
    // make sure data is up to date before trying to customize.
    ensureOutOfReset();
    updateQsfpData(true);
    updateCmisStateChanged(moduleStatus);
  }

  if (newTransceiverDetected) {
    // Data has been read for the new optics
    stateUpdateLocked(TransceiverStateMachineEvent::READ_EEPROM);
    // Issue an allPages=false update to pick up the new qsfp data after we
    // trigger READ_EEPROM event. Some Transceiver might pick up all the diag
    // capabilities and we can use this to make sure the current QsfpData has
    // updated pages without waiting for the next refresh
    updateQsfpData(false);
  }

  // The following should be deprecated after switching to use new state machine
  if (!FLAGS_use_new_state_machine) {
    if (customizeWanted) {
      customizeTransceiverLocked(getPortSpeed());

      tryRemediateLocked();
    }

    TransceiverSettings settings = getTransceiverSettingsInfo();
    // We found that some module did not enable Rx output squelch by default,
    // which introduced some difficulty to bring link back up when flapped.
    // Here we ensure that Rx output squelch is always enabled.
    if (auto hostLaneSettings = settings.hostLaneSettings()) {
      ensureRxOutputSquelchEnabled(*hostLaneSettings);
    }
  }

  if (customizeWanted || willRefresh) {
    // update either if data is stale or if we customized this
    // round. We update in the customization because we may have
    // written fields, but only need a partial update because all of
    // these fields are in the LOWER qsfp page. There are a small
    // number of writable fields on other qsfp pages, but we don't
    // currently use them.
    updateQsfpData(false);
    updateCmisStateChanged(moduleStatus);
  }

  updateCachedTransceiverInfoLocked(moduleStatus);
  // Only update prbs stats if the transceiver is present.
  // Should have this check inside of updatePrbsStats().
  // However updatePrbsStats() is a public function and not lock safe as
  // refresh() to get the qsfpModuleMutex_ first.
  // TODO: Need to rethink whether all the following prbs stats functions should
  // get the lock of qsfpModuleMutex_ first.
  if (present_) {
    updatePrbsStats();
  }
}

void QsfpModule::clearTransceiverPrbsStats(phy::Side side) {
  auto systemPrbs = systemPrbsStats_.wlock();
  auto linePrbs = linePrbsStats_.wlock();

  auto clearLaneStats = [](std::vector<phy::PrbsLaneStats>& laneStats) {
    for (auto& laneStat : laneStats) {
      laneStat.maxBer() = 0;
      laneStat.numLossOfLock() = 0;
      laneStat.timeSinceLastClear() = std::time(nullptr);
    }
  };
  if (side == phy::Side::SYSTEM) {
    clearLaneStats(*systemPrbs->laneStats());
  } else {
    clearLaneStats(*linePrbs->laneStats());
  }
}

void QsfpModule::updatePrbsStats() {
  auto systemPrbs = systemPrbsStats_.wlock();
  auto linePrbs = linePrbsStats_.wlock();

  // Initialize the stats if they didn't exist before
  if (!(*systemPrbs->laneStats()).size()) {
    systemPrbs->portId() = getID();
    systemPrbs->component() = phy::PrbsComponent::TRANSCEIVER_SYSTEM;
    systemPrbs->laneStats() = std::vector<phy::PrbsLaneStats>(numHostLanes());
  }
  if (!(*linePrbs->laneStats()).size()) {
    linePrbs->portId() = getID();
    linePrbs->component() = phy::PrbsComponent::TRANSCEIVER_LINE;
    linePrbs->laneStats() = std::vector<phy::PrbsLaneStats>(numMediaLanes());
  }

  auto updatePrbsStatEntry = [](const phy::PrbsStats& oldStat,
                                phy::PrbsStats& newStat) {
    for (const auto& oldLane : *oldStat.laneStats()) {
      for (auto& newLane : *newStat.laneStats()) {
        if (*newLane.laneId() != *oldLane.laneId()) {
          continue;
        }
        // Update numLossOfLock
        if (!(*newLane.locked()) && *oldLane.locked()) {
          newLane.numLossOfLock() = *oldLane.numLossOfLock() + 1;
        } else {
          newLane.numLossOfLock() = *oldLane.numLossOfLock();
        }
        // Update maxBer only if there is a lock
        if (*newLane.locked() && *newLane.ber() > *oldLane.maxBer()) {
          newLane.maxBer() = *newLane.ber();
        } else {
          newLane.maxBer() = *oldLane.maxBer();
        }
        // Update timeSinceLastLocked
        // If previously there was no lock and now there is, update
        // timeSinceLastLocked to now
        if (!(*oldLane.locked()) && *newLane.locked()) {
          newLane.timeSinceLastLocked() = *newStat.timeCollected();
        } else {
          newLane.timeSinceLastLocked() = *oldLane.timeSinceLastLocked();
        }
        newLane.timeSinceLastClear() = *oldLane.timeSinceLastClear();
      }
    }
  };

  auto sysPrbsState = getPortPrbsStateLocked(phy::Side::SYSTEM);
  auto linePrbsState = getPortPrbsStateLocked(phy::Side::LINE);
  // Only update stats when prbs is enabled
  if (sysPrbsState.checkerEnabled().has_value() &&
      sysPrbsState.checkerEnabled().value()) {
    auto stats = getPortPrbsStatsSideLocked(phy::Side::SYSTEM);
    updatePrbsStatEntry(*systemPrbs, stats);
    *systemPrbs = stats;
  }

  if (linePrbsState.checkerEnabled().has_value() &&
      linePrbsState.checkerEnabled().value()) {
    auto stats = getPortPrbsStatsSideLocked(phy::Side::LINE);
    updatePrbsStatEntry(*linePrbs, stats);
    *linePrbs = stats;
  }
}

bool QsfpModule::shouldRemediate() {
  // Always use i2cEvb to program transceivers if there's an i2cEvb
  auto shouldRemediateFunc = [this]() {
    lock_guard<std::mutex> g(qsfpModuleMutex_);
    return shouldRemediateLocked();
  };
  auto i2cEvb = qsfpImpl_->getI2cEventBase();
  if (!i2cEvb) {
    // Certain platforms cannot execute multiple I2C transactions in parallel
    // and therefore don't have an I2C evb thread
    return shouldRemediateFunc();
  } else {
    bool shouldRemediateResult = false;
    via(i2cEvb)
        .thenValue(
            [&shouldRemediateResult, shouldRemediateFunc](auto&&) mutable {
              shouldRemediateResult = shouldRemediateFunc();
            })
        .get();
    return shouldRemediateResult;
  }
}

bool QsfpModule::shouldRemediateLocked() {
  if (!supportRemediate()) {
    return false;
  }

  auto sysPrbsState = getPortPrbsStateLocked(phy::Side::SYSTEM);
  auto linePrbsState = getPortPrbsStateLocked(phy::Side::LINE);

  auto linePrbsEnabled =
      ((linePrbsState.generatorEnabled().has_value() &&
        linePrbsState.generatorEnabled().value()) ||
       (linePrbsState.checkerEnabled().has_value() &&
        linePrbsState.checkerEnabled().value()));
  auto sysPrbsEnabled =
      ((sysPrbsState.generatorEnabled().has_value() &&
        sysPrbsState.generatorEnabled().value()) ||
       (sysPrbsState.checkerEnabled().has_value() &&
        sysPrbsState.checkerEnabled().value()));

  if (linePrbsEnabled || sysPrbsEnabled) {
    XLOG(INFO) << "Skipping remediation because PRBS is enabled. System: "
               << sysPrbsEnabled << ", Line: " << linePrbsEnabled;
    return false;
  }

  auto now = std::time(nullptr);
  bool remediationEnabled =
      now > transceiverManager_->getPauseRemediationUntil();
  // Rather than immediately attempting to remediate a module,
  // we would like to introduce a bit delay to de-couple the consequences
  // of a remediation with the root cause that brought down the link.
  // This is an effort to help with debugging.
  // And for the first remediation, we don't want to wait for
  // `FLAGS_remediate_interval`, instead we just need to wait for
  // `FLAGS_initial_remediate_interval`. (D26014510)
  bool remediationCooled = false;
  if (lastDownTime_ > lastRemediateTime_) {
    // New lastDownTime_ means the port just recently went down
    remediationCooled =
        (now - lastDownTime_) > FLAGS_initial_remediate_interval;
  } else {
    remediationCooled = (now - lastRemediateTime_) > FLAGS_remediate_interval;
  }
  return remediationEnabled && remediationCooled;
}

void QsfpModule::customizeTransceiver(cfg::PortSpeed speed) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  if (present_) {
    customizeTransceiverLocked(speed);
  }
}

void QsfpModule::customizeTransceiverLocked(cfg::PortSpeed speed) {
  /*
   * This must be called with a lock held on qsfpModuleMutex_
   */
  if (customizationSupported()) {
    TransceiverSettings settings = getTransceiverSettingsInfo();

    // We want this on regardless of speed
    setPowerOverrideIfSupported(*settings.powerControl());

    if (speed != cfg::PortSpeed::DEFAULT) {
      setCdrIfSupported(speed, *settings.cdrTx(), *settings.cdrRx());
      setRateSelectIfSupported(
          speed, *settings.rateSelect(), *settings.rateSelectSetting());
    }
  } else {
    XLOG(DBG1) << "Customization not supported on " << qsfpImpl_->getName();
  }

  lastCustomizeTime_ = std::time(nullptr);
  needsCustomization_ = false;
}

std::optional<TransceiverStats> QsfpModule::getTransceiverStats() {
  auto transceiverStats = qsfpImpl_->getTransceiverStats();
  if (!transceiverStats.has_value()) {
    return {};
  }
  return transceiverStats.value();
}

folly::Future<std::pair<int32_t, std::unique_ptr<IOBuf>>>
QsfpModule::futureReadTransceiver(TransceiverIOParameters param) {
  // Always use i2cEvb to program transceivers if there's an i2cEvb
  auto i2cEvb = qsfpImpl_->getI2cEventBase();
  auto id = getID();
  if (!i2cEvb) {
    // Certain platforms cannot execute multiple I2C transactions in parallel
    // and therefore don't have an I2C evb thread
    return std::make_pair(id, readTransceiver(param));
  }
  // As with all the other i2c transactions, run in the i2c event base thread
  return via(i2cEvb).thenValue([&, param, id](auto&&) mutable {
    return std::make_pair(id, readTransceiver(param));
  });
}

std::unique_ptr<IOBuf> QsfpModule::readTransceiver(
    TransceiverIOParameters param) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  return readTransceiverLocked(param);
}

std::unique_ptr<IOBuf> QsfpModule::readTransceiverLocked(
    TransceiverIOParameters param) {
  /*
   * This must be called with a lock held on qsfpModuleMutex_
   */
  auto length = param.length().has_value() ? *(param.length()) : 1;
  auto iobuf = folly::IOBuf::createCombined(length);
  if (!present_) {
    return iobuf;
  }
  try {
    auto offset = *(param.offset());
    if (param.page().has_value()) {
      uint8_t page = *(param.page());
      // When the page is specified, first update byte 127 with the speciied
      // pageId
      qsfpImpl_->writeTransceiver(
          {TransceiverI2CApi::ADDR_QSFP, 127, sizeof(page)}, &page);
    }
    qsfpImpl_->readTransceiver(
        {TransceiverI2CApi::ADDR_QSFP, offset, length}, iobuf->writableData());
    // Mark the valid data in the buffer
    iobuf->append(length);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error reading data for transceiver:"
              << folly::to<std::string>(qsfpImpl_->getName()) << ": "
              << ex.what();
    throw;
  }
  return iobuf;
}

folly::Future<std::pair<int32_t, bool>> QsfpModule::futureWriteTransceiver(
    TransceiverIOParameters param,
    uint8_t data) {
  // Always use i2cEvb to program transceivers if there's an i2cEvb
  auto i2cEvb = qsfpImpl_->getI2cEventBase();
  auto id = getID();
  if (!i2cEvb) {
    // Certain platforms cannot execute multiple I2C transactions in parallel
    // and therefore don't have an I2C evb thread
    return std::make_pair(id, writeTransceiver(param, data));
  }
  // As with all the other i2c transactions, run in the i2c event base thread
  return via(i2cEvb).thenValue([&, param, id, data](auto&&) mutable {
    return std::make_pair(id, writeTransceiver(param, data));
  });
}

bool QsfpModule::writeTransceiver(TransceiverIOParameters param, uint8_t data) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  return writeTransceiverLocked(param, data);
}

bool QsfpModule::writeTransceiverLocked(
    TransceiverIOParameters param,
    uint8_t data) {
  /*
   * This must be called with a lock held on qsfpModuleMutex_
   */
  if (!present_) {
    return false;
  }
  try {
    auto offset = *(param.offset());
    if (param.page().has_value()) {
      uint8_t page = *(param.page());
      // When the page is specified, first update byte 127 with the speciied
      // pageId
      qsfpImpl_->writeTransceiver(
          {TransceiverI2CApi::ADDR_QSFP, 127, sizeof(page)}, &page);
    }
    qsfpImpl_->writeTransceiver(
        {TransceiverI2CApi::ADDR_QSFP, offset, sizeof(data)}, &data);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error writing data to transceiver:"
              << folly::to<std::string>(qsfpImpl_->getName()) << ": "
              << ex.what();
    throw;
  }
  return true;
}

SignalFlags QsfpModule::readAndClearCachedSignalFlags() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  SignalFlags signalFlag;
  // Store the cached data before clearing it.
  signalFlag.txLos() = *signalFlagCache_.txLos();
  signalFlag.rxLos() = *signalFlagCache_.rxLos();
  signalFlag.txLol() = *signalFlagCache_.txLol();
  signalFlag.rxLol() = *signalFlagCache_.rxLol();

  // Clear the cached data after read.
  signalFlagCache_.txLos() = 0;
  signalFlagCache_.rxLos() = 0;
  signalFlagCache_.txLol() = 0;
  signalFlagCache_.rxLol() = 0;
  return signalFlag;
}

std::map<int, MediaLaneSignals>
QsfpModule::readAndClearCachedMediaLaneSignals() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  // Store the cached data before clearing it.
  std::map<int, MediaLaneSignals> mediaSignals(mediaSignalsCache_);

  // Clear the cached data after read.
  for (auto& signal : mediaSignalsCache_) {
    signal.second.txFault() = false;
  }
  return mediaSignals;
}

ModuleStatus QsfpModule::readAndClearCachedModuleStatus() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  // Store the cached data before clearing it.
  ModuleStatus moduleStatus(moduleStatusCache_);

  // Clear the cached data after read.
  moduleStatusCache_ = ModuleStatus();

  return moduleStatus;
}

/*
 * opticsModulePortHwInit
 *
 * This is a virtual function which should  will do optics module's port level
 * hardware initialization. If some optics needs the port/lane level init then
 * the inheriting class should override/implement this function.
 * If nothing special is required for optics module's port level HW bring up
 * then we can just raise the Init done event to move the port state machine
 * to Initialized state.
 */
void QsfpModule::opticsModulePortHwInit(int modulePortId) {
  // Assume nothing special needs to be done for this optic's port level
  // HW init
  portStateMachines_[modulePortId].process_event(
      MODULE_PORT_EVENT_OPTICS_INITIALIZED);
}

/*
 * addModulePortStateMachines
 *
 * This is the helper function to create port state machine for all ports in
 * this module. This function will be called when MSM enters the discovered
 * state and number of ports being present in the module  is known
 */
void QsfpModule::addModulePortStateMachines() {
  // Create Port state machine for all ports in this optics module
  for (int i = 0; i < portsPerTransceiver_; i++) {
    portStateMachines_.push_back(
        msm::back::state_machine<modulePortStateMachine>());
  }
  // In Port State Machine keeping the object pointer to the QsfpModule
  // because in the event handler callback we need to access some data from
  // this object
  for (int i = 0; i < portsPerTransceiver_; i++) {
    portStateMachines_[i].get_attribute(qsfpModuleObjPtr) = this;
  }

  // After the port state machine is created, start its port level
  // hardware init so that the PSM can move to next state
  for (int i = 0; i < portsPerTransceiver_; i++) {
    opticsModulePortHwInit(i);
  }
}

/*
 * eraseModulePortStateMachines
 *
 * This is the helper function to remove all the port state machine for the
 * module. This is called when the module is physically removed
 */
void QsfpModule::eraseModulePortStateMachines() {
  portStateMachines_.clear();
  auto diagsCapability = diagsCapability_.wlock();
  if ((*diagsCapability).has_value()) {
    (*diagsCapability).reset();
  }
}

/*
 * genMsmModPortsDownEvent
 *
 * This is the helper function to generate the Module State Machine event -
 * Module Port Down. If the Agent indicates that all ports inside this module
 * are down then this Module Port Down event is generated to Module SM
 */
void QsfpModule::genMsmModPortsDownEvent() {
  int downports = 0;
  for (int i = 0; i < portStateMachines_.size(); i++) {
    if (portStateMachines_[i].current_state()[0] == 2) {
      downports++;
    }
  }
  // Check port down for N-1 ports only because current PSM port
  // state is in transition to  Down state
  if (downports >= portStateMachines_.size() - 1) {
    stateUpdateLocked(TransceiverStateMachineEvent::ALL_PORTS_DOWN);
  }
}

void QsfpModule::genMsmModPortsUpEvent() {
  stateUpdateLocked(TransceiverStateMachineEvent::PORT_UP);
}

/*
 * scheduleAgentPortSyncupTimeout
 *
 * Once the Module SM enters Discovered state, we need to wait for agent port
 * sync up to go to next state. If the sync up does not happen for some time
 * then we need to time out. Here we will spawn a timer function to check
 * agent sync up timeout
 */
void QsfpModule::scheduleAgentPortSyncupTimeout() {
  XLOG(DBG2) << "MSM: Scheduling Agent port sync timeout function for module "
             << qsfpImpl_->getName();

  // Schedule a function to do bring up / remediate after some time
  msmFunctionScheduler_.addFunctionOnce(
      [&]() {
        // Trigger the timeout event to MSM
        legacyModuleStateMachineStateUpdate(
            TransceiverStateMachineEvent::AGENT_SYNC_TIMEOUT);
      },
      // Name of the scheduled function/thread for identifying later
      folly::to<std::string>("ModuleStateMachine-", qsfpImpl_->getName()),
      // Delay after which this function needs to be invoked in different thread
      std::chrono::milliseconds(kStateMachineAgentPortSyncupTimeout * 1000));
  // Start the function scheduler now
  msmFunctionScheduler_.start();
}

/*
 * cancelAgentPortSyncupTimeout
 *
 * In the discovered state the agent sync timeout is scheduled. On exiting
 * this state we need to cancel this timeout function
 */
void QsfpModule::cancelAgentPortSyncupTimeout() {
  XLOG(DBG2) << "MSM: Cancelling Agent port sync timeout function for module "
             << qsfpImpl_->getNum();

  // Cancel the current scheduled function
  msmFunctionScheduler_.cancelFunction(
      folly::to<std::string>("ModuleStateMachine-", qsfpImpl_->getName()));

  // Stop the scheduler thread
  // msmFunctionScheduler_.shutdown();
}

/*
 * scheduleBringupRemediateFunction
 *
 * Once the Module SM enters Inactive state, we need to spawn a periodic
 * function which will attempt to bring up the module/port by doing either
 * bring up (first time only) or the remediate function.
 */
void QsfpModule::scheduleBringupRemediateFunction() {
  XLOG(DBG2) << "MSM: Scheduling Remediate/bringup function for module "
             << qsfpImpl_->getName();

  // Schedule a function to do bring up / remediate after some time
  msmFunctionScheduler_.addFunctionOnce(
      [&]() {
        lock_guard<std::mutex> g(qsfpModuleMutex_);
        if (moduleStateMachine_.get_attribute(moduleBringupDone)) {
          // Do the remediate function second time onwards
          stateUpdateLocked(TransceiverStateMachineEvent::REMEDIATE_DONE);
        } else {
          // Bring up to be attempted for first time only
          stateUpdateLocked(TransceiverStateMachineEvent::BRINGUP_DONE);
        }
      },
      // Name of the scheduled function/thread for identifying later
      folly::to<std::string>("ModuleStateMachine-", qsfpImpl_->getName()),
      // Delay after which this function needs to be invoked in different thread
      std::chrono::milliseconds(kStateMachineOpticsRemediateInterval * 1000));
  // Start the function scheduler now
  msmFunctionScheduler_.start();
}

/*
 * exitBringupRemediateFunction
 *
 * The Module SM is exiting the Inactive state, we had spawned a periodic
 * function to do bring up/remediate, we need to cancel the function
 * scheduled and stop the scheduled thread.
 */
void QsfpModule::exitBringupRemediateFunction() {
  // Cancel the current scheduled function
  msmFunctionScheduler_.cancelFunction(
      folly::to<std::string>("ModuleStateMachine-", qsfpImpl_->getName()));

  // Stop the scheduler thread
  // msmFunctionScheduler_.shutdown();
}

/*
 * checkAgentModulePortSyncup
 *
 * This function checks if the Agent has synced up the port status information
 * If it is done then we need to generate the port Up or port Down event to
 * the port state machine (PSM). This is to take care of the case when PSM
 * has entered Initialized state and the Agent to qsfp_service sync up has
 * already happened.
 */
void QsfpModule::checkAgentModulePortSyncup() {
  uint32_t systemPortId, modulePortId;
  // Look into the synced port information and generate the event if the
  // info is already present
  for (auto& port : ports_) {
    systemPortId = port.first;
    // Get the local module port id to identify the port state machine
    modulePortId = getSystemPortToModulePortIdLocked(systemPortId);

    // If the module port status has been synced up from agent then based
    // on port up/down status, raise the port status machine event
    if (!port.second.profileID()->empty()) {
      if (*port.second.up()) {
        // Raise port up event to PSM
        portStateMachines_[modulePortId].process_event(
            MODULE_PORT_EVENT_AGENT_PORT_UP);
      } else {
        // Raise port down event to PSM
        portStateMachines_[modulePortId].process_event(
            MODULE_PORT_EVENT_AGENT_PORT_DOWN);
      }
    }
  }
}

void QsfpModule::legacyModuleStateMachineStateUpdate(
    TransceiverStateMachineEvent event) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  stateUpdateLocked(event);
}

void QsfpModule::stateUpdateLocked(TransceiverStateMachineEvent event) {
  // Use this function to gate whether we should use the old state machine or
  // the new design with the StateUpdate list
  if (FLAGS_use_new_state_machine) {
    transceiverManager_->updateStateBlocking(getID(), event);
  } else {
    // Fall back to use the legacy logic
    switch (event) {
      case TransceiverStateMachineEvent::DETECT_TRANSCEIVER:
        moduleStateMachine_.process_event(MODULE_EVENT_OPTICS_DETECTED);
        return;
      case TransceiverStateMachineEvent::REMOVE_TRANSCEIVER:
        moduleStateMachine_.process_event(MODULE_EVENT_OPTICS_REMOVED);
        return;
      case TransceiverStateMachineEvent::RESET_TRANSCEIVER:
        moduleStateMachine_.process_event(MODULE_EVENT_OPTICS_RESET);
        return;
      case TransceiverStateMachineEvent::READ_EEPROM:
        moduleStateMachine_.process_event(MODULE_EVENT_EEPROM_READ);
        return;
      case TransceiverStateMachineEvent::ALL_PORTS_DOWN:
        moduleStateMachine_.process_event(MODULE_EVENT_PSM_MODPORTS_DOWN);
        return;
      case TransceiverStateMachineEvent::PORT_UP:
        moduleStateMachine_.process_event(MODULE_EVENT_PSM_MODPORT_UP);
        return;
      case TransceiverStateMachineEvent::TRIGGER_UPGRADE:
        moduleStateMachine_.process_event(MODULE_EVENT_TRIGGER_UPGRADE);
        return;
      case TransceiverStateMachineEvent::FORCED_UPGRADE:
        moduleStateMachine_.process_event(MODULE_EVENT_FORCED_UPGRADE);
        return;
      case TransceiverStateMachineEvent::AGENT_SYNC_TIMEOUT:
        moduleStateMachine_.process_event(MODULE_EVENT_AGENT_SYNC_TIMEOUT);
        return;
      case TransceiverStateMachineEvent::BRINGUP_DONE:
        moduleStateMachine_.process_event(MODULE_EVENT_BRINGUP_DONE);
        return;
      case TransceiverStateMachineEvent::REMEDIATE_DONE:
        moduleStateMachine_.process_event(MODULE_EVENT_REMEDIATE_DONE);
        return;
      case TransceiverStateMachineEvent::PROGRAM_IPHY:
      case TransceiverStateMachineEvent::PROGRAM_XPHY:
      case TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER:
      case TransceiverStateMachineEvent::RESET_TO_DISCOVERED:
      case TransceiverStateMachineEvent::RESET_TO_NOT_PRESENT:
      case TransceiverStateMachineEvent::REMEDIATE_TRANSCEIVER:
        throw FbossError(
            "Only new state machine can support TransceiverStateMachineEvent: ",
            apache::thrift::util::enumNameSafe(event));
    }
    throw FbossError(
        "Unsupported TransceiverStateMachineEvent: ",
        apache::thrift::util::enumNameSafe(event));
  }
}

int QsfpModule::getLegacyModuleStateMachineCurrentState() const {
  return moduleStateMachine_.current_state()[0];
}

void QsfpModule::setLegacyModuleStateMachineModulePointer(
    QsfpModule* modulePtr) {
  moduleStateMachine_.get_attribute(qsfpModuleObjPtr) = modulePtr;
}

MediaInterfaceCode QsfpModule::getModuleMediaInterface() {
  std::vector<MediaInterfaceId> mediaInterfaceCodes(numMediaLanes());
  if (!getMediaInterfaceId(mediaInterfaceCodes)) {
    return MediaInterfaceCode::UNKNOWN;
  }
  if (!mediaInterfaceCodes.empty()) {
    return mediaInterfaceCodes[0].get_code();
  }
  return MediaInterfaceCode::UNKNOWN;
}

void QsfpModule::programTransceiver(
    cfg::PortSpeed speed,
    bool needResetDataPath) {
  // Always use i2cEvb to program transceivers if there's an i2cEvb
  auto programTcvrFunc = [this, speed, needResetDataPath]() {
    lock_guard<std::mutex> g(qsfpModuleMutex_);
    if (present_) {
      // Make sure customize xcvr first so that we can set the application code
      // correctly and then call configureModule() later to program serdes like
      // Rx equalizer setting based on QSFP config
      customizeTransceiverLocked(speed);
      // updateQsdpData so that we can make sure the new application code in
      // cache gets updated before calling configureModule()
      updateQsfpData(false);
      // Current configureModule() actually assumes the locked is obtained.
      // See CmisModule::configureModule(). Need to clean it up in the future.
      configureModule();

      TransceiverSettings settings = getTransceiverSettingsInfo();
      // We found that some module did not enable Rx output squelch by default,
      // which introduced some difficulty to bring link back up when flapped.
      // Here we ensure that Rx output squelch is always enabled.
      if (auto hostLaneSettings = settings.hostLaneSettings()) {
        ensureRxOutputSquelchEnabled(*hostLaneSettings);
      }

      if (needResetDataPath) {
        resetDataPath();
      }

      // Since we're touching the transceiver, we need to update the cached
      // transceiver info
      updateQsfpData(false);
      updateCachedTransceiverInfoLocked({});
    }
  };

  auto i2cEvb = qsfpImpl_->getI2cEventBase();
  if (!i2cEvb) {
    // Certain platforms cannot execute multiple I2C transactions in parallel
    // and therefore don't have an I2C evb thread
    programTcvrFunc();
  } else {
    via(i2cEvb)
        .thenValue([programTcvrFunc](auto&&) mutable { programTcvrFunc(); })
        .get();
  }
}

void QsfpModule::publishSnapshots() {
  auto snapshotsLocked = snapshots_.wlock();
  snapshotsLocked->publishAllSnapshots();
  snapshotsLocked->publishFutureSnapshots();
}

bool QsfpModule::tryRemediate() {
  // Always use i2cEvb to program transceivers if there's an i2cEvb
  auto remediateTcvrFunc = [this]() {
    lock_guard<std::mutex> g(qsfpModuleMutex_);
    return tryRemediateLocked();
  };
  auto i2cEvb = qsfpImpl_->getI2cEventBase();
  if (!i2cEvb) {
    // Certain platforms cannot execute multiple I2C transactions in parallel
    // and therefore don't have an I2C evb thread
    return remediateTcvrFunc();
  } else {
    bool didRemediate = false;
    via(i2cEvb)
        .thenValue([&didRemediate, remediateTcvrFunc](auto&&) mutable {
          didRemediate = remediateTcvrFunc();
        })
        .get();
    return didRemediate;
  }
}

bool QsfpModule::tryRemediateLocked() {
  // Only update numRemediation_ iff this transceiver should remediate and
  // remediation actually happens
  if (shouldRemediateLocked() && remediateFlakyTransceiver()) {
    ++numRemediation_;
    return true;
  }
  return false;
}

void QsfpModule::markLastDownTime() {
  lastDownTime_ = std::time(nullptr);
}

/*
 * getBerFloatValue
 *
 * A utility function to convert the 16 bit BER value from module register to
 * the double value. This function is applicable to SFF as well as CMIS
 */
double QsfpModule::getBerFloatValue(uint8_t lsb, uint8_t msb) {
  int exponent = (lsb >> 3) & 0x1f;
  int mantissa = ((lsb & 0x7) << 8) | msb;

  exponent -= 24;
  double berVal = mantissa * exp10(exponent);
  return berVal;
}
} // namespace fboss
} // namespace facebook
