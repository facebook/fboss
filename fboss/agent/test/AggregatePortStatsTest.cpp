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

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

namespace {
cfg::SwitchConfig createConfig(AggregatePortID id, const std::string& name) {
  cfg::SwitchConfig config;
  config.ports_ref()->resize(2);
  *config.ports_ref()[0].logicalID_ref() = 1;
  *config.ports_ref()[0].state_ref() = cfg::PortState::ENABLED;
  *config.ports_ref()[1].logicalID_ref() = 2;
  *config.ports_ref()[1].state_ref() = cfg::PortState::ENABLED;

  config.vlans_ref()->resize(1);
  *config.vlans_ref()[0].id_ref() = 3;
  *config.vlans_ref()[0].name_ref() = "vlan3";
  *config.vlans_ref()[0].routable_ref() = true;

  config.interfaces_ref()->resize(1);
  *config.interfaces_ref()[0].intfID_ref() = 3;
  *config.interfaces_ref()[0].vlanID_ref() = 3;
  config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "1.2.3.4/24";
  config.interfaces_ref()[0].ipAddresses_ref()[1] =
      "2a03:2880:10:1f07:face:b00c:0:0/96";

  config.vlanPorts_ref()->resize(2);
  *config.vlanPorts_ref()[0].logicalPort_ref() = 1;
  *config.vlanPorts_ref()[0].vlanID_ref() = 3;
  *config.vlanPorts_ref()[0].emitTags_ref() = false;
  *config.vlanPorts_ref()[1].logicalPort_ref() = 2;
  *config.vlanPorts_ref()[1].vlanID_ref() = 3;
  *config.vlanPorts_ref()[1].emitTags_ref() = false;

  config.aggregatePorts_ref()->resize(1);
  *config.aggregatePorts_ref()[0].key_ref() = static_cast<uint16_t>(id);
  *config.aggregatePorts_ref()[0].name_ref() = name;
  *config.aggregatePorts_ref()[0].description_ref() = "double bundle";
  config.aggregatePorts_ref()[0].memberPorts_ref()->resize(2);
  *config.aggregatePorts_ref()[0].memberPorts_ref()[0].memberPortID_ref() = 1;
  *config.aggregatePorts_ref()[0].memberPorts_ref()[1].memberPortID_ref() = 2;

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
  AggregatePort::PartnerState pState{};

  auto config = createConfig(aggregatePortID, aggregatePortName);
  auto handle = createTestHandle(&config);
  auto sw = handle->getSw();

  CounterCache counters(sw);

  auto oldAggPort = getAggregatePort(sw, aggregatePortID);

  ProgramForwardingAndPartnerState addPort1ToAggregatePort(
      PortID(1), aggregatePortID, AggregatePort::Forwarding::ENABLED, pState);
  sw->updateStateNoCoalescing(
      "Adding first port to AggregatePort", addPort1ToAggregatePort);

  ProgramForwardingAndPartnerState addPort2ToAggregatePort(
      PortID(2), aggregatePortID, AggregatePort::Forwarding::ENABLED, pState);
  sw->updateStateNoCoalescing(
      "Adding second port tn AggrgatePort", addPort2ToAggregatePort);

  waitForStateUpdates(sw);
  auto newAggPort = getAggregatePort(sw, aggregatePortID);
  LinkAggregationManager::recordStatistics(sw, oldAggPort, newAggPort);

  counters.update();
  EXPECT_TRUE(counters.checkExist(flapsCounterName));
  counters.checkDelta(flapsCounterName, 1);
}

TEST(AggregatePortStats, FlapTwice) {
  const AggregatePortID aggregatePortID = AggregatePortID(1);
  const auto aggregatePortName = "Port-Channel1";
  const auto flapsCounterName = "Port-Channel1.flaps.sum";
  AggregatePort::PartnerState pState{};

  auto config = createConfig(aggregatePortID, aggregatePortName);
  auto handle = createTestHandle(&config);
  auto sw = handle->getSw();

  CounterCache counters(sw);

  auto baseAggPort = getAggregatePort(sw, aggregatePortID);

  ProgramForwardingAndPartnerState addPort1ToAggregatePort(
      PortID(1), aggregatePortID, AggregatePort::Forwarding::ENABLED, pState);
  sw->updateStateNoCoalescing(
      "Adding first port to AggregatePort", addPort1ToAggregatePort);

  ProgramForwardingAndPartnerState addPort2ToAggregatePort(
      PortID(2), aggregatePortID, AggregatePort::Forwarding::ENABLED, pState);
  sw->updateStateNoCoalescing(
      "Adding second port to AggrgatePort", addPort2ToAggregatePort);

  waitForStateUpdates(sw);
  auto initialAggPort = getAggregatePort(sw, aggregatePortID);
  LinkAggregationManager::recordStatistics(sw, baseAggPort, initialAggPort);

  ProgramForwardingAndPartnerState removePort2FromAggregatePort(
      PortID(2), aggregatePortID, AggregatePort::Forwarding::DISABLED, pState);
  sw->updateStateNoCoalescing(
      "Removing second port from AggregatePort", removePort2FromAggregatePort);

  waitForStateUpdates(sw);
  auto finalAggPort = getAggregatePort(sw, aggregatePortID);
  LinkAggregationManager::recordStatistics(sw, initialAggPort, finalAggPort);

  counters.update();
  EXPECT_TRUE(counters.checkExist(flapsCounterName));
  counters.checkDelta(flapsCounterName, 2);
}

TEST(AggregatePortStats, UpdateAggregatePortName) {
  const AggregatePortID aggregatePortID = AggregatePortID(1);

  const auto initialAggregatePortName = "Port-Channel1";
  const auto initialFlapsCounterName = "Port-Channel1.flaps.sum";

  const auto updatedAggregatePortName = "Port-Channel001";
  const auto updatedFlapsCounterName = "Port-Channel001.flaps.sum";

  SwSwitch* sw = nullptr;
  AggregatePort::PartnerState pState{};

  std::shared_ptr<AggregatePort> baseAggPort = nullptr;
  auto config = createConfig(aggregatePortID, initialAggregatePortName);
  auto handle = createTestHandle(&config);
  sw = handle->getSw();
  baseAggPort = getAggregatePort(sw, aggregatePortID);

  CounterCache counters(sw);

  std::shared_ptr<AggregatePort> initialAggPort = nullptr;
  ProgramForwardingAndPartnerState addPort1ToAggregatePort(
      PortID(1), aggregatePortID, AggregatePort::Forwarding::ENABLED, pState);
  sw->updateStateNoCoalescing(
      "Adding first port to AggregatePort", addPort1ToAggregatePort);

  ProgramForwardingAndPartnerState addPort2ToAggregatePort(
      PortID(2), aggregatePortID, AggregatePort::Forwarding::ENABLED, pState);
  sw->updateStateNoCoalescing(
      "Adding second port tn AggregatePort", addPort2ToAggregatePort);

  waitForStateUpdates(sw);
  initialAggPort = getAggregatePort(sw, aggregatePortID);

  LinkAggregationManager::recordStatistics(sw, baseAggPort, initialAggPort);
  counters.update();
  EXPECT_TRUE(counters.checkExist(initialFlapsCounterName));

  std::shared_ptr<AggregatePort> updatedAggPort = nullptr;
  sw->stats()
      ->aggregatePort(aggregatePortID)
      ->aggregatePortNameChanged(updatedAggregatePortName);

  ProgramForwardingAndPartnerState removePort2FromAggregatePort(
      PortID(2), aggregatePortID, AggregatePort::Forwarding::DISABLED, pState);
  sw->updateStateNoCoalescing(
      "Removing second port from AggregatePort", removePort2FromAggregatePort);

  waitForStateUpdates(sw);
  updatedAggPort = getAggregatePort(sw, aggregatePortID);

  LinkAggregationManager::recordStatistics(sw, initialAggPort, updatedAggPort);
  counters.update();
  EXPECT_FALSE(counters.checkExist(initialFlapsCounterName));
  EXPECT_TRUE(counters.checkExist(updatedFlapsCounterName));
}
