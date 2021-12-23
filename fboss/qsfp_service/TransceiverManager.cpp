// Copyright 2021-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/TransceiverManager.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/TransceiverStateMachineUpdate.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

using namespace std::chrono;

namespace facebook {
namespace fboss {

TransceiverManager::TransceiverManager(
    std::unique_ptr<TransceiverPlatformApi> api,
    std::unique_ptr<PlatformMapping> platformMapping)
    : qsfpPlatApi_(std::move(api)),
      platformMapping_(std::move(platformMapping)),
      stateMachines_(setupTransceiverToStateMachineHelper()),
      tcvrToPortInfo_(setupTransceiverToPortInfo()) {
  // Cache the static mapping based on platformMapping_
  const auto& platformPorts = platformMapping_->getPlatformPorts();
  const auto& chips = platformMapping_->getChips();
  for (const auto& [portIDInt, platformPort] : platformPorts) {
    PortID portID = PortID(portIDInt);
    const auto& portName = *platformPort.mapping_ref()->name_ref();
    portNameToPortID_.emplace(portName, portID);
    SwPortInfo portInfo;
    portInfo.name = portName;
    portInfo.tcvrID = utility::getTransceiverId(platformPort, chips);
    portToSwPortInfo_.emplace(portID, std::move(portInfo));
  }
  // Now we might need to start threads
  startThreads();
}

TransceiverManager::~TransceiverManager() {
  // Stop all the threads before shutdown
  isExiting_ = true;
  stopThreads();
}

const TransceiverManager::PortNameMap&
TransceiverManager::getPortNameToModuleMap() const {
  if (portNameToModule_.empty()) {
    const auto& platformPorts = platformMapping_->getPlatformPorts();
    for (const auto& it : platformPorts) {
      auto port = it.second;
      auto transceiverId =
          utility::getTransceiverId(port, platformMapping_->getChips());
      if (!transceiverId) {
        continue;
      }

      auto& portName = *(port.mapping_ref()->name_ref());
      portNameToModule_[portName] = transceiverId.value();
    }
  }

  return portNameToModule_;
}

const std::set<std::string> TransceiverManager::getPortNames(
    TransceiverID tcvrId) const {
  std::set<std::string> ports;
  auto it = portGroupMap_.find(tcvrId);
  if (it != portGroupMap_.end() && !it->second.empty()) {
    for (const auto& port : it->second) {
      if (auto portName = port.name_ref()) {
        ports.insert(*portName);
      }
    }
  }
  return ports;
}

const std::string TransceiverManager::getPortName(TransceiverID tcvrId) const {
  auto portNames = getPortNames(tcvrId);
  return portNames.empty() ? "" : *portNames.begin();
}

TransceiverManager::TransceiverToStateMachineHelper
TransceiverManager::setupTransceiverToStateMachineHelper() {
  // Set up NewModuleStateMachine map
  TransceiverToStateMachineHelper stateMachineMap;
  if (FLAGS_use_new_state_machine) {
    for (auto chip : platformMapping_->getChips()) {
      if (*chip.second.type_ref() != phy::DataPlanePhyChipType::TRANSCEIVER) {
        continue;
      }
      auto tcvrID = TransceiverID(*chip.second.physicalID_ref());
      stateMachineMap.emplace(
          tcvrID,
          std::make_unique<TransceiverStateMachineHelper>(this, tcvrID));
    }
  }
  return stateMachineMap;
}

TransceiverManager::TransceiverToPortInfo
TransceiverManager::setupTransceiverToPortInfo() {
  TransceiverToPortInfo tcvrToPortInfo;
  if (FLAGS_use_new_state_machine) {
    for (auto chip : platformMapping_->getChips()) {
      if (*chip.second.type_ref() != phy::DataPlanePhyChipType::TRANSCEIVER) {
        continue;
      }
      auto tcvrID = TransceiverID(*chip.second.physicalID_ref());
      auto portToPortInfo = std::make_unique<folly::Synchronized<
          std::unordered_map<PortID, TransceiverPortInfo>>>();
      tcvrToPortInfo.emplace(tcvrID, std::move(portToPortInfo));
    }
  }
  return tcvrToPortInfo;
}

void TransceiverManager::startThreads() {
  if (FLAGS_use_new_state_machine) {
    // Setup all TransceiverStateMachineHelper thread
    for (auto& stateMachineHelper : stateMachines_) {
      stateMachineHelper.second->startThread();
    }

    XLOG(DBG2) << "Started TransceiverStateMachineUpdateThread";
    updateEventBase_ = std::make_unique<folly::EventBase>();
    updateThread_.reset(new std::thread([=] {
      this->threadLoop(
          "TransceiverStateMachineUpdateThread", updateEventBase_.get());
    }));
  }
}

void TransceiverManager::stopThreads() {
  // We use runInEventBaseThread() to terminateLoopSoon() rather than calling it
  // directly here.  This ensures that any events already scheduled via
  // runInEventBaseThread() will have a chance to run.
  if (updateThread_) {
    updateEventBase_->runInEventBaseThread(
        [this] { updateEventBase_->terminateLoopSoon(); });
    updateThread_->join();
    XLOG(DBG2) << "Terminated TransceiverStateMachineUpdateThread";
  }
  // Drain any pending updates by calling handlePendingUpdates directly.
  bool updatesDrained = false;
  do {
    handlePendingUpdates();
    {
      std::unique_lock guard(pendingUpdatesLock_);
      updatesDrained = pendingUpdates_.empty();
    }
  } while (!updatesDrained);
  // And finally stop all TransceiverStateMachineHelper thread
  for (auto& stateMachineHelper : stateMachines_) {
    stateMachineHelper.second->stopThread();
  }
}

void TransceiverManager::threadLoop(
    folly::StringPiece name,
    folly::EventBase* eventBase) {
  initThread(name);
  eventBase->loopForever();
}

void TransceiverManager::updateStateBlocking(
    TransceiverID id,
    TransceiverStateMachineEvent event) {
  auto result = updateStateBlockingWithoutWait(id, event);
  if (result) {
    result->wait();
  }
}

std::shared_ptr<BlockingTransceiverStateMachineUpdateResult>
TransceiverManager::updateStateBlockingWithoutWait(
    TransceiverID id,
    TransceiverStateMachineEvent event) {
  auto result = std::make_shared<BlockingTransceiverStateMachineUpdateResult>();
  auto update = std::make_unique<BlockingTransceiverStateMachineUpdate>(
      id, event, result);
  if (updateState(std::move(update))) {
    // Only return blocking result if the update has been added in queue
    return result;
  }
  return nullptr;
}

bool TransceiverManager::updateState(
    std::unique_ptr<TransceiverStateMachineUpdate> update) {
  if (isExiting_) {
    XLOG(WARN) << "Skipped queueing update:" << update->getName()
               << ", since exit already started";
    return false;
  }
  if (!updateEventBase_) {
    XLOG(WARN) << "Skipped queueing update:" << update->getName()
               << ", since updateEventBase_ is not created yet";
    return false;
  }
  {
    std::unique_lock guard(pendingUpdatesLock_);
    pendingUpdates_.push_back(*update.release());
  }

  // Signal the update thread that updates are pending.
  // We call runInEventBaseThread() with a static function pointer since this
  // is more efficient than having to allocate a new bound function object.
  updateEventBase_->runInEventBaseThread(handlePendingUpdatesHelper, this);
  return true;
}

void TransceiverManager::handlePendingUpdatesHelper(TransceiverManager* mgr) {
  return mgr->handlePendingUpdates();
}
void TransceiverManager::handlePendingUpdates() {
  // Get the list of updates to run.
  // We might pull multiple updates off the list at once if several updates were
  // scheduled before we had a chance to process them.
  // In some case we might also end up finding 0 updates to process if a
  // previous handlePendingUpdates() call processed multiple updates.
  StateUpdateList updates;
  std::set<TransceiverID> toBeUpdateTransceivers;
  {
    std::unique_lock guard(pendingUpdatesLock_);
    // Each TransceiverStateMachineUpdate should be able to process at the same
    // time as we already have lock protection in ExternalPhy and QsfpModule.
    // Therefore, we should just put all the pending update into the updates
    // list as long as they are from totally different transceivers.
    auto iter = pendingUpdates_.begin();
    while (iter != pendingUpdates_.end()) {
      auto [_, isInserted] =
          toBeUpdateTransceivers.insert(iter->getTransceiverID());
      if (!isInserted) {
        // Stop when we find another update for the same transceiver
        break;
      }
      ++iter;
    }
    updates.splice(
        updates.begin(), pendingUpdates_, pendingUpdates_.begin(), iter);
  }

  // handlePendingUpdates() is invoked once for each update, but a previous
  // call might have already processed everything.  If we don't have anything
  // to do just return early.
  if (updates.empty()) {
    return;
  }

  XLOG(DBG2) << "About to update " << updates.size()
             << " TransceiverStateMachine";
  // To expedite all these different transceivers state update, use Future
  std::vector<folly::Future<folly::Unit>> stateUpdateTasks;
  auto iter = updates.begin();
  while (iter != updates.end()) {
    TransceiverStateMachineUpdate* update = &(*iter);
    ++iter;

    auto stateMachineItr = stateMachines_.find(update->getTransceiverID());
    if (stateMachineItr == stateMachines_.end()) {
      XLOG(WARN) << "Unrecognize Transceiver:" << update->getTransceiverID()
                 << ", can't find StateMachine for it. Skip updating.";
      delete update;
      continue;
    }

    stateUpdateTasks.push_back(
        folly::via(stateMachineItr->second->getEventBase())
            .thenValue([update, stateMachineItr](auto&&) {
              XLOG(INFO) << "Preparing TransceiverStateMachine update for "
                         << update->getName();
              // Hold the module state machine lock
              const auto& lockedStateMachine =
                  stateMachineItr->second->getStateMachine().wlock();
              update->applyUpdate(*lockedStateMachine);
            })
            .thenError(
                folly::tag_t<std::exception>{},
                [update](const std::exception& ex) {
                  update->onError(ex);
                  delete update;
                }));
  }
  folly::collectAll(stateUpdateTasks).wait();

  // Notify all of the updates of success and delete them.
  while (!updates.empty()) {
    std::unique_ptr<TransceiverStateMachineUpdate> update(&updates.front());
    updates.pop_front();
    update->onSuccess();
  }
}

TransceiverStateMachineState TransceiverManager::getCurrentState(
    TransceiverID id) const {
  auto stateMachineItr = stateMachines_.find(id);
  if (stateMachineItr == stateMachines_.end()) {
    throw FbossError("Transceiver:", id, " doesn't exist");
  }

  const auto& lockedStateMachine =
      stateMachineItr->second->getStateMachine().rlock();
  auto curStateOrder = *lockedStateMachine->current_state();
  auto curState = getStateByOrder(curStateOrder);
  XLOG(DBG4) << "Current transceiver:" << static_cast<int32_t>(id)
             << ", state order:" << curStateOrder
             << ", state:" << apache::thrift::util::enumNameSafe(curState);
  return curState;
}

std::vector<TransceiverID> TransceiverManager::triggerProgrammingEvents() {
  std::vector<TransceiverID> programmedTcvrs;
  int32_t numProgramIphy{0}, numProgramXphy{0}, numProgramTcvr{0};
  BlockingStateUpdateResultList results;
  steady_clock::time_point begin = steady_clock::now();
  for (auto& stateMachine : stateMachines_) {
    bool needProgramIphy{false}, needProgramXphy{false}, needProgramTcvr{false};
    {
      const auto& lockedStateMachine =
          stateMachine.second->getStateMachine().rlock();
      needProgramIphy = !lockedStateMachine->get_attribute(isIphyProgrammed);
      needProgramXphy = !lockedStateMachine->get_attribute(isXphyProgrammed);
      needProgramTcvr =
          !lockedStateMachine->get_attribute(isTransceiverProgrammed);
    }
    auto tcvrID = stateMachine.first;
    if (needProgramIphy) {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID, TransceiverStateMachineEvent::PROGRAM_IPHY)) {
        programmedTcvrs.push_back(tcvrID);
        ++numProgramIphy;
        results.push_back(result);
      }
    } else if (needProgramXphy && phyManager_ != nullptr) {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID, TransceiverStateMachineEvent::PROGRAM_XPHY)) {
        programmedTcvrs.push_back(tcvrID);
        ++numProgramXphy;
        results.push_back(result);
      }
    } else if (needProgramTcvr) {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID, TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER)) {
        programmedTcvrs.push_back(tcvrID);
        ++numProgramTcvr;
        results.push_back(result);
      }
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(DBG2, !programmedTcvrs.empty())
      << "triggerProgrammingEvents has " << numProgramIphy
      << " IPHY programming, " << numProgramXphy << " XPHY programming, "
      << numProgramTcvr << " TCVR programming. Total execute time(ms):"
      << duration_cast<milliseconds>(steady_clock::now() - begin).count();
  return programmedTcvrs;
}

