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
      portToTcvrMap_(getPortToTcvrMap(platformMapping_)),
      stateMachineControllers_(setupPortToStateMachineControllerMap()) {}

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

bool PortManager::hasPortFinishedIphyProgramming(PortID portId) const {
  static const std::array<PortStateMachineState, 5> kPastIphyProgrammedStates{
      PortStateMachineState::IPHY_PORTS_PROGRAMMED,
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

void PortManager::publishLinkSnapshots(PortID portId) {}

void PortManager::restoreWarmBootPhyState() {}

void PortManager::triggerAgentConfigChangeEvent() {}

void PortManager::updateTransceiverPortStatus() noexcept {}

void PortManager::restoreAgentConfigAppliedInfo() {}

void PortManager::updateNpuPortStatusCache(
    std::map<int, facebook::fboss::NpuPortStatus>& portStatus) {}

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
  const auto portNameStr = getPortNameByPortId(portId);
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

} // namespace facebook::fboss
