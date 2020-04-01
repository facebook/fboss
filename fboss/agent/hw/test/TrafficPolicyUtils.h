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
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <string>

/*
 * This utility is to provide utils for bcm traffic tests.
 */

namespace facebook::fboss::utility {
void addDscpAclToCfg(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    uint32_t dscp);

void addClassIDAclToCfg(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClass);

void addQueueMatcher(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    int queueId,
    const std::optional<std::string>& counterName = std::nullopt);

void addTrafficCounter(
    cfg::SwitchConfig* config,
    const std::string& counterName);

cfg::QosPolicy* addDscpQosPolicy(
    cfg::SwitchConfig* cfg,
    const std::string& name,
    std::vector<std::pair<uint16_t, std::vector<int16_t>>> map);
void delQosPolicy(cfg::SwitchConfig* cfg, const std::string& name);

void setDefaultQosPolicy(cfg::SwitchConfig* cfg, const std::string& name);
void unsetDefaultQosPolicy(cfg::SwitchConfig* cfg);
void overrideQosPolicy(
    cfg::SwitchConfig* cfg,
    int port,
    const std::string& name);

void setCPUQosPolicy(cfg::SwitchConfig* cfg, const std::string& name);
void unsetCPUQosPolicy(cfg::SwitchConfig* cfg);

} // namespace facebook::fboss::utility
