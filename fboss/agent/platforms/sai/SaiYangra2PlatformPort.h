/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/sai/SaiYangraPlatformPort.h"

namespace facebook::fboss {

class SaiYangra2PlatformPort : public SaiYangraPlatformPort {
 public:
  SaiYangra2PlatformPort(PortID id, SaiPlatform* platform)
      : SaiYangraPlatformPort(id, platform) {}
};

} // namespace facebook::fboss
