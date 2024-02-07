// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricConnectivityManager.h"
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
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"

#include "fboss/agent/hw/switch_asics/Jericho2Asic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/hw/switch_asics/Ramon3Asic.h"
#include "fboss/agent/hw/switch_asics/RamonAsic.h"

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
    case PlatformType::PLATFORM_MERU800BIA: {
      static Meru800biaPlatformMapping meru800bia;
      return &meru800bia;
    }
    case PlatformType::PLATFORM_JANGA800BIC: {
      static Janga800bicPlatformMapping janga800bic;
      return &janga800bic;
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
    case PlatformType::PLATFORM_JANGA800BIC:
      return 1024;

    default:
      return 0;
  }
  return 0;
}

static const HwAsic& getHwAsicForAsicType(const cfg::AsicType& asicType) {
  /*
   * hwAsic is used to invoke methods such as getMaxPorts,
   * getVirtualDevices. For these methods, following attributes don't
   * matter. Hence set to some pre-defined values.
   * Using pre-defined values (instead of deriving dynamically from dsfNode)
   * allows us to use static hwAsic objects here.
   */
  int64_t switchId = 0;
  int16_t switchIndex = 0;
  std::optional<cfg::Range64> systemPortRange = std::nullopt;
  folly::MacAddress mac("02:00:00:00:0F:0B");

  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_JERICHO2: {
      static Jericho2Asic jericho2Asic{
          cfg::SwitchType::VOQ,
          switchId,
          switchIndex,
          systemPortRange,
          mac,
          std::nullopt};
      return jericho2Asic;
    }
    case cfg::AsicType::ASIC_TYPE_JERICHO3: {
      static Jericho3Asic jericho3Asic{
          cfg::SwitchType::VOQ,
          switchId,
          switchIndex,
          systemPortRange,
          mac,
          std::nullopt};

      return jericho3Asic;
    }
    case cfg::AsicType::ASIC_TYPE_RAMON: {
      static RamonAsic ramonAsic{
          cfg::SwitchType::FABRIC,
          switchId,
          switchIndex,
          systemPortRange,
          mac,
          std::nullopt};

      return ramonAsic;
    }
    case cfg::AsicType::ASIC_TYPE_RAMON3: {
      static Ramon3Asic ramon3Asic{
          cfg::SwitchType::FABRIC,
          switchId,
          switchIndex,
          systemPortRange,
          mac,
          std::nullopt};

      return ramon3Asic;
    }
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_YUBA:
      break;
  }

  throw FbossError(
      "Invalid Asic Type: ", apache::thrift::util::enumNameSafe(asicType));
}

uint32_t getFabricPortsPerVirtualDevice(const cfg::AsicType asicType) {
  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
      return 192;
    case cfg::AsicType::ASIC_TYPE_RAMON:
      return 192;
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
      return 160;
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      return 256;
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_YUBA:
      throw FbossError(
          "Fabric ports are not applicable for: ",
          apache::thrift::util::enumNameSafe(asicType));
  }

  throw FbossError(
      "Invalid Asic Type: ", apache::thrift::util::enumNameSafe(asicType));
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
    iter->second.switchType() = *hwEndpoint.switchType();

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

std::map<PortID, FabricEndpoint>
FabricConnectivityManager::processConnectivityInfo(
    const std::map<PortID, FabricEndpoint>& hwConnectivity) {
  // use the hw connectivity + expected connectivity info to derive if there is
  // a match or not
  for (const auto& hwConnectivityEntry : hwConnectivity) {
    processConnectivityInfoForPort(
        hwConnectivityEntry.first, hwConnectivityEntry.second);
  }
  return currentNeighborConnectivity_;
}

// Detect mismatch in expected vs. actual connectivity.
// Points to cabling issues: no or wrong connection.
bool FabricConnectivityManager::isConnectivityInfoMismatch(
    const PortID& portId) {
  const auto& iter = currentNeighborConnectivity_.find(portId);
  if (iter != currentNeighborConnectivity_.end()) {
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
    // specific port is missing from the reachability DB
    // treat it like mimssing info
    return true;
  }

  const auto& endpoint = iter->second;
  if (!*endpoint.isAttached()) {
    // absence of attached point implies issue with connectivity/cabling
    // but can be tracked by mismatch check above
    return false;
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

std::map<PortID, FabricEndpoint>
FabricConnectivityManager::getConnectivityInfo() {
  return currentNeighborConnectivity_;
}

} // namespace facebook::fboss
