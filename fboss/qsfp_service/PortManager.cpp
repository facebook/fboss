// Copyright 2021-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/PortManager.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/json/DynamicConverter.h>
#include "fboss/agent/Utils.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"

namespace facebook::fboss {
namespace {

bool isTransceiverComponent(
    const facebook::fboss::phy::PortComponent& component) {
  return component == facebook::fboss::phy::PortComponent::TRANSCEIVER_SYSTEM ||
      component == facebook::fboss::phy::PortComponent::TRANSCEIVER_LINE;
}

bool isXphyComponent(const facebook::fboss::phy::PortComponent& component) {
  return component == phy::PortComponent::GB_SYSTEM ||
      component == phy::PortComponent::GB_LINE;
}

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

std::map<int, NpuPortStatus> getNpuPortStatus(
    const std::map<int32_t, PortStatus>& portStatus) {
  std::map<int, NpuPortStatus> npuPortStatus;
  for (const auto& [portId, status] : portStatus) {
    NpuPortStatus npuStatus;
    npuStatus.portId = portId;
    npuStatus.operState = folly::copy(status.up().value());
    npuStatus.portEnabled = folly::copy(status.enabled().value());
    npuStatus.profileID = status.profileID().value();
    npuPortStatus.emplace(portId, npuStatus);
  }
  return npuPortStatus;
}

PortStateMachineState operStateToPortState(bool operState) {
  return operState ? PortStateMachineState::PORT_UP
                   : PortStateMachineState::PORT_DOWN;
}

PortStateMachineEvent getPortStatusChangeEvent(PortStateMachineState newState) {
  return newState == PortStateMachineState::PORT_UP
      ? PortStateMachineEvent::PORT_EV_SET_PORT_UP
      : PortStateMachineEvent::PORT_EV_SET_PORT_DOWN;
}

phy::Side prbsComponentToPhySide(phy::PortComponent component) {
  switch (component) {
    case phy::PortComponent::ASIC:
      throw FbossError("qsfp_service doesn't support program ASIC prbs");
    case phy::PortComponent::GB_SYSTEM:
    case phy::PortComponent::TRANSCEIVER_SYSTEM:
      return phy::Side::SYSTEM;
    case phy::PortComponent::GB_LINE:
    case phy::PortComponent::TRANSCEIVER_LINE:
      return phy::Side::LINE;
  };
  throw FbossError(
      "Unsupported prbs component: ",
      apache::thrift::util::enumNameSafe(component));
}
} // namespace

PortManager::PortManager(
    TransceiverManager* transceiverManager,
    std::unique_ptr<PhyManager> phyManager,
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
        threads)
    : platformMapping_(platformMapping),
      transceiverManager_(transceiverManager),
      phyManager_(std::move(phyManager)),
      cachedXphyPorts_(getXphyPortsCache()),
      threads_(threads),
      tcvrToPortMap_(getTcvrToPortMap(platformMapping_)),
      portToTcvrMap_(getPortToTcvrMap(platformMapping_)),
      portNameToPortID_(setupPortNameToPortIDMap()),
      stateMachineControllers_(setupPortToStateMachineControllerMap()),
      tcvrToInitializedPorts_(setupTcvrToSynchronizedPortSet()) {
  // Initialize PhyManager publish callback
  if (phyManager_) {
    phyManager_->setPublishPhyCb(
        [this](auto&& portName, auto&& newInfo, auto&& portStats) {
          if (newInfo.has_value()) {
            publishPhyStateToFsdb(
                std::string(portName), std::move(*newInfo->state()));
            publishPhyStatToFsdb(
                std::string(portName), std::move(*newInfo->stats()));
          } else {
            publishPhyStateToFsdb(std::string(portName), std::nullopt);
          }
          if (portStats.has_value()) {
            publishPortStatToFsdb(std::move(portName), std::move(*portStats));
          }
        });
  }
}

PortManager::~PortManager() {
  if (!isExiting_) {
    setGracefulExitingFlag();
    stopThreads();
  }
}

void PortManager::gracefulExit() {
  XLOG(INFO) << "[Exit] Starting PortManager graceful exit";
  setGracefulExitingFlag();

  setWarmBootState();

  stopThreads();

  if (phyManager_) {
    phyManager_->gracefulExit();
  }

  XLOG(INFO) << "[Exit] Ending PortManager graceful exit";
}

void PortManager::stopThreads() {
  drainAllStateMachineUpdates();

  if (updateThread_) {
    updateEventBase_->runInEventBaseThreadAndWait(
        [this] { updateEventBase_->terminateLoopSoon(); });
    updateThread_->join();
    XLOG(DBG2) << "Terminated PortStateMachineUpdateThread";
  }

  if (updateThreadHeartbeat_) {
    updateThreadHeartbeat_.reset();
  }
}

void PortManager::threadLoop(
    folly::StringPiece name,
    folly::EventBase* eventBase) {
  initThread(name);
  eventBase->loopForever();
}

void PortManager::init() {
  XLOG(DBG2) << "Started PortStateMachineUpdateThread";
  updateEventBase_ = std::make_unique<folly::EventBase>();
  updateThread_.reset(new std::thread([=, this] {
    this->threadLoop("PortStateMachineUpdateThread", updateEventBase_.get());
  }));

  auto heartbeatStatsFunc = [](int /* delay */, int /* backLog */) {};
  updateThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      updateEventBase_.get(),
      "updateThreadHeartbeat",
      FLAGS_state_machine_update_thread_heartbeat_ms,
      heartbeatStatsFunc);

  if (transceiverManager_->canWarmBoot()) {
    restoreAgentConfigAppliedInfo();
  }

  initExternalPhyMap();
}

void PortManager::syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) {
  // Get transceiver ids that we want to update ports for.
  std::set<TransceiverID> tcvrIDs;
  for (const auto& [_, portStatus] : *ports) {
    if (auto tcvrIdx = portStatus.transceiverIdx()) {
      tcvrIDs.insert(TransceiverID(*tcvrIdx->transceiverId()));
    }
  }

  updatePortActiveState(*ports);

  // Only fetch the transceivers for the input ports.
  for (auto tcvrId : tcvrIDs) {
    // We only want to return info for transceivers that are not absent.
    const auto& tcvrInfo =
        transceiverManager_->getTransceiverInfoOptional(tcvrId);
    if (!tcvrInfo) {
      continue;
    }
    info[tcvrId] = *tcvrInfo;
  }
}

bool PortManager::initExternalPhyMap(bool forceWarmboot) {
  return true;
}

void PortManager::programXphyPort(
    PortID portId,
    cfg::PortProfileID portProfileId) {
  // This is used solely through Thrift API.
  if (!phyManager_) {
    throw FbossError("Unable to program xphy port when PhyManager is not set");
  }

  // Check if port requires XPHY programming.
  if (cachedXphyPorts_.find(portId) == cachedXphyPorts_.end()) {
    XLOG(DBG2) << "Skip programming xphy port for Port=" << portId;
    return;
  }

  // TODO(smenta): If Y-Cable will be supported on XPHY ports, we need to
  // iterate through all transceivers for the given port.
  auto tcvrID = getLowestIndexedTransceiverForPort(portId);
  std::optional<TransceiverInfo> tcvrInfo =
      transceiverManager_->getTransceiverInfoOptional(tcvrID);

  if (!tcvrInfo) {
    auto portNameStr = getPortNameByPortIdOrThrow(portId);
    SW_PORT_LOG(WARNING, "", portNameStr, portId)
        << "Port doesn't have transceiver info for transceiver id:" << tcvrID;
  }

  phyManager_->programOnePort(
      portId, portProfileId, tcvrInfo, false /* needResetDataPath */);
}

