// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricLinkMonitoring.h"
#include "fboss/agent/AgentFeatures.h"

#include <atomic>

#include "fboss/agent/DsfNodeUtils.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/VoqUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

namespace {
// Static flag for test mode
std::atomic<bool> testModeEnabled{false};
} // namespace

// Test-only function to enable test mode
void FabricLinkMonitoring::setTestMode(bool enabled) {
  testModeEnabled.store(enabled);
}

bool FabricLinkMonitoring::isTestMode() {
  return testModeEnabled.load();
}

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
      << "FabricLinkMon: Port ID " << portId
      << " not found in the port ID to switch ID map!";
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
      << "FabricLinkMon: Remote switch VD mapping not found for switch: "
      << remoteSwitchId;

  const auto vd2PortsIter = remoteSwitch2VdIter->second.find(vd);
  CHECK(vd2PortsIter != remoteSwitch2VdIter->second.end())
      << "FabricLinkMon: VD port mapping not found for VD: " << vd;

  // Find this port's position in the ordered list
  CHECK(port.name().has_value())
      << "FabricLinkMon: Missing port name in parallel link offset compuration for port ID: "
      << *port.logicalID();
  const auto& portName = *port.name();

  const auto& portList = vd2PortsIter->second;
  const auto portIt = std::find(portList.begin(), portList.end(), portName);
  CHECK(portIt != portList.end())
      << "FabricLinkMon: Port not found in VD port list for VD: " << vd
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
      << "FabricLinkMon: Local switch ID missing in switch settings!";
  SwitchID localSwitchId = SwitchID(*config->switchSettings()->switchId());
  bool isDualStageNetwork = utility::isDualStage(*config);

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
    int maxNumSwitchIds;
    int linksAtLevel;
    int maxParallelLinks;
    int switchIdOffset;
    if (isVoqSwitch_ || isConnectedToVoqSwitch(config, remoteSwitchId)) {
      // This is for a port connecting a VoQ switch and L1 fabric.
      // Identify a base switchID such that the max possible switch ID
      // will still fall within the allowed limit.
      if (isDualStageNetwork) {
        maxNumSwitchIds = kDualStageMaxLeafL1FabricLinkMonitoringSwitchIds;
        switchIdBase = kMaxUsableVoqSwitchId - maxNumSwitchIds;
        CHECK_GT(switchIdBase, kDualStageMaxGlobalSwitchId)
            << "FabricLinkMon: Fabric link monitoring base switch ID should be > "
            << kDualStageMaxGlobalSwitchId;
      } else {
        maxNumSwitchIds = kSingleStageMaxLeafL1FabricLinkMonitoringSwitchIds;
        switchIdBase = kMaxUsableVoqSwitchId - maxNumSwitchIds;
        CHECK_GE(switchIdBase, kSingleStageMaxGlobalSwitchId)
            << "FabricLinkMon: Fabric link monitoring base switch ID should be >= "
            << kSingleStageMaxGlobalSwitchId;
      }
      maxParallelLinks = maxParallelLeafToL1Links_;
      switchIdOffset = getSwitchIdOffset(localSwitchId, remoteSwitchId);
      // Just pick the first VDs link count as all VDs should have the same link
      // count
      linksAtLevel = numLeafL1Links_.begin()->second;
    } else {
      // This is for a port connecting L1 and L2 fabric
      switchIdBase = kFabricLinkMonitoringL1L2BaseSwitchId;
      maxNumSwitchIds = kDualStageMaxL1L2FabricLinkMonitoringSwitchIds;
      maxParallelLinks = maxParallelL1ToL2Links_;
      switchIdOffset = getSwitchIdOffset(localSwitchId, remoteSwitchId);
      // Just pick the first VDs link count as all VDs should have the same link
      // count
      linksAtLevel = numL1L2Links_.begin()->second;
    }
    int parallelLinkOffset =
        calculateParallelLinkOffset(port, remoteSwitchId, vd, maxParallelLinks);
    int offset = switchIdOffset * maxParallelLinks + vd + parallelLinkOffset;
    // SwitchID allocated should be in the specific range bounded by the maximum
    // number of switchIDs possible for the 2 roles connected by the port/link.
    CHECK_LE(linksAtLevel, maxNumSwitchIds)
        << "FabricLinkMon: Fabric link monitoring links: " << linksAtLevel
        << " should be <= the max switch IDs available for link monitoring: "
        << maxNumSwitchIds;
    portId2LinkSwitchId_[portId] = switchIdBase + (offset % maxNumSwitchIds);
    XLOG(DBG3) << "FabricLinkMon: Fabric Link Mon - Port name:"
               << port.name().value_or("Unknown") << " Port ID:" << portId
               << " Link Switch ID:" << portId2LinkSwitchId_[portId]
               << " localSwitchId:" << localSwitchId
               << " remoteSwitchId:" << remoteSwitchId
               << " switchIdBase:" << switchIdBase
               << " switchIdOffset:" << switchIdOffset
               << " maxParallelLinks:" << maxParallelLinks << " vd:" << vd
               << " parallelLinkOffset:" << parallelLinkOffset
               << " maxNumSwitchIds:" << maxNumSwitchIds << " offset:" << offset
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
          "FabricLinkMon: DSF node should be one of interface node, l1 or l2 fabric switch!");
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

    // Keep track of the mapping from PortID to VD
    auto vd = getVirtualDeviceIdForLink(config, port, remoteSwitchId);
    portId2Vd_[PortID(*port.logicalID())] = vd;

    // Find the number of links at each network layer
    updateLinkCounts(config, remoteSwitchId, vd);

    CHECK(port.name().has_value())
        << "FabricLinkMon: Missing port name for port with ID: "
        << *port.logicalID();
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
    const SwitchID& neighborSwitchId,
    const int vd) {
  if (isVoqSwitch_ || isConnectedToVoqSwitch(config, neighborSwitchId)) {
    numLeafL1Links_[vd] = numLeafL1Links_[vd] + 1;
  } else {
    numL1L2Links_[vd] = numL1L2Links_[vd] + 1;
  }
}

