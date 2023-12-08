// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

using facebook::fboss::DeltaFunctions::forEachAdded;
using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::forEachRemoved;

namespace facebook::fboss {

void FabricConnectivityManager::addPort(const std::shared_ptr<Port>& swPort) {
  // Non-Faric port connectivity is handled by LLDP
  if (swPort->getPortType() != cfg::PortType::FABRIC_PORT) {
    return;
  }

  FabricEndpoint expectedEndpoint;
  const auto& expectedNeighbors =
      swPort->getExpectedNeighborValues()->toThrift();
  if (expectedNeighbors.size()) {
    CHECK_EQ(expectedNeighbors.size(), 1);
    const auto& neighbor = expectedNeighbors.front();
    expectedEndpoint.expectedSwitchName() = *neighbor.remoteSystem();
    expectedEndpoint.expectedPortName() = *neighbor.remotePort();
  }

  currentNeighborConnectivity_[swPort->getID()] = expectedEndpoint;
}

void FabricConnectivityManager::removePort(
    const std::shared_ptr<Port>& swPort) {
  currentNeighborConnectivity_.erase(swPort->getID());
}

void FabricConnectivityManager::addDsfNode(
    const std::shared_ptr<DsfNode>& dsfNode) {
  switchIdToDsfNode_[dsfNode->getID()] = dsfNode;
  switchNameToSwitchIDs_[dsfNode->getName()].insert(dsfNode->getSwitchId());
}

void FabricConnectivityManager::removeDsfNode(
    const std::shared_ptr<DsfNode>& dsfNode) {
  switchIdToDsfNode_.erase(dsfNode->getID());
  switchNameToSwitchIDs_[dsfNode->getName()].erase(dsfNode->getSwitchId());
}

void FabricConnectivityManager::updatePorts(const StateDelta& delta) {
  forEachChanged(
      delta.getPortsDelta(),
      [&](const std::shared_ptr<Port>& oldSwPort,
          const std::shared_ptr<Port>& newSwPort) {
        removePort(oldSwPort);
        addPort(newSwPort);
      },
      [&](const std::shared_ptr<Port>& newPort) { addPort(newPort); },
      [&](const std::shared_ptr<Port>& deletePort) { removePort(deletePort); });
}

void FabricConnectivityManager::updateDsfNodes(const StateDelta& delta) {
  forEachChanged(
      delta.getDsfNodesDelta(),
      [&](const std::shared_ptr<DsfNode> oldDsfNode,
          const std::shared_ptr<DsfNode> newDsfNode) {
        removeDsfNode(oldDsfNode);
        addDsfNode(newDsfNode);
      },
      [&](const std::shared_ptr<DsfNode> newDsfNode) {
        addDsfNode(newDsfNode);
      },
      [&](const std::shared_ptr<DsfNode> deleteDsfNode) {
        removeDsfNode(deleteDsfNode);
      });
}

void FabricConnectivityManager::stateUpdated(const StateDelta& delta) {
  updatePorts(delta);
  updateDsfNodes(delta);
}

FabricEndpoint FabricConnectivityManager::processConnectivityInfoForPort(
    const PortID& portId,
    const FabricEndpoint& hwEndpoint) {
  return FabricEndpoint{};
}

bool FabricConnectivityManager::isConnectivityInfoMismatch(
    const PortID& portId) {
  return true;
}

bool FabricConnectivityManager::isConnectivityInfoMissing(
    const PortID& portId) {
  return false;
}

std::map<PortID, FabricEndpoint>
FabricConnectivityManager::getConnectivityInfo() {
  return {};
}

} // namespace facebook::fboss
