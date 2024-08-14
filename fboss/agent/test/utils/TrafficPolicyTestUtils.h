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

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/packet/IPProto.h"

#include <string>

/*
 * This utility is to provide utils for bcm traffic tests.
 */

namespace facebook::fboss::utility {
cfg::AclEntry* addDscpAclToCfg(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    const std::string& aclName,
    uint32_t dscp);

void addL4SrcPortAclToCfg(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    const std::string& aclName,
    IP_PROTO proto,
    uint32_t l4SrcPort);

void addL4DstPortAclToCfg(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    const std::string& aclName,
    IP_PROTO proto,
    uint32_t l4DstPort);

void addSetDscpAndEgressQueueActionToCfg(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    uint8_t dscp,
    int queueId,
    bool isSai);

void addL2ClassIDAndTtlAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassL2,
    std::optional<cfg::Ttl> ttl = std::nullopt);

void addNeighborClassIDAndTtlAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassNeighbor,
    std::optional<cfg::Ttl> ttl = std::nullopt);

void addRouteClassIDAndTtlAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassRoute,
    std::optional<cfg::Ttl> ttl = std::nullopt);

void addL2ClassIDDropAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassL2);

void addNeighborClassIDDropAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassNeighbor);

void addRouteClassIDDropAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassRoute);

void addQueueMatcher(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    int queueId,
    bool isSai,
    const std::optional<std::string>& counterName = std::nullopt);

void addTrafficCounter(
    cfg::SwitchConfig* config,
    const std::string& counterName,
    std::optional<std::vector<cfg::CounterType>> counterTypes);

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
