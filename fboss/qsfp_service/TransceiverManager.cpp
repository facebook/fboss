// Copyright 2021-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/TransceiverManager.h"

#include <fb303/ThreadCachedServiceData.h>
#include <folly/DynamicConverter.h>
#include <folly/FileUtil.h>
#include <folly/json.h>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/TransceiverStateMachineUpdate.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

using namespace std::chrono;

// allow us to configure the qsfp_service dir so that the qsfp cold boot test
// can run concurrently with itself
DEFINE_string(
    qsfp_service_volatile_dir,
    "/dev/shm/fboss/qsfp_service",
    "Path to the directory in which we store the qsfp_service's cold boot flag");

DEFINE_bool(
    can_qsfp_service_warm_boot,
    true,
    "Enable/disable warm boot functionality for qsfp_service");

DEFINE_int32(
    state_machine_update_thread_heartbeat_ms,
    10000,
    "State machine update thread's heartbeat interval (ms)");

namespace {
constexpr auto kForceColdBootFileName = "cold_boot_once_qsfp_service";
constexpr auto kWarmBootFlag = "can_warm_boot";
constexpr auto kWarmbootStateFileName = "qsfp_service_state";
constexpr auto kPhyStateKey = "phy";
constexpr auto kAgentConfigAppliedInfoStateKey = "agentConfigAppliedInfo";
constexpr auto kAgentConfigLastAppliedInMsKey = "agentConfigLastAppliedInMs";
constexpr auto kAgentConfigLastColdbootAppliedInMsKey =
    "agentConfigLastColdbootAppliedInMs";
static constexpr auto kStateMachineThreadHeartbeatMissed =
    "state_machine_thread_heartbeat_missed";
} // namespace

namespace facebook::fboss {

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
    const auto& portName = *platformPort.mapping()->name();
    portNameToPortID_.insert(PortNameIdMap::value_type(portName, portID));
    SwPortInfo portInfo;
    portInfo.name = portName;
    portInfo.tcvrID = utility::getTransceiverId(platformPort, chips);
    portToSwPortInfo_.emplace(portID, std::move(portInfo));
  }
}

TransceiverManager::~TransceiverManager() {
  // Make sure if gracefulExit() is not called, we will still stop the threads
  if (!isExiting_) {
    isExiting_ = true;
    stopThreads();
  }
}

void TransceiverManager::readWarmBootStateFile() {
  CHECK(canWarmBoot_);

  std::string warmBootJson;
  const auto& warmBootStateFile = warmBootStateFileName();
  if (!folly::readFile(warmBootStateFile.c_str(), warmBootJson)) {
    XLOG(WARN) << "Warm Boot state file: " << warmBootStateFile
               << " doesn't exist, skip restoring warm boot state";
    return;
  }

  warmBootState_ = folly::parseJson(warmBootJson);
}

void TransceiverManager::init() {
  // Check whether we can warm boot
  canWarmBoot_ = checkWarmBootFlags();
  if (!FLAGS_can_qsfp_service_warm_boot) {
    canWarmBoot_ = false;
  }
  XLOG(INFO) << "Will attempt " << (canWarmBoot_ ? "WARM" : "COLD") << " boot";
  if (!canWarmBoot_) {
    // Since this is going to be cold boot, we need to remove the can_warm_boot
    // file
    removeWarmBootFlag();
  } else {
    // Read the warm boot state file for a warm boot
    readWarmBootStateFile();
    restoreAgentConfigAppliedInfo();
  }

  // Now we might need to start threads
  startThreads();

  // Initialize the PhyManager all ExternalPhy for the system
  initExternalPhyMap();
  // Initialize the I2c bus
  initTransceiverMap();
}

void TransceiverManager::restoreAgentConfigAppliedInfo() {
  if (warmBootState_.isNull()) {
    return;
  }
  if (const auto& agentConfigAppliedIt =
          warmBootState_.find(kAgentConfigAppliedInfoStateKey);
      agentConfigAppliedIt != warmBootState_.items().end()) {
    auto agentConfigAppliedInfo = agentConfigAppliedIt->second;
    ConfigAppliedInfo wbConfigAppliedInfo;
    // Restore the last agent config applied timestamp from warm boot state if
    // it exists
    if (const auto& lastAppliedIt =
            agentConfigAppliedInfo.find(kAgentConfigLastAppliedInMsKey);
        lastAppliedIt != agentConfigAppliedInfo.items().end()) {
      wbConfigAppliedInfo.lastAppliedInMs() =
          folly::convertTo<long>(lastAppliedIt->second);
    }
    // Restore the last agent coldboot timestamp from warm boot state if
    // it exists
    if (const auto& lastColdBootIt =
            agentConfigAppliedInfo.find(kAgentConfigLastColdbootAppliedInMsKey);
        lastColdBootIt != agentConfigAppliedInfo.items().end()) {
      wbConfigAppliedInfo.lastColdbootAppliedInMs() =
          folly::convertTo<long>(lastColdBootIt->second);
    }

    configAppliedInfo_ = wbConfigAppliedInfo;
  }
}

void TransceiverManager::gracefulExit() {
  steady_clock::time_point begin = steady_clock::now();
  XLOG(INFO) << "[Exit] Starting TransceiverManager graceful exit";
  // Stop all the threads before shutdown
  isExiting_ = true;
  stopThreads();
  steady_clock::time_point stopThreadsDone = steady_clock::now();
  XLOG(INFO) << "[Exit] Stopped all state machine threads. Stop time: "
             << duration_cast<duration<float>>(stopThreadsDone - begin).count();

  // Set all warm boot related files before gracefully shut down
  setWarmBootState();
  setCanWarmBoot();
  steady_clock::time_point setWBFilesDone = steady_clock::now();
  XLOG(INFO) << "[Exit] Done creating Warm Boot related files. Stop time: "
             << duration_cast<duration<float>>(setWBFilesDone - stopThreadsDone)
                    .count()
             << std::endl
             << "[Exit] Total TransceiverManager graceful Exit time: "
             << duration_cast<duration<float>>(setWBFilesDone - begin).count();
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

      auto& portName = *(port.mapping()->name());
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
      ports.insert(*port.mapping()->name());
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
  TransceiverToStateMachineHelper stateMachineMap;
  for (auto chip : platformMapping_->getChips()) {
    if (*chip.second.type() != phy::DataPlanePhyChipType::TRANSCEIVER) {
      continue;
    }
    auto tcvrID = TransceiverID(*chip.second.physicalID());
    stateMachineMap.emplace(
        tcvrID, std::make_unique<TransceiverStateMachineHelper>(this, tcvrID));
  }
  return stateMachineMap;
}

