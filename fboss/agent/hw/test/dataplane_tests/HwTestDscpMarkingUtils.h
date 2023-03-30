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
uint8_t kIcpDscp(const HwAsic* hwAsic);

void addDscpMarkingAcls(cfg::SwitchConfig* config, const HwAsic* hwAsic);
void addDscpCounterAcl(cfg::SwitchConfig* config, const HwAsic* hwAsic);
void addDscpMarkingAclTable(cfg::SwitchConfig* config, const HwAsic* hwAsic);
void addDscpAclEntryWithCounter(
    cfg::SwitchConfig* config,
    const std::string& aclTableName,
    const HwAsic* hwAsic);
void addDscpAclTable(
    cfg::SwitchConfig* config,
    int16_t priority,
    bool addTtlQualifier,
    const HwAsic* hwAsic);
void delDscpMatchers(cfg::SwitchConfig* config);
} // namespace facebook::fboss::utility
