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

cfg::PortProfileID PlatformPort::getProfileIDBySpeed(
    cfg::PortSpeed speed) const {
  // If we don't have a platform config, just return a default profile
  // (this prevents tests without a platform config from crashing, and
  // an exception will still be thrown when trying to actually program
  // platforms using the new config with PROFILE_DEFAULT in real use)
  auto platformPortEntry = getPlatformPortEntry();
  if (!platformPortEntry.has_value() || speed == cfg::PortSpeed::DEFAULT) {
    return cfg::PortProfileID::PROFILE_DEFAULT;
  }

  for (auto profile : platformPortEntry->supportedProfiles) {
    auto profileID = profile.first;
    if (auto profileCfg = platform_->getPortProfileConfig(profileID)) {
      if (profileCfg->speed == speed) {
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

  throw FbossError(
      "Platform port ",
      getPortID(),
      " has no profile for speed ",
      apache::thrift::util::enumNameSafe(speed));
}

} // namespace facebook::fboss
