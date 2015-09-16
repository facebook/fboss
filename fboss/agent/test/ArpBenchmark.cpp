/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <boost/cast.hpp>

#include <folly/Benchmark.h>
#include <folly/Memory.h>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/hw/sim/SimPlatform.h"
#include "fboss/agent/hw/sim/SimSwitch.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

using namespace facebook::fboss;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::MacAddress;
using folly::make_unique;
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;

namespace {

// Global state used by the benchmarks
unique_ptr<SwSwitch> sw;
unique_ptr<MockRxPacket> arpRequest_10_0_0_1;
unique_ptr<MockRxPacket> arpRequest_10_0_0_5;

unique_ptr<SwSwitch> setupSwitch() {
  MacAddress localMac("02:00:01:00:00:01");
  auto sw = make_unique<SwSwitch>(make_unique<SimPlatform>(localMac, 10));
  sw->init();

  auto updateFn = [&](const shared_ptr<SwitchState>& oldState) {
    auto state = oldState->clone();

    // Add VLAN 1, and ports 1-9 which belong to it.
    auto vlan1 = make_shared<Vlan>(VlanID(1), "Vlan1");
    state->addVlan(vlan1);
    for (int idx = 1; idx < 10; ++idx) {
      vlan1->addPort(PortID(idx), false);
    }
    // Add Interface 1 to VLAN 1
    auto intf1 = make_shared<Interface>
      (InterfaceID(1), RouterID(0), VlanID(1),
       "interface1", MacAddress("02:00:01:00:00:01"), 9000);
    Interface::Addresses addrs1;
    addrs1.emplace(IPAddress("10.0.0.1"), 24);
    addrs1.emplace(IPAddress("192.168.0.1"), 24);
    intf1->setAddresses(addrs1);
    state->addIntf(intf1);

    // Set up an arp response table for VLAN 1 with entries for
    // 10.0.0.1 and 192.168.0.1
    auto respTable1 = make_shared<ArpResponseTable>();
    respTable1->setEntry(IPAddressV4("10.0.0.1"),
                         MacAddress("00:02:00:00:00:01"),
                         InterfaceID(3));
    respTable1->setEntry(IPAddressV4("192.168.0.1"),
                         MacAddress("00:02:00:00:00:02"),
                         InterfaceID(4));
    state->getVlans()->getVlan(VlanID(1))->setArpResponseTable(respTable1);
    return state;
  };

  sw->updateStateBlocking("setup", updateFn);
  return sw;
}

void init() {
  // Initialize the switch
  sw = setupSwitch();

  // Create an ARP request for 10.0.0.1
  arpRequest_10_0_0_1 = MockRxPacket::fromHex(
      // dst mac, src mac
      "ff ff ff ff ff ff  00 02 00 01 02 03"
      // 802.1q, VLAN 1
      "81 00  00 01"
      // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
      "08 06  00 01  08 00  06  04"
      // ARP Request
      "00 01"
      // Sender MAC
      "00 02 00 01 02 03"
      // Sender IP: 10.0.0.15
      "0a 00 00 0f"
      // Target MAC
      "00 00 00 00 00 00"
      // Target IP: 10.0.0.1
      "0a 00 00 01"
      );
  arpRequest_10_0_0_1->padToLength(68);
  arpRequest_10_0_0_1->setSrcPort(PortID(1));
  arpRequest_10_0_0_1->setSrcVlan(VlanID(1));

  // Create an ARP request for 10.0.0.5
  arpRequest_10_0_0_5 = MockRxPacket::fromHex(
      // dst mac, src mac
      "ff ff ff ff ff ff  00 02 00 01 02 03"
      // 802.1q, VLAN 1
      "81 00  00 01"
      // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
      "08 06  00 01  08 00  06  04"
      // ARP Request
      "00 01"
      // Sender MAC
      "00 02 00 01 02 03"
      // Sender IP: 10.0.0.15
      "0a 00 00 0f"
      // Target MAC
      "00 00 00 00 00 00"
      // Target IP: 10.0.0.5
      "0a 00 00 05"
      );
  arpRequest_10_0_0_5->padToLength(68);
  arpRequest_10_0_0_5->setSrcPort(PortID(1));
  arpRequest_10_0_0_5->setSrcVlan(VlanID(1));
}

} // unnamed namespace

BENCHMARK(ArpRequest, numIters) {
  BENCHMARK_SUSPEND {
    SimSwitch* sim = boost::polymorphic_downcast<SimSwitch*>(sw->getHw());
    sim->resetTxCount();
  }

  // Send the packet to the switch numIters times
  for (size_t n = 0; n < numIters; ++n) {
    sw->packetReceived(arpRequest_10_0_0_1->clone());
  }

  BENCHMARK_SUSPEND {
    // Make sure the SwSwitch sent out 1 packet for each iteration,
    // just to verify that it was actually sending ARP replies
    SimSwitch* sim = boost::polymorphic_downcast<SimSwitch*>(sw->getHw());
    CHECK_EQ(sim->getTxCount(), numIters);
  }
}

BENCHMARK(ArpRequestNotMine, numIters) {
  BENCHMARK_SUSPEND {
    SimSwitch* sim = boost::polymorphic_downcast<SimSwitch*>(sw->getHw());
    sim->resetTxCount();
  }

  // Send the packet to the switch numIters times
  for (size_t n = 0; n < numIters; ++n) {
    sw->packetReceived(arpRequest_10_0_0_5->clone());
  }

  BENCHMARK_SUSPEND {
    // This request wasn't for one of our IPs, so no outgoing packets
    // should have been generated.
    SimSwitch* sim = boost::polymorphic_downcast<SimSwitch*>(sw->getHw());
    CHECK_EQ(sim->getTxCount(), 0);
  }
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  // Setting up the switch is fairly expensive.  Do this once before we run the
  // benchmark functions so we don't have to do it inside the benchmark
  // functions.
  //
  // (We could do the initialization inside the benchmark functions using
  // BENCHMARK_SUSPEND, but the packet handling code is cheap enough that even
  // the small overhead of BENCHMARK_SUSPEND negatively impacts the results.)
  init();

  folly::runBenchmarks();
  return 0;
}
