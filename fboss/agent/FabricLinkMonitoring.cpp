// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricLinkMonitoring.h"

#include "fboss/agent/Utils.h"
#include "fboss/agent/VoqUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

// Constructor initializes the fabric link monitoring system by processing
// DSF nodes and link information from the switch configuration
FabricLinkMonitoring::FabricLinkMonitoring(const cfg::SwitchConfig* config)
    : isVoqSwitch_(
          *config->switchSettings()->switchType() == cfg::SwitchType::VOQ) {
  processDsfNodes(config);
  processLinkInfo(config);
  allocateSwitchIdForPorts(config);
}

// Returns mapping from port IDs to their corresponding switch IDs
const std::map<PortID, SwitchID>&
FabricLinkMonitoring::getPort2LinkSwitchIdMapping() const {
  return portId2LinkSwitchId_;
}

// Returns the switchID for a specific portId
SwitchID FabricLinkMonitoring::getSwitchIdForPort(const PortID& portId) const {
  const auto& portIter = portId2LinkSwitchId_.find(portId);
  CHECK(portIter != portId2LinkSwitchId_.end())
      << "Port ID " << portId << " not found in the port ID to switch ID map!";
  return portIter->second;
}

// In cases where there are parallel links between VDs of switches, each of the
// links need to be given a different switchID and both side of the link should
// allocate the same switch ID. With an ordered list of local ports on both
// sides of the link, the offset of the local port could be used.
int FabricLinkMonitoring::calculateParallelLinkOffset(
    const cfg::Port& port,
    SwitchID remoteSwitchId,
    int vd,
    int maxParallelLinks) {
  // A different offset is needed for every port only in case of multiple
  // parallel links.
  if (maxParallelLinks <= 1) {
    return 0;
  }

  // Find the port list for this remote switch and VD combination
  const auto remoteSwitch2VdIter =
      remoteSwitchId2Vd2Ports_.find(remoteSwitchId);
  CHECK(remoteSwitch2VdIter != remoteSwitchId2Vd2Ports_.end())
      << "Remote switch VD mapping not found for switch: " << remoteSwitchId;

  const auto vd2PortsIter = remoteSwitch2VdIter->second.find(vd);
  CHECK(vd2PortsIter != remoteSwitch2VdIter->second.end())
      << "VD port mapping not found for VD: " << vd;

  // Find this port's position in the ordered list
  CHECK(port.name().has_value())
      << "Missing port name in parallel link offset compuration for port ID: "
      << *port.logicalID();
  const auto& portName = *port.name();

  const auto& portList = vd2PortsIter->second;
  const auto portIt = std::find(portList.begin(), portList.end(), portName);
  CHECK(portIt != portList.end())
      << "Port not found in VD port list for VD: " << vd
      << ", port: " << portName;

  return static_cast<int>(std::distance(portList.begin(), portIt));
}

// Compute the offset based on local and remote switch IDs for use in the switch
// ID per link determination.
int FabricLinkMonitoring::getSwitchIdOffset(
    const SwitchID& localSwitchId,
    const SwitchID& remoteSwitchId) {
  auto getLeafL1SwitchIdOffset = [this](
                                     const SwitchID& leafSwitchId,
                                     const SwitchID& l1SwitchId) {
    constexpr int kNumSwitchIdsPerLeafSwitch{4}; // Switch IDs per leaf switch
    return (leafSwitchId - lowestLeafSwitchId_) / kNumSwitchIdsPerLeafSwitch +
        l1SwitchId - lowestL1SwitchId_;
  };
  auto getL1L2SwitchIdOffset =
      [this](const SwitchID& l1SwitchId, const SwitchID& l2SwitchId) {
        constexpr int kNumSwitchIdsPerL1Switch{4}; // Switch IDs per L1 switch
        return (l1SwitchId - lowestL1SwitchId_) / kNumSwitchIdsPerL1Switch +
            l2SwitchId - lowestL2SwitchId_;
      };

  // Leaf switch ID < L1 switch ID < L2 switch ID
  if (localSwitchId < lowestL1SwitchId_ || remoteSwitchId < lowestL1SwitchId_) {
    // We are dealing with leaf-L1 switches
    return localSwitchId < lowestL1SwitchId_
        ? getLeafL1SwitchIdOffset(localSwitchId, remoteSwitchId)
        : getLeafL1SwitchIdOffset(remoteSwitchId, localSwitchId);
  } else {
    // We are dealing with L1-L2 switches
    return localSwitchId < lowestL2SwitchId_
        ? getL1L2SwitchIdOffset(localSwitchId, remoteSwitchId)
        : getL1L2SwitchIdOffset(remoteSwitchId, localSwitchId);
  }
}

