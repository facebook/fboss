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
#include "fboss/agent/types.h"

/*
 * This utility is to provide utils for tests using DSCP Marking tests.
 */
namespace facebook::fboss {
class HwAsic;
}

namespace facebook::fboss::utility {

const std::vector<uint32_t>& kTcpPorts();
const std::vector<uint32_t>& kUdpPorts();

std::string getDscpAclTableName();
std::string kDscpCounterAclName();
std::string kCounterName();
std::string getIngressAclTableGroupName();
uint8_t kIcpDscp();

void addDscpMarkingAcls(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    bool isSai);
void addDscpCounterAcl(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    cfg::AclActionType actionType);
void addDscpMarkingAclTable(cfg::SwitchConfig* config, bool isSai);
void addDscpAclEntryWithCounter(
    cfg::SwitchConfig* config,
    const std::string& aclTableName,
    bool isSai);
void addDscpAclTable(
    cfg::SwitchConfig* config,
    const HwAsic* hwAsic,
    int16_t priority,
    bool addAllQualifiers,
    bool isSai);
void delDscpMatchers(cfg::SwitchConfig* config);
} // namespace facebook::fboss::utility
