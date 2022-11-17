/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTeFlowTestUtils.h"
#include <folly/IPAddress.h>
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss::utility {

using facebook::network::toBinaryAddress;
using folly::IPAddress;
using folly::StringPiece;

void setExactMatchCfg(HwSwitchEnsemble* hwSwitchEnsemble, int prefixLength) {
  cfg::ExactMatchTableConfig exactMatchTableConfigs;
  std::string teFlowTableName(cfg::switch_config_constants::TeFlowTableName());
  exactMatchTableConfigs.name() = teFlowTableName;
  exactMatchTableConfigs.dstPrefixLength() = prefixLength;
  auto newState = hwSwitchEnsemble->getProgrammedState()->clone();
  auto newSwitchSettings = newState->getSwitchSettings()->clone();
  newSwitchSettings->setExactMatchTableConfig({exactMatchTableConfigs});
  newState->resetSwitchSettings(newSwitchSettings);
  hwSwitchEnsemble->applyNewState(newState);
}

IpPrefix ipPrefix(StringPiece ip, int length) {
  IpPrefix result;
  result.ip() = toBinaryAddress(IPAddress(ip));
  result.prefixLength() = length;
  return result;
}

TeFlow makeFlowKey(std::string dstIp, uint16_t srcPort) {
  TeFlow flow;
  flow.srcPort() = srcPort;
  flow.dstPrefix() = ipPrefix(dstIp, 56);
  return flow;
}

std::shared_ptr<TeFlowEntry> makeFlowEntry(
    std::string dstIp,
    std::string nhopAdd,
    std::string ifName,
    uint16_t srcPort,
    std::string counterID) {
  auto flow = makeFlowKey(dstIp, srcPort);
  auto flowEntry = std::make_shared<TeFlowEntry>(flow);
  std::vector<NextHopThrift> nexthops;
  NextHopThrift nhop;
  nhop.address() = toBinaryAddress(IPAddress(nhopAdd));
  nhop.address()->ifName() = ifName;
  nexthops.push_back(nhop);
  flowEntry->setEnabled(true);
  flowEntry->setCounterID(counterID);
  flowEntry->setNextHops(nexthops);
  flowEntry->setResolvedNextHops(nexthops);
  return flowEntry;
}

std::vector<std::shared_ptr<TeFlowEntry>> makeFlowEntries(
    std::string dstIpStart,
    std::string nhopAdd,
    std::string ifName,
    uint16_t srcPort,
    uint32_t numEntries) {
  std::vector<std::shared_ptr<TeFlowEntry>> flowEntries;
  int count{0};
  int i{0};
  int j{0};
  while (++count <= numEntries) {
    std::string prefix = fmt::format("{}:{}:{}::", dstIpStart, i, j);
    std::string counter = fmt::format("counter{}", count);
    flowEntries.emplace_back(
        makeFlowEntry(prefix, nhopAdd, ifName, srcPort, counter));
    if (j++ > 255) {
      i++;
      j = 0;
    }
  }
  return flowEntries;
}

void addFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<TeFlowEntry>& flowEntry) {
  auto state = hwEnsemble->getProgrammedState()->clone();
  auto teFlows = state->getTeFlowTable()->modify(&state);
  teFlows->addNode(flowEntry);
  hwEnsemble->applyNewState(state, true);
}

void addFlowEntries(
    HwSwitchEnsemble* hwEnsemble,
    std::vector<std::shared_ptr<TeFlowEntry>>& flowEntries) {
  auto state = hwEnsemble->getProgrammedState()->clone();
  auto teFlows = state->getTeFlowTable()->modify(&state);
  for (auto& flowEntry : flowEntries) {
    teFlows->addNode(flowEntry);
  }
  hwEnsemble->applyNewState(state, true);
}

void deleteFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<TeFlowEntry>& flowEntry) {
  auto teFlows = hwEnsemble->getProgrammedState()->getTeFlowTable()->clone();
  teFlows->removeNode(flowEntry);
  auto newState = hwEnsemble->getProgrammedState()->clone();
  newState->resetTeFlowTable(teFlows);
  hwEnsemble->applyNewState(newState);
}

void deleteFlowEntries(
    HwSwitchEnsemble* hwEnsemble,
    std::vector<std::shared_ptr<TeFlowEntry>>& flowEntries) {
  auto teFlows = hwEnsemble->getProgrammedState()->getTeFlowTable()->clone();
  for (auto& flowEntry : flowEntries) {
    teFlows->removeNode(flowEntry);
  }
  auto newState = hwEnsemble->getProgrammedState()->clone();
  newState->resetTeFlowTable(teFlows);
  hwEnsemble->applyNewState(newState);
}

void modifyFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::string dstIp,
    uint16_t srcPort,
    std::string nhopAdd,
    std::string ifName,
    std::string counterID) {
  auto flow = makeFlowKey(dstIp, srcPort);
  auto teFlows = hwEnsemble->getProgrammedState()->getTeFlowTable()->clone();
  auto newFlowEntry = makeFlowEntry(dstIp, nhopAdd, ifName, srcPort, counterID);
  teFlows->updateNode(newFlowEntry);
  auto newState = hwEnsemble->getProgrammedState()->clone();
  newState->resetTeFlowTable(teFlows);
  hwEnsemble->applyNewState(newState);
}

void modifyFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<TeFlowEntry>& newFlowEntry,
    bool enable) {
  auto teFlows = hwEnsemble->getProgrammedState()->getTeFlowTable()->clone();
  newFlowEntry->setEnabled(enable);
  teFlows->updateNode(newFlowEntry);
  auto newState = hwEnsemble->getProgrammedState()->clone();
  newState->resetTeFlowTable(teFlows);
  hwEnsemble->applyNewState(newState);
}

} // namespace facebook::fboss::utility
