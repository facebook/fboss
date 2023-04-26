/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockPlatformMapping.h"

#include <gmock/gmock.h>

namespace facebook::fboss {

MockPlatformMapping::MockPlatformMapping() : Wedge100PlatformMapping() {
  for (auto& entry : platformPorts_) {
    auto& platformPort = entry.second;
    platformPort.mapping()->attachedCoreId() = 0;
    platformPort.mapping()->attachedCorePortIndex() = 0;
  }
}
} // namespace facebook::fboss
