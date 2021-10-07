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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/platforms/wedge/WedgePlatform.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"
#include "fboss/qsfp_service/lib/QsfpClient.h"

namespace facebook::fboss {

WedgePort::WedgePort(PortID id, WedgePlatform* platform)
    : BcmPlatformPort(id, platform) {}

void WedgePort::setBcmPort(BcmPort* port) {
  bcmPort_ = port;
}

/*
 * TODO: Not much code here yet.
 * For now, QSFP handling on wedge is currently managed by separate tool.
 * We need a little more time to sync up on Bcm APIs to get the LED
 * handling code open source.
 */

void WedgePort::preDisable(bool /*temporary*/) {}

void WedgePort::postDisable(bool /*temporary*/) {}

void WedgePort::preEnable() {}

void WedgePort::postEnable() {}

bool WedgePort::isMediaPresent() {
  return false;
}

folly::Future<TransceiverInfo> WedgePort::getFutureTransceiverInfo() const {
  auto qsfpCache = dynamic_cast<WedgePlatform*>(getPlatform())->getQsfpCache();
  return qsfpCache->futureGet(getTransceiverID().value());
}

folly::Future<TransmitterTechnology> WedgePort::getTransmitterTech(
    folly::EventBase* evb) const {
  DCHECK(evb);

  // If there's no transceiver this is a backplane port.
  // However, we know these are using copper, so pass that along
  if (!supportsTransceiver()) {
    return folly::makeFuture<TransmitterTechnology>(
        TransmitterTechnology::COPPER);
  }
  int32_t transID = static_cast<int32_t>(getTransceiverID().value());
  auto getTech = [](TransceiverInfo info) {
    if (auto cable = info.cable_ref()) {
      return *cable->transmitterTech_ref();
    }
    return TransmitterTechnology::UNKNOWN;
  };
  auto handleError = [transID](const folly::exception_wrapper& e) {
    XLOG(ERR) << "Error retrieving info for transceiver " << transID
              << " Exception: " << folly::exceptionStr(e);
    return TransmitterTechnology::UNKNOWN;
  };
  return getFutureTransceiverInfo().via(evb).thenValueInline(getTech).thenError(
      std::move(handleError));
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

void WedgePort::linkStatusChanged(bool /*up*/, bool /*adminUp*/) {}

void WedgePort::externalState(PortLedExternalState) {}

bool WedgePort::isControllingPort() const {
  if (!bcmPort_ || !bcmPort_->getPortGroup()) {
    return false;
  }
  return bcmPort_->getPortGroup()->controllingPort() == bcmPort_;
}

bool WedgePort::supportsTransceiver() const {
  return getTransceiverID().has_value();
}

std::optional<ChannelID> WedgePort::getChannel() const {
  const auto& tcvrList = getTransceiverLanes();
  if (!tcvrList.empty()) {
    // All the transceiver lanes should use the same transceiver id
    return ChannelID(*tcvrList[0].lane_ref());
  } else {
    return std::nullopt;
  }
}

std::vector<int32_t> WedgePort::getChannels() const {
  if (!port_) {
    return {};
  }
  const auto& tcvrList = getTransceiverLanes(port_->getProfileID());
  return folly::gen::from(tcvrList) |
      folly::gen::map(
             [&](const phy::PinID& entry) { return *entry.lane_ref(); }) |
      folly::gen::as<std::vector<int32_t>>();
}

TransceiverIdxThrift WedgePort::getTransceiverMapping() const {
  TransceiverIdxThrift xcvr;
  if (auto transceiverID = getTransceiverID()) {
    xcvr.transceiverId_ref() = static_cast<int32_t>(*transceiverID);
    xcvr.channels_ref() = getChannels();
  }
  return xcvr;
}

PortStatus WedgePort::toThrift(const std::shared_ptr<Port>& port) {
  // TODO: make it possible to generate a PortStatus struct solely
  // from a Port SwitchState node. Currently you need platform to get
  // transceiver mapping, which is not ideal.
  PortStatus status;
  *status.enabled_ref() = port->isEnabled();
  *status.up_ref() = port->isUp();
  *status.speedMbps_ref() = static_cast<int64_t>(port->getSpeed());
  if (supportsTransceiver()) {
    status.transceiverIdx_ref().emplace(getTransceiverMapping());
  }
  return status;
}

BcmPortGroup::LaneMode WedgePort::getLaneMode() const {
  // TODO (aeckert): it would be nicer if the BcmPortGroup wrote its
  // lane mode to a member variable of ours on changes. That way we
  // don't need to traverse these pointers so often. That also has the
  // benefit of changing LED on portgroup changes, not just on/off.
  // The one shortcoming of this is that we need to write four times
  // (once for each WedgePort). We could add a notion of PortGroups
  // to the platform as well, though that is probably a larger change
  // since the bcm code does not know if the platform supports
  // PortGroups or not.
  auto myPortGroup = bcmPort_->getPortGroup();
  if (!myPortGroup) {
    // assume single
    return BcmPortGroup::LaneMode::SINGLE;
  }
  return myPortGroup->laneMode();
}
} // namespace facebook::fboss
