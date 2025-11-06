/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_hw_tests/AgentQosTestBase.h"

namespace facebook::fboss {

void AgentQosTestBase::verifyDscpQueueMapping(
    const std::map<int, std::vector<uint8_t>>& queueToDscp) {
  auto setup = [=, this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    resolveNeighborAndProgramRoutes(ecmpHelper, kEcmpWidth);
  };

  auto verify = [=, this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto portId = ecmpHelper.ecmpPortDescriptorAt(0).phyPortID();
    std::optional<SystemPortID> sysPortId;
    if (getSw()->getSwitchInfoTable().haveVoqSwitches()) {
      auto switchId = switchIdForPort(portId);
      sysPortId = getSystemPortID(portId, getProgrammedState(), switchId);
    }
    for (bool frontPanel : {false, true}) {
      XLOG(DBG2) << "verify send packets "
                 << (frontPanel ? "out of port" : "switched");
      utility::sendPktAndVerifyQueueHit(
          queueToDscp,
          getSw(),
          [this, frontPanel](int dscp) { sendPacket(dscp, frontPanel); },
          portId,
          sysPortId);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

void AgentQosTestBase::sendPacket(uint8_t dscp, bool frontPanel) {
  auto vlanId = getVlanIDForTx();
  auto intfMac =
      utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
  auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
  auto txPacket = utility::makeUDPTxPacket(
      getSw(),
      vlanId,
      srcMac, // src mac
      intfMac, // dst mac
      folly::IPAddressV6("2620:0:1cfe:face:b00c::3"), // src ip
      folly::IPAddressV6("2620:0:1cfe:face:b00c::4"), // dst ip
      8000, // l4 src port
      8001, // l4 dst port
      dscp << 2, // shifted by 2 bits for ECN
      255 // ttl
  );
  // port is in LB mode, so it will egress and immediately loop back.
  // Since it is not re-written, it should hit the pipeline as if it
  // ingressed on the port, and be properly queued.
  if (frontPanel) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto outPort = ecmpHelper.ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
    getSw()->sendPacketOutOfPortAsync(std::move(txPacket), outPort);
  } else {
    getSw()->sendPacketSwitchedAsync(std::move(txPacket));
  }
}

} // namespace facebook::fboss
