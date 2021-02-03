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
#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/qsfp_service/StatsPublisher.h"
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
    300,
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

TransceiverID QsfpModule::getID() const {
  return TransceiverID(qsfpImpl_->getNum());
}

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
  CHECK_GT(portsPerTransceiver_, 0);

  // set last up time to be current time since we don't know if the
  // port was up before we just restarted.
  // Setting up the last working time as current time minus 3 mins so
  // that the first remediation takes pace 2 minutes from now and the
  // subsequent remediation takes places every 5 minutes if needed.
  lastWorkingTime_ = std::time(nullptr) -
      (FLAGS_remediate_interval - FLAGS_initial_remediate_interval);

  // Keeping the QsfpModule object raw pointer inside the Module State Machine
  // as an FSM attribute. This will be used when FSM invokes the state
  // transition or event handling function and in the callback we need something
  // from QsfpModule object
  opticsModuleStateMachine_.get_attribute(qsfpModuleObjPtr) = this;
}

QsfpModule::~QsfpModule() {}

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
    XLOG(INFO) << "Port: " << folly::to<std::string>(qsfpImpl_->getName())
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

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    Channel chan;
    chan.channel_ref() = i;
    info.channels_ref()->push_back(chan);
  }
  if (!getSensorsPerChanInfo(*info.channels_ref())) {
    info.channels_ref()->clear();
  }
  if (auto transceiverStats = getTransceiverStats()) {
    info.stats_ref() = *transceiverStats;
  }
  info.signalFlag_ref() = getSignalFlagInfo();
  info.extendedSpecificationComplianceCode_ref() =
      getExtendedSpecificationComplianceCode();
  info.transceiverManagementInterface_ref() = managementInterface();

  return info;
}

bool QsfpModule::safeToCustomize() const {
  if (ports_.size() < portsPerTransceiver_) {
    XLOG(INFO) << "Not all ports present in transceiver " << getID()
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

void QsfpModule::transceiverPortsChanged(
    const std::map<uint32_t, PortStatus>& ports) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);

  for (auto& it : ports) {
    CHECK(
        TransceiverID(
            *it.second.transceiverIdx_ref().value_or({}).transceiverId_ref()) ==
        getID());
    ports_[it.first] = std::move(it.second);
  }

  // update the present_ field (and will set dirty_ if presence change detected)
  detectPresenceLocked();

  if (safeToCustomize()) {
    needsCustomization_ = true;
  } else {
    // Since we don't have positive confirmation all ports are down,
    // update the lastWorkingTime_ to now.
    lastWorkingTime_ = std::time(nullptr);
  }

  if (dirty_) {
    // data is stale. This could happen immediately after plugging a
    // port in. Refresh inline in this case in order to not return
    // stale data.
    refreshLocked();
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

  auto customizeWanted = customizationWanted(FLAGS_customize_interval);
  auto willRefresh = !dirty_ && shouldRefresh(FLAGS_qsfp_data_refresh_interval);
  if (!dirty_ && !customizeWanted && !willRefresh) {
    return;
  }

  if (dirty_) {
    // make sure data is up to date before trying to customize.
    ensureOutOfReset();
    updateQsfpData(true);
  }

  if (customizeWanted) {
    customizeTransceiverLocked(getPortSpeed());

    if (shouldRemediate(FLAGS_remediate_interval)) {
      remediateFlakyTransceiver();
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
  }

  // assign
  *info_.wlock() = parseDataLocked();
}

bool QsfpModule::shouldRemediate(time_t cooldown) const {
  auto now = std::time(nullptr);
  bool remediationEnabled =
      now > transceiverManager_->getPauseRemediationUntil();
  bool remediationCooled =
      now - std::max(lastWorkingTime_, lastRemediateTime_) > cooldown;
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
    opticsModulePortStateMachine_.push_back(
        msm::back::state_machine<modulePortStateMachine>());
  }
  // In Port State Machine keeping the object pointer to the QsfpModule
  // because in the event handler callback we need to access some data from
  // this object
  for (int i = 0; i < portsPerTransceiver_; i++) {
    opticsModulePortStateMachine_[i].get_attribute(qsfpModuleObjPtr) = this;
  }
}

/*
 * eraseModulePortStateMachines
 *
 * This is the helper function to remove all the port state machine for the
 * module. This is called when the module is physically removed
 */
void QsfpModule::eraseModulePortStateMachines() {
  opticsModulePortStateMachine_.clear();
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
  opticsModuleStateMachine_.process_event(MODULE_EVENT_PSM_MODPORT_UP);
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
  for (int i = 0; i < opticsModulePortStateMachine_.size(); i++) {
    if (opticsModulePortStateMachine_[i].current_state()[0] == 2) {
      downports++;
    }
  }
  // Check port down for N-1 ports only because current PSM port
  // state is in transition to  Down state
  if (downports >= opticsModulePortStateMachine_.size() - 1) {
    opticsModuleStateMachine_.process_event(MODULE_EVENT_PSM_MODPORTS_DOWN);
  }
}

} // namespace fboss
} // namespace facebook