// Allocate a switch ID per link in the network satisfying the below:
// 1. Every fabric link is allocated a switch ID
// 2. Both sides of a fabric link should get the same switch ID
// 3. Switch ID is unique per J3 ASIC and VD in R3
// 4. The number of switch IDs between RDSW-FDSW / FDSW-SDSW is limited
// 5. All the switches in the network will use fabric link mon switch
//    IDs from the same range
// 6. Parallel links are possible in the network between layers and needs
//    to be accounted for in switch ID computation
void FabricLinkMonitoring::allocateSwitchIdForPorts(
    const cfg::SwitchConfig* config) {
  CHECK(config->switchSettings()->switchId().has_value())
      << "Local switch ID missing in switch settings!";
  SwitchID localSwitchId = SwitchID(*config->switchSettings()->switchId());

  for (const auto& port : *config->ports()) {
    if (*port.portType() != cfg::PortType::FABRIC_PORT ||
        !port.expectedNeighborReachability()->size()) {
      // Fabric link monitoring is applicable only for fabric ports with
      // valid expected neighbors! Not all fabric ports are used in some
      // of the roles and those fabric ports wont have expected neighbors.
      continue;
    }
    PortID portId = PortID(*port.logicalID());
    const auto& [remoteSwitchName, _] = getExpectedNeighborAndPortName(port);

    auto remoteSwitchIter = switchName2SwitchId_.find(remoteSwitchName);
    CHECK(remoteSwitchIter != switchName2SwitchId_.end());
    SwitchID remoteSwitchId = remoteSwitchIter->second;

    auto portVdIter = portId2Vd_.find(portId);
    CHECK(portVdIter != portId2Vd_.end());
    int vd = portVdIter->second;

    int switchIdBase;
    int linksAtLevel;
    int maxParallelLinks;
    int switchIdOffset;
    if (isVoqSwitch_ || isConnectedToVoqSwitch(config, remoteSwitchId)) {
      // Port connecting a VoQ switch and L1 fabric
      switchIdBase = kFabricLinkMonitoringLeafBaseSwitchId;
      linksAtLevel = numLeafToL1Links_;
      maxParallelLinks = maxParallelLeafToL1Links_;
      switchIdOffset = getSwitchIdOffset(localSwitchId, remoteSwitchId);
    } else {
      // Port connecting L1 and L2 fabric
      switchIdBase = kFabricLinkMonitoringLevel2BaseSwitchId;
      linksAtLevel = numL1ToL2Links_;
      maxParallelLinks = maxParallelL1ToL2Links_;
      switchIdOffset = getSwitchIdOffset(localSwitchId, remoteSwitchId);
    }
    int parallelLinkOffset =
        calculateParallelLinkOffset(port, remoteSwitchId, vd, maxParallelLinks);
    int offset = switchIdOffset * maxParallelLinks + vd + parallelLinkOffset;
    // SwitchID allocated should be in the specific range bounded by the number
    // of links at that layer.
    portId2LinkSwitchId_[portId] = switchIdBase + (offset % linksAtLevel);
    XLOG(DBG3) << "Fabric Link Mon: Port ID:" << portId
               << " Link Switch ID:" << portId2LinkSwitchId_[portId]
               << " localSwitchId:" << localSwitchId
               << " remoteSwitchId:" << remoteSwitchId
               << " switchIdBase:" << switchIdBase
               << " switchIdOffset:" << switchIdOffset
               << " maxParallelLinks:" << maxParallelLinks << " vd:" << vd
               << " parallelLinkOffset:" << parallelLinkOffset
               << " linksAtLevel:" << linksAtLevel;
  }
}

void FabricLinkMonitoring::processDsfNodes(const cfg::SwitchConfig* config) {
  switchName2SwitchId_.clear();
  for (const auto& [_, dsfNode] : *config->dsfNodes()) {
    auto fabricLevel = dsfNode.fabricLevel().value_or(0);
    auto nodeSwitchId = SwitchID(*dsfNode.switchId());

    updateSwitchNameMapping(dsfNode, nodeSwitchId);
    updateLowestSwitchIds(dsfNode, nodeSwitchId, fabricLevel);
  }
}

// Populate the switch name to switch ID mapping. If multiple names map to the
// same switchID, cache just the lowest switch ID available.
void FabricLinkMonitoring::updateSwitchNameMapping(
    const auto& dsfNode,
    const SwitchID& nodeSwitchId) {
  const auto& nodeName = *dsfNode.name();
  auto [it, inserted] =
      switchName2SwitchId_.try_emplace(nodeName, nodeSwitchId);
  if (!inserted && nodeSwitchId < it->second) {
    it->second = nodeSwitchId;
  }
}

