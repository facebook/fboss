// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"

namespace facebook::fboss {

class SaiBcmWedge100PlatformPort : public SaiBcmPlatformPort {
 public:
  void linkStatusChanged(bool up, bool adminUp) override;
};

} // namespace facebook::fboss