TransceiverManager::TransceiverToPortInfo
TransceiverManager::setupTransceiverToPortInfo() {
  TransceiverToPortInfo tcvrToPortInfo;
  for (auto chip : platformMapping_->getChips()) {
    if (*chip.second.type() != phy::DataPlanePhyChipType::TRANSCEIVER) {
      continue;
    }
    auto tcvrID = TransceiverID(*chip.second.physicalID());
    auto portToPortInfo = std::make_unique<
        folly::Synchronized<std::unordered_map<PortID, TransceiverPortInfo>>>();
    tcvrToPortInfo.emplace(tcvrID, std::move(portToPortInfo));
  }
  return tcvrToPortInfo;
}

void TransceiverManager::startThreads() {
  // Setup all TransceiverStateMachineHelper thread
  for (auto& stateMachineHelper : stateMachines_) {
    stateMachineHelper.second->startThread();
    heartbeats_.push_back(stateMachineHelper.second->getThreadHeartbeat());
  }

  XLOG(DBG2) << "Started TransceiverStateMachineUpdateThread";
  updateEventBase_ = std::make_unique<folly::EventBase>();
  updateThread_.reset(new std::thread([=] {
    this->threadLoop(
        "TransceiverStateMachineUpdateThread", updateEventBase_.get());
  }));

  auto heartbeatStatsFunc = [this](int /* delay */, int /* backLog */) {};
  heartbeats_.push_back(std::make_shared<ThreadHeartbeat>(
      updateEventBase_.get(),
      "updateThreadHeartbeat",
      FLAGS_state_machine_update_thread_heartbeat_ms,
      heartbeatStatsFunc));

  // Create a watchdog that will monitor the heartbeats of all the threads and
  // increment the missed counter when there is no heartbeat on at least one
  // thread in the last FLAGS_state_machine_update_thread_heartbeat_ms * 10 time
  heartbeatWatchdog_ = std::make_unique<ThreadHeartbeatWatchdog>(
      std::chrono::milliseconds(
          FLAGS_state_machine_update_thread_heartbeat_ms * 10),
      [this]() {
        stateMachineThreadHeartbeatMissedCount_ += 1;
        tcData().setCounter(
            kStateMachineThreadHeartbeatMissed,
            stateMachineThreadHeartbeatMissedCount_);
      });
  for (auto heartbeat : heartbeats_) {
    heartbeatWatchdog_->startMonitoringHeartbeat(heartbeat);
  }
  // Kick off the heartbeat monitoring
  heartbeatWatchdog_->start();
}

void TransceiverManager::stopThreads() {
  if (heartbeatWatchdog_) {
    heartbeatWatchdog_->stop();
    heartbeatWatchdog_.reset();
  }
  for (auto heartbeat_ : heartbeats_) {
    heartbeat_.reset();
  }

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

const state_machine<TransceiverStateMachine>&
TransceiverManager::getStateMachineForTesting(TransceiverID id) const {
  auto stateMachineItr = stateMachines_.find(id);
  if (stateMachineItr == stateMachines_.end()) {
    throw FbossError("Transceiver:", id, " doesn't exist");
  }
  const auto& lockedStateMachine =
      stateMachineItr->second->getStateMachine().rlock();
  return *lockedStateMachine;
}

bool TransceiverManager::getNeedResetDataPath(TransceiverID id) const {
  auto stateMachineItr = stateMachines_.find(id);
  if (stateMachineItr == stateMachines_.end()) {
    throw FbossError("Transceiver:", id, " doesn't exist");
  }
  return stateMachineItr->second->getStateMachine().rlock()->get_attribute(
      needResetDataPath);
}

std::vector<TransceiverID> TransceiverManager::triggerProgrammingEvents() {
  std::vector<TransceiverID> programmedTcvrs;
  int32_t numProgramIphy{0}, numProgramXphy{0}, numProgramTcvr{0},
      numPrepareTcvr{0};
  BlockingStateUpdateResultList results;
  steady_clock::time_point begin = steady_clock::now();
  for (auto& stateMachine : stateMachines_) {
    bool needProgramIphy{false}, needProgramXphy{false}, needProgramTcvr{false},
        moduleStateReady{false};
    {
      const auto& lockedStateMachine =
          stateMachine.second->getStateMachine().rlock();
      needProgramIphy = !lockedStateMachine->get_attribute(isIphyProgrammed);
      needProgramXphy = !lockedStateMachine->get_attribute(isXphyProgrammed);
      needProgramTcvr =
          !lockedStateMachine->get_attribute(isTransceiverProgrammed);
      moduleStateReady =
          (getStateByOrder(*lockedStateMachine->current_state()) ==
           TransceiverStateMachineState::TRANSCEIVER_READY);
    }
    auto tcvrID = stateMachine.first;
    if (needProgramIphy) {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID, TransceiverStateMachineEvent::TCVR_EV_PROGRAM_IPHY)) {
        programmedTcvrs.push_back(tcvrID);
        ++numProgramIphy;
        results.push_back(result);
      }
    } else if (needProgramXphy && phyManager_ != nullptr) {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID, TransceiverStateMachineEvent::TCVR_EV_PROGRAM_XPHY)) {
        programmedTcvrs.push_back(tcvrID);
        ++numProgramXphy;
        results.push_back(result);
      }
    } else if (needProgramTcvr) {
      std::shared_ptr<BlockingTransceiverStateMachineUpdateResult> result{
          nullptr};

      if (moduleStateReady) {
        result = updateStateBlockingWithoutWait(
            tcvrID, TransceiverStateMachineEvent::TCVR_EV_PROGRAM_TRANSCEIVER);
        if (result) {
          ++numProgramTcvr;
        }
      } else {
        result = updateStateBlockingWithoutWait(
            tcvrID, TransceiverStateMachineEvent::TCVR_EV_PREPARE_TRANSCEIVER);
        if (result) {
          ++numPrepareTcvr;
        }
      }
      if (result) {
        programmedTcvrs.push_back(tcvrID);
        results.push_back(result);
      }
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG_IF(DBG2, !programmedTcvrs.empty())
      << "triggerProgrammingEvents has " << numProgramIphy
      << " IPHY programming, " << numProgramXphy << " XPHY programming, "
      << numProgramTcvr << " TCVR programming, " << numPrepareTcvr
      << " TCVR prepare. Total execute time(ms):"
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

void TransceiverManager::resetProgrammedIphyPortToPortInfo(TransceiverID id) {
  if (auto it = tcvrToPortInfo_.find(id); it != tcvrToPortInfo_.end()) {
    auto portToPortInfoWithLock = it->second->wlock();
    portToPortInfoWithLock->clear();
  }
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

void TransceiverManager::programExternalPhyPorts(
    TransceiverID id,
    bool needResetDataPath) {
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

    phyManager->programOnePort(
        portID, portInfo.profile, transceiver, needResetDataPath);
    XLOG(INFO) << "Programmed XPHY port for Transceiver=" << id
               << ", Port=" << portID << ", Profile="
               << apache::thrift::util::enumNameSafe(portInfo.profile)
               << ", needResetDataPath=" << needResetDataPath;
  }
}

TransceiverInfo TransceiverManager::getTransceiverInfo(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(id); it != lockedTransceivers->end()) {
    return it->second->getTransceiverInfo();
  } else {
    TransceiverInfo absentTcvr;
    absentTcvr.present() = false;
    absentTcvr.port() = id;
    absentTcvr.timeCollected() = std::time(nullptr);
    absentTcvr.tcvrState()->timeCollected() = std::time(nullptr);
    absentTcvr.tcvrStats()->timeCollected() = std::time(nullptr);
    return absentTcvr;
  }
}

