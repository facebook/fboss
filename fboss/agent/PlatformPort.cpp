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
    if (auto profileCfg = platform_->getPortProfileConfig(profileID)) {
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

} // namespace facebook::fboss