phy::PhyInfo PortManager::getXphyInfo(PortID portId) {
  if (!phyManager_) {
    throw FbossError("Unable to get xphy info when PhyManager is not set");
  }

  if (auto phyInfoOpt = phyManager_->getXphyInfo(portId)) {
    return *phyInfoOpt;
  } else {
    throw FbossError("Unable to get xphy info for port: ", portId);
  }
}

void PortManager::updateAllXphyPortsStats() {
  if (!phyManager_) {
    // If there's no PhyManager for such platform, skip updating xphy stats
    return;
  }
  // Then we need to update all the programmed port xphy stats
  phyManager_->updateAllXphyPortsStats();
}

std::vector<PortID> PortManager::getMacsecCapablePorts() const {
  if (!phyManager_) {
    return {};
  }
  return phyManager_->getPortsSupportingFeature(
      phy::ExternalPhy::Feature::MACSEC);
}

std::string PortManager::listHwObjects(
    std::vector<HwObjectType>& hwObjects,
    bool cached) const {
  if (!phyManager_) {
    return "";
  }
  return phyManager_->listHwObjects(hwObjects, cached);
}

bool PortManager::getSdkState(const std::string& filename) const {
  if (!phyManager_) {
    return false;
  }
  return phyManager_->getSdkState(filename);
}

std::string PortManager::getPortInfo(const std::string& portNameStr) {
  if (!phyManager_) {
    return "";
  }
  auto swPort = getPortIDByPortNameOrThrow(portNameStr);
  return phyManager_->getPortInfoStr(PortID(swPort));
}

void PortManager::setPortLoopbackState(
    const std::string& portNameStr,
    phy::PortComponent component,
    bool setLoopback) {
  auto portId = getPortIDByPortNameOrThrow(portNameStr);
  if (!isXphyComponent(component) && !isTransceiverComponent(component)) {
    XLOG(INFO)
        << " TransceiverManager::setPortLoopbackState - component not supported "
        << apache::thrift::util::enumNameSafe(component);
    return;
  }

  XLOG(INFO) << " TransceiverManager::setPortLoopbackState Port "
             << static_cast<int>(portId);

  if (isXphyComponent(component)) {
    phyManager_->setPortLoopbackState(PortID(portId), component, setLoopback);
  } else {
    transceiverManager_->setPortLoopbackStateTransceiver(
        portId, portNameStr, component, setLoopback);
  }
}

void PortManager::setPortAdminState(
    const std::string& portNameStr,
    phy::PortComponent component,
    bool setAdminUp) {
  auto portId = getPortIDByPortNameOrThrow(portNameStr);
  if (!isXphyComponent(component)) {
    XLOG(INFO)
        << " TransceiverManager::setPortAdminState - component not supported "
        << apache::thrift::util::enumNameSafe(component);
    return;
  }

  XLOG(INFO) << " TransceiverManager::setPortAdminState Port "
             << static_cast<int>(portId);
  phyManager_->setPortAdminState(PortID(portId), component, setAdminUp);
}

void PortManager::getSymbolErrorHistogram(
    CdbDatapathSymErrHistogram& symErr,
    const std::string& portNameStr) {}

const std::set<std::string> PortManager::getPortNames(
    TransceiverID tcvrId) const {
  std::set<std::string> portNames;
  auto tcvrToPortMapItr = tcvrToPortMap_.find(tcvrId);
  if (tcvrToPortMapItr == tcvrToPortMap_.end() ||
      tcvrToPortMapItr->second.size() == 0) {
    throw FbossError("No ports found for transceiver ", tcvrId);
  }

  for (const auto& port : tcvrToPortMapItr->second) {
    portNames.insert(getPortNameByPortIdOrThrow(port));
  }
  return portNames;
}

PortStateMachineState PortManager::getPortState(PortID portId) const {
  auto stateMachineItr = stateMachineControllers_.find(portId);
  if (stateMachineItr == stateMachineControllers_.end()) {
    throw FbossError("Port:", portId, " doesn't exist");
  }
  return stateMachineItr->second->getCurrentState();
}

