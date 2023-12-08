// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/platforms/common/meru400bfu/Meru400bfuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400bia/Meru400biaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400biu/Meru400biuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"

using facebook::fboss::DeltaFunctions::forEachAdded;
using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::forEachRemoved;

namespace {

using namespace facebook::fboss;

static const PlatformMapping* FOLLY_NULLABLE
getPlatformMappingForDsfNode(const PlatformType platformType) {
  switch (platformType) {
    case PlatformType::PLATFORM_MERU400BIU: {
      static Meru400biuPlatformMapping meru400biu;
      return &meru400biu;
    }
    case PlatformType::PLATFORM_MERU400BIA: {
      static Meru400biaPlatformMapping meru400bia;
      return &meru400bia;
    }
    case PlatformType::PLATFORM_MERU400BFU: {
      static Meru400bfuPlatformMapping meru400bfu;
      return &meru400bfu;
    }
    case PlatformType::PLATFORM_MERU800BFA: {
      static Meru800bfaPlatformMapping meru800bfa;
      return &meru800bfa;
    }
    case PlatformType::PLATFORM_MERU800BIA: {
      static Meru800biaPlatformMapping meru800bia;
      return &meru800bia;
    }
    default:
      break;
  }
  return nullptr;
}

uint32_t getRemotePortOffset(const PlatformType platformType) {
  switch (platformType) {
    case PlatformType::PLATFORM_MERU400BIU:
      return 256;
    case PlatformType::PLATFORM_MERU400BIA:
      return 256;
    case PlatformType::PLATFORM_MERU400BFU:
      return 0;
    case PlatformType::PLATFORM_MERU800BFA:
      return 0;
    case PlatformType::PLATFORM_MERU800BIA:
      return 1024;

    default:
      return 0;
  }
  return 0;
}
} // namespace

namespace facebook::fboss {

void FabricConnectivityManager::updateExpectedSwitchIdAndPortIdForPort(
    PortID portID) {
  auto& fabricEndpoint = currentNeighborConnectivity_[portID];
  if (!fabricEndpoint.expectedSwitchName().has_value() ||
      !fabricEndpoint.expectedPortName().has_value()) {
    return;
  }

  auto expectedSwitchName = fabricEndpoint.expectedSwitchName().value();
  auto expectedPortName = fabricEndpoint.expectedPortName().value();

  auto it = switchNameToSwitchIDs_.find(expectedSwitchName);
  if (it == switchNameToSwitchIDs_.end()) {
    fabricEndpoint.expectedSwitchId().reset();
    fabricEndpoint.expectedPortId().reset();
    return;
  }

  if (it->second.size() == 0) {
    throw FbossError("No switchID for switch: ", expectedSwitchName);
  }

  auto baseSwitchId = *it->second.begin();
  const auto platformMapping = getPlatformMappingForDsfNode(
      switchIdToDsfNode_[baseSwitchId]->getPlatformType());

  if (!platformMapping) {
    throw FbossError("Unable to find platform mapping for port: ", portID);
  }

  auto virtualDeviceId = platformMapping->getVirtualDeviceID(expectedPortName);
  if (!virtualDeviceId.has_value()) {
    throw FbossError("Unable to find virtual device id for port: ", portID);
  }

  fabricEndpoint.expectedSwitchId() = baseSwitchId + virtualDeviceId.value();
  fabricEndpoint.expectedPortId() =
      platformMapping->getPortID(expectedPortName) -
      getRemotePortOffset(switchIdToDsfNode_[baseSwitchId]->getPlatformType());

  XLOG(DBG2) << "Local port: " << static_cast<int>(portID)
             << " Expected Peer SwitchName: " << expectedSwitchName
             << " Expected Peer PortName: " << expectedPortName
             << " Expected Peer SwitchID: "
             << fabricEndpoint.expectedSwitchId().value()
             << " Expected Peer PortID: "
             << fabricEndpoint.expectedPortId().value();
}

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
  updateExpectedSwitchIdAndPortIdForPort(swPort->getID());
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

  /*
   * If the dsfNodeMap is updated, switchIdToDsfNode_ and switchNameToSwitchIDs_
   * would be updated. Thus, recompute expected {switchId, portId} for ports
   * from expected {switchName, portName}.
   */
  if (!DeltaFunctions::isEmpty(delta.getDsfNodesDelta())) {
    for (const auto& [portID, fabricEndpoint] : currentNeighborConnectivity_) {
      updateExpectedSwitchIdAndPortIdForPort(portID);
    }
  }
}

void FabricConnectivityManager::stateUpdated(const StateDelta& delta) {
  updatePorts(delta);
  updateDsfNodes(delta);
}

FabricEndpoint FabricConnectivityManager::processConnectivityInfoForPort(
    const PortID& portId,
    const FabricEndpoint& hwEndpoint) {
  const auto& iter = currentNeighborConnectivity_.find(portId);
  if (iter != currentNeighborConnectivity_.end()) {
    // Populate actual isAttached, switchId, switchName, portId, portName
    iter->second.isAttached() = *hwEndpoint.isAttached();
    iter->second.switchId() = *hwEndpoint.switchId();
    iter->second.portId() = *hwEndpoint.portId();

    // updateExpectedSwitchIdAndPortIdForPort uses platform, virtualDeviceId to
    // derive expected{Switch, Port}Id from expected{Switch, Port}Name.
    // Thus, if actual{Switch, Port}Id matches expected{Switch, Port}Id,
    // expected matches actual and thus we can use expected{Switch, Port}Name to
    // populate actual{Switch, Port}Name.

    if (iter->second.expectedSwitchId().has_value() &&
        iter->second.expectedSwitchId().value() == iter->second.switchId() &&
        iter->second.expectedSwitchName().has_value()) {
      iter->second.switchName() = iter->second.expectedSwitchName().value();
    }

    if (iter->second.expectedPortId().has_value() &&
        iter->second.expectedPortId().value() == iter->second.portId() &&
        iter->second.expectedPortName().has_value()) {
      iter->second.portName() = iter->second.expectedPortName().value();
    }
  }

  return iter->second;
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
