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

TransmitterTechnology WedgePort::getTransmitterTech() const {
  auto trans = getTransceiverID();

  // If null means that there's no transceiver because this is likely
  // a backplane port. However, we know these are using copper, so
  // pass that along
  if (!trans) {
    return TransmitterTechnology::COPPER;
  }

  int32_t transID = static_cast<int32_t>(*trans);
  try {
    std::map<int32_t, TransceiverInfo> info_map;

    folly::EventBase eb;
    auto client = QsfpClient::createClient(&eb);
    auto options = QsfpClient::getRpcOptions();
    client->sync_getTransceiverInfo(options, info_map, {transID});

    // If it doesn't exist in the map, this will insert a new element
    // (and retrieve the default value - an empty TransceiverInfo -
    // which suits us fine)
    TransceiverInfo info = info_map[transID];

    if (info.__isset.cable && info.cable.__isset.transmitterTech) {
      return info.cable.transmitterTech;
    }
  } catch (const std::exception& e) {
    // This can happen for a variety of reasons ranging from
    // thrift problems to invalid input sent to the server
    // Let's just catch them all
    LOG(ERROR) << "Error retrieving info for transceiver " << transID
               << " Exception: " << folly::exceptionStr(e);
  }
  // Default to this if we have trouble retrieving the value for some reason
  return TransmitterTechnology::UNKNOWN;
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
  auto runCustomize = [transID, speed, speedString, eventBase]() {
    LOG(INFO) << "Sending qsfp customize request for transceiver "
              << transID << " to speed " << speedString;
    auto client = QsfpClient::createClient(eventBase);
    auto options = QsfpClient::getRpcOptions();
    client->future_customizeTransceiver(options, transID, speed)
      .onError([transID, speedString](const std::exception& e) {
        // This can happen for a variety of reasons ranging from
        // thrift problems to invalid input sent to the server
        // Let's just catch them all
        LOG(ERROR) << "Unable to customize transceiver " << transID
                   << " for speed " << speedString
                   << ". Exception: " << e.what();
        });
  };
  eventBase->runInEventBaseThread(runCustomize);
}

}} // facebook::fboss