void TransceiverManager::programInternalPhyPorts(TransceiverID id) {
  std::map<int32_t, cfg::PortProfileID> programmedIphyPorts;
  if (auto overridePortAndProfileIt =
          overrideTcvrToPortAndProfileForTest_.find(id);
      overridePortAndProfileIt != overrideTcvrToPortAndProfileForTest_.end()) {
    // NOTE: This is only used for testing.
    for (const auto& [portID, profileID] : overridePortAndProfileIt->second) {
      programmedIphyPorts.emplace(portID, profileID);
    }
  } else {
    // Then call wedge_agent programInternalPhyPorts
    auto wedgeAgentClient = utils::createWedgeAgentClient();
    wedgeAgentClient->sync_programInternalPhyPorts(
        programmedIphyPorts, getTransceiverInfo(id), false);
  }

  std::string logStr = folly::to<std::string>(
      "programInternalPhyPorts() for Transceiver=", id, " return [");
  for (const auto& [portID, profileID] : programmedIphyPorts) {
    logStr = folly::to<std::string>(
        logStr,
        portID,
        " : ",
        apache::thrift::util::enumNameSafe(profileID),
        ", ");
  }
  XLOG(INFO) << logStr << "]";

  // Now update the programmed SW port to profile mapping
  if (auto portToPortInfoIt = tcvrToPortInfo_.find(id);
      portToPortInfoIt != tcvrToPortInfo_.end()) {
    auto portToPortInfoWithLock = portToPortInfoIt->second->wlock();
    portToPortInfoWithLock->clear();
    for (auto [portID, profileID] : programmedIphyPorts) {
      TransceiverPortInfo portInfo;
      portInfo.profile = profileID;
      portToPortInfoWithLock->emplace(PortID(portID), portInfo);
    }
  }
}