// Switch IDs for links are limited and hence need to ensure that the
// num links between leaf-L1 and L1-L2 are within the expected bounds.
void FabricLinkMonitoring::validateLinkLimits() const {
  auto throwExceededMaxExpectedLinkCount =
      [](int linkCount, int maxLinkCount, const std::string& linkTypeStr) {
        throw FbossError(
            "FabricLinkMon: Too many ",
            linkTypeStr,
            " links - ",
            linkCount,
            ", max expected ",
            maxLinkCount);
      };
  int leafL1Links =
      numLeafL1Links_.size() ? numLeafL1Links_.begin()->second : 0;
  if (isVoqSwitch_) {
    // RDSW / EDSW to FDSW link count is fixed in all deployments.
    // However, this is not applicable for the reverse direction.
    if (leafL1Links > kDsfMaxLeafFabricLinksPerVd) {
      throwExceededMaxExpectedLinkCount(
          leafL1Links, kDsfMaxLeafFabricLinksPerVd, "leaf to L1");
    }
  } else if (leafL1Links) {
    // Dual stage network will have some L1 to L2 links
    bool isDualStageNetwork = numL1L2Links_.size() > 0;

    // FDSW side of RDSW / EDSW  -> FDSW links. The number of links
    // here depends on the type of deployment.
    if (FLAGS_dsf_single_stage_r192_f40_e32 &&
        (leafL1Links > kDsfR192F40E32MaxL1ToLeafLinksPerVd)) {
      throwExceededMaxExpectedLinkCount(
          leafL1Links, kDsfR192F40E32MaxL1ToLeafLinksPerVd, "L1 to leaf");
    } else if (
        FLAGS_dsf_single_stage_r128_f40_e16_8k_sys_ports &&
        (leafL1Links > kDsfR128F40E16MaxL1ToLeafLinksPerVd)) {
      throwExceededMaxExpectedLinkCount(
          leafL1Links, kDsfR128F40E16MaxL1ToLeafLinksPerVd, "L1 to leaf");
    } else if (
        FLAGS_type_dctype1_janga &&
        (leafL1Links > kDsfMtiaMaxL1ToLeafLinksPerVd)) {
      throwExceededMaxExpectedLinkCount(
          leafL1Links, kDsfMtiaMaxL1ToLeafLinksPerVd, "L1 to leaf");
    } else if (
        isDualStageNetwork &&
        (leafL1Links > kDsfDualStageMaxL1ToLeafLinksPerVd)) {
      throwExceededMaxExpectedLinkCount(
          leafL1Links, kDsfDualStageMaxL1ToLeafLinksPerVd, "L1 to leaf");
    }
  }
  int l1L2Links = numL1L2Links_.size() ? numL1L2Links_.begin()->second : 0;
  if (leafL1Links && l1L2Links > kDsfMaxL1ToL2SwitchLinksPerVd) {
    // FDSW side of FDSW -> SDSW links
    throwExceededMaxExpectedLinkCount(
        l1L2Links, kDsfMaxL1ToL2SwitchLinksPerVd, "L1 to L2");
  } else if (!leafL1Links && l1L2Links > kDsfMaxL2ToL1SwitchLinksPerVd) {
    // SDSW side of SDSW -> FDSW links
    throwExceededMaxExpectedLinkCount(
        l1L2Links, kDsfMaxL2ToL1SwitchLinksPerVd, "L2 to L1");
  }
}

