// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/sai/SaiBcmGalaxyPlatformPort.h"

namespace facebook::fboss {

void SaiBcmGalaxyPlatformPort::linkStatusChanged(
    bool /*up*/,
    bool /*adminUp*/) {
  // TODO(pshaikh): implement LED status change
}

} // namespace facebook::fboss
