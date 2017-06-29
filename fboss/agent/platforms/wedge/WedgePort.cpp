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

void WedgePort::preDisable(bool temporary) {
}

void WedgePort::postDisable(bool temporary) {
}

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
  auto transceiverId = static_cast<int32_t>(*getTransceiverID());
  auto getTransceiverInfo =
      [transceiverId](std::unique_ptr<QsfpServiceAsyncClient> client) {
        auto options = QsfpClient::getRpcOptions();
        // std::map<int32_t, TransceiverInfo> info_map;
        return client->future_getTransceiverInfo(options, {transceiverId});
      };
  auto fromMap =
      [transceiverId](std::map<int, facebook::fboss::TransceiverInfo> infoMap) {
        return infoMap[transceiverId];
      };
  return clientFuture.via(evb).then(getTransceiverInfo).then(fromMap);
}

folly::Future<TransmitterTechnology> WedgePort::getTransmitterTech(
    folly::EventBase* evb) const {
  if (!evb) {
    evb = platform_->getEventBase();
  }
  auto trans = getTransceiverID();
  int32_t transID = static_cast<int32_t>(*trans);

  // If null means that there's no transceiver because this is likely
  // a backplane port. However, we know these are using copper, so
  // pass that along
  if (!trans) {
    return folly::makeFuture<TransmitterTechnology>(
        TransmitterTechnology::COPPER);
  }
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
  return getTransceiverInfo(evb).then(getTech).onError(std::move(handleError));
}

void WedgePort::statusIndication(bool enabled, bool link,
                                 bool ingress, bool egress,
                                 bool discards, bool errors) {
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

bool WedgePort::shouldCustomizeTransceiver() const {
  auto trans = getTransceiverID();
  if (!trans) {
    // No qsfp atatched to customize
    VLOG(4) << "Not customising qsfps of port " << id_
            << " as it has no transceiver.";
    return false;
  } else if (!isControllingPort()) {
    auto channel = getChannel();
    auto chan = channel ? folly::to<std::string>(*channel) : "Unknown";

    // We only want to customise on the first channel - this is the actual
    // speed the transceiver should be configured for
    // Other channels may be disabled with other speeds set
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
  auto eventBase = platform_->getEventBase();
  if (!eventBase) {
    LOG(ERROR) << "No valid eventbase to use with async customizeTransceivers"
               << " call. Skipping call.";
    return;
  }
  auto speedString = cfg::_PortSpeed_VALUES_TO_NAMES.find(speed_)->second;
  auto& speed = speed_;
  auto clientFuture = QsfpClient::createClient(eventBase);
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
  clientFuture.via(eventBase).then(doCustomize).onError(std::move(handleError));
}

}} // facebook::fboss
