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
  // Set a dummy name for profile matching in platform mapping
  port.mapping_ref()->name_ref() = "eth1/1/1";

  for (auto i = 0; i < numPorts; i++) {
    setPlatformPort(i, port);
  }

  phy::PortProfileConfig profile;
  *profile.speed_ref() = cfg::PortSpeed::HUNDREDG;

  cfg::PlatformPortProfileConfigEntry configEntry;
  cfg::PlatformPortConfigFactor factor;
  factor.profileID_ref() = cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91;
  configEntry.profile_ref() = profile;
  configEntry.factor_ref() = factor;
  mergePlatformSupportedProfile(configEntry);
}
} // namespace fboss
} // namespace facebook