// Keep track of the lowest switch IDs for leaf / L1 and L2, which is needed to
// find the offset of switch ID of a specific layer.
void FabricLinkMonitoring::updateLowestSwitchIds(
    const auto& dsfNode,
    const SwitchID& nodeSwitchId,
    const int fabricLevel) {
  if (*dsfNode.type() == cfg::DsfNodeType::INTERFACE_NODE) {
    lowestLeafSwitchId_ = std::min(nodeSwitchId, lowestLeafSwitchId_);
  } else {
    if (fabricLevel == 1) {
      lowestL1SwitchId_ = std::min(nodeSwitchId, lowestL1SwitchId_);
    } else if (fabricLevel == 2) {
      lowestL2SwitchId_ = std::min(nodeSwitchId, lowestL2SwitchId_);
    } else {
      throw FbossError(
          "DSF node should be one of interface node, l1 or l2 fabric switch!");
    }
  }
}

// Run through the local ports and associate each local port with a VD. Also,
// prepare the map needed to track the local/remote port names connecting
// between the same VDs of the same switches, which are basically parallel
// links.
void FabricLinkMonitoring::processLinkInfo(const cfg::SwitchConfig* config) {
  std::map<
      SwitchID,
      std::map<int32_t, std::vector<std::pair<std::string, std::string>>>>
      remoteSwitchId2Vd2PortNamePairs;

  for (const auto& port : *config->ports()) {
    if (port.portType() != cfg::PortType::FABRIC_PORT ||
        port.expectedNeighborReachability()->empty()) {
      // Skip non fabric ports and ports without expected neighbor info
      continue;
    }

    const auto& [remoteNodeName, remotePortName] =
        getExpectedNeighborAndPortName(port);
    const auto remoteSwitchIt = switchName2SwitchId_.find(remoteNodeName);
    if (remoteSwitchIt == switchName2SwitchId_.end()) {
      // Skip the missing neighbor switch ID
      continue;
    }

    const auto remoteSwitchId = remoteSwitchIt->second;

    // Find the number of links at each network layer
    updateLinkCounts(config, remoteSwitchId);

    // Keep track of the mapping from PortID to VD
    auto vd = getVirtualDeviceIdForLink(config, port, remoteSwitchId);
    portId2Vd_[PortID(*port.logicalID())] = vd;

    CHECK(port.name().has_value())
        << "Missing port name for port with ID: " << *port.logicalID();
    // Keep track of local/remote port per VD for each remote SwitchID
    const auto& localPortName = *port.name();
    remoteSwitchId2Vd2PortNamePairs[remoteSwitchId][vd].emplace_back(
        localPortName, remotePortName);
  }

  // Ensure that the link counts at various layers are within the bounds
  // of the number of switch IDs planned.
  validateLinkLimits();
  // Determine the ordering to use for parallel links
  sequenceParallelLinksToVds(config, remoteSwitchId2Vd2PortNamePairs);
}

// Keep track of the number of links between leaf and L1 and
// also between L1 and L2 in the network.
void FabricLinkMonitoring::updateLinkCounts(
    const cfg::SwitchConfig* config,
    const SwitchID& neighborSwitchId) {
  if (isVoqSwitch_ || isConnectedToVoqSwitch(config, neighborSwitchId)) {
    ++numLeafToL1Links_;
  } else {
    ++numL1ToL2Links_;
  }
}

// Switch IDs for links are limited and hence need to ensure that the
// num links between leaf-L1 and L1-L2 are within the expected bounds.
void FabricLinkMonitoring::validateLinkLimits() const {
  if (numLeafToL1Links_ > kFabricLinkMonitoringMaxLeafSwitchIds) {
    throw FbossError(
        "Too many leaf to L1 links, max expected ",
        kFabricLinkMonitoringMaxLeafSwitchIds);
  }
  if (numL1ToL2Links_ > kFabricLinkMonitoringMaxLevel2SwitchIds) {
    throw FbossError(
        "Too many L1 to L2 links, max expected ",
        kFabricLinkMonitoringMaxLevel2SwitchIds);
  }
}

