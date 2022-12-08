/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"
#include "fboss/lib/fpga/MinipackLed.h"

namespace facebook::fboss {

class SaiBcmFujiPlatformPort : public SaiBcmPlatformPort, MultiPimPlatformPort {
 public:
  explicit SaiBcmFujiPlatformPort(PortID id, SaiPlatform* platform)
      : SaiBcmPlatformPort(id, platform),
        MultiPimPlatformPort(id, getPlatformPortEntry()) {}
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState lfs) override;

 private:
  MinipackLed::Color internalLedState_{MinipackLed::Color::OFF};
};

} // namespace facebook::fboss