std::unordered_map<PortID, TransceiverManager::TransceiverPortInfo>
TransceiverManager::getProgrammedIphyPortToPortInfo(TransceiverID id) const {
  if (auto tcvrToPortInfo_It = tcvrToPortInfo_.find(id);
      tcvrToPortInfo_It != tcvrToPortInfo_.end()) {
    return *(tcvrToPortInfo_It->second->rlock());
  }
  return {};
}

std::unordered_map<PortID, cfg::PortProfileID>
TransceiverManager::getOverrideProgrammedIphyPortAndProfileForTest(
    TransceiverID id) const {
  if (auto portAndProfileIt = overrideTcvrToPortAndProfileForTest_.find(id);
      portAndProfileIt != overrideTcvrToPortAndProfileForTest_.end()) {
    return portAndProfileIt->second;
  }
  return {};
}

void TransceiverManager::programExternalPhyPorts(TransceiverID id) {
  auto phyManager = getPhyManager();
  if (!phyManager) {
    return;
  }
  // Get programmed iphy port profile
  const auto& programmedPortToPortInfo = getProgrammedIphyPortToPortInfo(id);
  if (programmedPortToPortInfo.empty()) {
    // This is due to the iphy ports are disabled. So no need to program xphy
    XLOG(DBG2) << "Skip programming xphy ports for Transceiver=" << id
               << ". Can't find programmed iphy port and port info";
    return;
  }
  const auto& supportedXphyPorts = phyManager->getXphyPorts();
  const auto& transceiver = getTransceiverInfo(id);
  for (const auto& [portID, portInfo] : programmedPortToPortInfo) {
    if (std::find(
            supportedXphyPorts.begin(), supportedXphyPorts.end(), portID) ==
        supportedXphyPorts.end()) {
      XLOG(DBG2) << "Skip programming xphy ports for Transceiver=" << id
                 << ", Port=" << portID << ". Can't find supported xphy";
      continue;
    }

    phyManager->programOnePort(portID, portInfo.profile, transceiver);
    XLOG(INFO) << "Programmed XPHY port for Transceiver=" << id
               << ", Port=" << portID << ", Profile="
               << apache::thrift::util::enumNameSafe(portInfo.profile);
  }
}

