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
#include <iomanip>
#include <string>
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
DEFINE_bool(use_new_state_machine, false, "Use the new state machine logic");

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
// Miniphoton module part number
static constexpr auto kMiniphotonPartNumber = "LUX1626C4AD";

TransceiverID QsfpModule::getID() const {
  return TransceiverID(qsfpImpl_->getNum());
}

// Converts power from milliwatts to decibel-milliwatts
double QsfpModule::mwToDb(double value) {
  if (value == 0) {
    return -45;
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
  flags.warn_ref()->low_ref() = (*data & (1 << offset));
  flags.warn_ref()->high_ref() = (*data & (1 << ++offset));
  flags.alarm_ref()->low_ref() = (*data & (1 << ++offset));
  flags.alarm_ref()->high_ref() = (*data & (1 << ++offset));

  return flags;
}

QsfpModule::QsfpModule(
    TransceiverManager* transceiverManager,
    std::unique_ptr<TransceiverImpl> qsfpImpl,
    unsigned int portsPerTransceiver)
    : transceiverManager_(transceiverManager),
      qsfpImpl_(std::move(qsfpImpl)),
      portsPerTransceiver_(portsPerTransceiver) {
  // Setting up the last down time as current time minus the difference
  // between remediate_interval and initial_remediate_interval so
  // that the first remediation takes place initial_remediate_interval from now
  // and the subsequent remediation takes places every remediate_interval.
  lastDownTime_ = std::time(nullptr) -
      (FLAGS_remediate_interval - FLAGS_initial_remediate_interval);

  // Keeping the QsfpModule object raw pointer inside the Module State Machine
  // as an FSM attribute. This will be used when FSM invokes the state
  // transition or event handling function and in the callback we need something
  // from QsfpModule object
  moduleStateMachine_.get_attribute(qsfpModuleObjPtr) = this;
}

QsfpModule::~QsfpModule() {
  // The transceiver has been removed
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  stateUpdate(ModuleStateMachineEvent::OPTICS_REMOVED);
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
               << " QSFP status changed to " << currentQsfpStatus;
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
    info.present_ref() = present_;
    info.transceiver_ref() = type();
    info.port_ref() = qsfpImpl_->getNum();
    *info_.wlock() = info;
  }
  return currentQsfpStatus;
}

TransceiverInfo QsfpModule::parseDataLocked() {
  TransceiverInfo info;
  auto channel_count = numMediaLanes();
  info.present_ref() = present_;
  info.transceiver_ref() = type();
  info.port_ref() = qsfpImpl_->getNum();
  if (!present_) {
    return info;
  }

  info.sensor_ref() = getSensorInfo();
  info.vendor_ref() = getVendorInfo();
  info.cable_ref() = getCableInfo();
  if (auto threshold = getThresholdInfo()) {
    info.thresholds_ref() = *threshold;
  }
  info.settings_ref() = getTransceiverSettingsInfo();
  info.mediaLaneSignals_ref() = std::vector<MediaLaneSignals>(channel_count);
  info.hostLaneSignals_ref() = std::vector<HostLaneSignals>(numHostLanes());

  for (int i = 0; i < channel_count; i++) {
    Channel chan;
    chan.channel_ref() = i;
    info.channels_ref()->push_back(chan);
  }
  if (!getSensorsPerChanInfo(*info.channels_ref())) {
    info.channels_ref()->clear();
  }

  if (!getSignalsPerMediaLane(*info.mediaLaneSignals_ref())) {
    info.mediaLaneSignals_ref()->clear();
    info.mediaLaneSignals_ref().reset();
  }
  if (auto mediaLaneSignals = info.mediaLaneSignals_ref()) {
    cacheMediaLaneSignals(*mediaLaneSignals);
  }
  if (!getSignalsPerHostLane(*info.hostLaneSignals_ref())) {
    info.hostLaneSignals_ref()->clear();
    info.hostLaneSignals_ref().reset();
  }

  if (auto transceiverStats = getTransceiverStats()) {
    info.stats_ref() = *transceiverStats;
  }
  info.signalFlag_ref() = getSignalFlagInfo();
  cacheSignalFlags(getSignalFlagInfo());
  info.extendedSpecificationComplianceCode_ref() =
      getExtendedSpecificationComplianceCode();
  info.transceiverManagementInterface_ref() = managementInterface();

  info.identifier_ref() = getIdentifier();
  info.status_ref() = getModuleStatus();

  if (auto vdmStats = getVdmDiagsStatsInfo()) {
    info.vdmDiagsStats_ref() = *vdmStats;
  }

  info.timeCollected_ref() = lastRefreshTime_;
  info.remediationCounter_ref() = numRemediation_;
  info.eepromCsumValid_ref() = verifyEepromChecksums();

  return info;
}

