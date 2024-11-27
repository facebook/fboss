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
#include <exception>
#include <string>

#include <folly/Range.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

struct AgentConfig;
/*
 * Get switch info from config
 */
const std::map<int64_t, cfg::SwitchInfo> getSwitchInfoFromConfig(
    const AgentConfig* config);

const std::map<int64_t, cfg::SwitchInfo> getSwitchInfoFromConfig(
    const cfg::SwitchConfig* config);

const std::map<int64_t, cfg::SwitchInfo> getSwitchInfoFromConfig();

const std::optional<cfg::SdkVersion> getSdkVersionFromConfigImpl(
    const cfg::SwitchConfig* config);

const std::optional<cfg::SdkVersion> getSdkVersionFromConfig();

const std::optional<cfg::SdkVersion> getSdkVersionFromConfig(
    const AgentConfig* config);

bool withinRange(const cfg::SystemPortRanges& ranges, InterfaceID intfId);

bool withinRange(const cfg::SystemPortRanges& ranges, SystemPortID sysPortId);
} // namespace facebook::fboss
