/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>
#include <string>

#include "fboss/agent/AggregatePortStats.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

namespace {
cfg::SwitchConfig createConfig(AggregatePortID id, const std::string& name) {
  cfg::SwitchConfig config;
  config.ports.resize(2);
  config.ports[0].logicalID = 1;
  config.ports[0].state = cfg::PortState::ENABLED;
  config.ports[1].logicalID = 2;
  config.ports[1].state = cfg::PortState::ENABLED;

  config.vlans.resize(1);
  config.vlans[0].id = 3;
  config.vlans[0].name = "vlan3";
  config.vlans[0].routable = true;

  config.interfaces.resize(1);
  config.interfaces[0].intfID = 3;
  config.interfaces[0].vlanID = 3;
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "1.2.3.4/24";
  config.interfaces[0].ipAddresses[1] = "2a03:2880:10:1f07:face:b00c:0:0/96";

  config.vlanPorts.resize(2);
  config.vlanPorts[0].logicalPort = 1;
  config.vlanPorts[0].vlanID = 3;
  config.vlanPorts[0].emitTags = false;
  config.vlanPorts[1].logicalPort = 2;
  config.vlanPorts[1].vlanID = 3;
  config.vlanPorts[1].emitTags = false;

  config.__isset.aggregatePorts = true;
  config.aggregatePorts.resize(1);
  config.aggregatePorts[0].key = static_cast<uint16_t>(id);
  config.aggregatePorts[0].name = name;
  config.aggregatePorts[0].description = "double bundle";
  config.aggregatePorts[0].memberPorts.resize(2);
  config.aggregatePorts[0].memberPorts[0].memberPortID = 1;
  config.aggregatePorts[0].memberPorts[1].memberPortID = 2;

  return config;
}

std::shared_ptr<AggregatePort> getAggregatePort(
    SwSwitch* sw,
    AggregatePortID id) {
  return sw->getState()->getAggregatePorts()->getAggregatePort(id);
}

} // namespace

TEST(AggregatePortStats, FlapOnce) {
  const AggregatePortID aggregatePortID = AggregatePortID(1);
  const auto aggregatePortName = "Port-Channel1";
  const auto flapsCounterName = "Port-Channel1.flaps.sum";

  auto config = createConfig(aggregatePortID, aggregatePortName);
  auto handle = createTestHandle(&config);
  auto sw = handle->getSw();

  CounterCache counters(sw);

  auto oldAggPort = getAggregatePort(sw, aggregatePortID);

  ProgramForwardingState addPort1ToAggregatePort(
      PortID(1), aggregatePortID, AggregatePort::Forwarding::ENABLED);
  sw->updateStateNoCoalescing(
      "Adding first port to AggregatePort", addPort1ToAggregatePort);

  ProgramForwardingState addPort2ToAggregatePort(
      PortID(2), aggregatePortID, AggregatePort::Forwarding::ENABLED);
  sw->updateStateNoCoalescing(
      "Adding second port tn AggrgatePort", addPort2ToAggregatePort);

  waitForStateUpdates(sw);
  auto newAggPort = getAggregatePort(sw, aggregatePortID);
  AggregatePortStats::recordStatistics(sw, oldAggPort, newAggPort);

  counters.update();
  EXPECT_TRUE(counters.checkExist(flapsCounterName));
  counters.checkDelta(flapsCounterName, 1);
}

TEST(AggregatePortStats, FlapTwice) {
  const AggregatePortID aggregatePortID = AggregatePortID(1);
  const auto aggregatePortName = "Port-Channel1";
  const auto flapsCounterName = "Port-Channel1.flaps.sum";

  auto config = createConfig(aggregatePortID, aggregatePortName);
  auto handle = createTestHandle(&config);
  auto sw = handle->getSw();

  CounterCache counters(sw);

  auto baseAggPort = getAggregatePort(sw, aggregatePortID);

  ProgramForwardingState addPort1ToAggregatePort(
      PortID(1), aggregatePortID, AggregatePort::Forwarding::ENABLED);
  sw->updateStateNoCoalescing(
      "Adding first port to AggregatePort", addPort1ToAggregatePort);

  ProgramForwardingState addPort2ToAggregatePort(
      PortID(2), aggregatePortID, AggregatePort::Forwarding::ENABLED);
  sw->updateStateNoCoalescing(
      "Adding second port to AggrgatePort", addPort2ToAggregatePort);

  waitForStateUpdates(sw);
  auto initialAggPort = getAggregatePort(sw, aggregatePortID);
  AggregatePortStats::recordStatistics(sw, baseAggPort, initialAggPort);

  ProgramForwardingState removePort2FromAggregatePort(
      PortID(2), aggregatePortID, AggregatePort::Forwarding::DISABLED);
  sw->updateStateNoCoalescing(
      "Removing second port from AggregatePort", removePort2FromAggregatePort);

  waitForStateUpdates(sw);
  auto finalAggPort = getAggregatePort(sw, aggregatePortID);
  AggregatePortStats::recordStatistics(sw, initialAggPort, finalAggPort);

  counters.update();
  EXPECT_TRUE(counters.checkExist(flapsCounterName));
  counters.checkDelta(flapsCounterName, 2);
}
