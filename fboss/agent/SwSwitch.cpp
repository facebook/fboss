/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SwSwitch.h"

#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/IPv4Handler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/RouteUpdateLogger.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/UnresolvedNhopsProber.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/capture/PktCaptureManager.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/StateUpdateHelpers.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/TransceiverMap.h"
#include "fboss/agent/Transceiver.h"
#include "fboss/agent/TransceiverImpl.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/SfpModule.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/PortRemediator.h"
#include "common/stats/ServiceData.h"
#include <folly/FileUtil.h>
#include <folly/MacAddress.h>
#include <folly/String.h>
#include <folly/Demangle.h>
#include <chrono>
#include <condition_variable>
#include <glog/logging.h>

using folly::EventBase;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using folly::make_unique;
using folly::StringPiece;
using std::lock_guard;
using std::map;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

using namespace std::chrono;

DEFINE_string(config, "", "The path to the local JSON configuration file");

namespace {

std::string switchRunStateStr(
  facebook::fboss::SwSwitch::SwitchRunState runstate) {
  switch (runstate) {
    case facebook::fboss::SwSwitch::SwitchRunState::UNINITIALIZED:
      return "UNINITIALIZED";
    case facebook::fboss::SwSwitch::SwitchRunState::INITIALIZED:
      return "INITIALIZED";
    case facebook::fboss::SwSwitch::SwitchRunState::CONFIGURED:
      return "CONFIGURED";
    case facebook::fboss::SwSwitch::SwitchRunState::FIB_SYNCED:
      return "FIB_SYNCED";
    case facebook::fboss::SwSwitch::SwitchRunState::EXITING:
      return "EXITING";
    default:
      return "Unknown";
  }
}

facebook::fboss::PortStatus fillInPortStatus(
    const facebook::fboss::Port& port,
    const facebook::fboss::SwSwitch* sw) {
  facebook::fboss::PortStatus status;
  status.enabled = (port.getState() ==
      facebook::fboss::cfg::PortState::UP ? true : false);
  status.up = sw->isPortUp(port.getID());
  status.speedMbps = (int) port.getWorkingSpeed();

  try {
    auto tm = sw->getTransceiverMapping(port.getID());
    status.transceiverIdx.channelId = tm.first;
    status.transceiverIdx.__isset.channelId = true;
    status.transceiverIdx.transceiverId = tm.second;
    status.__isset.transceiverIdx = true;
    status.present = sw->getTransceiver(tm.second)->isPresent();
    status.__isset.present = true;
  } catch (const facebook::fboss::FbossError& err) {
    // No problem, we just don't set the other info
  }
  return status;
}

string getPortUpName(const shared_ptr<facebook::fboss::Port>& port) {
  return port->getName().empty()
    ? folly::to<string>("port", port->getID(), ".up")
    : port->getName() + ".up";
}

inline void updatePortStatusCounters(const facebook::fboss::StateDelta& delta) {
  facebook::fboss::DeltaFunctions::forEachChanged(
    delta.getPortsDelta(),
    [&] (const shared_ptr<facebook::fboss::Port>& oldPort,
         const shared_ptr<facebook::fboss::Port>& newPort) {
      if (oldPort->getName() == newPort->getName()) {
        return;
      }
      long val = facebook::fbData->getCounter(getPortUpName(oldPort));
      facebook::fbData->clearCounter(getPortUpName(oldPort));
      facebook::fbData->setCounter(getPortUpName(newPort), val);
    },
    [&] (const shared_ptr<facebook::fboss::Port>& newPort) {
      facebook::fbData->setCounter(getPortUpName(newPort), 0);
    },
    [&] (const shared_ptr<facebook::fboss::Port>& oldPort) {
      facebook::fbData->clearCounter(getPortUpName(oldPort));
    }
  );
}
}

