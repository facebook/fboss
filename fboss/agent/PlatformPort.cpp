/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/PlatformPort.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Platform.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss {

PlatformPort::PlatformPort(PortID id, Platform* platform)
    : id_(id), platform_(platform) {}

std::ostream& operator<<(std::ostream& os, PortLedExternalState lfs) {
  switch (lfs) {
    case PortLedExternalState::NONE:
      os << "None";
      break;
    case PortLedExternalState::CABLING_ERROR:
      os << "Cabling Error";
      break;
    case PortLedExternalState::EXTERNAL_FORCE_ON:
      os << "Turned ON externally by a Thrift call";
      break;
    case PortLedExternalState::EXTERNAL_FORCE_OFF:
      os << "Turned OFF externally by a Thrift call";
      break;
  }
  return os;
}

const std::optional<cfg::PlatformPortEntry> PlatformPort::getPlatformPortEntry()
    const {
  const auto& platformPorts = platform_->getPlatformPorts();
  if (auto itPlatformPort = platformPorts.find(id_);
      itPlatformPort != platformPorts.end()) {
    return itPlatformPort->second;
  }
  return std::nullopt;
}

std::vector<phy::PinConfig> PlatformPort::getIphyPinConfigs(
    cfg::PortProfileID profileID) const {
  return platform_->getPlatformMapping()->getPortIphyPinConfigs(id_, profileID);
}

cfg::PortProfileID PlatformPort::getProfileIDBySpeed(
    cfg::PortSpeed speed) const {
  auto profile = getProfileIDBySpeedIf(speed);
  if (!profile.has_value()) {
    throw FbossError(
        "Platform port ",
        getPortID(),
        " has no profile for speed ",
        apache::thrift::util::enumNameSafe(speed));
  }
  return profile.value();
}

std::optional<cfg::PortProfileID> PlatformPort::getProfileIDBySpeedIf(
    cfg::PortSpeed speed) const {
  // If we don't have a platform config, just return a default profile
  // (this prevents tests without a platform config from crashing, and
  // an exception will still be thrown when trying to actually program
  // platforms using the new config with PROFILE_DEFAULT in real use)
  auto platformPortEntry = getPlatformPortEntry();
  if (!platformPortEntry.has_value() || speed == cfg::PortSpeed::DEFAULT) {
    return cfg::PortProfileID::PROFILE_DEFAULT;
  }

  for (auto profile : *platformPortEntry->supportedProfiles_ref()) {
    auto profileID = profile.first;
    if (auto profileCfg = getPortProfileConfigIf(profileID)) {
      if (*profileCfg->speed_ref() == speed) {
        return profileID;
      }
    } else {
      throw FbossError(
          "Platform port ",
          getPortID(),
          " has invalid profile ",
          apache::thrift::util::enumNameSafe(profileID));
    }
  }
  return std::nullopt;
}

const phy::PortProfileConfig PlatformPort::getPortProfileConfig(
    cfg::PortProfileID profileID) const {
  auto profile = getPortProfileConfigIf(profileID);
  if (!profile.has_value()) {
    throw FbossError(
        "No port profile with id ",
        apache::thrift::util::enumNameSafe(profileID),
        " found in PlatformConfig for port ",
        id_);
  }
  return profile.value();
}

const std::optional<phy::PortProfileConfig>
PlatformPort::getPortProfileConfigIf(cfg::PortProfileID profileID) const {
  folly::EventBase evb;
  std::optional<ExtendedSpecComplianceCode> transceiverSpecComplianceCode =
      platform_->needExtendedSpecComplianceCode()
      ? getTransceiverExtendedSpecCompliance(&evb).getVia(&evb)
      : std::nullopt;
  if (transceiverSpecComplianceCode.has_value()) {
    return platform_->getPortProfileConfig(PlatformPortProfileConfigMatcher(
        profileID, id_, transceiverSpecComplianceCode.value()));
  } else {
    return platform_->getPortProfileConfig(
        PlatformPortProfileConfigMatcher(profileID, id_));
  }
}

// This should only be called by platforms that actually have
// an external phy
std::optional<int32_t> PlatformPort::getExternalPhyID() {
  auto platformPortEntry = getPlatformPortEntry();
  if (!platformPortEntry) {
    throw FbossError(
        "No PlatformPortEntry found for port with ID ", getPortID());
  }
  const auto& chips = getPlatform()->getDataPlanePhyChips();
  if (chips.empty()) {
    throw FbossError("Not platform data plane phy chips");
  }

  const auto& xphy = utility::getDataPlanePhyChips(
      *platformPortEntry, chips, phy::DataPlanePhyChipType::XPHY);
  if (xphy.empty()) {
    return std::nullopt;
  } else {
    // One port should only has one xphy id
    CHECK_EQ(xphy.size(), 1);
    return *xphy.begin()->second.physicalID_ref();
  }
}

folly::Future<std::optional<ExtendedSpecComplianceCode>>
PlatformPort::getTransceiverExtendedSpecCompliance(
    folly::EventBase* evb) const {
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

} // namespace facebook::fboss