void TransceiverManager::programTransceiver(
    TransceiverID id,
    bool needResetDataPath) {
  // Get programmed iphy port profile
  const auto& programmedPortToPortInfo = getProgrammedIphyPortToPortInfo(id);
  if (programmedPortToPortInfo.empty()) {
    // This is due to the iphy ports are disabled. So no need to program tcvr
    XLOG(DBG2) << "Skip programming Transceiver=" << id
               << ". Can't find programmed iphy port and port info";
    return;
  }

  ProgramTransceiverState programTcvrState;
  for (const auto& portToPortInfo : programmedPortToPortInfo) {
    auto portProfile = portToPortInfo.second.profile;
    auto portName = getPortNameByPortId(portToPortInfo.first);
    if (!portName.has_value()) {
      throw FbossError(
          "Can't find a portName for portId ", portToPortInfo.first);
    }
    uint8_t tcvrStartLane = 0;
    // Use platform mapping to fetch the transceiver start lane given the port
    // id and profile id
    auto tcvrHostLanes = platformMapping_->getTransceiverHostLanes(
        PlatformPortProfileConfigMatcher(
            portProfile /* profileID */,
            portToPortInfo.first /* portID */,
            std::nullopt /* portConfigOverrideFactor */));
    if (tcvrHostLanes.empty()) {
      throw FbossError("tcvrHostLanes empty for portId ", portToPortInfo.first);
    }
    // tcvrHostLanes is an ordered set. So begin() gives us the first lane
    tcvrStartLane = *tcvrHostLanes.begin();

    if (tcvrStartLane > 8) {
      throw FbossError(
          "Invalid start lane of ",
          tcvrStartLane,
          " for portId ",
          portToPortInfo.first);
    }
    auto profileCfgOpt = platformMapping_->getPortProfileConfig(
        PlatformPortProfileConfigMatcher(portProfile));
    if (!profileCfgOpt) {
      throw FbossError(
          "Can't find profile config for profileID=",
          apache::thrift::util::enumNameSafe(portProfile));
    }
    const auto speed = *profileCfgOpt->speed();
    TransceiverPortState portState;
    portState.portName = *portName;
    portState.startHostLane = tcvrStartLane;
    portState.speed = speed;
    programTcvrState.ports.emplace(*portName, portState);
  }

  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip programming Transceiver=" << id
               << ". Transeciver is not present";
    return;
  }

  tcvrIt->second->programTransceiver(programTcvrState, needResetDataPath);
  XLOG(INFO) << "Programmed Transceiver for Transceiver=" << id
             << (needResetDataPath ? " with" : " without")
             << " resetting data path";
}

/*
 * readyTransceiver
 *
 * Calls the module type specific function to check their power control
 * configuration and if needed, corrects it. Returns if the module is in ready
 * state to proceed further with programming.
 */
bool TransceiverManager::readyTransceiver(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip Ready Checking Transceiver=" << id
               << ". Transeciver is not present";
    return true;
  }

  return tcvrIt->second->readyTransceiver();
}

bool TransceiverManager::tryRemediateTransceiver(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip remediating Transceiver=" << id
               << ". Transeciver is not present";
    return false;
  }
  bool allPortsDown;
  std::vector<std::string> portsToRemediate;
  std::tie(allPortsDown, portsToRemediate) = areAllPortsDown(id);
  bool didRemediate =
      tcvrIt->second->tryRemediate(allPortsDown, portsToRemediate);
  XLOG_IF(INFO, didRemediate)
      << "Remediated Transceiver for Transceiver=" << id
      << " and ports=" << folly::join(",", portsToRemediate);
  return didRemediate;
}

bool TransceiverManager::supportRemediateTransceiver(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Transceiver=" << id
               << " is not present and can't support remediate";
    return false;
  }
  return tcvrIt->second->supportRemediate();
}

