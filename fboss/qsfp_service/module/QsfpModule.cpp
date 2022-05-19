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
  // As QsfpModule needs to use state machine while TransceiverManager is
  // the main class to maintain state machine update, we need to make sure
  // transceiverManager_ can't be nullptr
  CHECK(transceiverManager_ != nullptr);

  markLastDownTime();
}

QsfpModule::~QsfpModule() {
  // The transceiver has been removed
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  stateUpdateLocked(TransceiverStateMachineEvent::REMOVE_TRANSCEIVER);
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

bool QsfpModule::setPortPrbs(
    phy::Side side,
    const prbs::InterfacePrbsState& prbs) {
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

  auto clearLaneStats = [this](std::vector<phy::PrbsLaneStats>& laneStats) {
    for (auto& laneStat : laneStats) {
      laneStat.ber() = 0;
      laneStat.maxBer() = 0;
      laneStat.numLossOfLock() = 0;
      laneStat.timeSinceLastClear() = std::time(nullptr);

      XLOG(INFO) << "Transceiver=" << qsfpImpl_->getName() << " Lane "
                 << *laneStat.laneId() << " ber and maxBer cleared";
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

  auto updatePrbsStatEntry =
      [this](const phy::PrbsStats& oldStat, phy::PrbsStats& newStat) {
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
            XLOG(DBG5) << "Transceiver=" << qsfpImpl_->getName() << " Lane "
                       << *newLane.laneId()
                       << " Lock=" << (*newLane.locked() ? "Y" : "N")
                       << " ber=" << *newLane.ber()
                       << " maxBer=" << *newLane.maxBer();

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

void QsfpModule::stateUpdateLocked(TransceiverStateMachineEvent event) {
  // Use this function to gate whether we should use the old state machine or
  // the new design with the StateUpdate list
  // If transceiverManager_ is null, we don't need to update the state machine
  // This should only be the case for some unit tests
  if (FLAGS_use_new_state_machine && transceiverManager_) {
    transceiverManager_->updateStateBlocking(getID(), event);
  }
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
