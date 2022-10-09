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
#include <string>
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/state/TeFlowEntry.h"

/*
 * This utility is to provide utils for bcm teflow tests.
 */

namespace facebook::fboss::utility {

void setExactMatchCfg(HwSwitchEnsemble* hwSwitchEnsemble, int prefixLength);

TeFlow makeFlowKey(std::string dstIp, uint16_t srcPort);

std::shared_ptr<TeFlowEntry> makeFlowEntry(
    std::string dstIp,
    std::string nhopAdd,
    std::string ifName,
    uint16_t srcPort,
    std::string counterID);

void addFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<TeFlowEntry>& flowEntry);

void deleteFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<TeFlowEntry>& flowEntry);

void modifyFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::string dstIp,
    uint16_t srcPort,
    std::string nhopAdd,
    std::string ifName,
    std::string counterID);

void modifyFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<TeFlowEntry>& newFlowEntry,
    bool enable);

uint64_t getTeFlowOutBytes(const HwSwitch* hw, std::string statName);

} // namespace facebook::fboss::utility