TransceiverInfo TransceiverManager::getTransceiverInfo(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(id); it != lockedTransceivers->end()) {
    return it->second->getTransceiverInfo();
  } else {
    TransceiverInfo absentTcvr;
    absentTcvr.present_ref() = false;
    absentTcvr.port_ref() = id;
    return absentTcvr;
  }
}

void TransceiverManager::programTransceiver(TransceiverID id) {
  // Get programmed iphy port profile
  const auto& programmedPortToPortInfo = getProgrammedIphyPortToPortInfo(id);
  if (programmedPortToPortInfo.empty()) {
    // This is due to the iphy ports are disabled. So no need to program tcvr
    XLOG(DBG2) << "Skip programming Transceiver=" << id
               << ". Can't find programmed iphy port and port info";
    return;
  }
  // TODO(joseph5wu) Usually we only need to program optical Transceiver which
  // doesn't need to support split-out copper cable for flex ports.
  // Which means for the optical transceiver, it usually has one programmed
  // iphy port and profile.
  // If in the future, we need to support flex port copper transceiver
  // programming, we might need to combine the speeds of all flex port to
  // program such transceiver.
  const auto profileID = programmedPortToPortInfo.begin()->second.profile;
  auto profileCfgOpt = platformMapping_->getPortProfileConfig(
      PlatformPortProfileConfigMatcher(profileID));
  if (!profileCfgOpt) {
    throw FbossError(
        "Can't find profile config for profileID=",
        apache::thrift::util::enumNameSafe(profileID));
  }
  const auto speed = *profileCfgOpt->speed_ref();

  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip programming Transceiver=" << id
               << ". Transeciver is not present";
    return;
  }
  tcvrIt->second->programTransceiver(speed);
  XLOG(INFO) << "Programmed Transceiver for Transceiver=" << id
             << " with speed=" << apache::thrift::util::enumNameSafe(speed);
}

