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
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {
struct AgentConfig;
}

namespace facebook::fboss::utility {

bool isDualStage(const cfg::AgentConfig& cfg);
bool isDualStage(const cfg::SwitchConfig& cfg);
bool isDualStage(const AgentConfig& cfg);

int64_t maxDsfSwitchId(const AgentConfig& cfg);
int64_t maxDsfSwitchId(const cfg::AgentConfig& cfg);
int64_t maxDsfSwitchId(const cfg::SwitchConfig& cfg);

} // namespace facebook::fboss::utility
