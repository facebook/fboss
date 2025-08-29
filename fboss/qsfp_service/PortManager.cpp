// Copyright 2021-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/PortManager.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss {
namespace {
TcvrToPortMap getTcvrToPortMap(
    const std::shared_ptr<const PlatformMapping> platformMapping) {
  if (!platformMapping) {
    return {};
  }
  return utility::getTcvrToPortMap(
      platformMapping->getPlatformPorts(), platformMapping->getChips());
}

PortToTcvrMap getPortToTcvrMap(
    const std::shared_ptr<const PlatformMapping> platformMapping) {
  if (!platformMapping) {
    return {};
  }
  return utility::getPortToTcvrMap(
      platformMapping->getPlatformPorts(), platformMapping->getChips());
}
} // namespace

PortManager::PortManager(
    TransceiverManager* transceiverManager,
    PhyManager* phyManager,
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
        threads)
    : platformMapping_(platformMapping),
      transceiverManager_(transceiverManager),
      phyManager_(phyManager),
      threads_(threads),
      tcvrToPortMap_(getTcvrToPortMap(platformMapping_)),
      portToTcvrMap_(getPortToTcvrMap(platformMapping_)) {}

PortManager::~PortManager() {}

void PortManager::gracefulExit() {}

void PortManager::init() {}

void PortManager::syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) {}

bool PortManager::initExternalPhyMap(bool forceWarmboot) {
  return true;
}

void PortManager::programXphyPort(
    PortID portId,
    cfg::PortProfileID portProfileId) {}

phy::PhyInfo PortManager::getXphyInfo(PortID portId) {
  return phy::PhyInfo();
}

void PortManager::updateAllXphyPortsStats() {}

std::vector<PortID> PortManager::getMacsecCapablePorts() const {
  return {};
}

std::string PortManager::listHwObjects(
    std::vector<HwObjectType>& hwObjects,
    bool cached) const {
  return "";
}

bool PortManager::getSdkState(std::string filename) const {
  return true;
}

std::string PortManager::getPortInfo(const std::string& portName) {
  return "";
}

void PortManager::setPortLoopbackState(
    const std::string& /* portName */,
    phy::PortComponent /* component */,
    bool /* setLoopback */) {}

void PortManager::setPortAdminState(
    const std::string& /* portName */,
    phy::PortComponent /* component */,
    bool /* setAdminUp */) {}

void PortManager::getSymbolErrorHistogram(
    CdbDatapathSymErrHistogram& symErr,
    const std::string& portName) {}

const std::set<std::string> PortManager::getPortNames(
    TransceiverID tcvrId) const {
  return {};
}

const std::string PortManager::getPortName(TransceiverID tcvrId) const {
  return "";
}

PortStateMachineState PortManager::getPortState(PortID portId) const {
  return PortStateMachineState::UNINITIALIZED;
}

PortID PortManager::getLowestIndexedPortForTransceiverPortGroup(
    PortID portId) const {
  TransceiverID tcvrId = getLowestIndexedTransceiverForPort(portId);

  // Find lowest indexed port assigned to the above transceiver.
  auto tcvrToPortMapItr = tcvrToPortMap_.find(tcvrId);
  if (tcvrToPortMapItr == tcvrToPortMap_.end() ||
      tcvrToPortMapItr->second.size() == 0) {
    throw FbossError("No ports found for transceiver ", tcvrId);
  }
  return tcvrToPortMapItr->second.at(0);
}

TransceiverID PortManager::getLowestIndexedTransceiverForPort(
    PortID portId) const {
  auto portToTcvrMapItr = portToTcvrMap_.find(portId);
  if (portToTcvrMapItr == portToTcvrMap_.end() ||
      portToTcvrMapItr->second.size() == 0) {
    throw FbossError("No transceiver found for port ", portId);
  }

  return portToTcvrMapItr->second.at(0);
}

bool PortManager::isLowestIndexedPortForTransceiverPortGroup(
    PortID portId) const {
  return portId == getLowestIndexedPortForTransceiverPortGroup(portId);
}

