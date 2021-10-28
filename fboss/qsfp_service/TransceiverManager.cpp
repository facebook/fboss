// Copyright 2021-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/TransceiverManager.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/qsfp_service/TransceiverStateMachineUpdate.h"

namespace facebook {
namespace fboss {

TransceiverManager::TransceiverManager(
    std::unique_ptr<TransceiverPlatformApi> api,
    std::unique_ptr<PlatformMapping> platformMapping)
    : qsfpPlatApi_(std::move(api)),
      platformMapping_(std::move(platformMapping)),
      stateMachines_(setupTransceiverToStateMachine()) {
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
      // Init state should be "TRANSCEIVER_STATE_NOT_PRESENT"
      auto stateMachine = std::make_unique<
          folly::Synchronized<state_machine<TransceiverStateMachine>>>();
      stateMachineMap.emplace(
          TransceiverID(*chip.second.physicalID_ref()),
          std::move(stateMachine));
    }
  }
  return stateMachineMap;
}

void TransceiverManager::startThreads() {
  if (FLAGS_use_new_state_machine) {
    updateThread_.reset(new std::thread([=] {
      this->threadLoop("qsfpModuleStateUpdateThread", &updateEventBase_);
      XLOG(DBG2) << "Started qsfpModuleStateUpdateThread";
    }));
  }
}
void TransceiverManager::stopThreads() {
  // We use runInEventBaseThread() to terminateLoopSoon() rather than calling it
  // directly here.  This ensures that any events already scheduled via
  // runInEventBaseThread() will have a chance to run.
  if (updateThread_) {
    updateEventBase_.runInEventBaseThread(
        [this] { updateEventBase_.terminateLoopSoon(); });
    updateThread_->join();
    XLOG(DBG2) << "Terminated qsfpModuleStateUpdateThread";
  }
  // TODO(joseph5wu) Might need to consider how to handle pending updates just
  // as wedge_agent
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
  auto stateMachineUpdate =
      std::make_unique<TransceiverStateMachineUpdate>(id, event);
  if (isExiting_) {
    XLOG(WARN) << "Skipped queueing update:" << stateMachineUpdate->getName()
               << ", since exit already started";
    return;
  }
  {
    std::unique_lock guard(pendingUpdatesLock_);
    pendingUpdates_.push_back(*stateMachineUpdate.release());
  }

  // Signal the update thread that updates are pending.
  // We call runInEventBaseThread() with a static function pointer since this
  // is more efficient than having to allocate a new bound function object.
  updateEventBase_.runInEventBaseThread(handlePendingUpdatesHelper, this);
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
  return getStateByOrder(*lockedStateMachine->current_state());
}

} // namespace fboss
} // namespace facebook