bool TransceiverManager::tryRemediateTransceiver(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip remediating Transceiver=" << id
               << ". Transeciver is not present";
    return false;
  }
  bool didRemediate = tcvrIt->second->tryRemediate();
  XLOG_IF(INFO, didRemediate)
      << "Remediated Transceiver for Transceiver=" << id;
  return didRemediate;
}

void TransceiverManager::updateTransceiverPortStatus() noexcept {
  if (!FLAGS_use_new_state_machine) {
    return;
  }

  steady_clock::time_point begin = steady_clock::now();
  std::map<int32_t, PortStatus> newPortToPortStatus;
  try {
    // Then call wedge_agent getPortStatus() to get current port status
    auto wedgeAgentClient = utils::createWedgeAgentClient();
    wedgeAgentClient->sync_getPortStatus(newPortToPortStatus, {});
  } catch (const std::exception& ex) {
    // We have retry mechanism to handle failure. No crash here
    XLOG(WARN) << "Failed to call wedge_agent getPortStatus(). "
               << folly::exceptionStr(ex);
    if (overrideAgentPortStatusForTesting_.empty()) {
      return;
    } else {
      XLOG(WARN) << "[TEST ONLY] Use overrideAgentPortStatusForTesting_ "
                 << "for wedge_agent getPortStatus()";
      newPortToPortStatus = overrideAgentPortStatusForTesting_;
    }
  }

  int numResetToDiscovered{0}, numResetToNotPresent{0}, numPortStatusChanged{0};
  auto genStateMachineResetEvent =
      [&numResetToDiscovered, &numResetToNotPresent](
          std::optional<TransceiverStateMachineEvent>& event,
          bool isTcvrPresent) {
        // Update present transceiver state machine back to DISCOVERED
        // and absent transeiver state machine back to NOT_PRESENT
        if (event.has_value()) {
          // If event is already set, no-op
          return;
        }
        if (isTcvrPresent) {
          ++numResetToDiscovered;
          event.emplace(TransceiverStateMachineEvent::RESET_TO_DISCOVERED);
        } else {
          ++numResetToNotPresent;
          event.emplace(TransceiverStateMachineEvent::RESET_TO_NOT_PRESENT);
        }
      };

  const auto& presentTransceivers = getPresentTransceivers();
  BlockingStateUpdateResultList results;
  for (auto& [tcvrID, portToPortInfo] : tcvrToPortInfo_) {
    bool portStatusChanged = false;
    bool anyPortUp = false;
    bool isTcvrPresent =
        (std::find(
             presentTransceivers.begin(), presentTransceivers.end(), tcvrID) !=
         presentTransceivers.end());
    bool isTcvrJustProgrammed =
        (getCurrentState(tcvrID) ==
         TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
    std::optional<TransceiverStateMachineEvent> event;
    { // lock block for portToPortInfo
      auto portToPortInfoWithLock = portToPortInfo->wlock();
      // All possible platform ports for such transceiver
      const auto& portIDs = getAllPlatformPorts(tcvrID);
      for (auto portID : portIDs) {
        auto portStatusIt = newPortToPortStatus.find(portID);
        auto cachedPortInfoIt = portToPortInfoWithLock->find(portID);
        // If portStatus from agent doesn't have such port
        if (portStatusIt == newPortToPortStatus.end()) {
          if (cachedPortInfoIt == portToPortInfoWithLock->end()) {
            continue;
          } else {
            // Agent remove such port, we need to trigger a state machine reset
            // to trigger programming to get the new sw ports
            portToPortInfoWithLock->erase(cachedPortInfoIt);
            genStateMachineResetEvent(event, isTcvrPresent);
          }
        } else { // If portStatus exists
          // But if the port is disabled, we don't need disabled ports in the
          // cache, since we only store enabled ports as we do in the
          // programInternalPhyPorts()
          if (!(*portStatusIt->second.enabled_ref())) {
            if (cachedPortInfoIt != portToPortInfoWithLock->end()) {
              portToPortInfoWithLock->erase(cachedPortInfoIt);
              genStateMachineResetEvent(event, isTcvrPresent);
            }
          } else {
            // Only care about enabled port status
            anyPortUp = anyPortUp || *portStatusIt->second.up_ref();
            // Agent create such port, we need to trigger a state machine
            // reset to trigger programming to get the new sw ports
            if (cachedPortInfoIt == portToPortInfoWithLock->end()) {
              TransceiverPortInfo portInfo;
              portInfo.status.emplace(portStatusIt->second);
              portToPortInfoWithLock->insert({portID, std::move(portInfo)});
              genStateMachineResetEvent(event, isTcvrPresent);
            } else {
              // Both agent and cache here have such port, update the cached
              // status
              if (!cachedPortInfoIt->second.status ||
                  *cachedPortInfoIt->second.status->up_ref() !=
                      *portStatusIt->second.up_ref()) {
                portStatusChanged = true;
              }
              cachedPortInfoIt->second.status.emplace(portStatusIt->second);
            }
          }
        }
      }
      // If event is not set, it means not reset event is needed, now check
      // whether we need port status event.
      // Make sure we update active state for a transceiver which just finished
      // programming
      if (!event && (portStatusChanged || isTcvrJustProgrammed)) {
        event.emplace(
            anyPortUp ? TransceiverStateMachineEvent::PORT_UP
                      : TransceiverStateMachineEvent::ALL_PORTS_DOWN);
        ++numPortStatusChanged;
      }

      // Make sure the port event will be added to the update queue under the
      // lock of portToPortInfo, so that it will make sure the cached status
      // and the state machine will be in sync
      if (event.has_value()) {
        if (auto result = updateStateBlockingWithoutWait(tcvrID, *event)) {
          results.push_back(result);
        }
      }
    } // lock block for portToPortInfo
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(
      DBG2,
      numResetToDiscovered + numResetToNotPresent + numPortStatusChanged > 0)
      << "updateTransceiverPortStatus has " << numResetToDiscovered
      << " transceivers state machines set back to discovered, "
      << numResetToNotPresent << " set back to not_present, "
      << numPortStatusChanged
      << " transceivers need to update port status. Total execute time(ms):"
      << duration_cast<milliseconds>(steady_clock::now() - begin).count();
}

void TransceiverManager::updateTransceiverActiveState(
    const std::set<TransceiverID>& tcvrs,
    const std::map<int32_t, PortStatus>& portStatus) noexcept {
  int numPortStatusChanged{0};
  BlockingStateUpdateResultList results;
  for (auto tcvrID : tcvrs) {
    auto tcvrToPortInfoIt = tcvrToPortInfo_.find(tcvrID);
    if (tcvrToPortInfoIt == tcvrToPortInfo_.end()) {
      XLOG(WARN) << "Unrecoginized Transceiver:" << tcvrID
                 << ", skip updateTransceiverActiveState()";
      continue;
    }
    XLOG(INFO) << "Syncing ports of transceiver " << tcvrID;
    bool portStatusChanged = false;
    bool anyPortUp = false;
    { // lock block for portToPortInfo
      auto portToPortInfoWithLock = tcvrToPortInfoIt->second->wlock();
      for (auto& [portID, tcvrPortInfo] : *portToPortInfoWithLock) {
        // Check whether there's a new port status for such port
        auto portStatusIt = portStatus.find(portID);
        // If port doesn't need to be updated, use the current cached status to
        // indicate whether we need a state update
        if (portStatusIt == portStatus.end()) {
          if (tcvrPortInfo.status) {
            anyPortUp = anyPortUp || *tcvrPortInfo.status->up_ref();
          }
        } else {
          // Only care about enabled port status
          if (*portStatusIt->second.enabled_ref()) {
            anyPortUp = anyPortUp || *portStatusIt->second.up_ref();
            if (!tcvrPortInfo.status ||
                *tcvrPortInfo.status->up_ref() !=
                    *portStatusIt->second.up_ref()) {
              portStatusChanged = true;
            }
            // And also update the cached port status
            tcvrPortInfo.status = portStatusIt->second;
          }
        }
      }

      // Make sure the port event will be added to the update queue under the
      // lock of portToPortInfo, so that it will make sure the cached status
      // and the state machine will be in sync
      if (portStatusChanged) {
        auto event = anyPortUp ? TransceiverStateMachineEvent::PORT_UP
                               : TransceiverStateMachineEvent::ALL_PORTS_DOWN;
        ++numPortStatusChanged;
        if (auto result = updateStateBlockingWithoutWait(tcvrID, event)) {
          results.push_back(result);
        }
      }
    } // lock block for portToPortInfo
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(DBG2, numPortStatusChanged > 0)
      << "updateTransceiverActiveState has " << numPortStatusChanged
      << " transceivers need to update port status.";
}

void TransceiverManager::refreshStateMachines() {
  // Step1: Fetch current port status from wedge_agent.
  // Since the following steps, like refreshTransceivers() might need to use
  // port status to decide whether it's safe to reset a transceiver.
  // Therefore, always do port status update first.
  updateTransceiverPortStatus();

  // Step2: Refresh all transceivers so that we can get an update
  // TransceiverInfo
  const auto& transceiverIds = refreshTransceivers();

  // Step3: Check whether there's a wedge_agent config change
  triggerAgentConfigChangeEvent(transceiverIds);

  if (FLAGS_use_new_state_machine) {
    // Step4: Once the transceivers are detected, trigger programming events
    const auto& programmedTcvrs = triggerProgrammingEvents();

    // Step5: Remediate inactive transceivers
    // Only need to remediate ports which are not recently finished
    // programming. Because if they only finished early stage programming like
    // iphy without programming xphy or tcvr, the ports of such transceiver
    // will still be not stable to be remediated.
    std::vector<TransceiverID> stableTcvrs;
    for (auto tcvrID : transceiverIds) {
      if (std::find(programmedTcvrs.begin(), programmedTcvrs.end(), tcvrID) ==
          programmedTcvrs.end()) {
        stableTcvrs.push_back(tcvrID);
      }
    }
    triggerRemediateEvents(stableTcvrs);
  }
}

void TransceiverManager::triggerAgentConfigChangeEvent(
    const std::vector<TransceiverID>& transceivers) {
  if (!FLAGS_use_new_state_machine) {
    return;
  }

  auto wedgeAgentClient = utils::createWedgeAgentClient();
  int64_t lastConfigApplied{0};
  try {
    lastConfigApplied = wedgeAgentClient->sync_getLastConfigAppliedInMs();
  } catch (const std::exception& ex) {
    // We have retry mechanism to handle failure. No crash here
    XLOG(WARN) << "Failed to call wedge_agent getLastConfigAppliedInMs(). "
               << folly::exceptionStr(ex);
    return;
  }

  // Now check if the new timestamp is later than the cached one.
  if (lastConfigApplied <= lastConfigAppliedInMs_) {
    return;
  }

  XLOG(INFO) << "New Agent config applied time:" << lastConfigApplied
             << " and last cached time:" << lastConfigAppliedInMs_
             << ". Issue all ports reprogramming events";
  // Update present transceiver state machine back to DISCOVERED
  // and absent transeiver state machine back to NOT_PRESENT
  int numResetToDiscovered{0}, numResetToNotPresent{0};
  BlockingStateUpdateResultList results;
  for (auto& stateMachine : stateMachines_) {
    auto tcvrID = stateMachine.first;
    if (std::find(transceivers.begin(), transceivers.end(), tcvrID) !=
        transceivers.end()) {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID, TransceiverStateMachineEvent::RESET_TO_DISCOVERED)) {
        ++numResetToDiscovered;
        results.push_back(result);
      }
    } else {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID, TransceiverStateMachineEvent::RESET_TO_NOT_PRESENT)) {
        ++numResetToNotPresent;
        results.push_back(result);
      }
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG(INFO) << "triggerAgentConfigChangeEvent has " << numResetToDiscovered
             << " transceivers state machines set back to discovered, "
             << numResetToNotPresent << " set back to not_present";
  lastConfigAppliedInMs_ = lastConfigApplied;
}

TransceiverManager::TransceiverStateMachineHelper::
    TransceiverStateMachineHelper(
        TransceiverManager* tcvrMgrPtr,
        TransceiverID tcvrID)
    : tcvrID_(tcvrID) {
  // Init state should be "TRANSCEIVER_STATE_NOT_PRESENT"
  auto lockedStateMachine = stateMachine_.wlock();
  lockedStateMachine->get_attribute(transceiverMgrPtr) = tcvrMgrPtr;
  lockedStateMachine->get_attribute(transceiverID) = tcvrID_;
}

void TransceiverManager::TransceiverStateMachineHelper::startThread() {
  updateEventBase_ = std::make_unique<folly::EventBase>();
  updateThread_.reset(
      new std::thread([this] { updateEventBase_->loopForever(); }));
}

void TransceiverManager::TransceiverStateMachineHelper::stopThread() {
  if (updateThread_) {
    updateEventBase_->terminateLoopSoon();
    updateThread_->join();
  }
}

void TransceiverManager::waitForAllBlockingStateUpdateDone(
    const TransceiverManager::BlockingStateUpdateResultList& results) {
  for (const auto& result : results) {
    result->wait();
  }
}

/*
 * getPortIDByPortName
 *
 * This function takes the port name string (eth2/1/1) and returns the
 * software port id (or the agent port id) for that
 */
std::optional<PortID> TransceiverManager::getPortIDByPortName(
    const std::string& portName) {
  auto portMapIt = portNameToPortID_.find(portName);
  if (portMapIt != portNameToPortID_.end()) {
    return portMapIt->second;
  }
  return std::nullopt;
}

std::vector<PortID> TransceiverManager::getAllPlatformPorts(
    TransceiverID tcvrID) const {
  std::vector<PortID> ports;
  for (const auto& [portID, portInfo] : portToSwPortInfo_) {
    if (portInfo.tcvrID && *portInfo.tcvrID == tcvrID) {
      ports.push_back(portID);
    }
  }
  return ports;
}

std::vector<TransceiverID> TransceiverManager::getPresentTransceivers() const {
  std::vector<TransceiverID> presentTcvrs;
  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& tcvrIt : *lockedTransceivers) {
    presentTcvrs.push_back(tcvrIt.first);
  }
  return presentTcvrs;
}

