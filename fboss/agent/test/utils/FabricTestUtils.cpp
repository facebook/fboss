/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/lib/CommonUtils.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

void checkFabricConnectivity(TestEnsembleIf* ensemble, SwitchID switchId) {
  auto expectedSwitchType =
      ensemble->getHwAsicTable()->getHwAsic(switchId)->getSwitchType();
  WITH_RETRIES({
    ensemble->updateStats();
    auto reachability = ensemble->getFabricConnectivity(switchId);
    EXPECT_EVENTUALLY_GT(reachability.size(), 0);
    for (auto [port, endpoint] : reachability) {
      if (!*endpoint.isAttached()) {
        continue;
      }

      int expectedPortId = -1;
      int expectedSwitchId = -1;

      if (endpoint.expectedPortId().has_value()) {
        expectedPortId = *endpoint.expectedPortId();
      }
      if (endpoint.expectedSwitchId().has_value()) {
        expectedSwitchId = *endpoint.expectedSwitchId();
      }

      XLOG(DBG2) << " On port: " << port
                 << " expected switch id: " << expectedSwitchId
                 << " got switch id: " << *endpoint.switchId()
                 << " expected port id: " << expectedPortId
                 << " got port id: " << *endpoint.portId()
                 << " got switch type: "
                 << apache::thrift::util::enumNameSafe(*endpoint.switchType())
                 << " expected switch type: "
                 << apache::thrift::util::enumNameSafe(expectedSwitchType);

      EXPECT_EVENTUALLY_EQ(*endpoint.switchId(), expectedSwitchId);
      EXPECT_EVENTUALLY_EQ(*endpoint.portId(), expectedPortId);
      EXPECT_EVENTUALLY_EQ(*endpoint.switchType(), expectedSwitchType);
    }

    EXPECT_EVENTUALLY_EQ(
        *(ensemble->getFabricReachabilityStats().mismatchCount()), 0);
    EXPECT_EVENTUALLY_EQ(
        *(ensemble->getFabricReachabilityStats().missingCount()), 0);
  });
}

void populatePortExpectedNeighborsToSelf(
    const std::vector<PortID>& ports,
    cfg::SwitchConfig& cfg) {
  const auto& dsfNode = cfg.dsfNodes()->begin()->second;
  for (const auto& portID : ports) {
    auto portCfg = findCfgPort(cfg, portID);
    cfg::PortNeighbor nbr;
    if (portCfg->portType() == cfg::PortType::FABRIC_PORT) {
      // this is for neighbor reachability test. Since
      // ports are in loopback, expect itself to be the neighbor
      // hence put remotePort to be the same as itself
      // expect port name to exist for the fabric ports
      if (portCfg->name().has_value()) {
        nbr.remotePort() = *portCfg->name();
      }
      nbr.remoteSystem() = *dsfNode.name();
      portCfg->expectedNeighborReachability() = {nbr};
    }
  }
}

void populatePortExpectedNeighborsToRemote(
    const std::vector<PortID>& ports,
    cfg::SwitchConfig& cfg,
    const std::vector<int>& remoteSwitchIds,
    int numParallelLinks) {
  CHECK_LE(remoteSwitchIds.size() * numParallelLinks, ports.size());
  auto portIter = ports.begin();
  for (int remoteSwitchId : remoteSwitchIds) {
    const auto& dsfNode = cfg.dsfNodes()->at(remoteSwitchId);
    for (int i = 0; i < numParallelLinks; i++) {
      CHECK(portIter != ports.end());
      auto portCfg = findCfgPort(cfg, *portIter);
      CHECK(portCfg->portType() == cfg::PortType::FABRIC_PORT);
      cfg::PortNeighbor nbr;
      if (portCfg->name().has_value()) {
        nbr.remotePort() = *portCfg->name();
      }
      nbr.remoteSystem() = *dsfNode.name();
      portCfg->expectedNeighborReachability() = {nbr};
      portIter++;
    }
  }
}

void checkPortFabricReachability(
    TestEnsembleIf* ensemble,
    SwitchID switchId,
    PortID portId) {
  WITH_RETRIES({
    ensemble->updateStats();
    auto reachability = ensemble->getFabricConnectivity(switchId);
    auto itr = reachability.find(portId);
    EXPECT_EVENTUALLY_TRUE(itr != reachability.end());
    auto endpoint = itr->second;
    EXPECT_EVENTUALLY_TRUE(*endpoint.isAttached());
    XLOG(DBG2) << " On port: " << portId
               << " got switch id: " << *endpoint.switchId();
    EXPECT_EVENTUALLY_EQ(
        SwitchID(*endpoint.switchId()),
        ensemble->scopeResolver().scope(portId).switchId());
    EXPECT_EVENTUALLY_EQ(
        *endpoint.switchType(),
        ensemble->getHwAsicTable()
            ->getHwAsic(SwitchID(*endpoint.switchId()))
            ->getSwitchType());
  });
}

void checkFabricPortsActiveState(
    TestEnsembleIf* ensemble,
    const std::vector<PortID>& fabricPortIds,
    bool expectActive) {
  WITH_RETRIES({
    for (const auto& portId : fabricPortIds) {
      auto fabricPort =
          ensemble->getProgrammedState()->getPorts()->getNodeIf(portId);
      EXPECT_EVENTUALLY_TRUE(fabricPort->isActive().has_value());
      EXPECT_EVENTUALLY_EQ(*fabricPort->isActive(), expectActive);
    }
  });
}
} // namespace facebook::fboss::utility