void TransceiverManager::updateTransceiverPortStatus() noexcept {
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
          event.emplace(
              TransceiverStateMachineEvent::TCVR_EV_RESET_TO_DISCOVERED);
        } else {
          ++numResetToNotPresent;
          event.emplace(
              TransceiverStateMachineEvent::TCVR_EV_RESET_TO_NOT_PRESENT);
        }
      };

  const auto& presentTransceivers = getPresentTransceivers();
  BlockingStateUpdateResultList results;
  for (auto& [tcvrID, portToPortInfo] : tcvrToPortInfo_) {
    std::unordered_set<PortID> statusChangedPorts;
    bool anyPortUp = false;
    bool isTcvrPresent =
        (presentTransceivers.find(tcvrID) != presentTransceivers.end());
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
          if (!(*portStatusIt->second.enabled())) {
            if (cachedPortInfoIt != portToPortInfoWithLock->end()) {
              portToPortInfoWithLock->erase(cachedPortInfoIt);
              genStateMachineResetEvent(event, isTcvrPresent);
            }
          } else {
            // Only care about enabled port status
            anyPortUp = anyPortUp || *portStatusIt->second.up();
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
                  *cachedPortInfoIt->second.status->up() !=
                      *portStatusIt->second.up()) {
                statusChangedPorts.insert(portID);
              }
              cachedPortInfoIt->second.status.emplace(portStatusIt->second);
            }
          }
        }
      }
      // If event is not set, it means not reset event is needed, now check
      // whether we need port status event.
      // Make sure we update active state for a transceiver which just
      // finished programming
      if (!event && ((!statusChangedPorts.empty()) || isTcvrJustProgrammed)) {
        event.emplace(
            anyPortUp ? TransceiverStateMachineEvent::TCVR_EV_PORT_UP
                      : TransceiverStateMachineEvent::TCVR_EV_ALL_PORTS_DOWN);
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
    // After releasing portToPortInfo lock, publishLinkSnapshots() will use
    // transceivers_ lock later
    for (auto portID : statusChangedPorts) {
      try {
        publishLinkSnapshots(portID);
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Port " << portID
                  << " failed publishLinkSnapshpts(): " << ex.what();
      }
    }
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
    std::unordered_set<PortID> statusChangedPorts;
    bool anyPortUp = false;
    bool isTcvrJustProgrammed =
        (getCurrentState(tcvrID) ==
         TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
    { // lock block for portToPortInfo
      auto portToPortInfoWithLock = tcvrToPortInfoIt->second->wlock();
      for (auto& [portID, tcvrPortInfo] : *portToPortInfoWithLock) {
        // Check whether there's a new port status for such port
        auto portStatusIt = portStatus.find(portID);
        // If port doesn't need to be updated, use the current cached status
        // to indicate whether we need a state update
        if (portStatusIt == portStatus.end()) {
          if (tcvrPortInfo.status) {
            anyPortUp = anyPortUp || *tcvrPortInfo.status->up();
          }
        } else {
          // Only care about enabled port status
          if (*portStatusIt->second.enabled()) {
            anyPortUp = anyPortUp || *portStatusIt->second.up();
            if (!tcvrPortInfo.status ||
                *tcvrPortInfo.status->up() != *portStatusIt->second.up()) {
              statusChangedPorts.insert(portID);
              // No need to do the transceiverRefresh() in this code path
              // because that will again enqueue state machine update on i2c
              // event base. That will result in deadlock with
              // stateMachineRefresh() generated update which also runs in same
              // i2c event base
            }
            // And also update the cached port status
            tcvrPortInfo.status = portStatusIt->second;
          }
        }
      }

      // Make sure the port event will be added to the update queue under the
      // lock of portToPortInfo, so that it will make sure the cached status
      // and the state machine will be in sync
      // Make sure we update active state for a transceiver which just
      // finished programming
      if ((!statusChangedPorts.empty()) || isTcvrJustProgrammed) {
        auto event = anyPortUp
            ? TransceiverStateMachineEvent::TCVR_EV_PORT_UP
            : TransceiverStateMachineEvent::TCVR_EV_ALL_PORTS_DOWN;
        ++numPortStatusChanged;
        if (auto result = updateStateBlockingWithoutWait(tcvrID, event)) {
          results.push_back(result);
        }
      }
    } // lock block for portToPortInfo
    // After releasing portToPortInfo lock, publishLinkSnapshots() will use
    // transceivers_ lock later
    for (auto portID : statusChangedPorts) {
      try {
        publishLinkSnapshots(portID);
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Port " << portID
                  << " failed publishLinkSnapshpts(): " << ex.what();
      }
    }
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
  const auto& presentXcvrIds = refreshTransceivers();

  // Step3: Check whether there's a wedge_agent config change
  triggerAgentConfigChangeEvent();

  // Step4: Once the transceivers are detected, trigger programming events
  const auto& programmedTcvrs = triggerProgrammingEvents();

  // Step5: Remediate inactive transceivers
  // Only need to remediate ports which are not recently finished
  // programming. Because if they only finished early stage programming like
  // iphy without programming xphy or tcvr, the ports of such transceiver
  // will still be not stable to be remediated.
  std::vector<TransceiverID> stableTcvrs;
  for (auto tcvrID : presentXcvrIds) {
    if (std::find(programmedTcvrs.begin(), programmedTcvrs.end(), tcvrID) ==
        programmedTcvrs.end()) {
      stableTcvrs.push_back(tcvrID);
    }
  }
  triggerRemediateEvents(stableTcvrs);
}

void TransceiverManager::triggerAgentConfigChangeEvent() {
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

  // Update present transceiver state machine back to DISCOVERED
  // and absent transeiver state machine back to NOT_PRESENT
  int numResetToDiscovered{0}, numResetToNotPresent{0};
  const auto& presentTransceivers = getPresentTransceivers();
  BlockingStateUpdateResultList results;
  for (auto& stateMachine : stateMachines_) {
    // Only need to set true to `needResetDataPath` attribute here. And leave
    // the state machine to change it to false once it finishes
    // programTransceiver
    if (resetDataPath) {
      stateMachine.second->getStateMachine().wlock()->get_attribute(
          needResetDataPath) = true;
    }
    auto tcvrID = stateMachine.first;
    if (presentTransceivers.find(tcvrID) != presentTransceivers.end()) {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID,
              TransceiverStateMachineEvent::TCVR_EV_RESET_TO_DISCOVERED)) {
        ++numResetToDiscovered;
        results.push_back(result);
      }
    } else {
      if (auto result = updateStateBlockingWithoutWait(
              tcvrID,
              TransceiverStateMachineEvent::TCVR_EV_RESET_TO_NOT_PRESENT)) {
        ++numResetToNotPresent;
        results.push_back(result);
      }
    }
  }
  waitForAllBlockingStateUpdateDone(results);
  XLOG(INFO) << "triggerAgentConfigChangeEvent has " << numResetToDiscovered
             << " transceivers state machines set back to discovered, "
             << numResetToNotPresent << " set back to not_present";
  configAppliedInfo_ = newConfigAppliedInfo;
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
  // Make sure this attr is false by default.
  lockedStateMachine->get_attribute(needResetDataPath) = false;
}

