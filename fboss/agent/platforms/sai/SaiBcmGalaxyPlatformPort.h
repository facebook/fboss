// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"

namespace facebook::fboss {

class SaiBcmGalaxyPlatformPort : public SaiBcmPlatformPort {
 public:
  SaiBcmGalaxyPlatformPort(PortID id, SaiPlatform* platform)
      : SaiBcmPlatformPort(id, platform) {}
  void linkStatusChanged(bool up, bool adminUp) override;
};

} // namespace facebook::fboss
