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
        threads,
    std::shared_ptr<QsfpFsdbSyncManager> fsdbSyncManager)
    : platformMapping_(platformMapping),
      transceiverManager_(transceiverManager),
      phyManager_(std::move(phyManager)),
      fsdbSyncManager_(std::move(fsdbSyncManager)),
      cachedXphyPorts_(getXphyPortsCache()),
      threads_(threads),
      tcvrToPortMap_(getTcvrToPortMap(platformMapping_)),
      portToTcvrMap_(getPortToTcvrMap(platformMapping_)),
      portNameToPortID_(setupPortNameToPortIDMap()),
      stateMachineControllers_(setupPortToStateMachineControllerMap()),
      tcvrToInitializedPorts_(setupTcvrToSynchronizedPortSet()),
      portToInitializedTcvrs_(setupPortToSynchronizedTcvrVec()) {
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
  auto tcvrId = getLowestIndexedStaticTransceiverForPort(portId);
  std::optional<TransceiverInfo> tcvrInfo =
      transceiverManager_->getTransceiverInfoOptional(tcvrId);

  if (!tcvrInfo) {
    auto portNameStr = getPortNameByPortIdOrThrow(portId);
    SW_PORT_LOG(WARNING, "", portNameStr, portId)
        << "Port doesn't have transceiver info for transceiver id:" << tcvrId;
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

PortID PortManager::getLowestIndexedInitializedPortForTransceiverPortGroup(
    PortID portId) const {
  TransceiverID tcvrId = getLowestIndexedStaticTransceiverForPort(portId);

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

std::optional<TransceiverID>
PortManager::getNonControllingTransceiverIdForMultiTcvr(
    TransceiverID tcvrId) const {
  auto tcvrMap = multiTcvrControllingToNonControllingTcvr_.rlock();
  if (auto itr = tcvrMap->find(tcvrId); itr != tcvrMap->end()) {
    return itr->second;
  }
  return std::nullopt;
}

TransceiverID PortManager::getLowestIndexedStaticTransceiverForPort(
    PortID portId) const {
  auto staticTcvrs = getStaticTransceiversForPort(portId);
  if (staticTcvrs.empty()) {
    throw FbossError("No static transceiver for port ", portId);
  }
  return staticTcvrs.at(0);
}

std::vector<TransceiverID> PortManager::getStaticTransceiversForPort(
    PortID portId) const {
  auto portToTcvrMapItr = portToTcvrMap_.find(portId);
  if (portToTcvrMapItr == portToTcvrMap_.end() ||
      portToTcvrMapItr->second.size() == 0) {
    throw FbossError("No transceiver found for port ", portId);
  }

  return portToTcvrMapItr->second;
}

bool PortManager::isLowestIndexedInitializedPortForTransceiverPortGroup(
    PortID portId) const {
  return portId ==
      getLowestIndexedInitializedPortForTransceiverPortGroup(portId);
}

std::vector<TransceiverID> PortManager::getInitializedTransceiverIdsForPort(
    PortID portId) const {
  auto initializedTcvrsIt = portToInitializedTcvrs_.find(portId);
  if (initializedTcvrsIt == portToInitializedTcvrs_.end()) {
    throw FbossError("No transceiver found for port ", portId);
  }

  auto initializedTcvrs = initializedTcvrsIt->second->rlock();
  return *initializedTcvrs;
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

cfg::PortProfileID PortManager::getPerTransceiverProfile(
    int numTcvrs,
    cfg::PortProfileID profileId) const {
  if (numTcvrs == 1) {
    return profileId;
  } else if (
      numTcvrs == 2 &&
      profileId == cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_COPPER) {
    return cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER;
  }
  throw FbossError(
      apache::thrift::util::enumNameSafe(profileId),
      " is not a supported dual-transceiver ProfileID.");
}

std::unordered_map<TransceiverID, std::map<int32_t, cfg::PortProfileID>>
PortManager::getMultiTransceiverPortProfileIDs(
    const TransceiverID& initTcvrId,
    const std::map<int32_t, cfg::PortProfileID>& agentPortToProfileIDs) const {
  // Check if we need to translate to per-transceiver profile mappings
  if (agentPortToProfileIDs.size() != 1) {
    // If we ever receive two portIDs, we can assume the current transceiver is
    // multi-port, meaning it cannot be subsuming an adjacent transceiver's
    // ports.
    return std::
        unordered_map<TransceiverID, std::map<int32_t, cfg::PortProfileID>>{
            {initTcvrId, agentPortToProfileIDs}};
  }

  auto [portId, profileId] = *agentPortToProfileIDs.begin();

  const auto tcvrChips = utility::getDataPlanePhyChips(
      platformMapping_->getPlatformPorts().find(portId)->second,
      platformMapping_->getChips(),
      phy::DataPlanePhyChipType::TRANSCEIVER,
      profileId);

  if (tcvrChips.size() == 1) {
    // If the current (portId, profileId) combination only supports one
    // transceiver, then we return the portToProfileIds returned from agent.
    return std::
        unordered_map<TransceiverID, std::map<int32_t, cfg::PortProfileID>>{
            {initTcvrId, agentPortToProfileIDs}};
  } else if (tcvrChips.size() > 2) {
    throw FbossError("Unexpected number of chips for port ", portId);
  }

  XLOG(DBG2)
      << "Changing programInternalPhyPorts output received for TransceiverID("
      << initTcvrId << " to dual transceiver config.";

  // Determine the per-tcvr profileId.
  cfg::PortProfileID perTcvrProfileId =
      getPerTransceiverProfile(tcvrChips.size() /* numTcvrs */, profileId);

  // Find the corresponding new portIds that support this new port speed for
  // each transceiver.

  // This lambda should only be used to find the port and profile for the
  // subsumed transceiver. We assume the subsumer port already has the intended
  // profile supported.
  auto findPortWithSingleTcvrAndProfile =
      [this](
          const phy::DataPlanePhyChip& chip, const cfg::PortProfileID profile) {
        auto platformPorts = utility::getPlatformPortsByChip(
            platformMapping_->getPlatformPorts(), chip);

        // We only care about controlling ports.
        std::vector<cfg::PlatformPortEntry> controllingPorts;
        for (auto& port : platformPorts) {
          if (*port.mapping()->id() != *port.mapping()->controllingPort()) {
            // Port is not a controlling port.
            continue;
          }
          if (port.supportedProfiles()->find(profile) ==
              port.supportedProfiles()->end()) {
            // Port doesn't support this profile.
            continue;
          }
          if (utility::getDataPlanePhyChips(
                  port,
                  platformMapping_->getChips(),
                  phy::DataPlanePhyChipType::TRANSCEIVER)
                  .size() != 1) {
            // Port is not a single transceiver port.
            continue;
          }
          return PortID(*port.mapping()->id());
        }

        throw FbossError(
            apache::thrift::util::enumNameSafe(profile),
            " is not a supported profile for TransceiverID(",
            *chip.physicalID(),
            ")");
      };

  std::unordered_map<TransceiverID, std::map<int32_t, cfg::PortProfileID>>
      newTcvrToPortToProfileId;
  // Set subsumer port profile (need to validate this profileId exists here -
  // otherwise can throw error)
  newTcvrToPortToProfileId[initTcvrId] = {
      {static_cast<int32_t>(portId), perTcvrProfileId}};

  // Set subsumee
  phy::DataPlanePhyChip subsumedTcvrChip;
  for (auto& [_, chip] : tcvrChips) {
    if (TransceiverID(*chip.physicalID()) != initTcvrId) {
      // This is the subsumed transceiver.
      subsumedTcvrChip = chip;
      break;
    }
  }
  newTcvrToPortToProfileId[TransceiverID(*subsumedTcvrChip.physicalID())] = {
      {static_cast<int32_t>(findPortWithSingleTcvrAndProfile(
           subsumedTcvrChip, perTcvrProfileId)),
       perTcvrProfileId}};

  return newTcvrToPortToProfileId;
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

  if (programmedIphyPorts.empty()) {
    XLOG(WARN)
        << "programInternalPhyPorts() returned empty map for Transceiver=" << id
        << ". Skipping port programming.";
    return;
  }

  // Only in use for dual transceiver mode.
  auto [agentPortIdInt, agentProfileId] = *programmedIphyPorts.begin();
  auto agentPortId = PortID(agentPortIdInt);
  PortID subsumedPortId{0};
  TransceiverID nonControllingTcvrId{0};

  // Getting tcvr->PortToProfileId in case we are in dual transceiver mode.
  const auto tcvrToPortToProfileId =
      getMultiTransceiverPortProfileIDs(id, programmedIphyPorts);
  bool useAgentPortIdForCache = tcvrToPortToProfileId.size() == 2;
  for (auto& [tcvrId, portToProfileId] : tcvrToPortToProfileId) {
    const auto& portToPortInfoIt =
        transceiverManager_->getSynchronizedProgrammedIphyPortToPortInfo(
            tcvrId);
    if (!portToPortInfoIt) {
      continue;
    }

    auto portToPortInfoWithLock = portToPortInfoIt->wlock();
    portToPortInfoWithLock->clear();
    for (auto& [portIdInt, profileId] : portToProfileId) {
      auto programmingPortId = PortID(portIdInt);
      if (tcvrId != id && programmingPortId != agentPortId) {
        nonControllingTcvrId = tcvrId;
        subsumedPortId = programmingPortId;
      }

      // For qsfp_service cache, to coordinate tcvr / port states in
      // multi-tcvr ports.
      auto cachePortId =
          useAgentPortIdForCache ? agentPortId : programmingPortId;

      // Update programming data - qsfp_service only cares about the profileId
      // that it needs to program transceivers at.
      TransceiverManager::TransceiverPortInfo portInfo;

      portInfo.profile = profileId;
      NpuPortStatus status{};
      status.portEnabled = true;
      status.operState = false;
      portInfo.status = status;

      // TODO(smenta) - Evaluate if should be programmedPortId.
      portToPortInfoWithLock->emplace(programmingPortId, portInfo);
      setTransceiverEnabledStatusInCache(cachePortId, tcvrId);
    }
  }

  // Update mappings for internal use.
  if (tcvrToPortToProfileId.size() == 2 && subsumedPortId != PortID(0)) {
    multiTcvrQsfpPortToAgentPort_.wlock()->insert(
        {subsumedPortId, agentPortId});
    multiTcvrControllingToNonControllingTcvr_.wlock()->insert(
        {id, nonControllingTcvrId});
  }

  if (!phyManager_) {
    // If phyManager_ doesn't exist, this means that all PHY programming is
    // complete.
    for (auto& [tcvrId, _] : tcvrToPortToProfileId) {
      transceiverManager_->markTransceiverReadyForProgramming(tcvrId, true);
    }
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
  if (auto nonControllingTcvrId =
          getNonControllingTransceiverIdForMultiTcvr(tcvrId)) {
    transceiverManager_->markTransceiverReadyForProgramming(
        *nonControllingTcvrId, true);
  }
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
  for (const auto& [tcvrId, initializedPorts] : tcvrToInitializedPorts_) {
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
      auto tcvrId = getLowestIndexedStaticTransceiverForPort(*portId);
      if (transceiverManager_->isTransceiverPortStateSupported(
              tcvrId, portState)) {
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

void PortManager::publishLinkSnapshots(const std::string& portNameStr) {
  publishLinkSnapshots(getPortIDByPortNameOrThrow(portNameStr));
}

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
 * detectTransceiverResetAndReinitializeCorrespondingDownPorts
 *
 * This function is called within the context of the normal refresh cycle. With
 * SW ports now being separately managed entitites from transceivers, we need to
 * make sure we re-initialize ports when transceivers are re-discovered and
 * ports are in a terminal state.
 */

void PortManager::
    detectTransceiverResetAndReinitializeCorrespondingDownPorts() {
  BlockingStateUpdateResultList results;
  // Fetch non-controlling transceivers.
  const auto controllingToNonControllingTcvrMap =
      *multiTcvrControllingToNonControllingTcvr_.rlock();

  // Iterate through controlling transceivers.
  for (auto& [tcvrId, lockedPortSetPtr] : tcvrToInitializedPorts_) {
    // Check if transceiver is in a non-programmed state. If so, programming
    // needs to be retriggered.
    auto currTcvrState = getTransceiverState(tcvrId);
    // If transceiver is just remediated, we assume we can program directly with
    // old transceiver settings, and have already marked that transceiver can
    // move to transceiver programmed. Port flaps would not be caused here
    // because the settings haven't been changed.
    auto isTcvrJustRemediated =
        transceiverManager_->transceiverJustRemediated(tcvrId);
    bool shouldReinitPorts =
        (currTcvrState == TransceiverStateMachineState::TRANSCEIVER_READY ||
         currTcvrState == TransceiverStateMachineState::DISCOVERED) &&
        !isTcvrJustRemediated;

    // Check if non-controlling transceiver is in a similar state. If so, we
    // should re-initialize port.
    auto itr = controllingToNonControllingTcvrMap.find(tcvrId);
    if (itr != controllingToNonControllingTcvrMap.end()) {
      auto secondTcvrState = getTransceiverState(itr->second);
      auto isSecondTcvrJustRemediated =
          transceiverManager_->transceiverJustRemediated(itr->second);

      shouldReinitPorts |=
          (secondTcvrState == TransceiverStateMachineState::TRANSCEIVER_READY ||
           secondTcvrState == TransceiverStateMachineState::DISCOVERED) &&
          !isSecondTcvrJustRemediated;
    }

    if (!shouldReinitPorts) {
      continue;
    }

    // Need to verify that all ports are down.
    auto portSet = *lockedPortSetPtr->rlock();
    bool allPortsDown{true};

    for (const auto& portId : portSet) {
      allPortsDown &=
          (getPortState(portId) == PortStateMachineState::PORT_DOWN);
    }
    shouldReinitPorts &= allPortsDown;

    if (!shouldReinitPorts) {
      continue;
    }

    for (const auto& portId : portSet) {
      if (auto result = enqueueStateUpdateForPortWithoutExecuting(
              portId, PortStateMachineEvent::PORT_EV_RESET_TO_INITIALIZED)) {
        results.push_back(result);
      }
    }
  }

  int numResults = results.size();
  if (numResults > 0) {
    executeStateUpdates();
    waitForAllBlockingStateUpdateDone(results);
  }

  XLOG_IF(DBG2, numResults > 0)
      << "detectTransceiverResetAndReinitializeCorrespondingDownPorts has "
      << numResults << " ports need to reset status.";
}

void PortManager::publishLinkSnapshots(PortID portId) {
  // Publish xphy snapshots if there's a phyManager and xphy ports
  if (phyManager_) {
    phyManager_->publishXphyInfoSnapshots(portId);
  }
  // Publish transceiver snapshots if there's a transceiver
  transceiverManager_->publishLinkSnapshotsTransceiver(portId);
}

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
  std::unordered_set<PortID> statusChangedPorts;

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
    bool isPortUpOrDown = stateMachineState == PortStateMachineState::PORT_UP ||
        stateMachineState == PortStateMachineState::PORT_DOWN;

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

      if (arePortTcvrsJustProgrammed ||
          ((stateMachineState != newState) && isPortUpOrDown)) {
        statusChangedPorts.insert(portId);
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

  for (auto portId : statusChangedPorts) {
    try {
      publishLinkSnapshots(portId);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Port " << portId
                << " failed publishLinkSnapshpts(): " << ex.what();
    }
  }

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
  // Fetch non-controlling transceivers.
  const auto controllingToNonControllingTcvrMap =
      *multiTcvrControllingToNonControllingTcvr_.rlock();

  auto multiTcvrQsfpToAgentPort = *multiTcvrQsfpPortToAgentPort_.rlock();
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

    std::vector<TransceiverID> tcvrsToUpdate{tcvrId};
    if (auto itr = controllingToNonControllingTcvrMap.find(tcvrId);
        itr != controllingToNonControllingTcvrMap.end()) {
      tcvrsToUpdate.push_back(itr->second);
    }

    for (const auto& initTcvrId : tcvrsToUpdate) {
      const auto& portToPortInfoIt =
          transceiverManager_->getSynchronizedProgrammedIphyPortToPortInfo(
              initTcvrId);
      if (!portToPortInfoIt) {
        continue;
      }

      auto portToPortInfoWithLock = portToPortInfoIt->wlock();
      for (auto& [qsfpPortId, portInfo] : *portToPortInfoWithLock) {
        PortID portId = qsfpPortId;
        if (auto itr = multiTcvrQsfpToAgentPort.find(qsfpPortId);
            itr != multiTcvrQsfpToAgentPort.end()) {
          portId = itr->second;
        }
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
    const auto& tcvrID = getLowestIndexedStaticTransceiverForPort(portId);
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
  std::unordered_set<PortID> statusChangedPorts;
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
    bool isPortUpOrDown = portState == PortStateMachineState::PORT_UP ||
        portState == PortStateMachineState::PORT_DOWN;
    auto newState = operStateToPortState(portStatus.operState);

    if (arePortTcvrsJustProgrammed ||
        (portStatus.portEnabled && isPortUpOrDown && (portState != newState))) {
      ++numPortStatusChanged;
      statusChangedPorts.insert(portId);
      auto event = getPortStatusChangeEvent(newState);
      if (auto result = updateStateBlockingWithoutWait(portId, event)) {
        results.push_back(result);
      }
    }
  }

  for (auto portId : statusChangedPorts) {
    try {
      publishLinkSnapshots(portId);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Port " << portId
                << " failed publishLinkSnapshpts(): " << ex.what();
    }
  }

  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(DBG2, numPortStatusChanged > 0)
      << "updatePortActiveState has " << numPortStatusChanged
      << " ports need to update port status.";
}

std::unordered_set<TransceiverID> PortManager::getTransceiversWithAllPortsInSet(
    const std::unordered_set<PortID>& ports) const {
  // Fetch non-controlling transceivers.
  const auto controllingToNonControllingTcvrMap =
      *multiTcvrControllingToNonControllingTcvr_.rlock();

  // Check if controlling transceiver is fully covered by ports argument.
  std::unordered_set<TransceiverID> transceivers;
  for (const auto& [tcvrId, enabledPorts] : tcvrToInitializedPorts_) {
    auto lockedEnabledPorts = enabledPorts->rlock();
    if (!lockedEnabledPorts->empty() &&
        std::all_of(
            lockedEnabledPorts->begin(),
            lockedEnabledPorts->end(),
            [&](const PortID& portId) { return ports.contains(portId); })) {
      // Add controlling transceiver.
      transceivers.insert(tcvrId);

      // Add non-controlling transceiver if it exits.
      if (auto itr = controllingToNonControllingTcvrMap.find(tcvrId);
          itr != controllingToNonControllingTcvrMap.end()) {
        transceivers.insert(itr->second);
      }
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

PortManager::PortToSynchronizedTcvrVec
PortManager::setupPortToSynchronizedTcvrVec() {
  PortToSynchronizedTcvrVec portToTcvrVec;
  for (const auto& [portId, _] : portToTcvrMap_) {
    portToTcvrVec.emplace(

        portId,
        std::make_unique<folly::Synchronized<std::vector<TransceiverID>>>());
  }

  return portToTcvrVec;
}

void PortManager::setPortEnabledStatusInCache(PortID portId, bool enabled) {
  auto tcvrId = getLowestIndexedStaticTransceiverForPort(portId);
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

void PortManager::setTransceiverEnabledStatusInCache(
    PortID portId,
    TransceiverID tcvrId) {
  auto initializedTcvrsIt = portToInitializedTcvrs_.find(portId);
  if (initializedTcvrsIt == portToInitializedTcvrs_.end()) {
    throw FbossError(
        "No port id found in initialized transceivers cache for port ", portId);
  }

  auto initializedTcvrs = initializedTcvrsIt->second->wlock();
  if (std::find(initializedTcvrs->begin(), initializedTcvrs->end(), tcvrId) !=
      initializedTcvrs->end()) {
    throw FbossError(
        "Transceiver ",
        tcvrId,
        " already exists in initialized transceivers cache for port ",
        portId);
  }

  initializedTcvrs->push_back(tcvrId);
}

void PortManager::clearEnabledTransceiversForPort(PortID portId) {
  auto initializedTcvrsIt = portToInitializedTcvrs_.find(portId);
  if (initializedTcvrsIt != portToInitializedTcvrs_.end()) {
    auto initializedTcvrs = initializedTcvrsIt->second->wlock();
    initializedTcvrs->clear();
  }
}

void PortManager::clearTransceiversReadyForProgramming(PortID portId) {
  auto initializedTcvrsIt = portToInitializedTcvrs_.find(portId);
  if (initializedTcvrsIt != portToInitializedTcvrs_.end()) {
    auto initializedTcvrs = initializedTcvrsIt->second->rlock();
    for (const auto& tcvrId : *initializedTcvrs) {
      transceiverManager_->markTransceiverReadyForProgramming(tcvrId, false);
    }
  }
}

void PortManager::clearMultiTcvrMappings(PortID portId) {
  multiTcvrQsfpPortToAgentPort_.wlock()->erase(portId);
  multiTcvrControllingToNonControllingTcvr_.wlock()->erase(
      getLowestIndexedStaticTransceiverForPort(portId));
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
  // Check to see if all of a port's transceivers are programmed.
  bool arePortTcvrsProgrammed{true};
  auto tcvrIds = getInitializedTransceiverIdsForPort(portId);
  for (const auto& tcvrId : tcvrIds) {
    if (transceiverManager_->getCurrentState(tcvrId) !=
        TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED) {
      auto portNameStr = getPortNameByPortIdOrThrow(portId);
      SW_PORT_LOG(INFO, "[SM]", portNameStr, portId)
          << "Assigned Transceiver " << tcvrId
          << " state is not TRANSCEIVER_PROGRAMMED: "
          << apache::thrift::util::enumNameSafe(getTransceiverState(tcvrId))
          << ". Not advancing PortStateMachine to TRANSCEIVERS_PROGRAMMED.";
      arePortTcvrsProgrammed = false;
      break;
    }
  }

  // If not all programmed, tell transceivers that they are ready to program
  // (they should have been marked for programming earlier, but unexpected
  // resets can happen).
  if (!arePortTcvrsProgrammed) {
    for (const auto& tcvrId : tcvrIds) {
      transceiverManager_->markTransceiverReadyForProgramming(tcvrId, true);
    }
  }

  return arePortTcvrsProgrammed;
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
  const auto& presentXcvrIds = transceiverManager_->refreshTransceivers();

  // Step 2: Fetch current port status from wedge_agent.
  updateTransceiverPortStatus();

  // Step 3: Update Current Port Statuses in TransceiverManager for Remediation
  // and Tcvr Insert
  updatePortActiveStatusInTransceiverManager();

  // Step 4: Reset port state machines for ports that have transceivers that
  // are recently discovered to retrigger programming.
  detectTransceiverResetAndReinitializeCorrespondingDownPorts();

  // Step 5: Check whether there's a wedge_agent config change
  triggerAgentConfigChangeEvent();

  // Step 6: Trigger port programming events.
  triggerProgrammingEvents();

  // Step 7: Trigger transceiver programming events.
  const auto& programmedTcvrs = transceiverManager_->triggerProgrammingEvents();

  // Step 8: Trigger remediation.
  std::vector<TransceiverID> stableTcvrs;
  for (const auto& tcvrID : presentXcvrIds) {
    if (std::find(programmedTcvrs.begin(), programmedTcvrs.end(), tcvrID) ==
        programmedTcvrs.end()) {
      stableTcvrs.push_back(tcvrID);
    }
  }
  transceiverManager_->triggerRemediateEvents(stableTcvrs);

  // Step 9: Publish PIM states to FSDB
  transceiverManager_->publishPimStatesToFsdb();

  // Step 10: Mark full initialization complete.
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

void PortManager::publishPhyStateToFsdb(
    std::string&& portNameStr,
    std::optional<phy::PhyState>&& newState) const {
  if (FLAGS_publish_state_to_fsdb) {
    fsdbSyncManager_->updatePhyState(
        std::move(portNameStr), std::move(newState));
  }
}

void PortManager::publishPhyStatToFsdb(
    std::string&& portNameStr,
    phy::PhyStats&& stat) const {
  if (FLAGS_publish_stats_to_fsdb) {
    fsdbSyncManager_->updatePhyStat(std::move(portNameStr), std::move(stat));
  }
}

void PortManager::publishPortStatToFsdb(
    std::string&& portNameStr,
    HwPortStats&& stat) const {
  if (FLAGS_publish_stats_to_fsdb) {
    fsdbSyncManager_->updatePortStat(std::move(portNameStr), std::move(stat));
  }
}

} // namespace facebook::fboss
