// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricReachabilityManager.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/common/meru400bfu/Meru400bfuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400biu/Meru400biuPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CFabricPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CVoqPlatformMapping.h"

using facebook::fboss::DeltaFunctions::forEachAdded;
using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::forEachRemoved;

namespace facebook::fboss {

void FabricReachabilityManager::addPort(const std::shared_ptr<Port> swPort) {
  expectedNeighborReachability_.emplace(
      swPort->getID(), swPort->getExpectedNeighborValues()->toThrift());
}

void FabricReachabilityManager::removePort(const std::shared_ptr<Port> swPort) {
  expectedNeighborReachability_.erase(swPort->getID());
}

void FabricReachabilityManager::addDsfNode(
    const std::shared_ptr<DsfNode> dsfNode) {
  dsfNodes_.emplace(dsfNode->getName(), dsfNode);
}

void FabricReachabilityManager::removeDsfNode(
    const std::shared_ptr<DsfNode> dsfNode) {
  dsfNodes_.erase(dsfNode->getName());
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

static PlatformMapping* FOLLY_NULLABLE getPlatformMappingForDsfNode(
    const cfg::AsicType asicType,
    const cfg::SwitchType switchType,
    int* remotePortOffset) {
  static Meru400biuPlatformMapping meru400biu;
  static Meru400bfuPlatformMapping meru400bfu;
  static Wedge400CVoqPlatformMapping w400cVoq;
  static Wedge400CFabricPlatformMapping w400cFabric;

  *remotePortOffset = 0;
  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_EBRO:
      CHECK(
          (switchType == cfg::SwitchType::VOQ) ||
          (switchType == cfg::SwitchType::FABRIC));
      if (switchType == cfg::SwitchType::VOQ) {
        return &w400cVoq;
      } else {
        return &w400cFabric;
      }
      break;
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
      *remotePortOffset = 256;
      return &meru400biu;
      break;
    case cfg::AsicType::ASIC_TYPE_RAMON:
      return &meru400bfu;
      break;
    default:
      break;
  }
  return nullptr;
}

FabricEndpoint FabricReachabilityManager::processReachabilityInfoForPort(
    const PortID& portId,
    const FabricEndpoint& hwEndpoint) {
  FabricEndpoint endpoint = hwEndpoint;
  // check if we have info on this endopint in our db, if not just return as it
  // evaluate the endpoint only if we have expected reachability for it or
  // if its attached
  const auto& iter = expectedNeighborReachability_.find(portId);
  if (*endpoint.isAttached() && iter != expectedNeighborReachability_.end()) {
    for (const auto& nbr : iter->second) {
      auto neighborSystem = *nbr.remoteSystem();
      auto neighborPortName = *nbr.remotePort();
      auto dsfNodeIter = dsfNodes_.find(neighborSystem);
      CHECK(dsfNodeIter != dsfNodes_.end());

      int remotePortOffset = 0;
      const auto platformMapping = getPlatformMappingForDsfNode(
          dsfNodeIter->second->getAsicType(),
          *endpoint.switchType(),
          &remotePortOffset);

      if (!platformMapping) {
        // no reachability info
        XLOG(ERR) << "Unable to find platform mapping for neighboring port: "
                  << *endpoint.portId();
        continue;
      }

      PortID neighborPortId = platformMapping->getPortID(neighborPortName);
      neighborPortId -= remotePortOffset;

      endpoint.expectedSwitchId() = dsfNodeIter->second->getSwitchId();
      endpoint.expectedPortId() = neighborPortId;
      endpoint.switchName() = neighborSystem;
      endpoint.portName() = neighborPortName;
      // for now its one fabric endpoint only
      break;
    }
  }

  // update the cache
  actualNeighborReachability_[portId] = endpoint;
  return endpoint;
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
