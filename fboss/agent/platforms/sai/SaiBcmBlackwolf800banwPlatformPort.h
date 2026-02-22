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
#include "fboss/agent/platforms/sai/SaiMeru800biaPlatformPort.h"

namespace facebook::fboss {

class SaiBcmBlackwolf800banwPlatformPort : public SaiMeru800biaPlatformPort {
 public:
  SaiBcmBlackwolf800banwPlatformPort(PortID id, SaiPlatform* platform)
      : SaiMeru800biaPlatformPort(id, platform) {}
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState lfs) override;

 private:
  uint32_t currentLedState_{0};
};

} // namespace facebook::fboss
