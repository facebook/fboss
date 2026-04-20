// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/CpuLatencyManager.h"

#include <gtest/gtest.h>
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

namespace {

// Mark a port as eligible for monitoring by giving it a neighbor reachability
// entry.
void makeEligiblePort(std::shared_ptr<SwitchState>& state, PortID portId) {
  auto portMap = state->getPorts()->modify(&state);
  auto port = portMap->getNodeIf(portId)->modify(&state);
  cfg::PortNeighbor neighbor;
  neighbor.remoteSystem() = "router1";
  neighbor.remotePort() = "eth0/1/1";
  port->setExpectedNeighborReachability({neighbor});
}

} // namespace

class CpuLatencyManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // testStateAWithPortsUp() gives us 20 UP INTERFACE_PORT ports.
    // Ports 1-10 are on Interface 1 (VLAN 1), ports 11-20 on Interface 55.
    auto state = testStateAWithPortsUp();
    // Mark ports 1 and 2 as eligible (with neighbor reachability).
    makeEligiblePort(state, PortID(1));
    makeEligiblePort(state, PortID(2));
    handle_ = createTestHandle(state);
    sw_ = handle_->getSw();

    // Add NDP neighbor entries via updateStateBlocking so
    // sendProbePackets() can resolve neighbor MACs without test mode.
    sw_->updateStateBlocking(
        "add NDP neighbors", [](const std::shared_ptr<SwitchState>& state) {
          auto newState = state->clone();
          auto ndpTable = newState->getInterfaces()
                              ->getNode(InterfaceID(1))
                              ->getNdpTable()
                              ->modify(InterfaceID(1), &newState);
          ndpTable->addEntry(
              folly::IPAddressV6("2401:db00:2110:3001::22"),
              folly::MacAddress("02:09:00:00:00:22"),
              PortDescriptor(PortID(1)),
              InterfaceID(1),
              NeighborState::REACHABLE);
          ndpTable->addEntry(
              folly::IPAddressV6("2401:db00:2110:3001::33"),
              folly::MacAddress("02:09:00:00:00:33"),
              PortDescriptor(PortID(2)),
              InterfaceID(1),
              NeighborState::REACHABLE);
          return newState;
        });
  }

  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_{nullptr};
};

} // namespace facebook::fboss
