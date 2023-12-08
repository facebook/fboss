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