void TransceiverManager::setOverrideAgentPortStatusForTesting(
    bool up,
    bool enabled,
    bool clearOnly) {
  // Use overrideTcvrToPortAndProfileForTest_ to prepare
  // overrideAgentPortStatusForTesting_
  overrideAgentPortStatusForTesting_.clear();
  if (clearOnly) {
    return;
  }
  for (const auto& it : overrideTcvrToPortAndProfileForTest_) {
    for (const auto& [portID, profileID] : it.second) {
      PortStatus status;
      status.enabled_ref() = enabled;
      status.up_ref() = up;
      status.profileID_ref() = apache::thrift::util::enumNameSafe(profileID);
      overrideAgentPortStatusForTesting_.emplace(portID, std::move(status));
    }
  }
}

bool TransceiverManager::areAllPortsDown(TransceiverID id) const noexcept {
  auto portToPortInfoIt = tcvrToPortInfo_.find(id);
  if (portToPortInfoIt == tcvrToPortInfo_.end()) {
    XLOG(WARN) << "Can't find Transceiver:" << id
               << " in cached tcvrToPortInfo_";
    return false;
  }
  auto portToPortInfoWithLock = portToPortInfoIt->second->rlock();
  for (const auto& [portID, portInfo] : *portToPortInfoWithLock) {
    if (!portInfo.status.has_value()) {
      // If no status set, assume ports are up so we won't trigger any
      // disruptive event
      return false;
    }
    if (*portInfo.status->up_ref()) {
      return false;
    }
  }
  return true;
}