// Find the virtual device ID for port. VD is determined as below:
// 1. VoQ switches dont have VD, so use the VD associated with
//    the remote fabric port on the link.
// 2. For non-VoQ switch, each fabric port is associated with a VD.
//    These switch links are such that VD0 connects to VD0, VD1 to
//    VD1 etc., so both side of a link are guaranteed to be in the
//    same VD.
int32_t FabricLinkMonitoring::getVirtualDeviceIdForLink(
    const cfg::SwitchConfig* config,
    const cfg::Port& port,
    const SwitchID& neighborSwitchId) {
  CHECK(config->switchSettings()->switchId().has_value())
      << "Local switch ID missing in switch settings!";
  CHECK(*port.portType() == cfg::PortType::FABRIC_PORT)
      << "Virtual device is applicable only for fabric ports, "
      << "not for port with ID " << *port.logicalID() << " of type "
      << apache::thrift::util::enumNameSafe(*port.portType());
  auto dsfNodeIter = isVoqSwitch_
      ? config->dsfNodes()->find(neighborSwitchId)
      : config->dsfNodes()->find(*config->switchSettings()->switchId());
  CHECK(port.name().has_value())
      << "Missing port name for port with ID: " << *port.logicalID();
  std::string portName =
      isVoqSwitch_ ? getExpectedNeighborAndPortName(port).second : *port.name();

  auto platformType = *dsfNodeIter->second.platformType();
  const auto platformMapping = getPlatformMappingForPlatformType(platformType);
  if (!platformMapping) {
    throw FbossError("Unable to find platform mapping!");
  }

  auto virtualDeviceId = platformMapping->getVirtualDeviceID(portName);
  if (!virtualDeviceId.has_value()) {
    throw FbossError("Unable to find virtual device id for port: ", portName);
  }

  return *virtualDeviceId;
}

void FabricLinkMonitoring::updateMaxParallelLinks(
    const cfg::SwitchConfig* config) {
  auto updateParallelLinkCounts = [](int& currentCount, int newCount) {
    if (currentCount != 0 && newCount != currentCount) {
      XLOG(WARN)
          << "Asymmetric topology with different number of parallel links between nodes";
    }
    if (newCount > currentCount) {
      currentCount = newCount;
    }
  };

  for (const auto& [remoteSwitchId, vd2Ports] : remoteSwitchId2Vd2Ports_) {
    for (const auto& [vd, ports] : vd2Ports) {
      const int newCount = static_cast<int>(ports.size());
      if (isVoqSwitch_ || isConnectedToVoqSwitch(config, remoteSwitchId)) {
        // Either a VoQ switch or connected to a VoQ switch
        updateParallelLinkCounts(maxParallelLeafToL1Links_, newCount);
      } else {
        updateParallelLinkCounts(maxParallelL1ToL2Links_, newCount);
      }
    }
  }
}

// It is possible that there are multiple links between the same switchID and
// VD. Switch ID allocation to link should be such that both sides of a link
// will be allocated the same switch ID. This function creates a canonical pair
// with the local and remote port names for all parallel links in a specific
// {SwitchID, VD}, sorts it and then use the sorted order to determine the order
// in which local ports should be sequenced while allocating switch IDs per
// link.
void FabricLinkMonitoring::sequenceParallelLinksToVds(
    const cfg::SwitchConfig* config,
    const std::map<
        SwitchID,
        std::map<int32_t, std::vector<std::pair<std::string, std::string>>>>&
        remoteSwitchId2Vd2PortNamePairs) {
  std::map<SwitchID, std::map<int32_t, std::vector<std::string>>>
      switchId2Vd2OrderedPorts;

  for (const auto& [switchId, vd2PortPairs] : remoteSwitchId2Vd2PortNamePairs) {
    // For each VD, get the {local port, remote port} list
    for (const auto& [vd, localRemotePortList] : vd2PortPairs) {
      std::vector<std::pair<std::pair<std::string, std::string>, std::string>>
          canonicalLinks;

      for (const auto& localRemotePort : localRemotePortList) {
        const auto& [localPortName, remotePortName] = localRemotePort;
        std::pair<std::string, std::string> canonical;
        // Canonical pair creation such that both sides of a link can create
        // the same ordering in a pair.
        if (localPortName <= remotePortName) {
          canonical = {localPortName, remotePortName};
        } else {
          canonical = {remotePortName, localPortName};
        }

        // Keep track of the local port name along with the canonical pair
        // so we know the local port ordering once the sorting is done based
        // on the canonical pair..
        canonicalLinks.emplace_back(canonical, localPortName);
      }

      // Sort based on the canonical pair
      std::sort(
          canonicalLinks.begin(),
          canonicalLinks.end(),
          [](const auto& a, const auto& b) { return a.first < b.first; });

      // From the sorted list, get the localPortName maintaining the order
      // in the sorted list
      std::vector<std::string> orderedPorts;
      orderedPorts.reserve(canonicalLinks.size());
      for (const auto& [_, localPortName] : canonicalLinks) {
        orderedPorts.push_back(localPortName);
      }
      switchId2Vd2OrderedPorts[switchId][vd] = std::move(orderedPorts);
    }
  }
  remoteSwitchId2Vd2Ports_ = std::move(switchId2Vd2OrderedPorts);
  // Given the parallel links are identified, find the max number of
  // parallel link at rhe various layers.
  updateMaxParallelLinks(config);
}
} // namespace facebook::fboss
