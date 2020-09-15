// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"

namespace facebook::fboss {

class SaiBcmWedge100PlatformPort : public SaiBcmPlatformPort {
 public:
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState) override;
  static std::optional<std::tuple<uint32_t, uint32_t>> getLedAndIndex(
      uint32_t port);
};

} // namespace facebook::fboss
