// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400bfu/Meru400bfuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400bia/Meru400biaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400biu/Meru400biuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaP1PlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"

#include "fboss/agent/hw/switch_asics/Jericho2Asic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/hw/switch_asics/Ramon3Asic.h"
#include "fboss/agent/hw/switch_asics/RamonAsic.h"

#include <sstream>
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
      static Meru800bfaPlatformMapping meru800bfa{
          true /*multiNpuPlatformMapping*/};
      return &meru800bfa;
    }
    case PlatformType::PLATFORM_MERU800BFA_P1: {
      static Meru800bfaP1PlatformMapping meru800bfa{
          true /*multiNpuPlatformMapping*/};
      return &meru800bfa;
    }
    case PlatformType::PLATFORM_MERU800BIA: {
      static Meru800biaPlatformMapping meru800bia;
      return &meru800bia;
    }
    case PlatformType::PLATFORM_JANGA800BIC: {
      static Janga800bicPlatformMapping janga800bic{
          true /*multiNpuPlatformMapping*/};
      return &janga800bic;
    }
    default:
      break;
  }
  return nullptr;
}

std::string toStr(const RemoteEndpoint& r) {
  std::stringstream ss;
  ss << " switchId : " << *r.switchId() << " switch name: " << *r.switchName()
     << " connecting ports: " << folly::join(", ", *r.connectingPorts());
  return ss.str();
}

std::string toStr(const FabricEndpoint& endpoint) {
  std::stringstream ss;
  ss << " Attached: " << *endpoint.isAttached() << std::endl;
  if (*endpoint.isAttached()) {
    ss << " Remote switch, id: " << *endpoint.switchId()
       << " name:" << endpoint.switchName().value_or("--") << std::endl;
    ss << " Remote port, id: " << *endpoint.portId()
       << " name:" << endpoint.portName().value_or("--") << std::endl;
    ss << " Switch Type: "
       << apache::thrift::util::enumNameSafe(*endpoint.switchType())
       << std::endl;
  }

  // Expected switch, port
  ss << " Expected switch, id: " << endpoint.expectedSwitchId().value_or(-1)
     << " name:" << endpoint.expectedSwitchName().value_or("--") << std::endl;
  ss << " Expected port, id: " << endpoint.expectedPortId().value_or(-1)
     << " name:" << endpoint.expectedPortName().value_or("--") << std::endl;
  return ss.str();
}

std::string toStr(const multiswitch::FabricConnectivityDelta& delta) {
  std::stringstream ss;
  ss << " Old endpoint: "
     << (delta.oldConnectivity() ? toStr(*delta.oldConnectivity()) : "null");
  ss << " New endpoint: "
     << (delta.newConnectivity() ? toStr(*delta.newConnectivity()) : "null");
  return ss.str();
}
} // namespace