bool QsfpModule::safeToCustomize() const {
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
    if (*status.up_ref()) {
      return false;
    }
    anyEnabled = anyEnabled || *status.enabled_ref();
  }

  // Only return safe if at least one port is enabled
  return anyEnabled;
}

bool QsfpModule::customizationWanted(time_t cooldown) const {
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
    auto newSpeed = cfg::PortSpeed(*status.speedMbps_ref());
    if (!(*status.enabled_ref()) || speed == newSpeed) {
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
  signalFlagCache_.txLos_ref() =
      *signalflag.txLos_ref() | *signalFlagCache_.txLos_ref();
  signalFlagCache_.rxLos_ref() =
      *signalflag.rxLos_ref() | *signalFlagCache_.rxLos_ref();
  signalFlagCache_.txLol_ref() =
      *signalflag.txLol_ref() | *signalFlagCache_.txLol_ref();
  signalFlagCache_.rxLol_ref() =
      *signalflag.rxLol_ref() | *signalFlagCache_.rxLol_ref();
}

void QsfpModule::cacheMediaLaneSignals(
    const std::vector<MediaLaneSignals>& mediaSignals) {
  for (const auto& signal : mediaSignals) {
    if (mediaSignalsCache_.find(*signal.lane_ref()) ==
        mediaSignalsCache_.end()) {
      // Initialize all lanes to false if an entry in the cache doesn't exist
      // yet
      mediaSignalsCache_[*signal.lane_ref()].lane_ref() = *signal.lane_ref();
      mediaSignalsCache_[*signal.lane_ref()].txFault_ref() = false;
    }
    if (auto txFault = signal.txFault_ref()) {
      if (*txFault) {
        mediaSignalsCache_[*signal.lane_ref()].txFault_ref() = true;
      }
    }
  }
}

void QsfpModule::transceiverPortsChanged(
    const std::map<uint32_t, PortStatus>& ports) {
  auto transceiverPortsChangedHandler = [&ports, this]() {
    lock_guard<std::mutex> g(qsfpModuleMutex_);
    // List of ports inside this module whose operation status has changed
    std::vector<uint32_t> changedPortList;
    auto anyStateChanged = false;

    for (auto& it : ports) {
      CHECK(
          TransceiverID(*it.second.transceiverIdx_ref()
                             .value_or({})
                             .transceiverId_ref()) == getID());

      // Record this port in the changed port list if:
      //  - The existing port status is empty (first time sync from agent)
      //  - The new port status synced from Agent is different from existing
      //  port status
      if (ports_[it.first].profileID_ref()->empty() ||
          (*ports_[it.first].up_ref() != *it.second.up_ref())) {
        if (*ports_[it.first].up_ref() != *it.second.up_ref()) {
          anyStateChanged = true;
        }
        changedPortList.push_back(it.first);
      }

      ports_[it.first] = std::move(it.second);
    }

    if (anyStateChanged) {
      snapshots_.wlock()->publishAllSnapshots();
      snapshots_.wlock()->publishFutureSnapshots(kNumCachedSnapshots);
    }

    // update the present_ field (and will set dirty_ if presence change
    // detected)
    detectPresenceLocked();

    if (safeToCustomize()) {
      needsCustomization_ = true;
      // safetocustomize helped confirmed that no port was up for this
      // transceiver. Record the time for future references.
      lastDownTime_ = std::time(nullptr) -
          (FLAGS_remediate_interval - FLAGS_initial_remediate_interval);
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

      if (*ports_[it].up_ref()) {
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
  detectPresenceLocked();

  bool newTransceiverDetected = false;
  auto customizeWanted = customizationWanted(FLAGS_customize_interval);
  auto willRefresh = !dirty_ && shouldRefresh(FLAGS_qsfp_data_refresh_interval);
  if (!dirty_ && !customizeWanted && !willRefresh) {
    return;
  }

  if (dirty_ && present_) {
    // A new transceiver has been detected
    stateUpdate(ModuleStateMachineEvent::OPTICS_DETECTED);
    newTransceiverDetected = true;
  } else if (dirty_ && !present_) {
    // The transceiver has been removed
    stateUpdate(ModuleStateMachineEvent::OPTICS_REMOVED);
  }

  if (dirty_) {
    // make sure data is up to date before trying to customize.
    ensureOutOfReset();
    updateQsfpData(true);
  }

  if (newTransceiverDetected) {
    // Data has been read for the new optics
    stateUpdate(ModuleStateMachineEvent::EEPROM_READ);
  }

  if (customizeWanted) {
    customizeTransceiverLocked(getPortSpeed());

    if (shouldRemediate(FLAGS_remediate_interval)) {
      remediateFlakyTransceiver();
      ++numRemediation_;
    }
  }

  TransceiverSettings settings = getTransceiverSettingsInfo();

  // We found that some module did not enable Rx output squelch by default,
  // which introduced some difficulty to bring link back up when flapped.
  // Here we ensure that Rx output squelch is always enabled.
  if (auto hostLaneSettings = settings.hostLaneSettings_ref()) {
    ensureRxOutputSquelchEnabled(*hostLaneSettings);
  }

  if (customizeWanted || willRefresh) {
    // update either if data is stale or if we customized this
    // round. We update in the customization because we may have
    // written fields, but only need a partial update because all of
    // these fields are in the LOWER qsfp page. There are a small
    // number of writable fields on other qsfp pages, but we don't
    // currently use them.
    updateQsfpData(false);
  }

  // assign
  auto info = parseDataLocked();
  phy::LinkSnapshot snapshot;
  snapshot.transceiverInfo_ref() = info;
  snapshots_.wlock()->addSnapshot(snapshot);
  *info_.wlock() = info;
}

bool QsfpModule::shouldRemediate(time_t cooldown) {
  auto now = std::time(nullptr);
  // Since Miniphton module is always showing as present and four ports
  // sharing a single optical module. Doing remediation on one port will
  // have side effect on the neighbor port as well. So we don't do
  // remediation as suggested by our HW optic team.
  if (getTransceiverInfo().vendor_ref().has_value() &&
      *getTransceiverInfo().vendor_ref()->partNumber_ref() ==
          kMiniphotonPartNumber) {
    return false;
  }
  bool remediationEnabled =
      now > transceiverManager_->getPauseRemediationUntil();
  // Rather than immediately attempting to remediate a module,
  // we would like to introduce a bit delay to de-couple the consequences
  // of a remediation with the root cause that brought down the link.
  // This is an effort to help with debugging.
  bool remediationCooled =
      now - std::max(lastDownTime_, lastRemediateTime_) > cooldown;
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
    setPowerOverrideIfSupported(*settings.powerControl_ref());

    if (speed != cfg::PortSpeed::DEFAULT) {
      setCdrIfSupported(speed, *settings.cdrTx_ref(), *settings.cdrRx_ref());
      setRateSelectIfSupported(
          speed, *settings.rateSelect_ref(), *settings.rateSelectSetting_ref());
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
  auto length = param.length_ref().has_value() ? *(param.length_ref()) : 1;
  auto iobuf = folly::IOBuf::createCombined(length);
  if (!present_) {
    return iobuf;
  }
  try {
    auto offset = *(param.offset_ref());
    if (param.page_ref().has_value()) {
      uint8_t page = *(param.page_ref());
      // When the page is specified, first update byte 127 with the speciied
      // pageId
      qsfpImpl_->writeTransceiver(
          TransceiverI2CApi::ADDR_QSFP, 127, sizeof(page), &page);
    }
    qsfpImpl_->readTransceiver(
        TransceiverI2CApi::ADDR_QSFP, offset, length, iobuf->writableData());
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
    auto offset = *(param.offset_ref());
    if (param.page_ref().has_value()) {
      uint8_t page = *(param.page_ref());
      // When the page is specified, first update byte 127 with the speciied
      // pageId
      qsfpImpl_->writeTransceiver(
          TransceiverI2CApi::ADDR_QSFP, 127, sizeof(page), &page);
    }
    qsfpImpl_->writeTransceiver(
        TransceiverI2CApi::ADDR_QSFP, offset, sizeof(data), &data);
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
  signalFlag.txLos_ref() = *signalFlagCache_.txLos_ref();
  signalFlag.rxLos_ref() = *signalFlagCache_.rxLos_ref();
  signalFlag.txLol_ref() = *signalFlagCache_.txLol_ref();
  signalFlag.rxLol_ref() = *signalFlagCache_.rxLol_ref();

  // Clear the cached data after read.
  signalFlagCache_.txLos_ref() = 0;
  signalFlagCache_.rxLos_ref() = 0;
  signalFlagCache_.txLol_ref() = 0;
  signalFlagCache_.rxLol_ref() = 0;
  return signalFlag;
}

std::map<int, MediaLaneSignals>
QsfpModule::readAndClearCachedMediaLaneSignals() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  // Store the cached data before clearing it.
  std::map<int, MediaLaneSignals> mediaSignals(mediaSignalsCache_);

  // Clear the cached data after read.
  for (auto& signal : mediaSignalsCache_) {
    signal.second.txFault_ref() = false;
  }
  return mediaSignals;
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
  if (diagsCapability_.has_value()) {
    diagsCapability_.reset();
  }
}

/*
 * genMsmModPortUpEvent
 *
 * This is the helper function to generate the Module State Machine event -
 * Module Port Up. Any port up indication from Agent invokes this function.
 * If the module supports n ports then port up indication for 1 to n ports
 * will cause this event
 */
void QsfpModule::genMsmModPortUpEvent() {
  moduleStateMachine_.process_event(MODULE_EVENT_PSM_MODPORT_UP);
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
    moduleStateMachine_.process_event(MODULE_EVENT_PSM_MODPORTS_DOWN);
  }
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
        lock_guard<std::mutex> g(qsfpModuleMutex_);
        // Trigger the timeout event to MSM
        moduleStateMachine_.process_event(MODULE_EVENT_AGENT_SYNC_TIMEOUT);
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
          moduleStateMachine_.process_event(MODULE_EVENT_REMEDIATE_DONE);
        } else {
          // Bring up to be attempted for first time only
          moduleStateMachine_.process_event(MODULE_EVENT_BRINGUP_DONE);
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
    if (!port.second.profileID_ref()->empty()) {
      if (*port.second.up_ref()) {
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

void QsfpModule::stateUpdate(ModuleStateMachineEvent event) {
  // Use this function to gate whether we should use the old state machine or
  // the new design with the StateUpdate list
  if (FLAGS_use_new_state_machine) {
    // TODO(joseph5wu) This will create a StateUpdate object in the pending list
    // of TransceiverManager and then have a separate thread to handle all the
    // pending updates, which is similar to wedge_agent SwitchState update.
    throw FbossError("Unsupported implementation of new state machine");
  } else {
    // Fall back to use the legacy logic
    switch (event) {
      case ModuleStateMachineEvent::OPTICS_DETECTED:
        moduleStateMachine_.process_event(MODULE_EVENT_OPTICS_DETECTED);
        return;
      case ModuleStateMachineEvent::OPTICS_REMOVED:
        moduleStateMachine_.process_event(MODULE_EVENT_OPTICS_REMOVED);
        return;
      case ModuleStateMachineEvent::OPTICS_RESET:
        moduleStateMachine_.process_event(MODULE_EVENT_OPTICS_RESET);
        return;
      case ModuleStateMachineEvent::EEPROM_READ:
        moduleStateMachine_.process_event(MODULE_EVENT_EEPROM_READ);
        return;
    }
    throw FbossError(
        "Unsupported ModuleStateMachineEvent: ",
        getModuleStateMachineEventName(event));
  }
}

} // namespace fboss
} // namespace facebook
