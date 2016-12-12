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
#include <folly/io/async/EventBase.h>

#include "fboss/agent/QsfpClient.h"

namespace facebook { namespace fboss {

WedgePort::WedgePort(PortID id, folly::Optional<TransceiverID> frontPanelPort,
    folly::Optional<ChannelID> channel)
  : id_(id),
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

void WedgePort::linkSpeedChanged(const cfg::PortSpeed& speed) {
  auto trans = getTransceiverID();
  if (!trans) {
    // No qsfp atatched to customize
    return;
  }

  auto transID = static_cast<int32_t>(*trans);
  // This should be resolved in BcmPort before calling
  if (speed == cfg::PortSpeed::DEFAULT) {
    LOG(ERROR) << "Unresolved speed: Unable to determine what qsfp settings "
               << "are needed for transceiver" << transID;
    return;
  }
  try {
    // Do we want to only make the call if this port is the PortGroup's
    // controlling port? Are we guaranteed that if one channel needs the change
    // all channels will have the change triggered?
    folly::EventBase eb;
    auto client = QsfpClient::createClient(&eb);
    auto options = QsfpClient::getRpcOptions();
    client->sync_customizeTransceiver(options, transID, speed);
  } catch (const std::exception& e) {
    // This can happen for a variety of reasons ranging from
    // thrift problems to invalid input sent to the server
    // Let's just catch them all
    LOG(ERROR) << "Unable to customize transceiver " << transID <<
      " for speed " << cfg::_PortSpeed_VALUES_TO_NAMES.find(speed)->second <<
      ". Exception: " << e.what();
  }
}
}} // facebook::fboss
