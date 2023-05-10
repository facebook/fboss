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
  auto newState = hwSwitchEnsemble->getProgrammedState();
  setExactMatchCfg(&newState, prefixLength);
  hwSwitchEnsemble->applyNewState(newState);
}

void setExactMatchCfg(std::shared_ptr<SwitchState>* state, int prefixLength) {
  cfg::ExactMatchTableConfig exactMatchTableConfigs;
  std::string teFlowTableName(cfg::switch_config_constants::TeFlowTableName());
  exactMatchTableConfigs.name() = teFlowTableName;
  exactMatchTableConfigs.dstPrefixLength() = prefixLength;
  auto newState = *state;
  if (newState->isPublished()) {
    newState = newState->clone();
  }
  auto newSwitchSettings = newState->getSwitchSettings()->clone();
  newSwitchSettings->setExactMatchTableConfig({exactMatchTableConfigs});
  newState->resetSwitchSettings(newSwitchSettings);
  *state = newState;
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

FlowEntry makeFlow(
    const std::string& dstIp,
    const std::string& nhop,
    const std::string& ifname,
    const uint16_t& srcPort,
    const std::string& counterID,
    const int& prefixLength) {
  FlowEntry flowEntry;
  flowEntry.flow()->srcPort() = srcPort;
  flowEntry.flow()->dstPrefix() = ipPrefix(dstIp, prefixLength);
  flowEntry.counterID() = counterID;
  flowEntry.nextHops()->resize(1);
  flowEntry.nextHops()[0].address() = toBinaryAddress(IPAddress(nhop));
  flowEntry.nextHops()[0].address()->ifName() = ifname;
  return flowEntry;
}

std::shared_ptr<std::vector<FlowEntry>> makeFlowEntries(
    const std::vector<std::string>& flows,
    const std::string& nhop,
    const std::string& ifname,
    const uint16_t& srcPort,
    const std::string& counterID,
    const int& prefixLength) {
  auto flowEntries = std::make_shared<std::vector<FlowEntry>>();
  for (const auto& prefix : flows) {
    auto flowEntry =
        makeFlow(prefix, nhop, ifname, srcPort, counterID, prefixLength);
    flowEntries->emplace_back(flowEntry);
  }
  return flowEntries;
}

std::shared_ptr<std::vector<TeFlow>> makeTeFlows(
    const std::vector<std::string>& flows,
    const uint16_t& srcPort) {
  auto teFlows = std::make_shared<std::vector<TeFlow>>();
  for (const auto& prefix : flows) {
    auto teFlow = makeFlowKey(prefix, srcPort);
    teFlows->emplace_back(teFlow);
  }
  return teFlows;
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

std::vector<std::shared_ptr<TeFlowEntry>>
FlowEntryGenerator::generateFlowEntries() const {
  std::vector<std::shared_ptr<TeFlowEntry>> flowEntries;
  for (auto index = 0; index < numEntries; index++) {
    flowEntries.emplace_back(makeFlowEntry(
        getDstIp(index), nhopAddress, ifName, srcPort, getCounterId(index)));
  }
  return flowEntries;
}

std::string FlowEntryGenerator::getCounterId(int index) const {
  return fmt::format("counter{}", index + 1);
}

std::string FlowEntryGenerator::getDstIp(int index) const {
  auto i = int(index / 256);
  auto j = index % 256;
  std::string prefix = fmt::format("{}:{}:{}::", dstIpStart, i, j);
  return prefix;
}

std::vector<std::shared_ptr<TeFlowEntry>> makeFlowEntries(
    std::string& dstIpStart,
    std::string& nhopAdd,
    std::string& ifName,
    uint16_t srcPort,
    uint32_t numEntries) {
  auto generator =
      FlowEntryGenerator(dstIpStart, nhopAdd, ifName, srcPort, numEntries);
  return generator.generateFlowEntries();
}

void addFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<TeFlowEntry>& flowEntry) {
  auto state = hwEnsemble->getProgrammedState()->clone();
  auto teFlows = state->getMultiSwitchTeFlowTable()->modify(&state);
  teFlows->addNode(flowEntry, hwEnsemble->scopeResolver().scope(flowEntry));
  hwEnsemble->applyNewState(state, true);
}

void addFlowEntries(
    HwSwitchEnsemble* hwEnsemble,
    std::vector<std::shared_ptr<TeFlowEntry>>& flowEntries) {
  auto state = hwEnsemble->getProgrammedState()->clone();
  auto teFlows = state->getMultiSwitchTeFlowTable()->modify(&state);
  for (auto& flowEntry : flowEntries) {
    teFlows->addNode(flowEntry, hwEnsemble->scopeResolver().scope(flowEntry));
  }
  hwEnsemble->applyNewState(state, true);
}

void addFlowEntries(
    std::shared_ptr<SwitchState>* state,
    std::vector<std::shared_ptr<TeFlowEntry>>& flowEntries,
    const SwitchIdScopeResolver& resolver) {
  auto teFlows = (*state)->getMultiSwitchTeFlowTable()->modify(state);
  for (auto& flowEntry : flowEntries) {
    teFlows->addNode(flowEntry, resolver.scope(flowEntry));
  }
}

void deleteFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<TeFlowEntry>& flowEntry) {
  auto teFlows =
      hwEnsemble->getProgrammedState()->getMultiSwitchTeFlowTable()->clone();
  teFlows->removeNode(flowEntry);
  auto newState = hwEnsemble->getProgrammedState()->clone();
  newState->resetTeFlowTable(teFlows);
  hwEnsemble->applyNewState(newState);
}

