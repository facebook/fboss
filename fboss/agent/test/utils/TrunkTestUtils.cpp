/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/TrunkTestUtils.h"
#include <memory>

#include "fboss/agent/if/gen-cpp2/AgentHwTestCtrl.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {
bool verifyAggregatePortMemberCount(
    AgentEnsemble& ensemble,
    AggregatePortID aggregatePortID,
    uint8_t currentCount) {
  auto aggPort = ensemble.getSw()->getState()->getAggregatePorts()->getNodeIf(
      aggregatePortID);
  if (!aggPort) {
    return false;
  }
  auto client = ensemble.getHwAgentTestClient(
      ensemble.getSw()->getScopeResolver()->scope(aggPort).switchId());
  const std::vector<int32_t> aggIds{aggregatePortID};
  std::vector<utility::AggPortInfo> aggPortInfos;
  client->sync_getAggPortInfo(aggPortInfos, aggIds);
  XLOG(DBG2) << "Total member count: " << *aggPortInfos[0].numMembers()
             << "Active member count:  " << *aggPortInfos[0].numActiveMembers();
  return currentCount == aggPortInfos[0].numActiveMembers();
}

int getAggregatePortCount(AgentEnsemble& ensemble) {
  auto client = ensemble.getHwAgentTestClient(
      ensemble.getSw()
          ->getScopeResolver()
          ->scope(ensemble.masterLogicalPortIds()[0])
          .switchId());
  return client->sync_getNumAggPorts();
}

bool verifyAggregatePort(AgentEnsemble& ensemble, AggregatePortID aggId) {
  auto aggPort =
      ensemble.getSw()->getState()->getAggregatePorts()->getNodeIf(aggId);
  if (!aggPort) {
    return false;
  }
  auto client = ensemble.getHwAgentTestClient(
      ensemble.getSw()->getScopeResolver()->scope(aggPort).switchId());
  const std::vector<int32_t> aggIds{aggId};
  std::vector<utility::AggPortInfo> aggPortInfos;
  client->sync_getAggPortInfo(aggPortInfos, aggIds);
  return *aggPortInfos[0].isPresent();
}

bool verifyPktFromAggregatePort(
    AgentEnsemble& ensemble,
    AggregatePortID aggId) {
  auto aggPort =
      ensemble.getSw()->getState()->getAggregatePorts()->getNodeIf(aggId);
  if (!aggPort) {
    return false;
  }
  auto client = ensemble.getHwAgentTestClient(
      ensemble.getSw()->getScopeResolver()->scope(aggPort).switchId());
  return client->sync_verifyPktFromAggPort(aggId);
}

} // namespace facebook::fboss::utility