std::vector<TransceiverID> PortManager::getTransceiverIdsForPort(
    PortID portId) const {
  auto portToTcvrMapItr = portToTcvrMap_.find(portId);
  if (portToTcvrMapItr == portToTcvrMap_.end() ||
      portToTcvrMapItr->second.size() == 0) {
    throw FbossError("No transceiver found for port ", portId);
  }
  return portToTcvrMapItr->second;
}

TransceiverStateMachineState PortManager::getTransceiverState(
    TransceiverID tcvrId) const {
  return TransceiverStateMachineState::NOT_PRESENT;
}

void PortManager::programInternalPhyPorts(TransceiverID id) {}

void PortManager::programExternalPhyPort(
    PortID portId,
    bool xphyNeedResetDataPath) {}

phy::PhyInfo PortManager::getPhyInfo(const std::string& portName) {
  return phy::PhyInfo();
}

std::unordered_map<PortID, TransceiverManager::TransceiverPortInfo>
PortManager::getProgrammedIphyPortToPortInfo(TransceiverID id) const {
  return {};
}

void PortManager::setOverrideAgentPortStatusForTesting(
    bool up,
    bool enabled,
    bool clearOnly) {}

void PortManager::setOverrideAgentConfigAppliedInfoForTesting(
    std::optional<ConfigAppliedInfo> configAppliedInfo) {}

void PortManager::getAllPortSupportedProfiles(
    std::map<std::string, std::vector<cfg::PortProfileID>>&
        supportedPortProfiles,
    bool checkOptics) {}

std::optional<PortID> PortManager::getPortIDByPortName(
    const std::string& portName) const {
  return std::nullopt;
}

std::string PortManager::getPortNameByPortId(PortID portId) const {
  return "";
}

std::vector<PortID> PortManager::getAllPlatformPorts(
    TransceiverID tcvrID) const {
  return {};
}

void PortManager::publishLinkSnapshots(const std::string& portName) {}

void PortManager::getInterfacePhyInfo(
    std::map<std::string, phy::PhyInfo>& phyInfos,
    const std::string& portName) {}

void PortManager::getAllInterfacePhyInfo(
    std::map<std::string, phy::PhyInfo>& phyInfos) {}

void PortManager::setPortPrbs(
    PortID portId,
    phy::PortComponent component,
    const phy::PortPrbsState& state) {}

phy::PrbsStats PortManager::getPortPrbsStats(
    PortID portId,
    phy::PortComponent component) const {
  return phy::PrbsStats();
}

void PortManager::clearPortPrbsStats(
    PortID portId,
    phy::PortComponent component) {}

void PortManager::setInterfacePrbs(
    const std::string& portName,
    phy::PortComponent component,
    const prbs::InterfacePrbsState& state) {}

phy::PrbsStats PortManager::getInterfacePrbsStats(
    const std::string& portName,
    phy::PortComponent component) const {
  return phy::PrbsStats();
}

void PortManager::getAllInterfacePrbsStats(
    std::map<std::string, phy::PrbsStats>& prbsStats,
    phy::PortComponent component) const {}

void PortManager::clearInterfacePrbsStats(
    const std::string& portName,
    phy::PortComponent component) {}

void PortManager::bulkClearInterfacePrbsStats(
    std::unique_ptr<std::vector<std::string>> interfaces,
    phy::PortComponent component) {}

void PortManager::syncNpuPortStatusUpdate(
    std::map<int, facebook::fboss::NpuPortStatus>& portStatus) {}

void PortManager::setPhyManager(std::unique_ptr<PhyManager> phyManager) {}

void PortManager::publishLinkSnapshots(PortID portID) {}

void PortManager::restoreWarmBootPhyState() {}

void PortManager::triggerAgentConfigChangeEvent() {}

void PortManager::updateTransceiverPortStatus() noexcept {}

void PortManager::restoreAgentConfigAppliedInfo() {}

void PortManager::updateNpuPortStatusCache(
    std::map<int, facebook::fboss::NpuPortStatus>& portStatus) {}
} // namespace facebook::fboss
