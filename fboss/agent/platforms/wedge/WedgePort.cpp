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
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
#include "fboss/qsfp_service/lib/QsfpClient.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

namespace facebook { namespace fboss {

WedgePort::WedgePort(
  PortID id,
  WedgePlatform* platform,
  folly::Optional<FrontPanelResources> frontPanel) :
    id_(id),
    platform_(platform),
    frontPanel_(frontPanel) {
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

folly::Future<TransceiverInfo> WedgePort::getTransceiverInfo() const {
  auto qsfpCache = platform_->getQsfpCache();
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
    if (info.__isset.cable &&
        info.cable_ref().value_unchecked().__isset.transmitterTech) {
      return info.cable_ref().value_unchecked().transmitterTech;
    }
    return TransmitterTechnology::UNKNOWN;
  };
  auto handleError = [transID](const std::exception& e) {
    XLOG(ERR) << "Error retrieving info for transceiver " << transID
              << " Exception: " << folly::exceptionStr(e);
    return TransmitterTechnology::UNKNOWN;
  };
  return getTransceiverInfo().then(evb, getTech).onError(
      std::move(handleError));
}

// Get correct transmitter setting.
folly::Future<folly::Optional<TxSettings>> WedgePort::getTxSettings(
    folly::EventBase* evb) const {
  DCHECK(evb);

  auto txOverrides = getTxOverrides();
  if (txOverrides.empty()) {
    return folly::makeFuture<folly::Optional<TxSettings>>(folly::none);
  }

  auto getTx = [overrides = std::move(txOverrides)](
                   TransceiverInfo info) -> folly::Optional<TxSettings> {
    if (info.__isset.cable &&
        info.cable_ref().value_unchecked().__isset.transmitterTech) {
      if (!info.cable_ref().value_unchecked().__isset.length) {
        return folly::Optional<TxSettings>();
      }
      auto cableMeters = std::max(
          1.0,
          std::min(
              3.0,
              info.cable_ref()
                  .value_unchecked()
                  .length_ref()
                  .value_unchecked()));
      const auto it = overrides.find(std::make_pair(
          info.cable_ref().value_unchecked().transmitterTech, cableMeters));
      if (it != overrides.cend()) {
        return it->second;
      }
    }
    // not enough cable info. return the default value
    return folly::Optional<TxSettings>();
  };
  auto transID = getTransceiverID();
  auto handleErr = [transID](const std::exception& e) {
    XLOG(ERR) << "Error retrieving cable info for transceiver " << *transID
              << " Exception: " << folly::exceptionStr(e);
    return folly::Optional<TxSettings>();
  };
  return getTransceiverInfo().then(evb, getTx).onError(std::move(handleErr));
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

void WedgePort::externalState(BcmPlatformPort::ExternalState) {}

void WedgePort::linkSpeedChanged(const cfg::PortSpeed& speed) {
  // Cache the current set speed
  speed_ = speed;
}

folly::Optional<cfg::PlatformPortSettings> WedgePort::getPlatformPortSettings(
    cfg::PortSpeed speed) {
  auto platformSettings = platform_->config()->thrift.get_platform();
  if (!platformSettings) {
    return folly::none;
  }

  auto portsIter = platformSettings->ports.find(id_);
  if (portsIter == platformSettings->ports.end()) {
    return folly::none;
  }

  auto portConfig = portsIter->second;
  auto speedIter = portConfig.supportedSpeeds.find(speed);
  if (speedIter == portConfig.supportedSpeeds.end()) {
    throw FbossError("Port ", id_, " does not support speed ", speed);
  }

  return speedIter->second;
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

folly::Optional<ChannelID> WedgePort::getChannel() const {
    if (!frontPanel_) {
      return folly::none;
    }
    return frontPanel_->channels[0];
}

std::vector<int32_t> WedgePort::getChannels() const {
  // TODO(aeckert): this is pretty hacky... we should really model
  // port groups in switch state somehow so this can be served purely
  // from switch state.
  if (!frontPanel_.hasValue()) {
    return {};
  }

  // TODO: change to combining frontPanel_->channels in all member ports
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

TransceiverIdxThrift WedgePort::getTransceiverMapping() const {
  if (!supportsTransceiver()) {
    return TransceiverIdxThrift();
  }
  return TransceiverIdxThrift(
    apache::thrift::FragileConstructor::FRAGILE,
    static_cast<int32_t>(*getTransceiverID()),
    0,  // TODO: deprecate
    getChannels());
}

PortStatus WedgePort::toThrift(const std::shared_ptr<Port>& port) {
  // TODO: make it possible to generate a PortStatus struct solely
  // from a Port SwitchState node. Currently you need platform to get
  // transceiver mapping, which is not ideal.
  PortStatus status;
  status.enabled = port->isEnabled();
  status.up = port->isUp();
  status.speedMbps = static_cast<int64_t>(port->getSpeed());
  if (supportsTransceiver()) {
    status.set_transceiverIdx(getTransceiverMapping());
  }
  return status;
}

}} // facebook::fboss
