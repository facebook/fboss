/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiFakePlatformMapping.h"
#include "fboss/agent/hw/test/FakeAgentConfigFactory.h"

namespace facebook {
namespace fboss {
SaiFakePlatformMapping::SaiFakePlatformMapping() : PlatformMapping() {
  auto agentConfig = utility::getFakeAgentConfig();
  for (auto& supportedProfile : *agentConfig.platform.supportedProfiles_ref()) {
    setSupportedProfile(supportedProfile.first, supportedProfile.second);
  }
  for (auto& platformPortEntry : *agentConfig.platform.platformPorts_ref()) {
    setPlatformPort(platformPortEntry.first, platformPortEntry.second);
  }
  for (auto& chip : *agentConfig.platform.chips_ref()) {
    setChip(chip.name, chip);
  }
}
} // namespace fboss
} // namespace facebook
