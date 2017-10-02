/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/wedge/WedgePort.h"

#include <folly/futures/Future.h>
#include <folly/gen/Base.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>

#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/platforms/wedge/WedgePlatform.h"
#include "fboss/agent/QsfpClient.h"

namespace facebook { namespace fboss {

WedgePort::WedgePort(
  PortID id,
  WedgePlatform* platform,
  folly::Optional<TransceiverID> frontPanelPort,
  folly::Optional<ChannelID> channel,
  const XPEs& egressXPEs) :
    BcmPlatformPort(egressXPEs),
    id_(id),
    platform_(platform),
    frontPanelPort_(frontPanelPort),
    channel_(channel) {
}

void WedgePort::setBcmPort(BcmPort* port) {
  bcmPort_ = port;
}

/*
 * TODO: Not much code here yet.
 * For now, QSFP handling on wedge is currently managed by separate tool.
 * We need a little more time to sync up on OpenNSL APIs to get the LED
 * handling code open source.
 */

void WedgePort::preDisable(bool /*temporary*/) {}

void WedgePort::postDisable(bool /*temporary*/) {}

void WedgePort::preEnable() {
}

void WedgePort::postEnable() {
}

bool WedgePort::isMediaPresent() {
  return false;
}

folly::Future<TransceiverInfo> WedgePort::getTransceiverInfo(
    folly::EventBase* evb) const {
  if (!evb) {
    evb = platform_->getEventBase();
  }
  auto clientFuture = QsfpClient::createClient(evb);
  folly::Optional<TransceiverID> transceiverId = getTransceiverID();
  auto getTransceiverInfo =
      [transceiverId](std::unique_ptr<QsfpServiceAsyncClient> client) {
        // This will throw if there is no transceiver on this port
        auto t = static_cast<int32_t>(transceiverId.value());
        auto options = QsfpClient::getRpcOptions();
        return client->future_getTransceiverInfo(options, {t});
      };
  auto fromMap =
      [transceiverId](std::map<int, facebook::fboss::TransceiverInfo> infoMap) {
        // This will throw if there is no transceiver on this port
        auto t = static_cast<int32_t>(transceiverId.value());
        return infoMap[t];
      };
  return clientFuture.then(evb, getTransceiverInfo).then(evb, fromMap);
}

folly::Future<TransmitterTechnology> WedgePort::getTransmitterTech(
    folly::EventBase* evb) const {
  if (!evb) {
    evb = platform_->getEventBase();
  }
  // If there's no transceiver this is a backplane port.
  // However, we know these are using copper, so pass that along
  if (!supportsTransceiver()) {
    return folly::makeFuture<TransmitterTechnology>(
        TransmitterTechnology::COPPER);
  }
  int32_t transID = static_cast<int32_t>(getTransceiverID().value());
  auto getTech = [](TransceiverInfo info) {
    if (info.__isset.cable && info.cable.__isset.transmitterTech) {
      return info.cable.transmitterTech;
    }
    return TransmitterTechnology::UNKNOWN;
  };
  auto handleError = [transID](const std::exception& e) {
    LOG(ERROR) << "Error retrieving info for transceiver " << transID
               << " Exception: " << folly::exceptionStr(e);
    return TransmitterTechnology::UNKNOWN;
  };
  return getTransceiverInfo(evb).then(evb, getTech).onError(
      std::move(handleError));
}

// Get correct transmitter setting.
folly::Future<folly::Optional<TxSettings>> WedgePort::getTxSettings(
    folly::EventBase* evb) const {
  auto txOverrides = getTxOverrides();
  if (txOverrides.empty()) {
    return folly::makeFuture<folly::Optional<TxSettings>>(folly::none);
  }

  if (!evb) {
    evb = platform_->getEventBase();
  }

  auto getTx = [overrides = std::move(txOverrides)](TransceiverInfo info)
      -> folly::Optional<TxSettings> {
    if (info.__isset.cable && info.cable.__isset.transmitterTech) {
      if (!info.cable.__isset.length) {
        return folly::Optional<TxSettings>();
      }
      auto cableMeters = std::max(1.0, std::min(3.0, info.cable.length));
      const auto it = overrides.find(
        std::make_pair(info.cable.transmitterTech, cableMeters));
      if (it != overrides.cend()) {
        return it->second;
      }
    }
    // not enough cable info. return the default value
    return folly::Optional<TxSettings>();
  };
  auto transID = getTransceiverID();
  auto handleErr = [transID](const std::exception& e) {
    LOG(ERROR) << "Error retrieving cable info for transceiver " << *transID
               << " Exception: " << folly::exceptionStr(e);
    return folly::Optional<TxSettings>();
  };
  return getTransceiverInfo(evb).then(evb, getTx).onError(std::move(handleErr));
}

void WedgePort::statusIndication(
    bool enabled,
    bool link,
    bool /*ingress*/,
    bool /*egress*/,
    bool /*discards*/,
    bool /*errors*/) {
  linkStatusChanged(link, enabled);
}

void WedgePort::linkStatusChanged(bool up, bool adminUp) {
  // If the link should be up but is not, let's make sure the qsfp
  // settings are correct
  if (!up && adminUp) {
    customizeTransceiver();
  }
}

void WedgePort::linkSpeedChanged(const cfg::PortSpeed& speed) {
  // Cache the current set speed
  speed_ = speed;
}

bool WedgePort::isControllingPort() const {
  if (!bcmPort_ || !bcmPort_->getPortGroup()) {
    return false;
  }
  return bcmPort_->getPortGroup()->controllingPort() == bcmPort_;
}

bool WedgePort::isInSingleMode() const {
  if (!bcmPort_ || !bcmPort_->getPortGroup()) {
    return false;
  }
  return bcmPort_->getPortGroup()->laneMode() == BcmPortGroup::LaneMode::SINGLE;
}

std::vector<int32_t> WedgePort::getChannels() const {
  // TODO(aeckert): this is pretty hacky... we should really model
  // port groups in switch state somehow so this can be served purely
  // from switch state.
  if (!getChannel().hasValue()) {
    return {};
  }

  auto base = static_cast<int32_t>(*getChannel());

  uint8_t numChannels = 1;
  if (bcmPort_ && bcmPort_->getPortGroup()) {
    auto pg = bcmPort_->getPortGroup();
    if (pg->laneMode() == BcmPortGroup::LaneMode::SINGLE) {
      if (base != 0) {
        return {};
      }
      numChannels = 4;
    } else if (pg->laneMode() == BcmPortGroup::LaneMode::DUAL) {
      if (base != 0 && base != 2) {
        return {};
      }
      numChannels = 2;
    }
  }

  return folly::gen::range(base, base + numChannels)
    | folly::gen::as<std::vector>();
}

bool WedgePort::shouldCustomizeTransceiver() const {
  auto trans = getTransceiverID();
  if (!trans) {
    // No qsfp atatched to customize
    VLOG(4) << "Not customising qsfps of port " << id_
            << " as it has no transceiver.";
    return false;
  } else if (!isControllingPort()) {
    // We only want to customise on the first channel - this is the actual
    // speed the transceiver should be configured for
    // Other channels may be disabled with other speeds set
    return false;
  } else if (!isInSingleMode()) {
    // If we are not in single mode (all 4 S gbps lanes treated as one
    // 4xS port) then customizing risks restarting other non-broken lanes
    return false;
  } else if (speed_ == cfg::PortSpeed::DEFAULT) {
    // This should be resolved in BcmPort before calling
    LOG(ERROR) << "Unresolved speed: Unable to determine what qsfp settings "
               << "are needed for transceiver"
               << folly::to<std::string>(*trans);
    return false;
  }

  return true;
}

void WedgePort::customizeTransceiver() {
  if (!shouldCustomizeTransceiver()) {
    return;
  }
  // We've already checked whether there is a transceiver id in needsCustomize
  auto transID = static_cast<int32_t>(*getTransceiverID());
  auto evb = platform_->getEventBase();
  if (!evb) {
    LOG(ERROR) << "No valid eventbase to use with async customizeTransceivers"
               << " call. Skipping call.";
    return;
  }
  auto speedString = cfg::_PortSpeed_VALUES_TO_NAMES.find(speed_)->second;
  auto& speed = speed_;
  auto clientFuture = QsfpClient::createClient(evb);
  auto doCustomize = [transID, speed, speedString](
                         std::unique_ptr<QsfpServiceAsyncClient> client) {
    LOG(INFO) << "Sending qsfp customize request for transceiver " << transID
              << " to speed " << speedString;
    auto options = QsfpClient::getRpcOptions();
    return client->future_customizeTransceiver(options, transID, speed);
  };
  auto handleError = [transID, speedString](const std::exception& e) {
    // This can happen for a variety of reasons ranging from
    // thrift problems to invalid input sent to the server
    // Let's just catch them all
    LOG(ERROR) << "Unable to customize transceiver " << transID << " for speed "
               << speedString << ". Exception: " << e.what();
  };
  clientFuture.then(evb, doCustomize).onError(std::move(handleError));
}

}} // facebook::fboss