void TransceiverManager::triggerRemediateEvents(
    const std::vector<TransceiverID>& stableTcvrs) {
  if (stableTcvrs.empty()) {
    return;
  }
  BlockingStateUpdateResultList results;
  for (auto tcvrID : stableTcvrs) {
    // For stabled transceivers, we check whether the current state machine
    // state is INACTIVE, which means all the ports are down for such
    // Transceiver, so that it's safe to call remediate
    auto curState = getCurrentState(tcvrID);
    if (curState != TransceiverStateMachineState::INACTIVE) {
      continue;
    }
    const auto& programmedPortToPortInfo =
        getProgrammedIphyPortToPortInfo(tcvrID);
    if (programmedPortToPortInfo.empty()) {
      // This is due to the iphy ports are disabled. So no need to remediate
      continue;
    }
    // Then check whether we should remediate so that we don't have to create
    // too many unnecessary state machine update
    auto lockedTransceivers = transceivers_.rlock();
    auto tcvrIt = lockedTransceivers->find(tcvrID);
    if (tcvrIt == lockedTransceivers->end()) {
      XLOG(DBG2) << "Skip remediating Transceiver=" << tcvrID
                 << ". Transeciver is not present";
      continue;
    }
    if (!tcvrIt->second->shouldRemediate()) {
      continue;
    }
    if (auto result = updateStateBlockingWithoutWait(
            tcvrID, TransceiverStateMachineEvent::REMEDIATE_TRANSCEIVER)) {
      results.push_back(result);
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(INFO, !results.empty())
      << "triggerRemediateEvents has " << results.size()
      << " transceivers kicked off remediation";
}

void TransceiverManager::markLastDownTime(TransceiverID id) noexcept {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip markLastDownTime for Transceiver=" << id
               << ". Transeciver is not present";
    return;
  }
  tcvrIt->second->markLastDownTime();
}

time_t TransceiverManager::getLastDownTime(TransceiverID id) const {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    throw FbossError(
        "Can't find Transceiver=", id, ". Transceiver is not present");
  }
  return tcvrIt->second->getLastDownTime();
}
} // namespace fboss
} // namespace facebook