// Find the virtual device ID for port. VD is determined as below:
// 1. VoQ switches dont have VD, so use the VD associated with
//    the remote fabric port on the link.
// 2. For non-VoQ switch, each fabric port is associated with a VD.
//    Given VDs could be different on the 2 sides of a link, use the
//    VD from the L1 fabric switch so that both sides can use the
//    same VD id in computations.
int32_t FabricLinkMonitoring::getVirtualDeviceIdForLink(
    const cfg::SwitchConfig* config,
    const cfg::Port& port,
    const SwitchID& neighborSwitchId) {
  CHECK(config->switchSettings()->switchId().has_value())
      << "FabricLinkMon: Local switch ID missing in switch settings!";
  CHECK(*port.portType() == cfg::PortType::FABRIC_PORT)
      << "FabricLinkMon: Virtual device is applicable only for fabric "
      << "port, not for port with ID " << *port.logicalID() << " of type "
      << apache::thrift::util::enumNameSafe(*port.portType());

  if (isTestMode()) {
    // In unit test cases, avoid dependency on platform mapping
    return *port.logicalID() % 4;
  }

  // Find the neighbor DSF node in case of VoQ switch or in case
  // of L2 switch.
  auto neighborSwitchIter = config->dsfNodes()->find(neighborSwitchId);
  CHECK(neighborSwitchIter != config->dsfNodes()->end())
      << "FabricLinkMon: DSF node missing for switchId: " << neighborSwitchId;
  bool useNeighborSwitchVd = isVoqSwitch_ ||
      (neighborSwitchIter->second.fabricLevel().has_value() &&
       *neighborSwitchIter->second.fabricLevel() == 2);
  auto dsfNodeIter = useNeighborSwitchVd
      ? neighborSwitchIter
      : config->dsfNodes()->find(*config->switchSettings()->switchId());
  CHECK(port.name().has_value())
      << "FabricLinkMon: Missing port name for port with ID: "
      << *port.logicalID();
  std::string portName = useNeighborSwitchVd
      ? getExpectedNeighborAndPortName(port).second
      : *port.name();

  auto platformType = *dsfNodeIter->second.platformType();
  const auto platformMapping = getPlatformMappingForPlatformType(platformType);
  if (!platformMapping) {
    throw FbossError("FabricLinkMon: Unable to find platform mapping!");
  }

  auto virtualDeviceId = platformMapping->getVirtualDeviceID(portName);
  if (!virtualDeviceId.has_value()) {
    throw FbossError(
        "FabricLinkMon: Unable to find virtual device id for port: ", portName);
  }

  return *virtualDeviceId;
}

void FabricLinkMonitoring::updateMaxParallelLinks(
    const cfg::SwitchConfig* config) {
  auto updateParallelLinkCounts = [](int& currentCount, int newCount) {
    if (currentCount != 0 && newCount != currentCount) {
      XLOG(WARN)
          << "FabricLinkMon: Asymmetric topology with different number of parallel links between nodes";
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
