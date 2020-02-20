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

#include "fboss/agent/platforms/sai/SaiPlatformPort.h"

namespace facebook::fboss {

class SaiWedge400CPort : public SaiPlatformPort {
 public:
  explicit SaiWedge400CPort(PortID id, SaiPlatform* platform)
      : SaiPlatformPort(id, platform) {}
};

} // namespace facebook::fboss