void deleteFlowEntries(
    HwSwitchEnsemble* hwEnsemble,
    std::vector<std::shared_ptr<TeFlowEntry>>& flowEntries) {
  auto newState = hwEnsemble->getProgrammedState();
  deleteFlowEntries(&newState, flowEntries);
  hwEnsemble->applyNewState(newState);
}

void deleteFlowEntries(
    std::shared_ptr<SwitchState>* state,
    std::vector<std::shared_ptr<TeFlowEntry>>& flowEntries) {
  auto teFlows = (*state)->getMultiSwitchTeFlowTable()->modify(state);
  for (auto& flowEntry : flowEntries) {
    teFlows->removeNode(flowEntry);
  }
}

void addSyncFlowEntries(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<std::vector<FlowEntry>>& flowEntries,
    bool addSync) {
  auto newState = hwEnsemble->getProgrammedState();
  TeFlowSyncer teFlowSyncer;
  newState =
      teFlowSyncer.programFlowEntries(newState, *flowEntries, {}, addSync);
  hwEnsemble->applyNewState(newState);
}

void deleteFlowEntries(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<std::vector<TeFlow>>& teFlows) {
  auto newState = hwEnsemble->getProgrammedState();
  TeFlowSyncer teFlowSyncer;
  newState = teFlowSyncer.programFlowEntries(newState, {}, *teFlows, false);
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
  auto teFlows =
      hwEnsemble->getProgrammedState()->getMultiSwitchTeFlowTable()->clone();
  auto newFlowEntry = makeFlowEntry(dstIp, nhopAdd, ifName, srcPort, counterID);
  teFlows->updateNode(
      newFlowEntry, hwEnsemble->scopeResolver().scope(newFlowEntry));
  auto newState = hwEnsemble->getProgrammedState()->clone();
  newState->resetTeFlowTable(teFlows);
  hwEnsemble->applyNewState(newState);
}

void modifyFlowEntry(
    HwSwitchEnsemble* hwEnsemble,
    std::shared_ptr<TeFlowEntry>& newFlowEntry,
    bool enable) {
  auto teFlows =
      hwEnsemble->getProgrammedState()->getMultiSwitchTeFlowTable()->clone();
  newFlowEntry->setEnabled(enable);
  teFlows->updateNode(
      newFlowEntry, hwEnsemble->scopeResolver().scope(newFlowEntry));
  auto newState = hwEnsemble->getProgrammedState()->clone();
  newState->resetTeFlowTable(teFlows);
  hwEnsemble->applyNewState(newState);
}

} // namespace facebook::fboss::utility