namespace facebook { namespace fboss {

SwSwitch::SwSwitch(std::unique_ptr<Platform> platform)
  : hw_(platform->getHwSwitch()),
    platform_(std::move(platform)),
    portRemediator_(new PortRemediator(this)),
    arp_(new ArpHandler(this)),
    ipv4_(new IPv4Handler(this)),
    ipv6_(new IPv6Handler(this)),
    nUpdater_(new NeighborUpdater(this)),
    pcapMgr_(new PktCaptureManager(this)),
    routeUpdateLogger_(new RouteUpdateLogger(this)),
    transceiverMap_(new TransceiverMap()) {
  // Create the platform-specific state directories if they
  // don't exist already.
  utilCreateDir(platform_->getVolatileStateDir());
  utilCreateDir(platform_->getPersistentStateDir());
}

SwSwitch::~SwSwitch() {
  stop();
}

void SwSwitch::stop() {
  setSwitchRunState(SwitchRunState::EXITING);

  LOG(INFO) << "Stopping SwSwitch...";

  // First tell the hw to stop sending us events by unregistering the callback
  // After this we should no longer receive packets or link state changed events
  // while we are destroying ourselves
  hw_->unregisterCallbacks();

  // Several member variables are performing operations in the background
  // thread.  Ask them to stop, before we shut down the background thread.
  //
  // It should be safe to destroy a StateObserver object without locking
  // if they are only ever accessed from the update thread. The destructor
  // of a class that extends AutoRegisterStateObserver will unregister
  // itself on the update thread so there should be no race. Unfortunately,
  // this is not the case for ipv6_ and tunMgr_, which may be accessed in a
  // packet handling callback as well while stopping the switch.
  //
  // TODO(aeckert): t6862022 is there to come up with a more stable concurrency
  // model for classes that observe state and/or handle packets.
  portRemediator_.reset();
  ipv6_.reset();
  nUpdater_.reset();
  if (lldpManager_) {
    lldpManager_->stop();
  }
  if (unresolvedNhopsProber_) {
    unresolvedNhopsProber_.reset();
  }

  // Stop tunMgr so we don't get any packets to process
  // in software that were sent to the switch ip or were
  // routed from kernel to the front panel tunnel interface.
  tunMgr_.reset();

  routeUpdateLogger_.reset();
  // stops the background and update threads.
  stopThreads();
}

bool SwSwitch::isFullyInitialized() const {
  auto state = getSwitchRunState();
  return state >= SwitchRunState::INITIALIZED &&
    state != SwitchRunState::EXITING;
}

bool SwSwitch::isInitialized() const {
  return getSwitchRunState() >= SwitchRunState::INITIALIZED;
}

bool SwSwitch::isFullyConfigured() const {
  auto state = getSwitchRunState();
  return state >= SwitchRunState::CONFIGURED &&
    state != SwitchRunState::EXITING;
}

bool SwSwitch::isConfigured() const {
  return getSwitchRunState() >= SwitchRunState::CONFIGURED;
}

bool SwSwitch::isFibSynced() const {
  return getSwitchRunState() >= SwitchRunState::FIB_SYNCED;
}

bool SwSwitch::isExiting() const {
  return getSwitchRunState() >= SwitchRunState::EXITING;
}

void SwSwitch::setSwitchRunState(SwitchRunState runState) {
  auto oldState = runState_.exchange(runState, std::memory_order_acq_rel);
  CHECK(oldState <= runState);
  VLOG(2) << "SwitchRunState changed from " << switchRunStateStr(oldState) <<
    " to " << switchRunStateStr(runState);
}

SwSwitch::SwitchRunState SwSwitch::getSwitchRunState() const {
  return runState_.load(std::memory_order_acquire);
}

void SwSwitch::gracefulExit() {
  if (isFullyInitialized()) {
    ipv6_->floodNeighborAdvertisements();
    arp_->floodGratuituousArp();
    // Stop handlers and threads before uninitializing h/w
    stop();
    folly::dynamic switchState = folly::dynamic::object;
    switchState[kSwSwitch] =  getState()->toFollyDynamic();
    // Cleanup if we ever initialized
    hw_->gracefulExit(switchState);
  }
}

void SwSwitch::getProductInfo(ProductInfo& productInfo) const {
  platform_->getProductInfo(productInfo);
}

bool SwSwitch::isPortUp(PortID port) const {
   if (getState()->getPort(port)->getState() == cfg::PortState::UP) {
     return hw_->isPortUp(port);
   }
   // Port not enabled
   return false;
}

void SwSwitch::registerNeighborListener(
    std::function<void(const std::vector<std::string>& added,
                       const std::vector<std::string>& deleted)> callback) {
  VLOG(2) << "Registering neighbor listener";
  lock_guard<mutex> g(neighborListenerMutex_);
  neighborListener_ = std::move(callback);
}

void SwSwitch::invokeNeighborListener(
    const std::vector<std::string>& added,
    const std::vector<std::string>& removed) {
  lock_guard<mutex> g(neighborListenerMutex_);
  if (neighborListener_) {
    neighborListener_(added, removed);
  }
}

bool SwSwitch::getAndClearNeighborHit(RouterID vrf, folly::IPAddress ip) {
  return hw_->getAndClearNeighborHit(vrf, ip);
}

void SwSwitch::exitFatal() const noexcept {
  folly::dynamic switchState = folly::dynamic::object;
  switchState[kSwSwitch] =  getState()->toFollyDynamic();
  switchState[kHwSwitch] = hw_->toFollyDynamic();
  if (!dumpStateToFile(platform_->getCrashSwitchStateFile(), switchState)) {
    LOG(ERROR) << "Unable to write switch state JSON to file";
  }
}

void SwSwitch::clearWarmBootCache() {
  hw_->clearWarmBootCache();
}

void SwSwitch::init(SwitchFlags flags) {
  flags_ = flags;
  auto hwInitRet = hw_->init(this);
  auto initialState = hwInitRet.switchState;
  bootType_ = hwInitRet.bootType;

  VLOG(0) << "hardware initialized in " <<
    hwInitRet.bootTime << " seconds; applying initial config";

  for (const auto& port : (*initialState->getPorts())) {
    auto maxSpeed = getMaxPortSpeed(port->getID());
    fbData->setCounter(getPortUpName(port), 0);
    port->setMaxSpeed(maxSpeed);
  }

  // Store the initial state
  initialState->publish();
  setStateInternal(initialState);

  platform_->onHwInitialized(this);

  // Notify the state observers of the initial state
  updateEventBase_.runInEventBaseThread([initialState, this]() {
      notifyStateObservers(StateDelta(std::make_shared<SwitchState>(),
                                      initialState));
  });

  if (flags & SwitchFlags::ENABLE_TUN) {
    tunMgr_ = folly::make_unique<TunManager>(this, &backgroundEventBase_);
    tunMgr_->startProbe();
  }

  startThreads();

  // Publish timers after we aked TunManager to do a probe. This
  // is not required but since both stats publishing and tunnel
  // interface probing happens on backgroundEventBase_ its somewhat
  // nicer to have tun inteface probing finish faster since then
  // we don't have to wait for the initial probe to complete before
  // applying initial config.
  if (flags & SwitchFlags::PUBLISH_STATS) {
    publishSwitchInfo(hwInitRet);
  }

  if (flags & SwitchFlags::ENABLE_LLDP) {
      lldpManager_ = folly::make_unique<LldpManager>(this);
  }

  if (flags & SwitchFlags::ENABLE_NHOPS_PROBER) {
      unresolvedNhopsProber_ = folly::make_unique<UnresolvedNhopsProber>(this);
      // Feed initial state.
      unresolvedNhopsProber_->stateUpdated(
          StateDelta(std::make_shared<SwitchState>(), getState()));
  }
  setSwitchRunState(SwitchRunState::INITIALIZED);
}

void SwSwitch::initialConfigApplied(const steady_clock::time_point& startTime) {
  // notify the hw
  hw_->initialConfigApplied();
  setSwitchRunState(SwitchRunState::CONFIGURED);

  if (tunMgr_) {
    // Perform initial sync of interfaces
    tunMgr_->sync(getState());
    // We check for syncing tun interface only on state changes after the
    // initial configuration is applied. This is really a hack to get around
    // 2 issues
    // a) On warm boot the initial state constructed from warm boot cache
    // does not know of interface addresses. This means if we sync tun interface
    // on applying initial boot up state we would blow away tunnel interferace
    // addresses, causing connectivity disruption. Once t4155406 is fixed we
    // should be able to remove this check.
    // b) Even if we were willing to live with the above, the TunManager code
    // does not properly track deleting of interface addresses, viz. when we
    // delete a interface's primary address, secondary addresses get blown away
    // as well. TunManager does not track this and tries to delete the
    // secondaries as well leading to errors, t4746261 is tracking this.
    tunMgr_->startObservingUpdates();
  }

  if (lldpManager_) {
      lldpManager_->start();
  }

  if (flags_ & SwitchFlags::PUBLISH_STATS) {
    publishInitTimes("fboss.ctrl.switch_configured",
       duration_cast<duration<float>>(steady_clock::now() - startTime).count());
  }
}

void SwSwitch::logRouteUpdates(
    const std::string& addr,
    uint8_t mask,
    const std::string& identifier) {
  RouteUpdateLoggingInstance r{
      RoutePrefix<folly::IPAddress>{folly::IPAddress{addr}, mask},
      identifier,
      false};
  routeUpdateLogger_->startLoggingForPrefix(r);
}

void SwSwitch::fibSynced() {
  if (getBootType() == BootType::WARM_BOOT) {
    clearWarmBootCache();
    routeUpdateLogger_->stopLoggingForIdentifier("fboss-agent-warmboot");
  }
  setSwitchRunState(SwitchRunState::FIB_SYNCED);
}

void SwSwitch::registerStateObserver(StateObserver* observer,
                                     const string name) {
  VLOG(2) << "Registering state observer: " << name;
  if (!updateEventBase_.isInEventBaseThread()) {
    updateEventBase_.runInEventBaseThreadAndWait([=]() {
        addStateObserver(observer, name);
    });
  } else {
    addStateObserver(observer, name);
  }
}

void SwSwitch::unregisterStateObserver(StateObserver* observer) {
  if (!updateEventBase_.isInEventBaseThread()) {
    updateEventBase_.runInEventBaseThreadAndWait([=]() {
      removeStateObserver(observer);
    });
  } else {
    removeStateObserver(observer);
  }
}

bool SwSwitch::stateObserverRegistered(StateObserver* observer) {
  DCHECK(updateEventBase_.isInEventBaseThread());
  return stateObservers_.find(observer) != stateObservers_.end();
}

void SwSwitch::removeStateObserver(StateObserver* observer) {
  DCHECK(updateEventBase_.isInEventBaseThread());
  auto nErased = stateObservers_.erase(observer);
  if (!nErased) {
    throw FbossError("State observer remove failed: observer does not exist");
  }
}

void SwSwitch::addStateObserver(StateObserver* observer, const string& name) {
  DCHECK(updateEventBase_.isInEventBaseThread());
  if (stateObserverRegistered(observer)) {
    throw FbossError("State observer add failed: ", name, " already exists");
  }
  stateObservers_.emplace(observer, name);
}

void SwSwitch::notifyStateObservers(const StateDelta& delta) {
  CHECK(updateEventBase_.inRunningEventBaseThread());
  if (isExiting()) {
    // Make sure the SwSwitch is not already being destroyed
    return;
  }
  updatePortStatusCounters(delta);
  for (auto observerName : stateObservers_) {
    try {
      auto observer = observerName.first;
      observer->stateUpdated(delta);
    } catch (const std::exception& ex) {
    // TODO: Figure out the best way to handle errors here.
      LOG(FATAL) << "error notifying " << observerName.second << " of update: "
                 << folly::exceptionStr(ex);
    }
  }
}

void SwSwitch::updateState(unique_ptr<StateUpdate> update) {
  // Put the update function on the queue.
  {
    folly::SpinLockGuard guard(pendingUpdatesLock_);
    pendingUpdates_.push_back(*update.release());
  }

  // Signal the background thread that updates are pending.
  // (We're using backgroundEventBase_ for now because it is convenient.
  // If in the future we find that this is undesirable for some reason we could
  // re-organize which tasks get performed in which threads.)
  //
  // We call runInEventBaseThread() with a static function pointer since this
  // is more efficient than having to allocate a new bound function object.
  updateEventBase_.runInEventBaseThread(handlePendingUpdatesHelper, this);
}

void SwSwitch::updateState(StringPiece name, StateUpdateFn fn) {
  auto update = make_unique<FunctionStateUpdate>(name, std::move(fn));
  updateState(std::move(update));
}

void SwSwitch::updateStateNoCoalescing(StringPiece name, StateUpdateFn fn) {
  auto update = make_unique<FunctionStateUpdate>(name, std::move(fn), false);
  updateState(std::move(update));
}

void SwSwitch::updateStateBlocking(folly::StringPiece name, StateUpdateFn fn) {
  auto result = std::make_shared<BlockingUpdateResult>();
  auto update = make_unique<BlockingStateUpdate>(name, std::move(fn), result);
  updateState(std::move(update));
  result->wait();
}

void SwSwitch::handlePendingUpdatesHelper(SwSwitch* sw) {
  sw->handlePendingUpdates();
}

void SwSwitch::handlePendingUpdates() {
  // Get the list of updates to run.
  //
  // We might pull multiple updates off the list at once if several updates
  // were scheduled before we had a chance to process them.  In some cases we
  // might also end up finding 0 updates to process if a previous
  // handlePendingUpdates() call processed multiple updates.
  StateUpdateList updates;
  {
    folly::SpinLockGuard guard(pendingUpdatesLock_);

    // When deciding how many elements to pull off the pendingUpdates_
    // list, we pull as many as we can, while making sure we don't
    // include any updates after an update that does not allow
    // coalescing.
    auto iter = pendingUpdates_.begin();
    while (iter != pendingUpdates_.end()) {
      StateUpdate* update = &(*iter);
      ++iter;
      if (!update->allowsCoalescing()) {
        break;
      }
    }
    updates.splice(updates.begin(), pendingUpdates_,
                   pendingUpdates_.begin(), iter);
  }

  // handlePendingUpdates() is invoked once for each update, but a previous
  // call might have already processed everything.  If we don't have anything
  // to do just return early.
  if (updates.empty()) {
    return;
  }

  // This function should never be called with valid updates while we are
  // not initialized yet
  DCHECK(isInitialized());

  // Call all of the update functions to prepare the new SwitchState
  auto origState = getState();
  auto state = origState;
  auto iter = updates.begin();
  while (iter != updates.end()) {
    StateUpdate* update = &(*iter);
    ++iter;

    shared_ptr<SwitchState> newState;
    LOG(INFO) << "preparing state update " << update->getName();
    try {
      newState = update->applyUpdate(state);
    } catch (const std::exception& ex) {
      // Call the update's onError() function, and then immediately delete
      // it (therefore removing it from the intrusive list).  This way we won't
      // call it's onSuccess() function later.
      update->onError(ex);
      delete update;
    }
    if (newState) {
      // Call publish after applying each StateUpdate.  This guarantees that
      // the next StateUpdate function will have clone the SwitchState before
      // making any changes.  This ensures that if a StateUpdate function ever
      // fails partway through it can't have partially modified our existing
      // state, leaving it in an invalid state.
      newState->publish();
      state = newState;
    }
  }

  // Now apply the update and notify subscribers
  if (state != origState) {
    applyUpdate(origState, state);
  }

  // Notify all of the updates of success, and delete them
  while (!updates.empty()) {
    unique_ptr<StateUpdate> update(&updates.front());
    updates.pop_front();
    update->onSuccess();
  }
}

int SwSwitch::getHighresSamplers(HighresSamplerList* samplers,
                                 const std::set<string>& counters) {
  int numCountersAdded = 0;

  // Group the counters by namespace to make it easier to get samplers
  std::map<folly::StringPiece, std::set<folly::StringPiece>> groupedCounters;
  for (const auto& c : counters) {
    StringPiece namespaceName, counterName;
    if (folly::split("::", c, namespaceName, counterName)) {
      groupedCounters[namespaceName].insert(counterName);
    } else {
      LOG(ERROR) << "Bad counter: " << c;
    }
  }

  for (const auto& namespaceGroup : groupedCounters) {
    const auto& namespaceString = namespaceGroup.first;
    const auto& counterSet = namespaceGroup.second;
    unique_ptr<HighresSampler> sampler;

    // Check for cross-platform samplers
    if (namespaceString.compare(DumbCounterSampler::kIdentifier) == 0) {
      sampler = make_unique<DumbCounterSampler>(counterSet);
    } else if (namespaceString.compare(InterfaceRateSampler::kIdentifier) ==
               0) {
      sampler = make_unique<InterfaceRateSampler>(counterSet);
    }

    if (sampler) {
      if (sampler->numCounters() > 0) {
        numCountersAdded += sampler->numCounters();
        samplers->push_back(std::move(sampler));
      } else {
        LOG(WARNING) << "Cannot use " << namespaceString;
      }
    } else {
      // Otherwise, check if the hwSwitch knows which samplers to add
      auto n = hw_->getHighresSamplers(samplers, namespaceString, counterSet);
      if (n > 0) {
        numCountersAdded += n;
      } else {
        LOG(WARNING) << "No counters added for namespace: " << namespaceString;
      }
    }
  }

  return numCountersAdded;
}

void SwSwitch::setStateInternal(std::shared_ptr<SwitchState> newState) {
  // This is one of the only two places that should ever directly access
  // stateDontUseDirectly_.  (getState() being the other one.)
  CHECK(newState->isPublished());
  folly::SpinLockGuard guard(stateLock_);
  stateDontUseDirectly_.swap(newState);
}

void SwSwitch::applyUpdate(const shared_ptr<SwitchState>& oldState,
                           const shared_ptr<SwitchState>& newState) {
  DCHECK_EQ(oldState, getState());
  auto start = std::chrono::steady_clock::now();
  LOG(INFO) << "Updating state: old_gen=" << oldState->getGeneration() <<
    " new_gen=" << newState->getGeneration();
  DCHECK_GT(newState->getGeneration(), oldState->getGeneration());

  StateDelta delta(oldState, newState);

  // If we are already exiting, abort the update
  if (isExiting()) {
    return;
  }

  // Publish the configuration as our active state.
  setStateInternal(newState);

  // Inform the HwSwitch of the change.
  //
  // Note that at this point we have already updated the state pointer and
  // released stateLock_, so the new state is already published and visible to
  // other threads.  This does mean that there is a window where the new state
  // is visible but the hardware is not using the new configuration yet.
  //
  // We could avoid this by holding a lock and block anyone from reading the
  // state while we update the hardware.  However, updating the hardware may
  // take a non-trivial amount of time, and blocking other users seems
  // undesirable.  So far I don't think this brief discrepancy should cause
  // major issues.
  try {
    hw_->stateChanged(delta);
  } catch (const std::exception& ex) {
    // Notify the hw_ of the crash so it can execute any device specific
    // tasks before we fatal. An example would be to dump the current hw state.
    //
    // Another thing we could try here is rolling back to the old state.
    hw_->exitFatal();
    LOG(FATAL) << "error applying state change to hardware: " <<
      folly::exceptionStr(ex);
  }

  // Notifies all observers of the current state update.
  notifyStateObservers(delta);

  auto end = std::chrono::steady_clock::now();
  auto duration =
    std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  stats()->stateUpdate(duration);
  VLOG(0) << "Update state took " << duration.count() << "us";
}

PortStats* SwSwitch::portStats(PortID portID) {
  return stats()->port(portID);
}

PortStats* SwSwitch::portStats(const RxPacket* pkt) {
  return stats()->port(pkt->getSrcPort());
}

map<int32_t, PortStatus> SwSwitch::getPortStatus() {
  map<int32_t, PortStatus> statusMap;
  for (const auto& p : *getState()->getPorts()) {
    statusMap[p->getID()] = fillInPortStatus(*p, this);
  }
  return statusMap;
}

cfg::PortSpeed SwSwitch::getPortSpeed(PortID port) const {
  return hw_->getPortSpeed(port);
}

cfg::PortSpeed SwSwitch::getMaxPortSpeed(PortID port) const {
  return hw_->getMaxPortSpeed(port);
}

PortStatus SwSwitch::getPortStatus(PortID port) {
  return fillInPortStatus(*getState()->getPort(port), this);
}

TransceiverIdx SwSwitch::getTransceiverMapping(PortID portID) const {
  return transceiverMap_->transceiverMapping(portID);
}

Transceiver* SwSwitch::getTransceiver(TransceiverID id) const {
  return transceiverMap_->transceiver(id);
}

map<TransceiverID, TransceiverInfo> SwSwitch::getTransceiversInfo() const {
  map<TransceiverID, TransceiverInfo> infos;
  int i = -1;
  for (const auto& it : *transceiverMap_) {
    TransceiverInfo info;
    it.second->getTransceiverInfo(info);
    infos[it.first] = info;
  }
  return infos;
}

TransceiverInfo SwSwitch::getTransceiverInfo(TransceiverID idx) const {
  TransceiverInfo info;
  Transceiver *t = getTransceiver(idx);
  if (!t) {
    throw FbossError("no such Transceiver ID", idx);
  }
  t->getTransceiverInfo(info);
  return info;
}

map<int32_t, SfpDom> SwSwitch::getSfpDoms() const {
  map<int32_t, SfpDom> domInfos;
  for (const auto& it : *transceiverMap_) {
    if (it.second->type() == TransceiverType::SFP) {
      SfpDom domInfo;
      it.second->getSfpDom(domInfo);
      domInfos[it.first] = domInfo;
    }
  }
  return domInfos;
}

// TODO(7154694):  Remove getSfpDom() support once getTranceiverInfo()
// is supported everywhere.

SfpDom SwSwitch::getSfpDom(PortID port) const {
  TransceiverIdx idx = getTransceiverMapping(port);
  SfpDom domInfo;
  Transceiver *t = getTransceiver(idx.second);
  if (t->type() != TransceiverType::SFP) {
    throw FbossError("Transceiver not SFP");
  }
  t->getSfpDom(domInfo);
  return domInfo;
}

void SwSwitch::addTransceiver(TransceiverID idx,
                              std::unique_ptr<Transceiver> trans) {
  transceiverMap_->addTransceiver(idx, std::move(trans));
}

void SwSwitch::addTransceiverMapping(PortID portID, ChannelID channelID,
                                     TransceiverID transceiverID) {
  transceiverMap_->addTransceiverMapping(portID, channelID, transceiverID);
}

void SwSwitch::detectTransceiver() {
  for (const auto& t : *transceiverMap_) {
    t.second.get()->detectTransceiver();
  }
}

void SwSwitch::updateTransceiverInfoFields() {
  for (const auto& t : *transceiverMap_) {
    t.second.get()->updateTransceiverInfoFields();
  }
}

SwitchStats* SwSwitch::createSwitchStats() {
  SwitchStats* s = new SwitchStats();
  stats_.reset(s);
  return s;
}

void SwSwitch::setPortStatusCounter(PortID port, bool up) {
  auto state = getState();
  if (!state) {
    // Make sure the state actually exists, this could be an issue if
    // called during initialization
    return;
  }
  fbData->setCounter(getPortUpName(state->getPort(port)), int(up));
}

void SwSwitch::packetReceived(std::unique_ptr<RxPacket> pkt) noexcept {
  PortID port = pkt->getSrcPort();
  try {
    handlePacket(std::move(pkt));
  } catch (const std::exception& ex) {
    stats()->port(port)->pktError();
    LOG(ERROR) << "error processing trapped packet: " <<
      folly::exceptionStr(ex);
    // Return normally, without letting the exception propagate to our caller.
    return;
  }
}

void SwSwitch::packetReceivedThrowExceptionOnError(
    std::unique_ptr<RxPacket> pkt) {
  handlePacket(std::move(pkt));
}

void SwSwitch::handlePacket(std::unique_ptr<RxPacket> pkt) {
  // If we are not fully initialized or are already exiting, don't handle
  // packets since the individual handlers, h/w sdk data structures
  // may not be ready or may already be (partially) destroyed
  if (!isFullyInitialized()) {
    return;
  }
  PortID port = pkt->getSrcPort();
  stats()->port(port)->trappedPkt();

  pcapMgr_->packetReceived(pkt.get());

  // The minimum required frame length for ethernet is 64 bytes.
  // Abort processing early if the packet is too short.
  auto len = pkt->getLength();
  if (len < 64) {
    stats()->port(port)->pktBogus();
    return;
  }

  // Parse the source and destination MAC, as well as the ethertype.
  Cursor c(pkt->buf());
  auto dstMac = PktUtil::readMac(&c);
  auto srcMac = PktUtil::readMac(&c);
  auto ethertype = c.readBE<uint16_t>();
  if (ethertype == 0x8100) {
    // 802.1Q
    c += 2; // Advance over the VLAN tag.  We ignore it for now
    ethertype = c.readBE<uint16_t>();
  }

  VLOG(5) << "trapped packet: src_port=" << pkt->getSrcPort() <<
    " vlan=" << pkt->getSrcVlan() <<
    " length=" << len <<
    " src=" << srcMac <<
    " dst=" << dstMac <<
    " ethertype=0x" << std::hex << ethertype <<
    " :: " << pkt->describeDetails();

  switch (ethertype) {
  case ArpHandler::ETHERTYPE_ARP:
    arp_->handlePacket(std::move(pkt), dstMac, srcMac, c);
    return;
  case LldpManager::ETHERTYPE_LLDP:
    if (lldpManager_) {
      lldpManager_->handlePacket(std::move(pkt), dstMac, srcMac, c);
      return;
    }
    break;
  case IPv4Handler::ETHERTYPE_IPV4:
    ipv4_->handlePacket(std::move(pkt), dstMac, srcMac, c);
    return;
  case IPv6Handler::ETHERTYPE_IPV6:
    ipv6_->handlePacket(std::move(pkt), dstMac, srcMac, c);
    return;
  default:
    break;
  }

  // If we are still here, we don't know what to do with this packet.
  // Increment a counter and just drop the packet on the floor.
  stats()->port(port)->pktUnhandled();
}

void SwSwitch::linkStateChanged(PortID portId, bool up) {
  LOG(INFO) << "link state changed: " << portId << " enabled = " << up;
  if (isFullyInitialized() && !up) {
    logLinkStateEvent(portId, up);
    setPortStatusCounter(portId, up);
    stats()->port(portId)->linkStateChange();
    backgroundEventBase_.runInEventBaseThread(
        [this, portId]() { nUpdater_->portDown(portId); });
  }
}

void SwSwitch::startThreads() {
  backgroundThread_.reset(new std::thread([=] {
      this->threadLoop("fbossBgThread", &backgroundEventBase_); }));
  updateThread_.reset(new std::thread([=] {
      this->threadLoop("fbossUpdateThread", &updateEventBase_); }));
}

void SwSwitch::stopThreads() {
  // We use runInEventBaseThread() to terminateLoopSoon() rather than calling it
  // directly here.  This ensures that any events already scheduled via
  // runInEventBaseThread() will have a chance to run.
  //
  // Alternatively, it would be nicer to update EventBase so it can notify
  // callbacks when the event loop is being stopped.
  if (backgroundThread_) {
    backgroundEventBase_.runInEventBaseThread(
        [this] { backgroundEventBase_.terminateLoopSoon(); });
  }
  if (updateThread_) {
    updateEventBase_.runInEventBaseThread(
        [this] { updateEventBase_.terminateLoopSoon(); });
  }
  if (backgroundThread_) {
    backgroundThread_->join();
  }
  if (updateThread_) {
    updateThread_->join();
  }
}

void SwSwitch::threadLoop(StringPiece name, EventBase* eventBase) {
  initThread(name);
  eventBase->loopForever();
}

std::unique_ptr<TxPacket> SwSwitch::allocatePacket(uint32_t size) {
  return hw_->allocatePacket(size);
}

std::unique_ptr<TxPacket> SwSwitch::allocateL3TxPacket(uint32_t l3Len) {
  const uint32_t l2Len = EthHdr::SIZE;
  const uint32_t minLen = 68;
  auto len = std::max(l2Len + l3Len, minLen);
  auto pkt = hw_->allocatePacket(len);
  auto buf = pkt->buf();
  // make sure the whole buffer is available
  buf->clear();
  // reserve for l2 header
  buf->advance(l2Len);
  return pkt;
}

void SwSwitch::sendPacketOutOfPort(std::unique_ptr<TxPacket> pkt,
                                   PortID portID) noexcept {
  pcapMgr_->packetSent(pkt.get());
  if (!hw_->sendPacketOutOfPort(std::move(pkt), portID)) {
    // Just log an error for now.  There's not much the caller can do about
    // send failures--even on successful return from sendPacket*() the
    // send may ultimately fail since it occurs asynchronously in the
    // background.
    LOG(ERROR) << "failed to send packet out port " << portID;
  }
}

void SwSwitch::sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept {
  pcapMgr_->packetSent(pkt.get());
  if (!hw_->sendPacketSwitched(std::move(pkt))) {
    // Just log an error for now.  There's not much the caller can do about
    // send failures--even on successful return from sendPacketSwitched() the
    // send may ultimately fail since it occurs asynchronously in the
    // background.
    LOG(ERROR) << "failed to send L2 switched packet";
  }
}

void SwSwitch::sendL3Packet(
    RouterID rid, std::unique_ptr<TxPacket> pkt) noexcept {
  folly::IOBuf *buf = pkt->buf();
  CHECK(!buf->isShared());
  // make sure the packet has enough headroom for L2 header and large enough
  // for the minimum size packet.
  const uint32_t l2Len = EthHdr::SIZE;
  const uint32_t l3Len = buf->length();
  const uint32_t minLen = 68;
  uint32_t tailRoom = (l2Len + l3Len >= minLen) ? 0 : minLen - l2Len - l3Len;
  if (buf->headroom() < l2Len || buf->tailroom() < tailRoom) {
    LOG(ERROR) << "Packet is not big enough headroom=" << buf->headroom()
               << " required=" << l2Len
               << ", tailroom=" << buf->tailroom()
               << " required=" << tailRoom;
    return;
  }
  uint16_t protocol;
  // we just need to read the first byte so that we know if it is v4 or v6
  try {
    RWPrivateCursor cursor(buf);
    uint8_t version = cursor.read<uint8_t>() >> 4;
    if (version == 4) {
      protocol = IPv4Handler::ETHERTYPE_IPV4;
    } else if (version == 6) {
      protocol = IPv6Handler::ETHERTYPE_IPV6;
    } else {
      throw FbossError("Wrong version number ", static_cast<int>(version),
                       " in the L3 packet to sent");
    }
    // TODO: Use the default platform MAC for now. That's the MAC used currently
    // by default on interfaces and gets programmed in HW. That MAC will
    // trigger L3 processing on the packet so that the correct source and
    // destination MAC will be set correctly by HW.
    // If a new HW does not use the same mechanism to trigger L3 processing, we
    // will need to revisit this code. In the worst case, we will do SW l3
    // lookup to find out the L2 info.
    folly::MacAddress cpuMac = platform_->getLocalMac();
    buf->prepend(l2Len);
    cursor.reset(buf);
    TxPacket::writeEthHeader(&cursor, cpuMac, cpuMac, getCPUVlan(), protocol);
    if (tailRoom) {
      // padding with 0
      memset(buf->writableTail(), 0, tailRoom);
      buf->append(tailRoom);
    }
    // We can look up the vlan to make sure it exists. However, the vlan can
    // be just deleted after the lookup. So, in order to do it correctly, we
    // will need to lock the state update. Even with that, this still cannot
    // guarantee the vlan we are looking here is the vlan when the packet is
    // originated from the host. Because of that, we are just going to send
    // the packet out to the HW. The HW will drop the packet if the vlan is
    // deleted.
    pcapMgr_->packetSent(pkt.get());
    hw_->sendPacketSwitched(std::move(pkt));
    stats()->pktFromHost(l3Len);
  } catch (const std::exception& ex) {
    LOG(ERROR) << "Failed to send out L3 packet :"
               << folly::exceptionStr(ex);
  }
}

bool SwSwitch::sendPacketToHost(std::unique_ptr<RxPacket> pkt) {
  if (tunMgr_) {
    return tunMgr_->sendPacketToHost(std::move(pkt));
  } else {
    return false;
  }
}

void SwSwitch::applyConfig(const std::string& reason) {
  // We don't need to hold a lock here. updateStateBlocking() does that for us.
  updateStateBlocking(
      reason,
      [&](const shared_ptr<SwitchState>& state) -> shared_ptr<SwitchState> {
        std::string configFilename = FLAGS_config;
        std::pair<shared_ptr<SwitchState>, std::string> rval;
        if (!configFilename.empty()) {
          LOG(INFO) << "Loading config from local config file "
                    << configFilename;
          rval = applyThriftConfigFile(state, configFilename, platform_.get(),
              &curConfig_);
        } else {
          // Loading config from default location. The message will be printed
          // there.
          rval = applyThriftConfigDefault(state, platform_.get(),
              &curConfig_);
        }
        if (!isValidStateUpdate(StateDelta(state, rval.first))) {
          throw FbossError("Invalid config passed in, skipping");
        }
        curConfigStr_ = rval.second;
        curConfig_.readFromJson(curConfigStr_.c_str());
        return rval.first;
      });
}

bool SwSwitch::isValidStateUpdate(
    const StateDelta& delta) const {
  return hw_->isValidStateUpdate(delta);
}

}} // facebook::fboss