void TransceiverManager::TransceiverStateMachineHelper::startThread() {
  updateEventBase_ = std::make_unique<folly::EventBase>("update thread");
  updateThread_.reset(
      new std::thread([this] { updateEventBase_->loopForever(); }));
  auto heartbeatStatsFunc = [this](int /* delay */, int /* backLog */) {};
  heartbeat_ = std::make_shared<ThreadHeartbeat>(
      updateEventBase_.get(),
      folly::to<std::string>("stateMachine_", tcvrID_, "_"),
      FLAGS_state_machine_update_thread_heartbeat_ms,
      heartbeatStatsFunc);
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
    const std::string& portName) const {
  auto portMapIt = portNameToPortID_.left.find(portName);
  if (portMapIt != portNameToPortID_.left.end()) {
    return portMapIt->second;
  }
  return std::nullopt;
}

/*
 * getPortNameByPortId
 *
 * This function takes the software port id and returns corresponding port name
 * string (ie: eth2/1/1)
 */
std::optional<std::string> TransceiverManager::getPortNameByPortId(
    PortID portId) const {
  auto portMapIt = portNameToPortID_.right.find(portId);
  if (portMapIt != portNameToPortID_.right.end()) {
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

std::set<TransceiverID> TransceiverManager::getPresentTransceivers() const {
  std::set<TransceiverID> presentTcvrs;
  auto lockedTransceivers = transceivers_.rlock();
  for (const auto& tcvrIt : *lockedTransceivers) {
    if (tcvrIt.second->isPresent()) {
      presentTcvrs.insert(tcvrIt.first);
    }
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
      status.enabled() = enabled;
      status.up() = up;
      status.profileID() = apache::thrift::util::enumNameSafe(profileID);
      overrideAgentPortStatusForTesting_.emplace(portID, std::move(status));
    }
  }
}

void TransceiverManager::setOverrideAgentConfigAppliedInfoForTesting(
    std::optional<ConfigAppliedInfo> configAppliedInfo) {
  overrideAgentConfigAppliedInfoForTesting_ = configAppliedInfo;
}

std::pair<bool, std::vector<std::string>> TransceiverManager::areAllPortsDown(
    TransceiverID id) const noexcept {
  auto portToPortInfoIt = tcvrToPortInfo_.find(id);
  if (portToPortInfoIt == tcvrToPortInfo_.end()) {
    XLOG(WARN) << "Can't find Transceiver:" << id
               << " in cached tcvrToPortInfo_";
    return {false, {}};
  }
  auto portToPortInfoWithLock = portToPortInfoIt->second->rlock();
  if (portToPortInfoWithLock->empty()) {
    XLOG(WARN) << "Can't find any programmed port for Transceiver:" << id
               << " in cached tcvrToPortInfo_";
    return {false, {}};
  }
  bool anyPortUp = false;
  std::vector<std::string> downPorts;
  for (const auto& [portID, portInfo] : *portToPortInfoWithLock) {
    if (!portInfo.status.has_value()) {
      // If no status set, assume ports are up so we won't trigger any
      // disruptive event
      return {false, {}};
    }
    if (*portInfo.status->up()) {
      anyPortUp = true;
    } else {
      auto portName = getPortNameByPortId(portID);
      if (portName.has_value()) {
        downPorts.push_back(*portName);
      }
    }
  }
  return {!anyPortUp, downPorts};
}

void TransceiverManager::triggerRemediateEvents(
    const std::vector<TransceiverID>& stableTcvrs) {
  if (stableTcvrs.empty()) {
    return;
  }
  BlockingStateUpdateResultList results;
  for (auto tcvrID : stableTcvrs) {
    const auto& programmedPortToPortInfo =
        getProgrammedIphyPortToPortInfo(tcvrID);
    if (programmedPortToPortInfo.empty()) {
      // This is due to the iphy ports are disabled. So no need to remediate
      continue;
    }

    auto curState = getCurrentState(tcvrID);
    // If we are not in the active or inactive state, don't try to remediate yet
    if (curState != TransceiverStateMachineState::ACTIVE &&
        curState != TransceiverStateMachineState::INACTIVE) {
      continue;
    }

    // If we are here because we are in active state, check if any of the ports
    // are down. If yes, try to remediate (partial). If we are here because we
    // are in inactive state, areAllPortsDown will return a non-empty list of
    // down ports anyways, so we will try to remediate
    if (areAllPortsDown(tcvrID).second.empty()) {
      continue;
    }

    // Then check whether we should remediate so that we don't have to create
    // too many unnecessary state machine update
    auto lockedTransceivers = transceivers_.rlock();
    auto tcvrIt = lockedTransceivers->find(tcvrID);
    if (tcvrIt == lockedTransceivers->end()) {
      XLOG(DBG2) << "Skip remediating Transceiver=" << tcvrID
                 << ". Transceiver is not present";
      continue;
    }
    if (!tcvrIt->second->shouldRemediate()) {
      continue;
    }
    if (auto result = updateStateBlockingWithoutWait(
            tcvrID,
            TransceiverStateMachineEvent::TCVR_EV_REMEDIATE_TRANSCEIVER)) {
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

void TransceiverManager::getInterfacePhyInfo(
    std::map<std::string, phy::PhyInfo>& phyInfos,
    const std::string& portName) {
  auto portIDOpt = getPortIDByPortName(portName);
  if (!portIDOpt) {
    throw FbossError(
        "Unrecoginized portName:", portName, ", can't find port id");
  }
  try {
    phyInfos[portName] = getXphyInfo(*portIDOpt);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Fetching PhyInfo for " << portName << " failed with "
              << ex.what();
  }
}

void TransceiverManager::publishLinkSnapshots(std::string portName) {
  auto portIDOpt = getPortIDByPortName(portName);
  if (!portIDOpt) {
    throw FbossError(
        "Unrecoginized portName:", portName, ", can't find port id");
  }
  publishLinkSnapshots(*portIDOpt);
}

void TransceiverManager::publishLinkSnapshots(PortID portID) {
  // Publish xphy snapshots if there's a phyManager and xphy ports
  if (phyManager_) {
    phyManager_->publishXphyInfoSnapshots(portID);
  }
  // Publish transceiver snapshots if there's a transceiver
  if (auto tcvrIDOpt = getTransceiverID(portID)) {
    auto lockedTransceivers = transceivers_.rlock();
    if (auto tcvrIt = lockedTransceivers->find(*tcvrIDOpt);
        tcvrIt != lockedTransceivers->end()) {
      tcvrIt->second->publishSnapshots();
    }
  }
}

std::optional<TransceiverID> TransceiverManager::getTransceiverID(
    PortID portID) {
  auto swPortInfo = portToSwPortInfo_.find(portID);
  if (swPortInfo == portToSwPortInfo_.end()) {
    throw FbossError("Failed to find SwPortInfo for port ID ", portID);
  }
  return swPortInfo->second.tcvrID;
}

bool TransceiverManager::verifyEepromChecksums(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  auto tcvrIt = lockedTransceivers->find(id);
  if (tcvrIt == lockedTransceivers->end()) {
    XLOG(DBG2) << "Skip verifying eeprom checksum for Transceiver=" << id
               << ". Transceiver is not present";
    return true;
  }
  return tcvrIt->second->verifyEepromChecksums();
}

bool TransceiverManager::checkWarmBootFlags() {
  // Return true if coldBootOnceFile does not exist and canWarmBoot file exists
  const auto& forceColdBootFile = forceColdBootFileName();
  bool forceColdBoot = removeFile(forceColdBootFile);
  if (forceColdBoot) {
    XLOG(INFO) << "Force Cold Boot file: " << forceColdBootFile << " is set";
    return false;
  }

  const auto& warmBootFile = warmBootFlagFileName();
  // Instead of removing the can_warm_boot file, we keep it unless it's a
  // coldboot, so that qsfp_service crash can still use warm boot.
  bool canWarmBoot = checkFileExists(warmBootFile);
  XLOG(INFO) << "Warm Boot flag: " << warmBootFile << " is "
             << (canWarmBoot ? "set" : "missing");
  return canWarmBoot;
}

void TransceiverManager::removeWarmBootFlag() {
  removeFile(warmBootFlagFileName());
}

std::string TransceiverManager::forceColdBootFileName() {
  return folly::to<std::string>(
      FLAGS_qsfp_service_volatile_dir, "/", kForceColdBootFileName);
}

std::string TransceiverManager::warmBootFlagFileName() {
  return folly::to<std::string>(
      FLAGS_qsfp_service_volatile_dir, "/", kWarmBootFlag);
}

std::string TransceiverManager::warmBootStateFileName() const {
  return folly::to<std::string>(
      FLAGS_qsfp_service_volatile_dir, "/", kWarmbootStateFileName);
}

void TransceiverManager::setWarmBootState() {
  // Store necessary information of qsfp_service state into the warmboot state
  // file. This can be the lane id vector of each port from PhyManager or
  // transceiver info or the last config applied timestamp from agent
  folly::dynamic qsfpServiceState = folly::dynamic::object;
  steady_clock::time_point begin = steady_clock::now();
  if (phyManager_) {
    qsfpServiceState[kPhyStateKey] = phyManager_->getWarmbootState();
    phyManager_->gracefulExit();
  }

  folly::dynamic agentConfigAppliedWbState = folly::dynamic::object;
  agentConfigAppliedWbState[kAgentConfigLastAppliedInMsKey] =
      *configAppliedInfo_.lastAppliedInMs();
  if (auto lastAgentColdBootTime =
          configAppliedInfo_.lastColdbootAppliedInMs()) {
    agentConfigAppliedWbState[kAgentConfigLastColdbootAppliedInMsKey] =
        *lastAgentColdBootTime;
  }
  qsfpServiceState[kAgentConfigAppliedInfoStateKey] = agentConfigAppliedWbState;

  steady_clock::time_point getWarmbootState = steady_clock::now();
  XLOG(INFO)
      << "[Exit] Finish getting warm boot state. Time: "
      << duration_cast<duration<float>>(getWarmbootState - begin).count();
  folly::writeFile(
      folly::toPrettyJson(qsfpServiceState), warmBootStateFileName().c_str());
  steady_clock::time_point serializeState = steady_clock::now();
  XLOG(INFO) << "[Exit] Finish writing warm boot state to file. Time: "
             << duration_cast<duration<float>>(
                    serializeState - getWarmbootState)
                    .count();
}

void TransceiverManager::setCanWarmBoot() {
  const auto& warmBootFile = warmBootFlagFileName();
  auto createFd = createFile(warmBootFile);
  close(createFd);
  XLOG(INFO) << "Wrote can warm boot flag: " << warmBootFile;
}

void TransceiverManager::restoreWarmBootPhyState() {
  // Only need to restore warm boot state if this is a warm boot
  if (!canWarmBoot_) {
    XLOG(INFO) << "[Cold Boot] No need to restore warm boot state";
    return;
  }

  if (const auto& phyStateIt = warmBootState_.find(kPhyStateKey);
      phyManager_ && phyStateIt != warmBootState_.items().end()) {
    phyManager_->restoreFromWarmbootState(phyStateIt->second);
  }
}

namespace {
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

void TransceiverManager::setInterfacePrbs(
    std::string portName,
    phy::PortComponent component,
    const prbs::InterfacePrbsState& state) {
  // Get the port ID first
  auto portId = getPortIDByPortName(portName);
  if (!portId.has_value()) {
    throw FbossError("Can't find a portID for portName ", portName);
  }

  // Sanity check
  if (!state.generatorEnabled().has_value() &&
      !state.checkerEnabled().has_value()) {
    throw FbossError("Neither generator or checker specified for PRBS setting");
  }

  if (component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
      component == phy::PortComponent::TRANSCEIVER_LINE) {
    if (auto tcvrID = getTransceiverID(portId.value())) {
      phy::Side side = prbsComponentToPhySide(component);
      auto lockedTransceivers = transceivers_.rlock();
      if (auto it = lockedTransceivers->find(*tcvrID);
          it != lockedTransceivers->end()) {
        if (!it->second->setPortPrbs(side, state)) {
          throw FbossError("Failed to set PRBS on transceiver ", *tcvrID);
        }
      } else {
        throw FbossError("Can't find transceiver ", *tcvrID);
      }
    } else {
      throw FbossError("Can't find transceiverID for portID ", portId.value());
    }
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
        portId.value(), prbsComponentToPhySide(component), phyPrbs);
  }
}

phy::PrbsStats TransceiverManager::getPortPrbsStats(
    PortID portId,
    phy::PortComponent component) {
  phy::Side side = prbsComponentToPhySide(component);
  if (component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
      component == phy::PortComponent::TRANSCEIVER_LINE) {
    auto lockedTransceivers = transceivers_.rlock();
    if (auto tcvrID = getTransceiverID(portId)) {
      if (auto it = lockedTransceivers->find(*tcvrID);
          it != lockedTransceivers->end()) {
        return it->second->getPortPrbsStats(side);
      } else {
        throw FbossError("Can't find transceiver ", *tcvrID);
      }
    } else {
      throw FbossError("Can't find transceiverID for portID ", portId);
    }
  } else {
    if (!phyManager_) {
      throw FbossError("Current platform doesn't support xphy");
    }
    phy::PrbsStats stats;
    stats.laneStats() = phyManager_->getPortPrbsStats(portId, side);
    stats.portId() = portId;
    stats.component() = component;
    return stats;
  }
}

void TransceiverManager::clearPortPrbsStats(
    PortID portId,
    phy::PortComponent component) {
  phy::Side side = prbsComponentToPhySide(component);
  if (component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
      component == phy::PortComponent::TRANSCEIVER_LINE) {
    auto lockedTransceivers = transceivers_.rlock();
    if (auto tcvrID = getTransceiverID(portId)) {
      if (auto it = lockedTransceivers->find(*tcvrID);
          it != lockedTransceivers->end()) {
        it->second->clearTransceiverPrbsStats(side);
      } else {
        throw FbossError("Can't find transceiver ", *tcvrID);
      }
    } else {
      throw FbossError("Can't find transceiverID for portID ", portId);
    }
  } else if (!phyManager_) {
    throw FbossError("Current platform doesn't support xphy");
  } else {
    phyManager_->clearPortPrbsStats(portId, prbsComponentToPhySide(component));
  }
}

std::vector<prbs::PrbsPolynomial>
TransceiverManager::getTransceiverPrbsCapabilities(
    TransceiverID tcvrID,
    phy::Side side) {
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(tcvrID);
      it != lockedTransceivers->end()) {
    return it->second->getPrbsCapabilities(side);
  }
  return std::vector<prbs::PrbsPolynomial>();
}

void TransceiverManager::getSupportedPrbsPolynomials(
    std::vector<prbs::PrbsPolynomial>& prbsCapabilities,
    std::string portName,
    phy::PortComponent component) {
  phy::Side side = prbsComponentToPhySide(component);
  if (component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
      component == phy::PortComponent::TRANSCEIVER_LINE) {
    if (portNameToModule_.find(portName) == portNameToModule_.end()) {
      throw FbossError("Can't find transceiver module for port ", portName);
    }
    prbsCapabilities = getTransceiverPrbsCapabilities(
        TransceiverID(portNameToModule_[portName]), side);
  } else {
    throw FbossError(
        "PRBS on ",
        apache::thrift::util::enumNameSafe(component),
        " not supported by qsfp_service");
  }
}

void TransceiverManager::setPortPrbs(
    PortID portId,
    phy::PortComponent component,
    const phy::PortPrbsState& state) {
  auto portName = getPortNameByPortId(portId);
  if (!portName.has_value()) {
    throw FbossError("Can't find a portName for portId ", portId);
  }

  prbs::InterfacePrbsState newState;
  newState.polynomial() = prbs::PrbsPolynomial(state.polynominal().value());
  newState.generatorEnabled() = state.enabled().value();
  newState.checkerEnabled() = state.enabled().value();
  setInterfacePrbs(portName.value(), component, newState);
}

void TransceiverManager::getInterfacePrbsState(
    prbs::InterfacePrbsState& prbsState,
    std::string portName,
    phy::PortComponent component) {
  if (auto portID = getPortIDByPortName(portName)) {
    if (component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
        component == phy::PortComponent::TRANSCEIVER_LINE) {
      if (auto tcvrID = getTransceiverID(*portID)) {
        phy::Side side = prbsComponentToPhySide(component);
        auto lockedTransceivers = transceivers_.rlock();
        if (auto it = lockedTransceivers->find(*tcvrID);
            it != lockedTransceivers->end()) {
          prbsState = it->second->getPortPrbsState(side);
          return;
        } else {
          throw FbossError("Can't find transceiver ", *tcvrID);
        }
      } else {
        throw FbossError("Can't find transceiverID for portID ", *portID);
      }
    } else {
      throw FbossError(
          "getInterfacePrbsState not supported on component ",
          apache::thrift::util::enumNameSafe(component));
    }
  } else {
    throw FbossError("Can't find a portID for portName ", portName);
  }
}

phy::PrbsStats TransceiverManager::getInterfacePrbsStats(
    std::string portName,
    phy::PortComponent component) {
  if (auto portID = getPortIDByPortName(portName)) {
    return getPortPrbsStats(*portID, component);
  }
  throw FbossError("Can't find a portID for portName ", portName);
}

void TransceiverManager::clearInterfacePrbsStats(
    std::string portName,
    phy::PortComponent component) {
  if (auto portID = getPortIDByPortName(portName)) {
    clearPortPrbsStats(*portID, component);
  } else {
    throw FbossError("Can't find a portID for portName ", portName);
  }
}

std::optional<DiagsCapability> TransceiverManager::getDiagsCapability(
    TransceiverID id) const {
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(id); it != lockedTransceivers->end()) {
    return it->second->getDiagsCapability();
  }
  XLOG(WARN) << "Return nullopt DiagsCapability for Transceiver=" << id
             << ". Transeciver is not present";
  return std::nullopt;
}

void TransceiverManager::setDiagsCapability(TransceiverID id) {
  auto lockedTransceivers = transceivers_.rlock();
  if (auto it = lockedTransceivers->find(id); it != lockedTransceivers->end()) {
    return it->second->setDiagsCapability();
  }
  XLOG(DBG2) << "Skip setting DiagsCapability for Transceiver=" << id
             << ". Transceiver is not present";
}

Transceiver* FOLLY_NULLABLE TransceiverManager::overrideTransceiverForTesting(
    TransceiverID id,
    std::unique_ptr<Transceiver> overrideTcvr) {
  auto lockedTransceivers = transceivers_.wlock();
  // Keep the same logic as updateTransceiverMap()
  if (auto it = lockedTransceivers->find(id); it != lockedTransceivers->end()) {
    it->second->removeTransceiver();
    lockedTransceivers->erase(it);
  }
  // Only set the override transceiver if it's not null so that we can support
  // removing transceiver in tests
  if (overrideTcvr) {
    lockedTransceivers->emplace(id, std::move(overrideTcvr));
    return lockedTransceivers->at(id).get();
  } else {
    return nullptr;
  }
}

std::vector<TransceiverID> TransceiverManager::refreshTransceivers(
    const std::unordered_set<TransceiverID>& transceivers) {
  std::vector<TransceiverID> transceiverIds;
  std::vector<folly::Future<folly::Unit>> futs;

  {
    auto lockedTransceivers = transceivers_.rlock();
    auto nTransceivers =
        transceivers.empty() ? lockedTransceivers->size() : transceivers.size();
    XLOG(INFO) << "Start refreshing " << nTransceivers << " transceivers...";

    for (const auto& transceiver : *lockedTransceivers) {
      TransceiverID id = TransceiverID(transceiver.second->getID());
      // If we're trying to refresh a subset and this transceiver is not in that
      // subset, skip it.
      if (!transceivers.empty() &&
          transceivers.find(id) == transceivers.end()) {
        continue;
      }
      XLOG(DBG3) << "Fired to refresh TransceiverID=" << id;
      transceiverIds.push_back(id);
      futs.push_back(transceiver.second->futureRefresh());
    }

    folly::collectAll(futs.begin(), futs.end()).wait();
    XLOG(INFO) << "Finished refreshing " << nTransceivers << " transceivers";
  }

  publishTransceiversToFsdb(transceiverIds);

  return transceiverIds;
}

void TransceiverManager::resetTransceiver(
    std::unique_ptr<std::vector<std::string>> /* portNames */,
    ResetType /* resetType */,
    ResetAction /* resetAction */) {}

void TransceiverManager::setPauseRemediation(
    int32_t timeout,
    std::unique_ptr<std::vector<std::string>> portList) {
  if (!portList.get() || portList->empty()) {
    pauseRemediationUntil_ = std::time(nullptr) + timeout;
  } else {
    auto lockedTransceivers = transceivers_.rlock();
    for (auto port : *portList) {
      if (portNameToModule_.find(port) == portNameToModule_.end()) {
        throw FbossError("Can't find transceiver module for port ", port);
      }

      auto it =
          lockedTransceivers->find(TransceiverID(portNameToModule_.at(port)));
      if (it != lockedTransceivers->end()) {
        it->second->setModulePauseRemediation(timeout);
      }
    }
  }
}

void TransceiverManager::getPauseRemediationUntil(
    std::map<std::string, int32_t>& info,
    std::unique_ptr<std::vector<std::string>> portList) {
  if (!portList.get() || portList->empty()) {
    info["all"] = pauseRemediationUntil_;
  } else {
    auto lockedTransceivers = transceivers_.rlock();
    for (auto port : *portList) {
      if (portNameToModule_.find(port) == portNameToModule_.end()) {
        throw FbossError("Can't find transceiver module for port ", port);
      }
      auto it =
          lockedTransceivers->find(TransceiverID(portNameToModule_.at(port)));
      if (it != lockedTransceivers->end()) {
        info[port] = it->second->getModulePauseRemediationUntil();
      }
    }
  }
}

void TransceiverManager::setPortLoopbackState(
    std::string portName,
    phy::PortComponent component,
    bool setLoopback) {
  auto swPort = getPortIDByPortName(portName);
  if (!swPort.has_value()) {
    throw FbossError(
        folly::sformat("setPortLoopbackState: Invalid port {}", portName));
  }
  if (component != phy::PortComponent::GB_SYSTEM &&
      component != phy::PortComponent::GB_LINE) {
    XLOG(INFO)
        << " TransceiverManager::setPortLoopbackState - component not supported "
        << static_cast<int>(component);
    return;
  }

  XLOG(INFO) << " TransceiverManager::setPortLoopbackState Port "
             << static_cast<int>(swPort.value());
  getPhyManager()->setPortLoopbackState(
      PortID(swPort.value()), component, setLoopback);
}

void TransceiverManager::setPortAdminState(
    std::string portName,
    phy::PortComponent component,
    bool setAdminUp) {
  auto swPort = getPortIDByPortName(portName);
  if (!swPort.has_value()) {
    throw FbossError(
        folly::sformat("setPortAdminState: Invalid port {}", portName));
  }
  if (component != phy::PortComponent::GB_SYSTEM &&
      component != phy::PortComponent::GB_LINE) {
    XLOG(INFO)
        << " TransceiverManager::setPortAdminState - component not supported "
        << static_cast<int>(component);
    return;
  }

  XLOG(INFO) << " TransceiverManager::setPortAdminState Port "
             << static_cast<int>(swPort.value());
  getPhyManager()->setPortAdminState(
      PortID(swPort.value()), component, setAdminUp);
}

/*
 * getAllPortPhyInfo
 *
 * Get the map of software port id to PortPhyInfo in the system. This function
 * mainly for debugging
 */
std::map<uint32_t, phy::PhyIDInfo> TransceiverManager::getAllPortPhyInfo() {
  std::map<uint32_t, phy::PhyIDInfo> resultMap;

  auto allPlatformPortsIt = platformMapping_->getPlatformPorts();
  for (auto platformPortIt : allPlatformPortsIt) {
    auto portId = platformPortIt.first;
    GlobalXphyID xphyId;
    try {
      xphyId = phyManager_->getGlobalXphyIDbyPortID(PortID(portId));
    } catch (FbossError& ex) {
      continue;
    }
    phy::PhyIDInfo phyIdInfo = phyManager_->getPhyIDInfo(xphyId);
    resultMap[portId] = phyIdInfo;
  }

  return resultMap;
}

/*
 * getPhyInfo
 *
 * Returns the phy line params for a port
 */
phy::PhyInfo TransceiverManager::getPhyInfo(const std::string& portName) {
  auto swPort = getPortIDByPortName(portName);
  if (!swPort.has_value()) {
    throw FbossError(folly::sformat("getPhyInfo: Invalid port {}", portName));
  }
  return getPhyManager()->getPhyInfo(PortID(swPort.value()));
}

std::string TransceiverManager::getPortInfo(std::string portName) {
  auto swPort = getPortIDByPortName(portName);
  if (!swPort.has_value()) {
    throw FbossError(folly::sformat("getPortInfo: Invalid port {}", portName));
  }
  return getPhyManager()->getPortInfoStr(PortID(swPort.value()));
}

} // namespace facebook::fboss
