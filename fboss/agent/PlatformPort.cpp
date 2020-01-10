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
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Platform.h"

namespace facebook::fboss {

PlatformPort::PlatformPort(PortID id, Platform* platform)
    : id_(id), platform_(platform) {}

std::ostream& operator<<(std::ostream& os, PlatformPort::ExternalState lfs) {
  switch (lfs) {
    case PlatformPort::ExternalState::NONE:
      os << "None";
      break;
    case PlatformPort::ExternalState::CABLING_ERROR:
      os << "Cabling Error";
      break;
  }
  return os;
}

const std::optional<cfg::PlatformPortEntry> PlatformPort::getPlatformPortEntry()
    const {
  if (auto platformPorts =
          platform_->config()->thrift.platform.platformPorts_ref()) {
    auto itPlatformPort = (*platformPorts).find(id_);
    if (itPlatformPort != (*platformPorts).end()) {
      return itPlatformPort->second;
    }
  }
  return std::nullopt;
}

cfg::PortProfileID PlatformPort::getProfileIDBySpeed(
    cfg::PortSpeed speed) const {
  auto platformProfiles =
      platform_->config()->thrift.platform.supportedProfiles_ref();

  // If we don't have a platform config, just return a default profile
  // (this prevents tests without a platform config from crashing, and
  // an exception will still be thrown when trying to actually program
  // platforms using the new config with PROFILE_DEFAULT in real use)
  if (!platformProfiles) {
    return cfg::PortProfileID::PROFILE_DEFAULT;
  }

  auto platformPortEntry = getPlatformPortEntry();
  if (!platformPortEntry.has_value()) {
    throw FbossError(
        "Platform port ", getPortID(), " has no platformPortEntry");
  }

  for (auto profile : platformPortEntry->supportedProfiles) {
    auto profileID = profile.first;
    if (auto it = platformProfiles->find(profileID);
        it == platformProfiles->end()) {
      throw FbossError(
          "Platform port ",
          getPortID(),
          " has invalid profile ",
          apache::thrift::util::enumNameSafe(profileID));
    } else if (it->second.speed == speed) {
      return profileID;
    }
  }

  throw FbossError(
      "Platform port ",
      getPortID(),
      " has no profile for speed ",
      apache::thrift::util::enumNameSafe(speed));
}

} // namespace facebook::fboss
