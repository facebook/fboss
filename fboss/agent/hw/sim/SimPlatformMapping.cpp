/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sim/SimPlatformMapping.h"

namespace facebook {
namespace fboss {

SimPlatformMapping::SimPlatformMapping(uint32_t numPorts) : PlatformMapping() {
  cfg::PlatformPortEntry port;
  port.supportedProfiles_ref()->emplace(
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91, cfg::PlatformPortConfig());
  for (auto i = 0; i < numPorts; i++) {
    setPlatformPort(i, port);
  }

  phy::PortProfileConfig profile;
  *profile.speed_ref() = cfg::PortSpeed::HUNDREDG;
  setSupportedProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91, profile);
}
} // namespace fboss
} // namespace facebook
