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
#include <cstdint>
#include <utility>
#include "fboss/lib/platforms/PlatformMode.h"

namespace facebook::fboss {

struct AgentConfig;
class Platform;

using PlatformInitFn = std::unique_ptr<Platform> (*)(
    std::unique_ptr<AgentConfig>,
    uint32_t,
    int16_t);

void fbossInit(int argc, char** argv);

} // namespace facebook::fboss
