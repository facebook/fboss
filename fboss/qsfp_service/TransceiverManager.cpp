// Copyright 2021-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/TransceiverManager.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/TransceiverStateMachineUpdate.h"

using namespace std::chrono;

namespace facebook {
namespace fboss {

TransceiverManager::TransceiverManager(
    std::unique_ptr<TransceiverPlatformApi> api,
    std::unique_ptr<PlatformMapping> platformMapping)
    : qsfpPlatApi_(std::move(api)),
      platformMapping_(std::move(platformMapping)),
      stateMachines_(setupTransceiverToStateMachine()),
      tcvrToPortAndProfile_(setupTransceiverToPortAndProfile()) {
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

TransceiverManager::TransceiverToStateMachine
TransceiverManager::setupTransceiverToStateMachine() {
  // Set up NewModuleStateMachine map
  TransceiverToStateMachine stateMachineMap;
  if (FLAGS_use_new_state_machine) {
    for (auto chip : platformMapping_->getChips()) {
      if (*chip.second.type_ref() != phy::DataPlanePhyChipType::TRANSCEIVER) {
        continue;
      }
      auto tcvrID = TransceiverID(*chip.second.physicalID_ref());
      // Init state should be "TRANSCEIVER_STATE_NOT_PRESENT"
      auto stateMachine = std::make_unique<
          folly::Synchronized<state_machine<TransceiverStateMachine>>>();
      {
        auto lockedStateMachine = stateMachine->wlock();
        lockedStateMachine->get_attribute(transceiverMgrPtr) = this;
        lockedStateMachine->get_attribute(transceiverID) = tcvrID;
      }
      stateMachineMap.emplace(tcvrID, std::move(stateMachine));
    }
  }
  return stateMachineMap;
}

TransceiverManager::TransceiverToPortAndProfile
TransceiverManager::setupTransceiverToPortAndProfile() {
  TransceiverToPortAndProfile tcvrToPortAndProfile;
  if (FLAGS_use_new_state_machine) {
    for (auto chip : platformMapping_->getChips()) {
      if (*chip.second.type_ref() != phy::DataPlanePhyChipType::TRANSCEIVER) {
        continue;
      }
      auto tcvrID = TransceiverID(*chip.second.physicalID_ref());
      auto portAndProfile = std::make_unique<folly::Synchronized<
          std::unordered_map<PortID, cfg::PortProfileID>>>();
      tcvrToPortAndProfile.emplace(tcvrID, std::move(portAndProfile));
    }
  }
  return tcvrToPortAndProfile;
}

void TransceiverManager::startThreads() {
  if (FLAGS_use_new_state_machine) {
    XLOG(DBG2) << "Started qsfpModuleStateUpdateThread";
    updateEventBase_ = std::make_unique<folly::EventBase>();
    updateThread_.reset(new std::thread([=] {
      this->threadLoop("qsfpModuleStateUpdateThread", updateEventBase_.get());
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
    XLOG(DBG2) << "Terminated qsfpModuleStateUpdateThread";
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
}

void TransceiverManager::threadLoop(
    folly::StringPiece name,
    folly::EventBase* eventBase) {
  initThread(name);
  eventBase->loopForever();
}

void TransceiverManager::updateState(
    TransceiverID id,
    TransceiverStateMachineEvent event) {
  auto update = std::make_unique<TransceiverStateMachineUpdate>(id, event);
  updateState(std::move(update));
}

void TransceiverManager::updateStateBlocking(
    TransceiverID id,
    TransceiverStateMachineEvent event) {
  auto result = std::make_shared<BlockingTransceiverStateMachineUpdateResult>();
  auto update = std::make_unique<BlockingTransceiverStateMachineUpdate>(
      id, event, result);
  updateState(std::move(update));
  // wait for the result
  result->wait();
}

void TransceiverManager::updateState(
    std::unique_ptr<TransceiverStateMachineUpdate> update) {
  if (isExiting_) {
    XLOG(WARN) << "Skipped queueing update:" << update->getName()
               << ", since exit already started";
    return;
  }
  {
    std::unique_lock guard(pendingUpdatesLock_);
    pendingUpdates_.push_back(*update.release());
  }

  // Signal the update thread that updates are pending.
  // We call runInEventBaseThread() with a static function pointer since this
  // is more efficient than having to allocate a new bound function object.
  updateEventBase_->runInEventBaseThread(handlePendingUpdatesHelper, this);
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
  {
    std::unique_lock guard(pendingUpdatesLock_);
    // So far we don't have to support non coalescing updates like wedge_agent
    // we can just dump all existing updates in pendingUpdates_ to this temp
    // StateUpdateList `updates`
    updates.splice(
        updates.begin(),
        pendingUpdates_,
        pendingUpdates_.begin(),
        pendingUpdates_.end());
  }

  // handlePendingUpdates() is invoked once for each update, but a previous
  // call might have already processed everything.  If we don't have anything
  // to do just return early.
  if (updates.empty()) {
    return;
  }

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

    XLOG(INFO) << "Preparing ModuleStateMachine update for "
               << update->getName();
    // Hold the module state machine lock
    const auto& lockedStateMachine = stateMachineItr->second->wlock();
    try {
      update->applyUpdate(*lockedStateMachine);
    } catch (const std::exception& ex) {
      // Call the update's onError() function, and then immediately delete
      // it (therefore removing it from the intrusive list).  This way we won't
      // call it's onSuccess() function later.
      update->onError(ex);
      delete update;
    }
  }

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

  const auto& lockedStateMachine = stateMachineItr->second->rlock();
  auto curStateOrder = *lockedStateMachine->current_state();
  auto curState = getStateByOrder(curStateOrder);
  XLOG(DBG4) << "Current transceiver:" << static_cast<int32_t>(id)
             << ", state order:" << curStateOrder
             << ", state:" << getTransceiverStateMachineStateName(curState);
  return curState;
}

void TransceiverManager::triggerProgrammingEvents() {
  if (!FLAGS_use_new_state_machine) {
    return;
  }
  int32_t numProgramIphy{0}, numProgramXphy{0}, numProgramTcvr{0};
  steady_clock::time_point begin = steady_clock::now();
  for (auto& stateMachine : stateMachines_) {
    bool needProgramIphy{false}, needProgramXphy{false}, needProgramTcvr{false};
    {
      const auto& lockedStateMachine = stateMachine.second->rlock();
      needProgramIphy = !lockedStateMachine->get_attribute(isIphyProgrammed);
      needProgramXphy = !lockedStateMachine->get_attribute(isXphyProgrammed);
      needProgramTcvr =
          !lockedStateMachine->get_attribute(isTransceiverProgrammed);
    }
    if (needProgramIphy) {
      numProgramIphy++;
      updateStateBlocking(
          stateMachine.first, TransceiverStateMachineEvent::PROGRAM_IPHY);
    } else if (needProgramXphy && phyManager_ != nullptr) {
      numProgramXphy++;
      updateStateBlocking(
          stateMachine.first, TransceiverStateMachineEvent::PROGRAM_XPHY);
    } else if (needProgramTcvr) {
      numProgramTcvr++;
      updateStateBlocking(
          stateMachine.first,
          TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER);
    }
  }
  XLOG(DBG2)
      << "triggerProgrammingEvents has " << numProgramIphy
      << " IPHY programming, " << numProgramXphy << " XPHY programming, "
      << numProgramTcvr << " TCVR programming. Total execute time(ms):"
      << duration_cast<milliseconds>(steady_clock::now() - begin).count();
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
        logStr, id, " : ", apache::thrift::util::enumNameSafe(profileID), ", ");
  }
  XLOG(INFO) << logStr << "]";

  // Now update the programmed SW port to profile mapping
  if (auto portAndProfileIt = tcvrToPortAndProfile_.find(id);
      portAndProfileIt != tcvrToPortAndProfile_.end()) {
    auto portAndProfileWithLock = portAndProfileIt->second->wlock();
    portAndProfileWithLock->clear();
    for (auto [portID, profileID] : programmedIphyPorts) {
      portAndProfileWithLock->emplace(PortID(portID), profileID);
    }
  }
}

std::unordered_map<PortID, cfg::PortProfileID>
TransceiverManager::getProgrammedIphyPortAndProfile(TransceiverID id) const {
  if (auto portAndProfileIt = tcvrToPortAndProfile_.find(id);
      portAndProfileIt != tcvrToPortAndProfile_.end()) {
    return *(portAndProfileIt->second->rlock());
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
  const auto& programmedPortAndProfile = getProgrammedIphyPortAndProfile(id);
  if (programmedPortAndProfile.empty()) {
    // This is due to the iphy ports are disabled. So no need to program xphy
    XLOG(DBG2) << "Skip programming xphy ports for Transceiver=" << id
               << ". Can't find programmed iphy port and profile";
    return;
  }
  const auto& transceiver = getTransceiverInfo(id);
  for (const auto& [portID, profileID] : programmedPortAndProfile) {
    phyManager->programOnePort(portID, profileID, transceiver);
    XLOG(INFO) << "Programmed XPHY port for Transceiver=" << id
               << ", Port=" << portID
               << ", Profile=" << apache::thrift::util::enumNameSafe(profileID);
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
  const auto& programmedPortAndProfile = getProgrammedIphyPortAndProfile(id);
  if (programmedPortAndProfile.empty()) {
    // This is due to the iphy ports are disabled. So no need to program tcvr
    XLOG(DBG2) << "Skip programming Transceiver=" << id
               << ". Can't find programmed iphy port and profile";
    return;
  }
  // TODO(joseph5wu) Usually we only need to program optical Transceiver which
  // doesn't need to support split-out copper cable for flex ports.
  // Which means for the optical transceiver, it usually has one programmed
  // iphy port and profile.
  // If in the future, we need to support flex port copper transceiver
  // programming, we might need to combine the speeds of all flex port to
  // program such transceiver.
  const auto profileID = programmedPortAndProfile.begin()->second;
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
} // namespace fboss
} // namespace facebook
