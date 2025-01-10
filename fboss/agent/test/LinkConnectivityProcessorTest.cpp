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

#include "fboss/agent/LinkConnectivityProcessor.h"

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using ::testing::_;

namespace {

std::map<PortID, multiswitch::FabricConnectivityDelta> makeConnectivity(
    std::optional<FabricEndpoint> endpoint,
    PortID portID) {
  multiswitch::FabricConnectivityDelta delta;
  if (endpoint) {
    delta.newConnectivity() = *endpoint;
  }
  std::map<PortID, multiswitch::FabricConnectivityDelta> connectivity;
  connectivity.insert({portID, delta});
  return connectivity;
}

int getSwitchId(cfg::SwitchType switchType) {
  return switchType == cfg::SwitchType::VOQ ? 1 : 2;
}

FabricEndpoint createFabricEndpoint(int switchID, PortID portID) {
  FabricEndpoint endpoint;
  endpoint.isAttached() = true;
  endpoint.switchName() = "switchName";
  endpoint.expectedSwitchName() = "switchName";
  endpoint.switchId() = switchID;
  endpoint.expectedSwitchId() = switchID;
  endpoint.portId() = portID;
  endpoint.expectedPortId() = portID;
  endpoint.portName() = "portName";
  endpoint.expectedPortName() = "portName";
  return endpoint;
}

TEST(LinkConnectivityProcessorTest, processSuccess) {
  auto switchType = cfg::SwitchType::NPU;
  cfg::SwitchConfig config = testConfigA(switchType);
  auto portID = PortID(1);

  auto handle = createTestHandle(&config, SwitchFlags::ENABLE_LLDP);
  auto sw = handle->getSw();
  auto state = sw->getState();

  auto endpoint = createFabricEndpoint(getSwitchId(switchType), portID);

  // When
  std::shared_ptr<SwitchState> result = LinkConnectivityProcessor::process(
      *sw->getScopeResolver(),
      *sw->getHwAsicTable(),
      state,
      makeConnectivity(endpoint, portID));

  // Then
  auto port = sw->getState()->getPorts()->getNodeIf(portID);
  CHECK(port);
  CHECK(!port->getLedPortExternalState());
  EXPECT_EQ(port->getActiveErrors().size(), 0);
}

TEST(LinkConnectivityProcessorTest, processLoopDetected) {
  auto switchType = cfg::SwitchType::NPU;
  cfg::SwitchConfig config = testConfigA(switchType);
  auto portID = PortID(1);

  auto handle = createTestHandle(&config, SwitchFlags::ENABLE_LLDP);
  auto sw = handle->getSw();
  auto state = sw->getState();

  // When
  auto localSwitchId = 0; // Loop
  {
    auto endpoint = createFabricEndpoint(localSwitchId, portID);
    std::shared_ptr<SwitchState> result = LinkConnectivityProcessor::process(
        *sw->getScopeResolver(),
        *sw->getHwAsicTable(),
        state,
        makeConnectivity(endpoint, portID));

    // Then
    auto port = result->getPorts()->getNodeIf(portID);
    CHECK(port);
    CHECK(port->getLedPortExternalState());
    CHECK(port->getLedPortExternalState().has_value());
    EXPECT_EQ(
        port->getLedPortExternalState().value(),
        PortLedExternalState::CABLING_ERROR_LOOP_DETECTED);
  }

  // When cabling is corrected
  auto remoteSwitchId = getSwitchId(switchType);
  {
    auto endpoint = createFabricEndpoint(remoteSwitchId, portID);
    std::shared_ptr<SwitchState> result = LinkConnectivityProcessor::process(
        *sw->getScopeResolver(),
        *sw->getHwAsicTable(),
        state,
        makeConnectivity(endpoint, portID));

    // Then error should be cleared
    auto port = result->getPorts()->getNodeIf(portID);
    CHECK(port);
    CHECK(port->getLedPortExternalState());
    CHECK(port->getLedPortExternalState().has_value());
    EXPECT_EQ(
        port->getLedPortExternalState().value(), PortLedExternalState::NONE);
    EXPECT_EQ(port->getActiveErrors().size(), 0);
  }
}

TEST(LinkConnectivityProcessorTest, processMissingNeighbor) {
  auto switchType = cfg::SwitchType::NPU;
  cfg::SwitchConfig config = testConfigA(switchType);
  auto portID = PortID(1);

  auto handle = createTestHandle(&config, SwitchFlags::ENABLE_LLDP);
  auto sw = handle->getSw();
  sw->updateStateBlocking(
      "Bring Ports Up", [](const std::shared_ptr<SwitchState>& state) {
        return bringAllPortsUp(state);
      });
  auto state = sw->getState();

  FabricEndpoint endpoint;
  endpoint.isAttached() = true;
  endpoint.switchName() = "switchName";
  endpoint.expectedSwitchName() = "switchName";
  endpoint.switchId() = getSwitchId(switchType);
  // endpoint.expectedSwitchId() = getSwitchId(switchType);
  endpoint.portId() = portID;
  endpoint.expectedPortId() = portID;
  endpoint.portName() = "portName";
  endpoint.expectedPortName() = "portName";

  // When neighbor is missing
  {
    std::shared_ptr<SwitchState> result = LinkConnectivityProcessor::process(
        *sw->getScopeResolver(),
        *sw->getHwAsicTable(),
        state,
        makeConnectivity(endpoint, portID));

    // Then
    auto port = result->getPorts()->getNodeIf(portID);
    CHECK(port);
    CHECK(port->getLedPortExternalState());
    CHECK(port->getLedPortExternalState().has_value());
    EXPECT_EQ(
        port->getLedPortExternalState().value(),
        PortLedExternalState::CABLING_ERROR);
    EXPECT_EQ(port->getActiveErrors().size(), 1);
    EXPECT_EQ(
        port->getActiveErrors().at(0), PortError::MISSING_EXPECTED_NEIGHBOR);
  }

  // When neighbor is restored
  endpoint.expectedSwitchId() = getSwitchId(switchType);
  {
    std::shared_ptr<SwitchState> result = LinkConnectivityProcessor::process(
        *sw->getScopeResolver(),
        *sw->getHwAsicTable(),
        state,
        makeConnectivity(endpoint, portID));

    // Then error should be cleared
    auto port = result->getPorts()->getNodeIf(portID);
    CHECK(port);
    CHECK(port->getLedPortExternalState());
    CHECK(port->getLedPortExternalState().has_value());
    EXPECT_EQ(
        port->getLedPortExternalState().value(), PortLedExternalState::NONE);
    EXPECT_EQ(port->getActiveErrors().size(), 0);
  }
}

TEST(LinkConnectivityProcessorTest, processMismatchedNeighbor) {
  auto switchType = cfg::SwitchType::NPU;
  cfg::SwitchConfig config = testConfigA(switchType);
  *config.ports()[0].routable() = true;
  config.ports()[0].Port::name() = "FooP0";
  config.ports()[0].Port::description() = "FooP0 Port Description here";
  config.ports()[0].expectedLLDPValues()[cfg::LLDPTag::SYSTEM_NAME] =
      "somesysname0";
  config.ports()[0].expectedLLDPValues()[cfg::LLDPTag::PORT_DESC] =
      "someportdesc0";

  auto handle = createTestHandle(&config, SwitchFlags::ENABLE_LLDP);
  auto sw = handle->getSw();
  auto state = sw->getState();

  // Prepare port and endpoint
  auto portID = PortID(1);
  auto endpoint = createFabricEndpoint(getSwitchId(switchType), portID);
  auto endpointCorrectSwitchName = endpoint.switchName().value();
  endpoint.switchName() = "mismatchedSwitchName";

  // When neighbor is mismatched
  {
    std::shared_ptr<SwitchState> result = LinkConnectivityProcessor::process(
        *sw->getScopeResolver(),
        *sw->getHwAsicTable(),
        state,
        makeConnectivity(endpoint, portID));

    // Then
    auto port = result->getPorts()->getNodeIf(portID);
    CHECK(port);
    CHECK(port->getLedPortExternalState());
    CHECK(port->getLedPortExternalState().has_value());
    EXPECT_EQ(
        port->getLedPortExternalState().value(),
        PortLedExternalState::CABLING_ERROR);
    EXPECT_EQ(port->getActiveErrors().size(), 1);
    EXPECT_EQ(port->getActiveErrors().at(0), PortError::MISMATCHED_NEIGHBOR);
  }

  // When port is fixed
  endpoint.switchName() = endpointCorrectSwitchName;
  {
    std::shared_ptr<SwitchState> result = LinkConnectivityProcessor::process(
        *sw->getScopeResolver(),
        *sw->getHwAsicTable(),
        state,
        makeConnectivity(endpoint, portID));

    // Then error should be cleared
    auto port = result->getPorts()->getNodeIf(portID);
    CHECK(port);
    CHECK(port->getLedPortExternalState());
    CHECK(port->getLedPortExternalState().has_value());
    EXPECT_EQ(
        port->getLedPortExternalState().value(), PortLedExternalState::NONE);
    EXPECT_EQ(port->getActiveErrors().size(), 0);
  }
}

} // namespace
