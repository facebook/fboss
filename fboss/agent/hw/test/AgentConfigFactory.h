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
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/types.h"

#include <vector>

/*
 * This utility is to provide utils for test.
 */

namespace facebook::fboss::utility {
const uint16_t kMaxPorts = 128;

cfg::AgentConfig getAgentConfig();

} // namespace facebook::fboss::utility