namespace facebook::fboss {

void toAppend(const RemoteEndpoint& endpoint, folly::fbstring* result) {
  result->append(toStr(endpoint));
}
void toAppend(const RemoteEndpoint& endpoint, std::string* result) {
  *result += toStr(endpoint);
}

void toAppend(const FabricEndpoint& endpoint, folly::fbstring* result) {
  result->append(toStr(endpoint));
}
void toAppend(const FabricEndpoint& endpoint, std::string* result) {
  *result += toStr(endpoint);
}

void toAppend(
    const multiswitch::FabricConnectivityDelta& delta,
    folly::fbstring* result) {
  result->append(toStr(delta));
}
void toAppend(
    const multiswitch::FabricConnectivityDelta& delta,
    std::string* result) {
  *result += toStr(delta);
}

std::ostream& operator<<(std::ostream& os, const RemoteEndpoint& endpoint) {
  os << toStr(endpoint);
  return os;
}
std::ostream& operator<<(std::ostream& os, const FabricEndpoint& endpoint) {
  os << toStr(endpoint);
  return os;
}
std::ostream& operator<<(
    std::ostream& os,
    const multiswitch::FabricConnectivityDelta& delta) {
  os << toStr(delta);
  return os;
}

void FabricConnectivityManager::updateExpectedSwitchIdAndPortIdForPort(
    PortID portID) {
  auto& fabricEndpoint = currentNeighborConnectivity_[portID];
  if (!fabricEndpoint.expectedSwitchName().has_value() ||
      !fabricEndpoint.expectedPortName().has_value()) {
    // Incomplete or missing expected neighbor info. Clear out
    // any previously derived expectedSwitchId/expectedPortId
    fabricEndpoint.expectedSwitchId().reset();
    fabricEndpoint.expectedPortId().reset();
    fabricEndpoint.switchName().reset();
    fabricEndpoint.portName().reset();
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
  auto expectedPortID = platformMapping->getPortID(expectedPortName) -
      getRemotePortOffset(switchIdToDsfNode_[baseSwitchId]->getPlatformType());

  const auto& hwAsic =
      getHwAsicForAsicType(switchIdToDsfNode_[baseSwitchId]->getAsicType());

  // Find portID offset from given ASIC in a multi ASIC system
  expectedPortID %= hwAsic.getMaxPorts();

  // Find vid offset from given vid in a multi vid ASIC system
  auto vidOffsetInAsic = virtualDeviceId.value() % hwAsic.getVirtualDevices();

  // Find portID offset from given vid in a multi vid ASIC systen
  auto portsPerVirtualDeviceId = getFabricPortsPerVirtualDevice(
      switchIdToDsfNode_[baseSwitchId]->getAsicType());
  expectedPortID -= portsPerVirtualDeviceId * vidOffsetInAsic;

  fabricEndpoint.expectedPortId() = expectedPortID;

  XLOG(DBG2) << "Local port: " << static_cast<int>(portID)
             << " Expected Peer SwitchName: " << expectedSwitchName
             << " Expected Peer PortName: " << expectedPortName
             << " Expected Peer SwitchID: "
             << fabricEndpoint.expectedSwitchId().value()
             << " Expected Peer PortID: "
             << fabricEndpoint.expectedPortId().value();
}

void FabricConnectivityManager::addOrUpdatePort(
    const std::shared_ptr<Port>& swPort) {
  // Non-Faric port connectivity is handled by LLDP
  if (swPort->getPortType() != cfg::PortType::FABRIC_PORT) {
    return;
  }

  fabricPortId2Name_[swPort->getID()] = swPort->getName();
  auto& expectedEndpoint = currentNeighborConnectivity_[swPort->getID()];
  const auto& expectedNeighbors =
      swPort->getExpectedNeighborValues()->toThrift();
  if (expectedNeighbors.size()) {
    CHECK_EQ(expectedNeighbors.size(), 1);
    const auto& neighbor = expectedNeighbors.front();
    expectedEndpoint.expectedSwitchName() = *neighbor.remoteSystem();
    expectedEndpoint.expectedPortName() = *neighbor.remotePort();
  } else {
    expectedEndpoint.expectedSwitchName().reset();
    expectedEndpoint.expectedPortName().reset();
  }
  updateExpectedSwitchIdAndPortIdForPort(swPort->getID());
}

void FabricConnectivityManager::removePort(
    const std::shared_ptr<Port>& swPort) {
  currentNeighborConnectivity_.erase(swPort->getID());
  fabricPortId2Name_.erase(swPort->getID());
}

void FabricConnectivityManager::addDsfNode(
    const std::shared_ptr<DsfNode>& dsfNode) {
  switchIdToDsfNode_[dsfNode->getID()] = dsfNode;
  switchNameToSwitchIDs_[dsfNode->getName()].insert(dsfNode->getSwitchId());

  // DSFNodeMap for multi-core devices is spaced out by the number of cores.
  // However, SAI implementation may return peer switchId for any core.
  // Construct a map to process it.
  //
  // For example, for a 2-core, 2-NPU device, DSFNodeMap contains:
  //  1024 => fdsw1
  //  1026 => fdsw1
  //  1028 => fdsw2
  //  1030 => fdsw2
  //  ...
  //
  //  Build a map as:
  //  1024 => 1024, fdsw1
  //  1025 => 1024, fdsw1
  //  1026 => 1026, fdsw1
  //  1027 => 1026, fdsw1
  //  1028 => 1028, fdsw2
  //  1029 => 1028, fdsw2
  //  ...
  auto baseSwitchId = dsfNode->getID();
  const auto& hwAsic =
      getHwAsicForAsicType(switchIdToDsfNode_[baseSwitchId]->getAsicType());
  for (auto currSwitchId = baseSwitchId;
       currSwitchId < baseSwitchId + hwAsic.getNumCores();
       currSwitchId++) {
    switchIdToBaseSwitchIdAndSwitchName_[currSwitchId] =
        std::make_pair(baseSwitchId, dsfNode->getName());
  }
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
        addOrUpdatePort(newSwPort);
      },
      [&](const std::shared_ptr<Port>& newPort) { addOrUpdatePort(newPort); },
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

std::optional<PortID> FabricConnectivityManager::getActualPortIdForSwitch(
    int32_t portId,
    uint64_t switchId,
    uint64_t baseSwitchId,
    const auto& switchName) {
  auto switchNameIter = switchNameToSwitchIDs_.find(switchName);
  if (switchNameIter == switchNameToSwitchIDs_.end()) {
    return std::nullopt;
  }

  auto iter = switchNameIter->second.find(baseSwitchId);
  if (iter == switchNameIter->second.end()) {
    return std::nullopt;
  }

  auto npuIndex = std::distance(switchNameIter->second.begin(), iter);
  const auto& hwAsic =
      getHwAsicForAsicType(switchIdToDsfNode_[baseSwitchId]->getAsicType());

  return PortID(
      portId + npuIndex * hwAsic.getMaxPorts() +
      ((switchId - baseSwitchId) *
       getFabricPortsPerVirtualDevice(
           switchIdToDsfNode_[baseSwitchId]->getAsicType())) +
      getRemotePortOffset(switchIdToDsfNode_[baseSwitchId]->getPlatformType()));
}

std::pair<std::optional<std::string>, std::optional<std::string>>
FabricConnectivityManager::getActualSwitchNameAndPortName(
    uint64_t switchId,
    int32_t portId) {
  std::optional<std::string> switchName{std::nullopt}, portName{std::nullopt};

  auto switchIdIter = switchIdToBaseSwitchIdAndSwitchName_.find(switchId);
  if (switchIdIter == switchIdToBaseSwitchIdAndSwitchName_.end()) {
    XLOG(ERR) << "Unknown Peer SwitchID: " << static_cast<int>(switchId);
  } else {
    SwitchID baseSwitchId;

    std::tie(baseSwitchId, switchName) = switchIdIter->second;

    auto actualPortId = getActualPortIdForSwitch(
        PortID(portId), SwitchID(switchId), baseSwitchId, switchName.value());
    if (actualPortId.has_value()) {
      const auto platformMapping = getPlatformMappingForDsfNode(
          switchIdToDsfNode_[baseSwitchId]->getPlatformType());
      if (!platformMapping) {
        throw FbossError("Unable to find platform mapping for port: ", portId);
      }
      portName = platformMapping->getPortNameByPortId(actualPortId.value());
    }
  }

  return std::make_pair(switchName, portName);
}

std::optional<multiswitch::FabricConnectivityDelta>
FabricConnectivityManager::processConnectivityInfoForPort(
    const PortID& portId,
    const FabricEndpoint& hwEndpoint) {
  std::optional<multiswitch::FabricConnectivityDelta> delta;
  std::optional<FabricEndpoint> old;
  auto iter = currentNeighborConnectivity_.find(portId);
  if (iter != currentNeighborConnectivity_.end()) {
    old = iter->second;
    // Populate actual isAttached, switchId, switchName, portId, portName
    iter->second.isAttached() = *hwEndpoint.isAttached();
    iter->second.switchId() = *hwEndpoint.switchId();
    iter->second.portId() = *hwEndpoint.portId();
    iter->second.switchType() = *hwEndpoint.switchType();

    // updateExpectedSwitchIdAndPortIdForPort uses platform, virtualDeviceId to
    // derive expected{Switch, Port}Id from expected{Switch, Port}Name.
    // Thus, if actual{Switch, Port}Id matches expected{Switch, Port}Id,
    // expected matches actual and thus we can use expected{Switch, Port}Name to
    // populate actual{Switch, Port}Name.

    if (iter->second.expectedSwitchId().has_value() &&
        iter->second.expectedSwitchId().value() == iter->second.switchId() &&
        iter->second.expectedSwitchName().has_value() &&
        iter->second.expectedPortId().has_value() &&
        iter->second.expectedPortId().value() == iter->second.portId() &&
        iter->second.expectedPortName().has_value()) {
      // actual{switchID, portID} == expected{switchID, portID}
      iter->second.switchName() = iter->second.expectedSwitchName().value();
      iter->second.portName() = iter->second.expectedPortName().value();
    } else {
      // Miscabling:
      //    - Connected to expected Switch but on wrong port
      //    - Connected to non-expected Switch
      // Expected switchName/portName are not set
      //
      // For any of these scenarios, derive SwitchName, PortName
      // from actual{switchID, portID}.
      auto [switchName, portName] = getActualSwitchNameAndPortName(
          *iter->second.switchId(), *iter->second.portId());

      if (switchName.has_value()) {
        iter->second.switchName() = switchName.value();
      }
      if (portName.has_value()) {
        iter->second.portName() = portName.value();
      }
    }
  } else {
    iter = currentNeighborConnectivity_.insert({portId, hwEndpoint}).first;
  }
  if (!old || (old != iter->second)) {
    delta = multiswitch::FabricConnectivityDelta();
    if (old.has_value()) {
      delta->oldConnectivity() = *old;
    }
    delta->newConnectivity() = iter->second;
  }
  return delta;
}

// Detect mismatch in expected vs. actual connectivity.
// Points to cabling issues: no or wrong connection.

bool FabricConnectivityManager::isConnectivityInfoMismatch(
    const FabricEndpoint& endpoint) {
  if (!*endpoint.isAttached()) {
    // No connectivity seen - typically implies a DOWN port.
    // Cannot conclude its a mismatch
    return false;
  }
  if (endpoint.expectedSwitchId().has_value() &&
      (endpoint.switchId() != endpoint.expectedSwitchId().value())) {
    return true;
  }
  if (endpoint.expectedPortId().has_value() &&
      (endpoint.portId() != endpoint.expectedPortId().value())) {
    return true;
  }

  if (endpoint.switchName() != endpoint.expectedSwitchName()) {
    // mismatch
    return true;
  }

  if (endpoint.portName() != endpoint.expectedPortName()) {
    // mismatch
    return true;
  }
  return false;
}

bool FabricConnectivityManager::isConnectivityInfoMismatch(
    const PortID& portId) {
  const auto& iter = currentNeighborConnectivity_.find(portId);
  if (iter != currentNeighborConnectivity_.end()) {
    const auto& endpoint = iter->second;
    return isConnectivityInfoMismatch(endpoint);
  }

  // no mismatch
  return false;
}

// Detect missing info. Points to configuration issue where expected
// connectivity info is not populated.
bool FabricConnectivityManager::isConnectivityInfoMissing(
    const PortID& portId) {
  const auto& iter = currentNeighborConnectivity_.find(portId);
  if (iter == currentNeighborConnectivity_.end()) {
    // specific port is missing from the reachability DB and
    // also switch state (else we would have added it during addPort).
    // So no connectivity is expected here
    return false;
  }

  const auto& endpoint = iter->second;
  if (!*endpoint.isAttached()) {
    // endpoint not attached, points to cabling connectivity issues
    // unless in cfg also we don't expect it to be present
    if (!endpoint.expectedSwitchId().has_value() &&
        !endpoint.expectedPortId().has_value()) {
      // not attached, not expected to be attached ..thse are fabric ports
      // which are down and only contribute to the noise.
      return false;
    }
    return true;
  }

  // if any of these parameters are not populated, we have missing
  // reachability info
  if (!(endpoint.expectedSwitchId().has_value() &&
        endpoint.expectedPortId().has_value() &&
        endpoint.switchName().has_value() &&
        endpoint.expectedSwitchName().has_value() &&
        endpoint.portName().has_value() &&
        endpoint.expectedPortName().has_value())) {
    return true;
  }

  // nothing missing
  return false;
}

const std::map<PortID, FabricEndpoint>&
FabricConnectivityManager::getConnectivityInfo() const {
  return currentNeighborConnectivity_;
}

std::map<int64_t, FabricConnectivityManager::RemoteConnectionGroups>
FabricConnectivityManager::getVirtualDeviceToRemoteConnectionGroups(
    const std::function<int(PortID)>& portToVirtualDevice) const {
  // Virtual device Id ->  map<numConnections, list<RemoteEndpoint>>
  std::map<int64_t, RemoteConnectionGroups>
      virtualDevice2RemoteConnectionGroups;

  // Local cache to maintain current state of remote endpoints for
  // a virtual device.
  std::map<int64_t, RemoteEndpoints> virtualDevice2RemoteEndpoints;
  for (const auto& [portId, fabricEndpoint] : currentNeighborConnectivity_) {
    if (!*fabricEndpoint.isAttached()) {
      continue;
    }
    auto portName = fabricPortId2Name_.find(portId)->second;
    auto virtualDeviceId = portToVirtualDevice(portId);
    //      platform_->getPlatformPort(portId)->getVirtualDeviceId();
    // CHECK(virtualDeviceId.has_value());
    // get connections for virtual device
    auto& virtualDeviceRemoteEndpoints =
        virtualDevice2RemoteEndpoints[virtualDeviceId];
    auto& remoteConnectionGroups =
        virtualDevice2RemoteConnectionGroups[virtualDeviceId];
    // Append to list of ports connecting to this virtual device
    RemoteEndpoint remoteEndpoint;
    remoteEndpoint.switchId() = *fabricEndpoint.switchId();
    remoteEndpoint.switchName() = fabricEndpoint.switchName().value_or("");
    remoteEndpoint.connectingPorts()->push_back(portName);
    auto ritr = virtualDeviceRemoteEndpoints.find(remoteEndpoint);
    if (ritr != virtualDeviceRemoteEndpoints.end()) {
      // Remote endpoint is already connected to this virtual device
      auto numExistingConnections = ritr->connectingPorts()->size();
      CHECK_NE(numExistingConnections, 0);
      // Remove this from remoteConnectionGroups as the numConnections will
      // change after we add portId. We will re add the entry after adding
      // portId
      remoteConnectionGroups[numExistingConnections].erase(remoteEndpoint);
      if (remoteConnectionGroups[numExistingConnections].size() == 0) {
        remoteConnectionGroups.erase(numExistingConnections);
      }
      // Add portId to this remote endpoint
      remoteEndpoint = *ritr;
      remoteEndpoint.connectingPorts()->push_back(portName);
      // Erase and add back new endpoint (can't update value in set)
      virtualDeviceRemoteEndpoints.erase(ritr);
    }
    // Add back updated(or newly discovered) remoteEndpoint
    virtualDeviceRemoteEndpoints.insert(remoteEndpoint);
    // Insert updated remote endpoint
    remoteConnectionGroups[remoteEndpoint.connectingPorts()->size()].insert(
        remoteEndpoint);
  }
  return virtualDevice2RemoteConnectionGroups;
}

int FabricConnectivityManager::virtualDevicesWithAsymmetricConnectivity(
    const std::map<int64_t, RemoteConnectionGroups>&
        virtualDevice2RemoteConnectionGroups) {
  int virtualDevicesWithAsymmetricConnectivity{0};
  for (const auto& [virtualDeviceId, remoteConnectionGroups] :
       virtualDevice2RemoteConnectionGroups) {
    if (remoteConnectionGroups.size() > 1) {
      ++virtualDevicesWithAsymmetricConnectivity;
      XLOG(DBG4) << " Asymmetric topology detected on virtual device: "
                 << virtualDeviceId;
      for (const auto& [numConnections, remoteConnections] :
           remoteConnectionGroups) {
        XLOG(DBG4) << " Remote endpoints with : " << numConnections
                   << " connections : " << folly::join(",", remoteConnections);
      }
    }
  }
  return virtualDevicesWithAsymmetricConnectivity;
}
} // namespace facebook::fboss