PortID PortManager::getLowestIndexedPortForTransceiverPortGroup(
    PortID portId) const {
  TransceiverID tcvrId = getLowestIndexedTransceiverForPort(portId);

  // Find lowest indexed port assigned to the above transceiver.
  auto tcvrToPortMapItr = tcvrToInitializedPorts_.find(tcvrId);
  if (tcvrToPortMapItr == tcvrToInitializedPorts_.end()) {
    throw FbossError("No ports found for transceiver ", tcvrId);
  }
  auto lockedInitializedPorts = tcvrToPortMapItr->second->rlock();
  if (lockedInitializedPorts->size() == 0) {
    throw FbossError("No ports found for transceiver ", tcvrId);
  }

  return *lockedInitializedPorts->begin();
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

bool PortManager::hasPortFinishedIphyProgramming(PortID portId) const {
  return getPortState(portId) == PortStateMachineState::IPHY_PORTS_PROGRAMMED ||
      hasPortFinishedXphyProgramming(portId);
  ;
}

bool PortManager::hasPortFinishedXphyProgramming(PortID portId) const {
  static const std::array<PortStateMachineState, 4> kPastIphyProgrammedStates{
      PortStateMachineState::XPHY_PORTS_PROGRAMMED,
      PortStateMachineState::TRANSCEIVERS_PROGRAMMED,
      PortStateMachineState::PORT_DOWN,
      PortStateMachineState::PORT_UP};

  return std::find(
             kPastIphyProgrammedStates.begin(),
             kPastIphyProgrammedStates.end(),
             getPortState(portId)) != kPastIphyProgrammedStates.end();
  ;
}

TransceiverStateMachineState PortManager::getTransceiverState(
    TransceiverID tcvrId) const {
  return transceiverManager_->getCurrentState(tcvrId);
}

void PortManager::programInternalPhyPorts(TransceiverID id) {
  std::map<int32_t, cfg::PortProfileID> programmedIphyPorts;

  const auto& overrideTcvrToPortAndProfileForTest =
      transceiverManager_->getOverrideTcvrToPortAndProfileForTesting();
  if (auto overridePortAndProfileIt =
          overrideTcvrToPortAndProfileForTest.find(id);
      overridePortAndProfileIt != overrideTcvrToPortAndProfileForTest.end()) {
    // NOTE: This is only used for testing.
    for (const auto& [portId, profileID] : overridePortAndProfileIt->second) {
      programmedIphyPorts.emplace(portId, profileID);
    }
  } else {
    // Then call wedge_agent programInternalPhyPorts
    auto wedgeAgentClient = utils::createWedgeAgentClient();
    wedgeAgentClient->sync_programInternalPhyPorts(
        programmedIphyPorts,
        transceiverManager_->getTransceiverInfo(id),
        false);
  }

  std::string logStr = folly::to<std::string>(
      "programInternalPhyPorts() for Transceiver=", id, " return [");
  for (const auto& [portId, profileID] : programmedIphyPorts) {
    logStr = folly::to<std::string>(
        logStr,
        portId,
        " : ",
        apache::thrift::util::enumNameSafe(profileID),
        ", ");
  }
  XLOG(INFO) << logStr << "]";

  // Now update the programmed SW port to profile mapping
  const auto& portToPortInfoIt =
      transceiverManager_->getSynchronizedProgrammedIphyPortToPortInfo(id);
  if (!portToPortInfoIt) {
    return;
  }

  auto portToPortInfoWithLock = portToPortInfoIt->wlock();
  portToPortInfoWithLock->clear();
  for (auto [portId, profileID] : programmedIphyPorts) {
    TransceiverManager::TransceiverPortInfo portInfo;
    portInfo.profile = profileID;
    NpuPortStatus status{};
    status.portEnabled = true;
    // Set port down by default, will be changed if / when
    // updateTransceiverPortStatus brings port up.
    status.operState = false;
    portInfo.status = status;
    portToPortInfoWithLock->emplace(PortID(portId), portInfo);
  }

  if (!phyManager_) {
    // If phyManager_ doesn't exist, this means that all PHY programming is
    // complete.
    transceiverManager_->markTransceiverReadyForProgramming(id, true);
  }
}

void PortManager::programExternalPhyPorts(
    TransceiverID tcvrId,
    bool xPhyNeedResetDataPath) {
  // This is used solely through internal state machine.
  if (phyManager_) {
    const auto& tcvrToInitializedPorts_It =
        tcvrToInitializedPorts_.find(tcvrId);
    if (tcvrToInitializedPorts_It == tcvrToInitializedPorts_.end()) {
      return;
    }

    const auto initializedPorts = *tcvrToInitializedPorts_It->second->rlock();
    const auto& portToPortInfoIt =
        transceiverManager_->getSynchronizedProgrammedIphyPortToPortInfo(
            tcvrId);
    if (!portToPortInfoIt) {
      // This is due to the iphy ports are disabled. So no need to program
      // xphy
      XLOG(DBG2) << "Skip programming xphy ports for Transceiver=" << tcvrId
                 << ". Can't find programmed iphy port and port info";
      return;
    }

    const auto& programmedPortToPortInfo = portToPortInfoIt->rlock();
    const auto& transceiverInfo =
        transceiverManager_->getTransceiverInfo(tcvrId);

    for (const auto& portId : initializedPorts) {
      if (cachedXphyPorts_.find(portId) == cachedXphyPorts_.end()) {
        XLOG(DBG2) << "Skip programming xphy port for Port=" << portId;
        continue;
      }

      auto portToPortInfoItr = programmedPortToPortInfo->find(portId);
      if (portToPortInfoItr == programmedPortToPortInfo->end()) {
        continue;
      }

      auto portProfile = portToPortInfoItr->second.profile;
      phyManager_->programOnePort(
          portId, portProfile, transceiverInfo, xPhyNeedResetDataPath);
      XLOG(INFO) << "Programmed XPHY port for Transceiver=" << tcvrId
                 << ", Port=" << portId << ", Profile="
                 << apache::thrift::util::enumNameSafe(portProfile)
                 << ", needResetDataPath=" << xPhyNeedResetDataPath;
    }
  }

  transceiverManager_->markTransceiverReadyForProgramming(tcvrId, true);
}

phy::PhyInfo PortManager::getPhyInfo(const std::string& portNameStr) {
  if (!phyManager_) {
    return phy::PhyInfo();
  }
  auto swPort = getPortIDByPortNameOrThrow(portNameStr);
  return phyManager_->getPhyInfo(PortID(swPort));
}

const std::map<int32_t, NpuPortStatus>&
PortManager::getOverrideAgentPortStatusForTesting() const {
  return overrideAgentPortStatusForTesting_;
}

void PortManager::setOverrideAgentPortStatusForTesting(
    const std::unordered_set<PortID>& upPortIds,
    const std::unordered_set<PortID>& enabledPortIds,
    bool clearOnly) {
  overrideAgentPortStatusForTesting_.clear();
  if (clearOnly) {
    return;
  }
  for (const auto& it :
       transceiverManager_->getOverrideTcvrToPortAndProfileForTesting()) {
    for (const auto& [portId, profileID] : it.second) {
      // If portIds is provided, only enable those ports; otherwise, use
      // 'enabled'
      NpuPortStatus status;
      status.portEnabled = enabledPortIds.find(portId) != enabledPortIds.end();
      status.operState = upPortIds.find(portId) != upPortIds.end();
      status.profileID = apache::thrift::util::enumNameSafe(profileID);
      overrideAgentPortStatusForTesting_.emplace(portId, status);
    }
  }
}

void PortManager::setOverrideAllAgentPortStatusForTesting(
    bool up,
    bool enabled,
    bool clearOnly) {
  overrideAgentPortStatusForTesting_.clear();
  if (clearOnly) {
    return;
  }

  for (const auto& it :
       transceiverManager_->getOverrideTcvrToPortAndProfileForTesting()) {
    for (const auto& [portId, profileID] : it.second) {
      NpuPortStatus status;
      status.portEnabled = enabled;
      status.operState = up;
      status.profileID = apache::thrift::util::enumNameSafe(profileID);
      overrideAgentPortStatusForTesting_.emplace(portId, status);
    }
  }
}

void PortManager::setOverrideAgentConfigAppliedInfoForTesting(
    std::optional<ConfigAppliedInfo> configAppliedInfo) {
  overrideAgentConfigAppliedInfoForTesting_ = configAppliedInfo;
}

void PortManager::getAllPortSupportedProfiles(
    std::map<std::string, std::vector<cfg::PortProfileID>>&
        supportedPortProfiles,
    bool checkOptics) {
  // Find the list of all available ports from agent config
  std::vector<std::string> availablePorts;
  for (const auto& [tcvrID, initializedPorts] : tcvrToInitializedPorts_) {
    auto lockedPorts = initializedPorts->rlock();
    for (const auto& port : *lockedPorts) {
      auto portNameStr = getPortNameByPortIdOrThrow(port);
      availablePorts.push_back(portNameStr);
    }
  }

  // Get all possible port profiles for all the ports from platform mapping.
  // Exclude the ports which are not configured by agent config
  auto allPossiblePortProfiles = platformMapping_->getAllPortProfiles();
  std::map<std::string, std::vector<cfg::PortProfileID>>
      allConfiguredPortProfiles;
  for (auto& portNameStr : availablePorts) {
    if (allPossiblePortProfiles.find(portNameStr) !=
        allPossiblePortProfiles.end()) {
      allConfiguredPortProfiles[portNameStr] =
          allPossiblePortProfiles[portNameStr];
    }
  }

  // If we don't need to check the optics support of the profile then return all
  // supported port profiles from the platform mapping which are configured by
  // agent config.
  if (!checkOptics) {
    supportedPortProfiles = allConfiguredPortProfiles;
    return;
  }

  // Otherwise, match optics support.
  for (auto& [portNameStr, portProfiles] : allConfiguredPortProfiles) {
    auto portId = getPortIDByPortName(portNameStr);
    if (!portId.has_value()) {
      continue;
    }
    // Check if the transceiver supports the port profile
    for (auto& profileID : portProfiles) {
      auto tcvrHostLanes = platformMapping_->getTransceiverHostLanes(
          PlatformPortProfileConfigMatcher(
              profileID /* profileID */,
              *portId /* portID */,
              std::nullopt /* portConfigOverrideFactor */));
      if (tcvrHostLanes.empty()) {
        continue;
      }
      auto tcvrStartLane = *tcvrHostLanes.begin();
      auto profileCfgOpt = platformMapping_->getPortProfileConfig(
          PlatformPortProfileConfigMatcher(profileID));
      if (!profileCfgOpt) {
        continue;
      }
      const auto speed = *profileCfgOpt->speed();
      TransceiverPortState portState;
      portState.portName = portNameStr;
      portState.startHostLane = tcvrStartLane;
      portState.speed = speed;
      portState.numHostLanes = tcvrHostLanes.size();
      portState.transmitterTech = profileCfgOpt->iphy()->medium().value_or({});

      // We should never have a portID in the platform mapping that doesn't have
      // an associated transceiverID.
      auto tcvrID = getLowestIndexedTransceiverForPort(*portId);
      if (transceiverManager_->isTransceiverPortStateSupported(
              tcvrID, portState)) {
        supportedPortProfiles[portNameStr].push_back(profileID);
      }
    }
  }
}

std::optional<PortID> PortManager::getPortIDByPortName(
    const std::string& portNameStr) const {
  auto portMapIt = portNameToPortID_.left.find(portNameStr);
  if (portMapIt != portNameToPortID_.left.end()) {
    return portMapIt->second;
  }
  return std::nullopt;
}

PortID PortManager::getPortIDByPortNameOrThrow(
    const std::string& portNameStr) const {
  auto portId = getPortIDByPortName(portNameStr);
  if (!portId) {
    throw FbossError("No PortID found for port name: ", portNameStr);
  }
  return *portId;
}

std::optional<std::string> PortManager::getPortNameByPortId(
    PortID portId) const {
  auto portMapIt = portNameToPortID_.right.find(portId);
  if (portMapIt != portNameToPortID_.right.end()) {
    return portMapIt->second;
  }
  return std::nullopt;
}

std::string PortManager::getPortNameByPortIdOrThrow(PortID portId) const {
  auto portNameStr = getPortNameByPortId(portId);
  if (!portNameStr) {
    throw FbossError("No port name found for PortID: ", portId);
  }
  return *portNameStr;
}

std::vector<PortID> PortManager::getAllPlatformPorts(
    TransceiverID tcvrID) const {
  auto tcvrToPortMapItr = tcvrToPortMap_.find(tcvrID);
  if (tcvrToPortMapItr == tcvrToPortMap_.end() ||
      tcvrToPortMapItr->second.size() == 0) {
    throw FbossError("No ports found for transceiver ", tcvrID);
  }
  return tcvrToPortMapItr->second;
}

void PortManager::publishLinkSnapshots(const std::string& portNameStr) {}

void PortManager::getInterfacePhyInfo(
    std::map<std::string, phy::PhyInfo>& phyInfos,
    const std::string& portNameStr) {
  auto portIDOpt = getPortIDByPortName(portNameStr);
  if (!portIDOpt) {
    throw FbossError(
        "Unrecoginized portName:", portNameStr, ", can't find port id");
  }
  try {
    phyInfos[portNameStr] = getXphyInfo(*portIDOpt);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Fetching PhyInfo for " << portNameStr << " failed with "
              << ex.what();
  }
}

void PortManager::getAllInterfacePhyInfo(
    std::map<std::string, phy::PhyInfo>& phyInfos) {
  for (const auto& [portNameStr, _] : portNameToPortID_) {
    getInterfacePhyInfo(phyInfos, portNameStr);
  }
}

void PortManager::setPortPrbs(
    PortID portId,
    phy::PortComponent component,
    const phy::PortPrbsState& state) {
  auto portNameStr = getPortNameByPortIdOrThrow(portId);

  prbs::InterfacePrbsState newState;
  newState.polynomial() = prbs::PrbsPolynomial(state.polynominal().value());
  newState.generatorEnabled() = state.enabled().value();
  newState.checkerEnabled() = state.enabled().value();
  setInterfacePrbs(portNameStr, component, newState);
}

phy::PrbsStats PortManager::getPortPrbsStats(
    PortID portId,
    phy::PortComponent component) const {
  phy::Side side = prbsComponentToPhySide(component);
  if (isTransceiverComponent(component)) {
    return transceiverManager_->getPortPrbsStatsTransceiver(portId, side);
  } else {
    if (!phyManager_) {
      throw FbossError("Current platform doesn't support xphy");
    }
    phy::PrbsStats stats;
    auto lanePrbsStats = phyManager_->getPortPrbsStats(portId, side);
    for (const auto& lane : lanePrbsStats) {
      stats.laneStats()->push_back(lane);
      auto timeCollected = lane.timeCollected().value();
      // Store most recent timeCollected across all lane stats
      if (timeCollected > stats.timeCollected()) {
        stats.timeCollected() = timeCollected;
      }
    }
    stats.portId() = portId;
    stats.component() = component;
    return stats;
  }
}

void PortManager::clearPortPrbsStats(
    PortID portId,
    phy::PortComponent component) {
  auto portNameStr = getPortNameByPortIdOrThrow(portId);
  phy::Side side = prbsComponentToPhySide(component);
  if (isTransceiverComponent(component)) {
    transceiverManager_->clearPortPrbsStatsTransceiver(
        portId, portNameStr, side);
  } else if (!phyManager_) {
    throw FbossError("Current platform doesn't support xphy");
  } else {
    phyManager_->clearPortPrbsStats(portId, prbsComponentToPhySide(component));
  }
}

void PortManager::setInterfacePrbs(
    const std::string& portNameStr,
    phy::PortComponent component,
    const prbs::InterfacePrbsState& state) {
  // Get the port ID first
  auto portId = getPortIDByPortNameOrThrow(portNameStr);

  // Sanity check
  if (!state.generatorEnabled().has_value() &&
      !state.checkerEnabled().has_value()) {
    throw FbossError("Neither generator or checker specified for PRBS setting");
  }

  if (isTransceiverComponent(component)) {
    transceiverManager_->setInterfacePrbsTransceiver(
        portId, portNameStr, component, state);
  } else {
    if (!phyManager_) {
      throw FbossError("Current platform doesn't support xphy");
    }
    // PhyManager is using old portPrbsState
    phy::PortPrbsState phyPrbs;
    phyPrbs.polynominal() = static_cast<int>(state.polynomial().value());
    phyPrbs.enabled() = (state.generatorEnabled().has_value() &&
                         state.generatorEnabled().value()) ||
        (state.checkerEnabled().has_value() && state.checkerEnabled().value());
    phyManager_->setPortPrbs(
        portId, prbsComponentToPhySide(component), phyPrbs);
  }
}

phy::PrbsStats PortManager::getInterfacePrbsStats(
    const std::string& portNameStr,
    phy::PortComponent component) const {
  return getPortPrbsStats(getPortIDByPortNameOrThrow(portNameStr), component);
}

void PortManager::getAllInterfacePrbsStats(
    std::map<std::string, phy::PrbsStats>& prbsStats,
    phy::PortComponent component) const {
  const auto& platformPorts = platformMapping_->getPlatformPorts();
  for (const auto& platformPort : platformPorts) {
    auto portNameStr = platformPort.second.mapping()->name();
    try {
      auto prbsStatsEntry = getInterfacePrbsStats(*portNameStr, component);
      prbsStats[*portNameStr] = prbsStatsEntry;
    } catch (const std::exception& ex) {
      // If PRBS is not enabled on this port, return
      // a default stats / State.
      XLOG(DBG2) << "Failed to get prbs stats for port " << *portNameStr
                 << " with error: " << ex.what();
      prbsStats[*portNameStr] = phy::PrbsStats();
    }
  }
}

void PortManager::clearInterfacePrbsStats(
    const std::string& portNameStr,
    phy::PortComponent component) {
  clearPortPrbsStats(getPortIDByPortNameOrThrow(portNameStr), component);
}

void PortManager::bulkClearInterfacePrbsStats(
    std::unique_ptr<std::vector<std::string>> interfaces,
    phy::PortComponent component) {
  for (const auto& interface : *interfaces) {
    clearInterfacePrbsStats(interface, component);
  }
}

void PortManager::syncNpuPortStatusUpdate(
    std::map<int, facebook::fboss::NpuPortStatus>& portStatus) {
  XLOG(INFO) << "Syncing NPU port status update";
  updateNpuPortStatusCache(portStatus);
  updateTransceiverPortStatus();
}

/*
 * detectTransceiverDiscoveredAndReinitializeCorrespondingPorts
 *
 * This function is called within the context of the normal refresh cycle. With
 * SW ports now being separately managed entitites from transceivers, we need to
 * make sure we re-initialize ports when transceivers are re-discovered.
 */

void PortManager::
    detectTransceiverDiscoveredAndReinitializeCorrespondingPorts() {
  BlockingStateUpdateResultList results;

  for (auto& [tcvrId, lockedPortSetPtr] : tcvrToInitializedPorts_) {
    // Check if transceiver is in a non-programmed state. If so, programming
    // needs to be retriggered.
    bool shouldReinitPorts{false};
    auto currTcvrState = getTransceiverState(tcvrId);
    auto previousTcvrStateIt = lastTcvrStates_.find(tcvrId);
    if (previousTcvrStateIt != lastTcvrStates_.end()) {
      shouldReinitPorts = currTcvrState != previousTcvrStateIt->second &&
          currTcvrState == TransceiverStateMachineState::DISCOVERED;
    }

    auto portSet = *lockedPortSetPtr->rlock();
    for (auto portId : portSet) {
      if (shouldReinitPorts) {
        if (auto result = updateStateBlockingWithoutWait(
                portId, PortStateMachineEvent::PORT_EV_RESET_TO_INITIALIZED)) {
          results.push_back(result);
        }
      }
    }

    lastTcvrStates_[tcvrId] = currTcvrState;
  }

  int numResults = results.size();
  waitForAllBlockingStateUpdateDone(results);

  XLOG_IF(DBG2, numResults > 0)
      << "detectTransceiverDiscoveredAndReinitializeCorrespondingPorts has "
      << numResults << " ports need to reset status.";
}

void PortManager::publishLinkSnapshots(PortID portId) {}

void PortManager::triggerAgentConfigChangeEvent() {
  auto wedgeAgentClient = utils::createWedgeAgentClient();
  ConfigAppliedInfo newConfigAppliedInfo;
  try {
    wedgeAgentClient->sync_getConfigAppliedInfo(newConfigAppliedInfo);
  } catch (const std::exception& ex) {
    // We have retry mechanism to handle failure. No crash here
    XLOG(WARN) << "Failed to call wedge_agent getConfigAppliedInfo(). "
               << folly::exceptionStr(ex);

    // For testing only, if overrideAgentConfigAppliedInfoForTesting_ is set,
    // use it directly; otherwise return without trigger any config changed
    // events
    if (overrideAgentConfigAppliedInfoForTesting_) {
      XLOG(INFO)
          << "triggerAgentConfigChangeEvent is using override ConfigAppliedInfo"
          << ", lastAppliedInMs="
          << *overrideAgentConfigAppliedInfoForTesting_->lastAppliedInMs()
          << ", lastColdbootAppliedInMs="
          << (overrideAgentConfigAppliedInfoForTesting_
                      ->lastColdbootAppliedInMs()
                  ? *overrideAgentConfigAppliedInfoForTesting_
                         ->lastColdbootAppliedInMs()
                  : 0);
      newConfigAppliedInfo = *overrideAgentConfigAppliedInfoForTesting_;
    } else {
      return;
    }
  }

  // Now check if the new timestamp is later than the cached one.
  if (*newConfigAppliedInfo.lastAppliedInMs() <=
      *configAppliedInfo_.lastAppliedInMs()) {
    return;
  }

  // Only need to reset data path if there's a new coldboot
  bool resetDataPath = false;
  std::string resetDataPathLog;
  if (auto lastColdbootAppliedInMs =
          newConfigAppliedInfo.lastColdbootAppliedInMs()) {
    if (auto oldLastColdbootAppliedInMs =
            configAppliedInfo_.lastColdbootAppliedInMs()) {
      resetDataPath = (*lastColdbootAppliedInMs > *oldLastColdbootAppliedInMs);
      if (resetDataPath) {
        resetDataPathLog = folly::to<std::string>(
            "Need reset data path. [Old Coldboot time:",
            *oldLastColdbootAppliedInMs,
            ", New Coldboot time:",
            *lastColdbootAppliedInMs,
            "]");
      }
    } else {
      // Always reset data path the cached info doesn't have coldboot config
      // applied time
      resetDataPath = true;
      resetDataPathLog = folly::to<std::string>(
          "Need reset data path. [Old Coldboot time:0, New Coldboot time:",
          *lastColdbootAppliedInMs,
          "]");
    }
  }

  XLOG(INFO) << "New Agent config applied time:"
             << *newConfigAppliedInfo.lastAppliedInMs()
             << " and last cached time:"
             << *configAppliedInfo_.lastAppliedInMs()
             << ". Issue all ports reprogramming events. " << resetDataPathLog;

  BlockingStateUpdateResultList results;
  for (const auto& [tcvrId, enabledPorts] : tcvrToInitializedPorts_) {
    auto lockedEnabledPorts = enabledPorts->rlock();
    for (auto portId : *lockedEnabledPorts) {
      if (auto result = updateStateBlockingWithoutWait(
              portId, PortStateMachineEvent::PORT_EV_RESET_TO_UNINITIALIZED)) {
        XLOG(ERR) << "Reset port " << portId << " to uninitialized";
        results.push_back(result);
      }
    }
  }
  waitForAllBlockingStateUpdateDone(results);

  transceiverManager_->triggerTransceiverEventsForAgentConfigChangeEvent(
      resetDataPath, newConfigAppliedInfo);
  for (auto& stateMachine : stateMachineControllers_) {
    stateMachine.second->getStateMachine().wlock()->get_attribute(
        xphyNeedResetDataPath) = resetDataPath;
  }

  configAppliedInfo_ = newConfigAppliedInfo;
}

void PortManager::updateTransceiverPortStatus() noexcept {
  steady_clock::time_point begin = steady_clock::now();
  std::unordered_set<PortID> portsForReset;

  // Get updated port status from agent.
  std::map<int32_t, NpuPortStatus> newPortToPortStatus;
  if (!overrideAgentPortStatusForTesting_.empty()) {
    XLOG(WARN) << "[TEST ONLY] Use overrideAgentPortStatusForTesting_ "
               << "for wedge_agent getPortStatus()";
    newPortToPortStatus = overrideAgentPortStatusForTesting_;
  } else if (FLAGS_subscribe_to_state_from_fsdb) {
    newPortToPortStatus = *npuPortStatusCache_.rlock();
  } else {
    try {
      // Then call wedge_agent getPortStatus() to get current port status
      auto wedgeAgentClient = utils::createWedgeAgentClient();
      std::map<int32_t, PortStatus> portStatus;
      wedgeAgentClient->sync_getPortStatus(portStatus, {});
      newPortToPortStatus = getNpuPortStatus(portStatus);
    } catch (const std::exception& ex) {
      // We have retry mechanism to handle failure. No crash here
      XLOG(WARN) << "Failed to call wedge_agent getPortStatus(). "
                 << folly::exceptionStr(ex);
    }
  }
  if (newPortToPortStatus.empty()) {
    XLOG(WARN) << "No port status to process in updateTransceiverPortStatus";
    return;
  }

  int numActiveStatusChanged{0}, numResetToUninitialized{0},
      numSetToInitialized{0};
  BlockingStateUpdateResultList results;
  std::vector<std::pair<PortID, bool>> newPortEnabledStatusForCache;

  for (const auto& [portId, _] : stateMachineControllers_) {
    PortStateMachineState stateMachineState = getPortState(portId);
    bool stateMachineEnabled =
        stateMachineState != PortStateMachineState::UNINITIALIZED;
    bool arePortTcvrsJustProgrammed =
        stateMachineState == PortStateMachineState::TRANSCEIVERS_PROGRAMMED;

    // Extract port data from agent.
    bool newPortStatusEnabled{false};
    auto portToPortItr = newPortToPortStatus.find(portId);
    if (portToPortItr != newPortToPortStatus.end()) {
      newPortStatusEnabled = portToPortItr->second.portEnabled;
    }

    if (newPortStatusEnabled && stateMachineEnabled) {
      PortStateMachineState newState =
          operStateToPortState(portToPortItr->second.operState);
      // The corresponding state machine is already enabled, so we might need
      // to update port up / port down status.
      if (arePortTcvrsJustProgrammed || stateMachineState != newState) {
        auto event = getPortStatusChangeEvent(newState);
        if (auto result =
                enqueueStateUpdateForPortWithoutExecuting(portId, event)) {
          ++numActiveStatusChanged;
          results.push_back(result);
        }
      }
    } else if (newPortStatusEnabled) {
      // We need to enable this port in our state machine.
      if (auto result = enqueueStateUpdateForPortWithoutExecuting(
              portId, PortStateMachineEvent::PORT_EV_INITIALIZE_PORT)) {
        ++numSetToInitialized;
        newPortEnabledStatusForCache.emplace_back(portId, true);
        results.push_back(result);
      }
    } else if (stateMachineEnabled) {
      // We need to disable this port in our state machine.
      if (auto result = enqueueStateUpdateForPortWithoutExecuting(
              portId, PortStateMachineEvent::PORT_EV_RESET_TO_UNINITIALIZED)) {
        ++numResetToUninitialized;
        portsForReset.insert(portId);
        newPortEnabledStatusForCache.emplace_back(portId, false);
        results.push_back(result);
      }
    }
  }

  // TODO(smenta) – Revisit ordering of these updates.
  // We want to collect the list of transceivers whose state machines need to be
  // reset based on the ports that are being reset. However, because we embed
  // updating the enabled ports cache in state machines directly, we need to
  // collect this list prior to executing the state machine updates.
  auto tcvrsForReset = getTransceiversWithAllPortsInSet(portsForReset);

  if (!results.empty()) {
    // Signal the update thread that updates are pending.
    executeStateUpdates();
    waitForAllBlockingStateUpdateDone(results);
  }

  // Clear any stale port data in TransceiverManager.
  transceiverManager_->resetProgrammedIphyPortToPortInfoForPorts(portsForReset);

  // Update transceiver state machines based on portsForReset. We only reset
  // transceivers that have all their enabled ports in this list.
  transceiverManager_->triggerResetEvents(tcvrsForReset);

  XLOG_IF(
      DBG2,
      numActiveStatusChanged + numResetToUninitialized + numSetToInitialized >
          0)
      << "updateTransceiverPortStatus has " << numActiveStatusChanged
      << " ports need to update port active status, " << numResetToUninitialized
      << " ports that need to be disabled, and " << numSetToInitialized
      << " ports that need to be enabled. Total execute time(ms): "
      << duration_cast<std::chrono::milliseconds>(steady_clock::now() - begin)
             .count();
}

void PortManager::updatePortActiveStatusInTransceiverManager() {
  for (auto& [tcvrId, lockedInitializedPorts] : tcvrToInitializedPorts_) {
    std::unordered_set<PortID> activePorts;
    // Create a copy to avoid holding two locks at once.
    std::set<PortID> initializedPorts = *lockedInitializedPorts->rlock();
    for (const auto& portId : initializedPorts) {
      auto portState = getPortState(portId);
      if (portState == PortStateMachineState::PORT_UP) {
        activePorts.insert(portId);
      }
    }

    const auto& portToPortInfoIt =
        transceiverManager_->getSynchronizedProgrammedIphyPortToPortInfo(
            tcvrId);
    if (!portToPortInfoIt) {
      continue;
    }

    auto portToPortInfoWithLock = portToPortInfoIt->wlock();
    for (auto& [portId, portInfo] : *portToPortInfoWithLock) {
      if (!portInfo.status.has_value()) {
        portInfo.status = NpuPortStatus{};
      }
      portInfo.status->portEnabled =
          initializedPorts.find(portId) != initializedPorts.end();
      portInfo.status->operState =
          activePorts.find(portId) != activePorts.end();
    }
  }
}

void PortManager::restoreAgentConfigAppliedInfo() {
  auto warmbootState = transceiverManager_->getWarmBootState();
  if (warmbootState.isNull()) {
    return;
  }
  if (const auto& agentConfigAppliedIt = warmbootState.find(
          TransceiverManager::kAgentConfigAppliedInfoStateKey);
      agentConfigAppliedIt != warmbootState.items().end()) {
    auto agentConfigAppliedInfo = agentConfigAppliedIt->second;
    ConfigAppliedInfo wbConfigAppliedInfo;
    // Restore the last agent config applied timestamp from warm boot state if
    // it exists
    if (const auto& lastAppliedIt = agentConfigAppliedInfo.find(
            TransceiverManager::kAgentConfigLastAppliedInMsKey);
        lastAppliedIt != agentConfigAppliedInfo.items().end()) {
      wbConfigAppliedInfo.lastAppliedInMs() =
          folly::convertTo<long>(lastAppliedIt->second);
    }
    // Restore the last agent coldboot timestamp from warm boot state if
    // it exists
    if (const auto& lastColdBootIt = agentConfigAppliedInfo.find(
            TransceiverManager::kAgentConfigLastColdbootAppliedInMsKey);
        lastColdBootIt != agentConfigAppliedInfo.items().end()) {
      wbConfigAppliedInfo.lastColdbootAppliedInMs() =
          folly::convertTo<long>(lastColdBootIt->second);
    }

    configAppliedInfo_ = wbConfigAppliedInfo;
  }
}

void PortManager::updateNpuPortStatusCache(
    std::map<int, facebook::fboss::NpuPortStatus>& portStatus) {
  npuPortStatusCache_.wlock()->swap(portStatus);
}

PortManager::PortToStateMachineControllerMap
PortManager::setupPortToStateMachineControllerMap() {
  if (!platformMapping_) {
    throw FbossError("Platform Mapping must be non-null.");
  }

  PortManager::PortToStateMachineControllerMap stateMachineMap;
  for (const auto& [portId, tcvrList] : portToTcvrMap_) {
    if (tcvrList.empty()) {
      XLOG(INFO) << "No transceivers found for " << portId
                 << ", skipping creation of PortStateMachineController.";
      continue;
    }
    auto stateMachineController =
        std::make_unique<PortManager::PortStateMachineController>(portId);
    auto& stateMachine = stateMachineController->getStateMachine();
    stateMachine.withWLock([&](auto& lockedStateMachine) {
      lockedStateMachine.get_attribute(portMgrPtr) = this;
    });
    stateMachineMap.emplace(portId, std::move(stateMachineController));
  }

  return stateMachineMap;
}

void PortManager::updateStateBlocking(PortID id, PortStateMachineEvent event) {
  auto result = updateStateBlockingWithoutWait(id, event);
  if (result) {
    result->wait();
  }
}
std::shared_ptr<BlockingStateMachineUpdateResult>
PortManager::updateStateBlockingWithoutWait(
    PortID id,
    PortStateMachineEvent event) {
  auto result = std::make_shared<BlockingStateMachineUpdateResult>();
  auto update = std::make_unique<PortManager::BlockingPortStateMachineUpdate>(
      event, result);
  if (!updateState(id, std::move(update))) {
    // Only return blocking result if the update has been added in queue
    result = nullptr;
  }
  return result;
}
std::shared_ptr<BlockingStateMachineUpdateResult>
PortManager::enqueueStateUpdateForPortWithoutExecuting(
    PortID id,
    PortStateMachineEvent event) {
  auto result = std::make_shared<BlockingStateMachineUpdateResult>();
  auto update = std::make_unique<PortManager::BlockingPortStateMachineUpdate>(
      event, result);
  if (!enqueueStateUpdate(id, std::move(update))) {
    // Only return blocking result if the update has been added in queue
    result = nullptr;
  }
  return result;
}
bool PortManager::updateState(
    const PortID& portId,
    std::unique_ptr<PortStateMachineUpdate> update) {
  if (!enqueueStateUpdate(portId, std::move(update))) {
    return false;
  }
  // Signal the update thread that updates are pending.
  executeStateUpdates();
  return true;
}
bool PortManager::enqueueStateUpdate(
    const PortID& portId,
    std::unique_ptr<PortStateMachineUpdate> update) {
  const auto portNameStr = getPortNameByPortIdOrThrow(portId);
  const std::string eventName =
      apache::thrift::util::enumNameSafe(update->getEvent());
  if (isExiting_) {
    PORT_SM_LOG(WARN, portNameStr, portId)
        << "Skipped queueing event: " << eventName
        << ", since exit already started";
    return false;
  }
  if (!updateEventBase_) {
    PORT_SM_LOG(WARN, portNameStr, portId)
        << "Skipped queueing event: " << eventName
        << ", since updateEventBase_ is not created yet";
    return false;
  }
  auto stateMachineItr = stateMachineControllers_.find(portId);
  if (stateMachineItr == stateMachineControllers_.end()) {
    PORT_SM_LOG(WARN, portNameStr, portId)
        << "Skipped queueing event: " << eventName
        << ", since PortStateMachine doesn't exist";
    return false;
  }
  stateMachineItr->second->enqueueUpdate(std::move(update));

  return true;
}
void PortManager::executeStateUpdates() {
  updateEventBase_->runInEventBaseThread(handlePendingUpdatesHelper, this);
}

void PortManager::handlePendingUpdatesHelper(PortManager* mgr) {
  return mgr->handlePendingUpdates();
};

void PortManager::handlePendingUpdates() {
  // Pending updates are stored within each PortStateMachineController, so this
  // function asks each StateMachineController to execute a single pending
  // update if possible.
  PORTMGR_SM_LOG(INFO) << "Trying to update all PortStateMachines";

  // To expedite all these different ports state update, use Future
  std::vector<folly::Future<folly::Unit>> stateUpdateTasks;
  // In a later diff, will change this to iterate only through enabled ports.
  for (auto& stateMachinePair : stateMachineControllers_) {
    const auto& portId = stateMachinePair.first;
    const auto& stateMachineControllerPtr = stateMachinePair.second;
    const auto& tcvrID = getLowestIndexedTransceiverForPort(portId);
    auto threadsItr = threads_->find(tcvrID);
    if (threadsItr == threads_->end()) {
      PORTMGR_SM_LOG(WARN) << "Can't find ThreadHelper for threadID " << tcvrID
                           << ". Skip updating PortStateMachine.";
      continue;
    }

    stateUpdateTasks.push_back(
        folly::via(threadsItr->second.getEventBase())
            .thenValue([&stateMachineControllerPtr](auto&&) {
              stateMachineControllerPtr->executeSingleUpdate();
            }));
  }
  folly::collectAll(stateUpdateTasks).wait();
};

void PortManager::waitForAllBlockingStateUpdateDone(
    const PortManager::BlockingStateUpdateResultList& results) {
  for (const auto& result : results) {
    if (isExiting_) {
      XLOG(INFO)
          << "Terminating waitForAllBlockingStateUpdateDone for graceful exit";
      return;
    }
    result->wait();
  }
};

void PortManager::drainAllStateMachineUpdates() {
  // Enforce no updates can be added while draining.
  for (auto& [_, stateMachineController] : stateMachineControllers_) {
    stateMachineController->blockNewUpdates();
  }

  // Make sure threads are actually active before we start draining.
  bool allStateMachineThreadsActive{true};
  for (auto& threadHelper : *threads_) {
    if (!threadHelper.second.isThreadActive()) {
      allStateMachineThreadsActive = false;
      break;
    }
  }

  if (!allStateMachineThreadsActive) {
    XLOG(INFO) << "All state machine threads are not active. Skip draining.";
    return;
  }

  // Drain any pending updates by calling handlePendingUpdates directly.
  bool updatesDrained = false;
  do {
    updatesDrained = true;
    executeStateUpdates();
    for (auto& [_, stateMachineController] : stateMachineControllers_) {
      if (stateMachineController->arePendingUpdates()) {
        updatesDrained = false;
        break;
      }
    }
  } while (!updatesDrained);
}

void PortManager::updatePortActiveState(
    const std::map<int32_t, PortStatus>& portStatusMap) noexcept {
  std::map<int32_t, NpuPortStatus> npuPortStatus =
      getNpuPortStatus(portStatusMap);

  int numPortStatusChanged{0};
  BlockingStateUpdateResultList results;

  for (const auto& [portIdInt, portStatus] : npuPortStatus) {
    PortStateMachineState portState;
    auto portId = PortID(portIdInt);
    try {
      portState = getPortState(portId);
    } catch (const FbossError& /* e */) {
      XLOG(WARN) << "Unrecoginized Port:" << portId
                 << ", skip updatePortActiveState()";
      continue;
    }

    XLOG(INFO) << "Syncing port status for port " << portId;
    bool arePortTcvrsJustProgrammed =
        portState == PortStateMachineState::TRANSCEIVERS_PROGRAMMED;
    auto newState = operStateToPortState(portStatus.operState);

    if (arePortTcvrsJustProgrammed ||
        (portStatus.portEnabled && portState != newState)) {
      ++numPortStatusChanged;
      auto event = getPortStatusChangeEvent(newState);
      if (auto result = updateStateBlockingWithoutWait(portId, event)) {
        results.push_back(result);
      }
    }
  }

  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(DBG2, numPortStatusChanged > 0)
      << "updatePortActiveState has " << numPortStatusChanged
      << " ports need to update port status.";
}

std::unordered_set<TransceiverID> PortManager::getTransceiversWithAllPortsInSet(
    const std::unordered_set<PortID>& ports) const {
  std::unordered_set<TransceiverID> transceivers;
  for (const auto& [tcvrID, enabledPorts] : tcvrToInitializedPorts_) {
    auto lockedEnabledPorts = enabledPorts->rlock();
    if (!lockedEnabledPorts->empty() &&
        std::all_of(
            lockedEnabledPorts->begin(),
            lockedEnabledPorts->end(),
            [&](const PortID& portId) { return ports.contains(portId); })) {
      transceivers.insert(tcvrID);
    }
  }

  return transceivers;
}

PortManager::TcvrToSynchronizedPortSet
PortManager::setupTcvrToSynchronizedPortSet() {
  TcvrToSynchronizedPortSet tcvrToPortSet;
  for (const auto& tcvrId :
       utility::getTransceiverIds(platformMapping_->getChips())) {
    tcvrToPortSet.emplace(

        tcvrId, std::make_unique<folly::Synchronized<std::set<PortID>>>());
  }

  return tcvrToPortSet;
}

void PortManager::setPortEnabledStatusInCache(PortID portId, bool enabled) {
  auto tcvrId = getLowestIndexedTransceiverForPort(portId);
  auto tcvrToInitializedPortsItr = tcvrToInitializedPorts_.find(tcvrId);
  if (tcvrToInitializedPortsItr == tcvrToInitializedPorts_.end()) {
    throw FbossError(
        "No transceiver id found in initialized ports cache for transceiver ",
        tcvrId);
  }

  auto lockedEnabledPorts = tcvrToInitializedPortsItr->second->wlock();
  if (enabled) {
    lockedEnabledPorts->insert(portId);
  } else {
    lockedEnabledPorts->erase(portId);
  }
}

const std::unordered_set<PortID> PortManager::getXphyPortsCache() {
  if (!phyManager_) {
    return {};
  }

  std::vector<PortID> portIds = phyManager_->getXphyPorts();
  return std::unordered_set<PortID>(portIds.begin(), portIds.end());
}

PortManager::PortNameIdMap PortManager::setupPortNameToPortIDMap() {
  if (!platformMapping_) {
    throw FbossError("Platform Mapping must be non-null.");
  }

  PortNameIdMap portNameToPortID;
  for (const auto& [portIDInt, platformPort] :
       platformMapping_->getPlatformPorts()) {
    PortID portId = PortID(portIDInt);
    if (auto portToTcvrMapItr = portToTcvrMap_.find(portId);
        portToTcvrMapItr == portToTcvrMap_.end() ||

        portToTcvrMapItr->second.empty()) {
      // Skip ports that are not associated with any transceivers
      continue;
    }

    const auto& portNameStr = *platformPort.mapping()->name();
    portNameToPortID.insert(PortNameIdMap::value_type(portNameStr, portId));
  }

  return portNameToPortID;
}

void PortManager::triggerProgrammingEvents() {
  int32_t numProgramIphy{0}, numProgramXphy{0}, numCheckTcvrsProgrammed{0};

  BlockingStateUpdateResultList results;
  steady_clock::time_point begin = steady_clock::now();

  for (const auto& [tcvrId, lockedPortSetPtr] : tcvrToInitializedPorts_) {
    auto portSet = *lockedPortSetPtr->rlock();
    for (auto portId : portSet) {
      const auto currentState = getPortState(portId);
      bool xphyEnabled = phyManager_ != nullptr;
      bool needProgramIphy = currentState == PortStateMachineState::INITIALIZED;
      bool needProgramXphy =
          currentState == PortStateMachineState::IPHY_PORTS_PROGRAMMED &&
          xphyEnabled;
      bool needCheckTcvrsProgrammed =
          currentState == PortStateMachineState::XPHY_PORTS_PROGRAMMED ||
          (currentState == PortStateMachineState::IPHY_PORTS_PROGRAMMED &&
           !xphyEnabled);
      if (needProgramIphy) {
        if (auto result = updateStateBlockingWithoutWait(
                portId, PortStateMachineEvent::PORT_EV_PROGRAM_IPHY)) {
          ++numProgramIphy;
          results.push_back(result);
        }
      } else if (needProgramXphy && xphyEnabled) {
        if (auto result = updateStateBlockingWithoutWait(
                portId, PortStateMachineEvent::PORT_EV_PROGRAM_XPHY)) {
          ++numProgramXphy;
          results.push_back(result);
        }
      } else if (needCheckTcvrsProgrammed) {
        if (auto result = updateStateBlockingWithoutWait(
                portId,
                PortStateMachineEvent::PORT_EV_CHECK_TCVRS_PROGRAMMED)) {
          results.push_back(result);
        }
      }
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(DBG2, !results.empty())
      << "triggerProgrammingEvents has " << numProgramIphy
      << " IPHY programming, " << numProgramXphy << " XPHY programming, "
      << numCheckTcvrsProgrammed
      << " check tcvrs programmed. Total execute time(ms):"
      << duration_cast<std::chrono::milliseconds>(steady_clock::now() - begin)
             .count();
}

bool PortManager::arePortTcvrsProgrammed(PortID portId) const {
  for (const auto& tcvrId : getTransceiverIdsForPort(portId)) {
    if (transceiverManager_->getCurrentState(tcvrId) !=
        TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED) {
      auto portNameStr = getPortNameByPortIdOrThrow(portId);
      SW_PORT_LOG(INFO, "[SM]", portNameStr, portId)
          << "Assigned Transceiver " << tcvrId
          << " state is not TRANSCEIVER_PROGRAMMED: "
          << apache::thrift::util::enumNameSafe(getTransceiverState(tcvrId))
          << ". Not advancing PortStateMachine to TRANSCEIVERS_PROGRAMMED.";
      return false;
    }
  }

  return true;
}

void PortManager::restoreWarmBootPhyState() {
  // Only need to restore warm boot state if this is a warm boot
  if (!transceiverManager_->canWarmBoot()) {
    XLOG(INFO) << "[Cold Boot] No need to restore warm boot state";
    return;
  }

  const auto& warmBootState = transceiverManager_->getWarmBootState();
  if (const auto& phyStateIt =
          warmBootState.find(TransceiverManager::kPhyStateKey);
      phyManager_ && phyStateIt != warmBootState.items().end()) {
    phyManager_->restoreFromWarmbootState(phyStateIt->second);
  }
}

void PortManager::setWarmBootState() {
  folly::dynamic phyWarmbootState = folly::dynamic(nullptr);
  if (phyManager_) {
    phyWarmbootState = phyManager_->getWarmbootState();
  }
  transceiverManager_->setWarmBootState(phyWarmbootState);
}

void PortManager::refreshStateMachines() {
  XLOG(INFO) << "refreshStateMachines started";
  // Step 1: Refresh all transceivers so that we can get updated present
  // transceivers and transceiver data.
  transceiverManager_->refreshTransceivers();

  // Step 2: Fetch current port status from wedge_agent.
  updateTransceiverPortStatus();

  // Step 3: Update Current Port Statuses in TransceiverManager for Remediation
  // and Tcvr Insert
  updatePortActiveStatusInTransceiverManager();

  // Step 4: Reset port state machines for ports that have transceivers that are
  // recently discovered to retrigger programming.
  detectTransceiverDiscoveredAndReinitializeCorrespondingPorts();

  // Step 5: Check whether there's a wedge_agent config change
  triggerAgentConfigChangeEvent();

  // Step 6: Trigger port programming events.
  triggerProgrammingEvents();

  // Step 7: Trigger transceiver programming events.
  transceiverManager_->triggerProgrammingEvents();

  // TODO(smenta) – Add support for triggering remediation.

  // TODO(smenta) – Need to add support for publishing PIM states.

  // Step 8: Mark full initialization complete.
  transceiverManager_->completeRefresh();
  setWarmBootState();

  XLOG(INFO) << "refreshStateMachines ended";
}

bool PortManager::getXphyNeedResetDataPath(PortID id) const {
  auto stateMachineItr = stateMachineControllers_.find(id);
  if (stateMachineItr == stateMachineControllers_.end()) {
    throw FbossError("Port:", id, " doesn't exist");
  }
  return stateMachineItr->second->getStateMachine().rlock()->get_attribute(
      xphyNeedResetDataPath);
}

void PortManager::programXphyPortPrbs(
    PortID portId,
    phy::Side side,
    const phy::PortPrbsState& prbs) {
  if (!phyManager_) {
    throw FbossError(
        "Unable to programXphyPortPrbs when PhyManager is not set");
  }

  phyManager_->setPortPrbs(portId, side, prbs);
}

phy::PortPrbsState PortManager::getXphyPortPrbs(
    const PortID& portId,
    phy::Side side) {
  return phyManager_->getPortPrbs(portId, side);
}

void PortManager::getPortStates(
    std::map<int32_t, PortStateMachineState>& states,
    std::unique_ptr<std::vector<int32_t>> ids) {
  for (const auto& id : *ids) {
    auto portId = PortID(id);
    try {
      states.emplace(id, getPortState(portId));
    } catch (const FbossError& /* e */) {
      XLOG(WARN) << "Unrecognized Port:" << portId;
    }
  }
}

} // namespace facebook::fboss
