// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/sai/SaiBcmWedge100PlatformPort.h"

namespace facebook::fboss {

void SaiBcmWedge100PlatformPort::linkStatusChanged(
    bool /*up*/,
    bool /*adminUp*/) {
  // TODO(pshaikh): implement LED status change
}

} // namespace facebook::fboss
