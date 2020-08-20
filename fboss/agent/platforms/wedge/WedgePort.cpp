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
#include <folly/logging/xlog.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/platforms/wedge/WedgePlatform.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"
#include "fboss/qsfp_service/lib/QsfpClient.h"

namespace facebook::fboss {

WedgePort::WedgePort(PortID id, WedgePlatform* platform)
    : WedgePort(id, platform, std::nullopt) {}

WedgePort::WedgePort(
    PortID id,
    WedgePlatform* platform,
    std::optional<FrontPanelResources> frontPanel)
    : BcmPlatformPort(id, platform), frontPanel_(frontPanel) {
  if (auto tcvrListOpt = getTransceiverLanes()) {
    const auto& tcvrList = *tcvrListOpt;
    // If the platform port comes with transceiver lanes
    if (!tcvrList.empty()) {
      // All the transceiver lanes should use the same transceiver id
      auto chipCfg =
          getPlatform()->getDataPlanePhyChip(*tcvrList[0].chip_ref());
      if (!chipCfg) {
        throw FbossError(
            "Port ",
            getPortID(),
            " is using platform unsupported chip ",
            *tcvrList[0].chip_ref());
      }
      transceiverID_.emplace(TransceiverID(*chipCfg->physicalID_ref()));
    }
  }
}

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

folly::Future<TransceiverInfo> WedgePort::getTransceiverInfo() const {
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
  return getTransceiverInfo().via(evb).thenValueInline(getTech).thenError(
      std::move(handleError));
}

folly::Future<std::optional<Cable>> WedgePort::getCableInfo(
    folly::EventBase* evb) const {
  auto getCable = [](TransceiverInfo info) -> std::optional<Cable> {
    return info.cable_ref().to_optional();
  };
  auto transID = getTransceiverID();
  auto handleErr = [transID](const std::exception& e) -> std::optional<Cable> {
    XLOG(ERR) << "Error retrieving cable info for transceiver " << *transID
              << " Exception: " << folly::exceptionStr(e);
    return std::nullopt;
  };
  return getTransceiverInfo()
      .via(evb)
      .thenValueInline(getCable)
      .thenError<std::exception>(std::move(handleErr));
}

folly::Future<std::optional<ExtendedSpecComplianceCode>>
WedgePort::getTransceiverExtendedSpecCompliance(folly::EventBase* evb) const {
  auto getTxcvExtendedSpecCompliance =
      [](TransceiverInfo info) -> std::optional<ExtendedSpecComplianceCode> {
    return info.extendedSpecificationComplianceCode_ref().to_optional();
  };
  auto transID = getTransceiverID();
  auto handleErr = [transID](const std::exception& e)
      -> std::optional<ExtendedSpecComplianceCode> {
    XLOG(ERR) << "Error retrieving ExtendedSpecCompliance info for transceiver "
              << *transID << " Exception: " << folly::exceptionStr(e);
    return std::nullopt;
  };
  return getTransceiverInfo()
      .via(evb)
      .thenValueInline(getTxcvExtendedSpecCompliance)
      .thenError<std::exception>(std::move(handleErr));
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
  auto tcvrListOpt = getTransceiverLanes();
  if (tcvrListOpt) {
    return !(*tcvrListOpt).empty();
  }

  // #TODO(joseph5wu) Will deprecate the frontPanel_ field once we switch to
  // get all platform port info from config
  return frontPanel_ != std::nullopt;
}

std::optional<ChannelID> WedgePort::getChannel() const {
  auto tcvrListOpt = getTransceiverLanes();
  if (tcvrListOpt) {
    const auto& tcvrList = *tcvrListOpt;
    if (!tcvrList.empty()) {
      // All the transceiver lanes should use the same transceiver id
      return ChannelID(*tcvrList[0].lane_ref());
    } else {
      return std::nullopt;
    }
  }

  // #TODO(joseph5wu) Will deprecate the frontPanel_ field once we switch to
  // get all platform port info from config
  if (!frontPanel_) {
    return std::nullopt;
  }
  return frontPanel_->channels[0];
}

std::vector<int32_t> WedgePort::getChannels() const {
  if (!port_) {
    return {};
  }
  const auto tcvrListOpt = getTransceiverLanes(port_->getProfileID());
  if (tcvrListOpt) {
    return folly::gen::from(tcvrListOpt.value()) |
        folly::gen::map(
               [&](const phy::PinID& entry) { return *entry.lane_ref(); }) |
        folly::gen::as<std::vector<int32_t>>();
  }

  // fallback to the frontPanel way of getting channels
  // TODO: remove this when all platforms support platform mapping since
  // getTransceiverLanes needs it
  if (!frontPanel_) {
    return {};
  }
  return std::vector<int32_t>(
      frontPanel_->channels.begin(), frontPanel_->channels.end());
}

TransceiverIdxThrift WedgePort::getTransceiverMapping() const {
  if (!supportsTransceiver()) {
    return TransceiverIdxThrift();
  }
  TransceiverIdxThrift xcvr;
  xcvr.transceiverId_ref() = static_cast<int32_t>(*getTransceiverID());
  xcvr.channels_ref() = getChannels();
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

std::optional<std::vector<phy::PinID>> WedgePort::getTransceiverLanes(
    std::optional<cfg::PortProfileID> profileID) const {
  auto platformPortEntry = getPlatformPortEntry();
  auto chips = getPlatform()->getDataPlanePhyChips();
  if (platformPortEntry && !chips.empty()) {
    return utility::getTransceiverLanes(*platformPortEntry, chips, profileID);
  }
  // If there's no platform port entry or chips from the config, fall back
  // to use old logic.
  // TODO(joseph) Will throw exception if there's no config after we fully
  // roll out the new config
  return std::nullopt;
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
