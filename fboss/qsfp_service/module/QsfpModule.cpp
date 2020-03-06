/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "QsfpModule.h"

#include <boost/assign.hpp>
#include <string>
#include <iomanip>
#include "fboss/agent/FbossError.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"
#include "fboss/qsfp_service/StatsPublisher.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

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

using std::memcpy;
using std::mutex;
using std::lock_guard;
using folly::IOBuf;

namespace facebook { namespace fboss {

TransceiverID QsfpModule::getID() const {
  return TransceiverID(qsfpImpl_->getNum());
}

/*
 * Given a byte, extract bit fields for various alarm flags;
 * note the we might want to use the lower or the upper nibble,
 * so offset is the number of the bit to start at;  this is usually
 * 0 or 4.
 */

FlagLevels QsfpModule::getQsfpFlags(const uint8_t *data,
                                    int offset) {
  FlagLevels flags;

  CHECK_GE(offset, 0);
  CHECK_LE(offset, 4);
  flags.warn.low = (*data & (1 << offset));
  flags.warn.high = (*data & (1 << ++offset));
  flags.alarm.low = (*data & (1 << ++offset));
  flags.alarm.high = (*data & (1 << ++offset));

  return flags;
}

QsfpModule::QsfpModule(
    std::unique_ptr<TransceiverImpl> qsfpImpl,
    unsigned int portsPerTransceiver)
    : qsfpImpl_(std::move(qsfpImpl)),
      portsPerTransceiver_(portsPerTransceiver) {
  CHECK_GT(portsPerTransceiver_, 0);

  // set last up time to be current time since we don't know if the
  // port was up before we just restarted.
  lastWorkingTime_ = std::time(nullptr);
}

QsfpModule::~QsfpModule() {}

void QsfpModule::getQsfpValue(int dataAddress, int offset, int length,
                              uint8_t* data) const {
  const uint8_t *ptr = getQsfpValuePtr(dataAddress, offset, length);

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

    // If a transceiver went from present to missing, clear the cached data.
    if (!present_) {
      info_.wlock()->reset();
    }
  }
  return currentQsfpStatus;
}

TransceiverInfo QsfpModule::parseDataLocked() {
  TransceiverInfo info;
  info.present = present_;
  info.transceiver = type();
  info.port = qsfpImpl_->getNum();
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
    chan.channel = i;
    info.channels.push_back(chan);
  }
  if (!getSensorsPerChanInfo(info.channels)) {
    info.channels.clear();
  }
  if (auto transceiverStats = getTransceiverStats()) {
    info.stats_ref() = *transceiverStats;
  }

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
      ports_.size(), " ports found in transceiver ", getID(),
      " (max=", portsPerTransceiver_, ")");
  }

  bool anyEnabled{false};
  for (const auto& port : ports_) {
    const auto& status = port.second;
    if (status.up) {
      return false;
    }
    anyEnabled = anyEnabled || status.enabled;
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
    auto newSpeed = cfg::PortSpeed(status.speedMbps);
    if (!status.enabled || speed == newSpeed) {
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
    const std::vector<std::pair<const int, PortStatus>>& ports) {
  lock_guard<std::mutex> g(qsfpModuleMutex_);

  for (auto& it : ports) {
    CHECK(
        TransceiverID(
            it.second.transceiverIdx_ref().value_or({}).transceiverId) ==
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
  return now - std::max(lastWorkingTime_, lastRemediateTime_) > cooldown;
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
    setPowerOverrideIfSupported(settings.powerControl);

    if (speed != cfg::PortSpeed::DEFAULT) {
      setCdrIfSupported(speed, settings.cdrTx, settings.cdrRx);
      setRateSelectIfSupported(
        speed, settings.rateSelect, settings.rateSelectSetting);
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

}} //namespace facebook::fboss
