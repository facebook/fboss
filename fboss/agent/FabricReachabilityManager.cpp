// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricReachabilityManager.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/common/cloud_ripper/CloudRipperFabricPlatformMapping.h"
#include "fboss/agent/platforms/common/cloud_ripper/CloudRipperVoqPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400bfu/Meru400bfuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400bia/Meru400biaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400biu/Meru400biuPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CFabricPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CVoqPlatformMapping.h"

using facebook::fboss::DeltaFunctions::forEachAdded;
using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::forEachRemoved;

namespace facebook::fboss {

void FabricReachabilityManager::addPort(const std::shared_ptr<Port>& swPort) {
  FabricEndpoint expectedEndpoint;
  const auto& neighborReachabilityList =
      swPort->getExpectedNeighborValues()->toThrift();
  if (neighborReachabilityList.size()) {
    // pick the first one, as we will only have 1 for now
    const auto& nbr = neighborReachabilityList.front();
    expectedEndpoint.expectedSwitchName() = *nbr.remoteSystem();
    expectedEndpoint.expectedPortName() = *nbr.remotePort();
  }
  actualNeighborReachability_[swPort->getID()] = expectedEndpoint;
}

void FabricReachabilityManager::removePort(
    const std::shared_ptr<Port>& swPort) {
  actualNeighborReachability_.erase(swPort->getID());
}

void FabricReachabilityManager::addDsfNode(
    const std::shared_ptr<DsfNode>& dsfNode) {
  dsfNameToNodesMap_[dsfNode->getName()] = dsfNode;
  dsfSwitchIdToNodesMap_[dsfNode->getSwitchId()] = dsfNode;
}

void FabricReachabilityManager::removeDsfNode(
    const std::shared_ptr<DsfNode>& dsfNode) {
  dsfNameToNodesMap_.erase(dsfNode->getName());
  dsfSwitchIdToNodesMap_.erase(dsfNode->getSwitchId());
}

void FabricReachabilityManager::updatePorts(const StateDelta& delta) {
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

void FabricReachabilityManager::updateDsfNodes(const StateDelta& delta) {
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

void FabricReachabilityManager::stateUpdated(const StateDelta& delta) {
  updatePorts(delta);
  updateDsfNodes(delta);
}

static const PlatformMapping* FOLLY_NULLABLE getPlatformMappingForDsfNode(
    const cfg::AsicType asicType,
    const PlatformType platformType,
    int* remotePortOffset) {
  static Meru400biuPlatformMapping meru400biu;
  static Meru400biaPlatformMapping meru400bia;
  static Meru400bfuPlatformMapping meru400bfu;
  static Wedge400CVoqPlatformMapping w400cVoq;
  static Wedge400CFabricPlatformMapping w400cFabric;
  static CloudRipperFabricPlatformMapping cloudRipperFabric;
  static CloudRipperVoqPlatformMapping cloudRipperVoq;

  *remotePortOffset = 0;
  switch (platformType) {
    case PlatformType::PLATFORM_WEDGE400C_VOQ:
      return &w400cVoq;
    case PlatformType::PLATFORM_WEDGE400C_FABRIC:
      return &w400cFabric;
    case PlatformType::PLATFORM_CLOUDRIPPER_VOQ:
      return &cloudRipperVoq;
    case PlatformType::PLATFORM_CLOUDRIPPER_FABRIC:
      return &cloudRipperFabric;
    case PlatformType::PLATFORM_MERU400BIU:
      *remotePortOffset = 256;
      return &meru400biu;
    case PlatformType::PLATFORM_MERU400BIA:
      *remotePortOffset = 256;
      return &meru400bia;
      break;
    case PlatformType::PLATFORM_MERU400BFU:
      return &meru400bfu;
      break;
    default:
      break;
  }
  return nullptr;
}

void FabricReachabilityManager::updateExpectedPortId(
    FabricEndpoint& endpoint,
    const cfg::AsicType asicType,
    const PlatformType platformType,
    const cfg::DsfNodeType dsfNodeType) {
  int remotePortOffset = 0;

  CHECK(
      (dsfNodeType == cfg::DsfNodeType::FABRIC_NODE) ||
      (dsfNodeType == cfg::DsfNodeType::INTERFACE_NODE));

  const auto platformMapping =
      getPlatformMappingForDsfNode(asicType, platformType, &remotePortOffset);
  if (!platformMapping) {
    // no reachability info
    XLOG(ERR) << "Unable to find platform mapping for neighboring port: "
              << *endpoint.portId();
    return;
  }

  if (endpoint.expectedPortName().has_value()) {
    // throw an exception if port name not found
    endpoint.expectedPortId() =
        platformMapping->getPortID(*endpoint.expectedPortName());
  }
}

void FabricReachabilityManager::updatePortName(
    FabricEndpoint& endpoint,
    const cfg::AsicType asicType,
    const PlatformType platformType) {
  int remotePortOffset = 0;
  const auto platformMapping =
      getPlatformMappingForDsfNode(asicType, platformType, &remotePortOffset);
  if (!platformMapping) {
    // no reachability info
    XLOG(ERR) << "Unable to find platform mapping for neighboring port: "
              << *endpoint.portId();
    return;
  }

  endpoint.portId() = *endpoint.portId() + remotePortOffset;
  const auto& platformPorts = platformMapping->getPlatformPorts();
  auto pitr = platformPorts.find(*endpoint.portId());
  if (pitr != platformPorts.end()) {
    // as per the hw info obtained
    endpoint.portName() = *pitr->second.mapping()->name();
  }
}

void FabricReachabilityManager::updateExpectedFabricEndpointInfo(
    FabricEndpoint& resEndpoint,
    const FabricEndpoint& expectedEndpoint) {
  if (expectedEndpoint.expectedPortName().has_value()) {
    resEndpoint.expectedPortName() = *expectedEndpoint.expectedPortName();
  }
  // update expectedSwitchName/Id
  if (expectedEndpoint.expectedSwitchName().has_value()) {
    resEndpoint.expectedSwitchName() = *expectedEndpoint.expectedSwitchName();
    auto dsfNodeIter =
        dsfNameToNodesMap_.find(*resEndpoint.expectedSwitchName());
    // get expected neighbor info based on cfg
    if (dsfNodeIter == dsfNameToNodesMap_.end()) {
      XLOG(WARNING)
          << "Unable to find the expected endpoint switchname in the dsf map "
          << *resEndpoint.expectedSwitchName();
      return;
    }
    resEndpoint.expectedSwitchId() = dsfNodeIter->second->getSwitchId();
    // get asic type is from dsfnode based on expected neighbor
    updateExpectedPortId(
        resEndpoint,
        dsfNodeIter->second->getAsicType(),
        dsfNodeIter->second->getPlatformType(),
        dsfNodeIter->second->getType());
  }
}

void FabricReachabilityManager::updateHwFabricEndpointInfo(
    FabricEndpoint& resEndpoint) {
  // no update is possible if endpoint is not attached
  if (*resEndpoint.isAttached()) {
    // get neighbor info based on HW, better exist in cfg
    auto dsfSwitchIdToNodeIter =
        dsfSwitchIdToNodesMap_.find(*resEndpoint.switchId());
    if (dsfSwitchIdToNodeIter == dsfSwitchIdToNodesMap_.end()) {
      XLOG(WARNING)
          << "Unable to find the endpoint id as from HW in the dsf map: "
          << *resEndpoint.switchId();
      return;
    }
    resEndpoint.switchName() = dsfSwitchIdToNodeIter->second->getName();
    // get asic type from dsfNode based on switchId from HW
    updatePortName(
        resEndpoint,
        dsfSwitchIdToNodeIter->second->getAsicType(),
        dsfSwitchIdToNodeIter->second->getPlatformType());
  }
}

FabricEndpoint FabricReachabilityManager::processReachabilityInfoForPort(
    const PortID& portId,
    const FabricEndpoint& hwEndpoint) {
  FabricEndpoint resEndpoint = hwEndpoint;
  // check if we have info on this endopint in our db, if not just return as it
  // evaluate the endpoint only if we have expected reachability for it or
  const auto& iter = actualNeighborReachability_.find(portId);
  if (iter != actualNeighborReachability_.end()) {
    const auto& expectedEndpoint = iter->second;
    updateExpectedFabricEndpointInfo(resEndpoint, expectedEndpoint);
    updateHwFabricEndpointInfo(resEndpoint);
  }

  // update the cache
  actualNeighborReachability_[portId] = resEndpoint;
  return resEndpoint;
}

std::map<PortID, FabricEndpoint>
FabricReachabilityManager::processReachabilityInfo(
    const std::map<PortID, FabricEndpoint>& hwReachability) {
  // use the hw reachability + expected eachability info to derive if there is a
  // match or not
  for (const auto& hwReachabilityEntry : hwReachability) {
    processReachabilityInfoForPort(
        hwReachabilityEntry.first, hwReachabilityEntry.second);
  }
  return actualNeighborReachability_;
}

std::map<PortID, FabricEndpoint>
FabricReachabilityManager::getReachabilityInfo() {
  // use the hw reachability + expected eachability info to derive if there is a
  // match or not, get from the cached version
  return actualNeighborReachability_;
}

} // namespace facebook::fboss
