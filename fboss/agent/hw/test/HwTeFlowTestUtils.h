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

void setExactMatchCfg(std::shared_ptr<SwitchState>* state, int prefixLength);

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

void addFlowEntries(
    HwSwitchEnsemble* hwEnsemble,
    std::vector<std::shared_ptr<TeFlowEntry>>& flowEntries);
void addFlowEntries(
    std::shared_ptr<SwitchState>* state,
    std::vector<std::shared_ptr<TeFlowEntry>>& flowEntries);

void deleteFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<TeFlowEntry>& flowEntry);

void deleteFlowEntries(
    HwSwitchEnsemble* hwEnsemble,
    std::vector<std::shared_ptr<TeFlowEntry>>& flowEntries);
void deleteFlowEntries(
    std::shared_ptr<SwitchState>* state,
    std::vector<std::shared_ptr<TeFlowEntry>>& flowEntries);

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

std::vector<std::shared_ptr<TeFlowEntry>> makeFlowEntries(
    std::string dstIp,
    std::string nhopAdd,
    std::string ifName,
    uint16_t srcPort,
    uint32_t numEntries);

uint64_t getTeFlowOutBytes(const HwSwitch* hw, std::string statName);

} // namespace facebook::fboss::utility
