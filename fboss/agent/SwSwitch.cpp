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

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>

#include "common/stats/ServiceData.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/IPv4Handler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/RouteUpdateLogger.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/UnresolvedNhopsProber.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/PortUpdateHandler.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/capture/PktCaptureManager.h"
#include "fboss/pcap_distribution_service/if/gen-cpp2/PcapPushSubscriber.h"
#include "fboss/pcap_distribution_service/if/gen-cpp2/pcap_pubsub_constants.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/StateUpdateHelpers.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/PortRemediator.h"
#include "fboss/agent/gen-cpp2/switch_config_types_custom_protocol.h"
#include "fboss/agent/ThriftHandler.h"
#include <folly/FileUtil.h>
#include <folly/MacAddress.h>
#include <folly/MapUtil.h>
#include <folly/String.h>
#include <folly/Logging.h>
#include <folly/SocketAddress.h>
#include <folly/system/ThreadName.h>
#include <folly/Demangle.h>
#include <chrono>
#include <exception>
#include <condition_variable>
#include <glog/logging.h>
#include <tuple>

using folly::EventBase;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::make_unique;
using folly::StringPiece;
using folly::SocketAddress;
using std::lock_guard;
using std::map;
using std::mutex;
using std::set;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::exception;

using namespace std::chrono;
using namespace apache::thrift;
using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;
using namespace apache::thrift::async;


DEFINE_string(config, "", "The path to the local JSON configuration file");
DEFINE_int32(thread_heartbeat_ms, 1000, "Thread heartbeat interval (ms)");
DEFINE_int32(
    distribution_timeout_ms,
    1000,
    "Timeout for sending to distribution_service (ms)");

namespace {

constexpr auto kOutOfSyncStateUpdate =
    "state update for failed hardware application";

/**
 * Transforms the IPAddressV6 to MacAddress. RFC 2464
 * 33:33:xx:xx:xx:xx (lower 32 bits are copied from addr)
 */
folly::MacAddress getMulticastMacAddr(folly::IPAddressV6 addr) {
  return folly::MacAddress::createMulticast(addr);
}

/**
 * Transforms IPAddressV4 to MacAddress. RFC 1112
 * 01:00:5E:0x:xx:xx - 01:00:5E:7x:xx:xx (lower 23 bits are copied from addr)
 */
folly::MacAddress getMulticastMacAddr(folly::IPAddressV4 addr) {
  std::array<uint8_t, 6> bytes = {{0x01, 0x00, 0x5E, 0x00, 0x00, 0x00}};
  auto addrBytes = addr.toBinary();
  bytes[3] = addrBytes[1] & 0x7f;   // Take only 7 bits
  bytes[4] = addrBytes[2];
  bytes[5] = addrBytes[3];
  return folly::MacAddress::fromBinary(folly::ByteRange(
      bytes.begin(), bytes.end()));
}

facebook::fboss::PortStatus fillInPortStatus(
    const facebook::fboss::Port& port,
    const facebook::fboss::SwSwitch* sw) {
  facebook::fboss::PortStatus status;
  status.enabled = port.isEnabled();
  status.up = port.isUp();
  status.speedMbps = static_cast<int>(port.getSpeed());

  try {
    status.transceiverIdx = sw->getPlatform()->getPortMapping(port.getID());
    status.__isset.transceiverIdx =
      status.transceiverIdx.__isset.transceiverId;
  } catch (const facebook::fboss::FbossError& err) {
    // No problem, we just don't set the other info
  }
  return status;
}
} // anonymous namespace

namespace facebook { namespace fboss {

class ChannelCloser : public CloseCallback {
 public:
  explicit ChannelCloser(SwSwitch* s) : s_(s) {}

  void channelClosed() override {
    CHECK(s_);
    s_->destroyPushClient();
  }

 private:
  SwSwitch* s_ = nullptr;
};

SwSwitch::SwSwitch(std::unique_ptr<Platform> platform)
  : hw_(platform->getHwSwitch()),
    platform_(std::move(platform)),
    portRemediator_(new PortRemediator(this)),
    closer_(new ChannelCloser(this)),
    arp_(new ArpHandler(this)),
    ipv4_(new IPv4Handler(this)),
    ipv6_(new IPv6Handler(this)),
    nUpdater_(new NeighborUpdater(this)),
    pcapMgr_(new PktCaptureManager(this)),
    routeUpdateLogger_(new RouteUpdateLogger(this)),
    portUpdateHandler_(new PortUpdateHandler(this)) {
  // Create the platform-specific state directories if they
  // don't exist already.
  utilCreateDir(platform_->getVolatileStateDir());
  utilCreateDir(platform_->getPersistentStateDir());

  // doesnt need to be guarded, only accessed by 1 event base
  pcapPusher_ = nullptr;
}


void SwSwitch::destroyPushClient(){
  distributionServiceReady_.store(false);
}

void SwSwitch::constructPushClient(uint16_t port) {
  auto creation = [&, port]() {
    SocketAddress addr("::1", port);
    auto socket =
        TAsyncSocket::newSocket(&pcapDistributionEventBase_, addr, 2000);
    auto chan = HeaderClientChannel::newChannel(socket);
    chan->setTimeout(FLAGS_distribution_timeout_ms);
    chan->setCloseCallback(closer_.get());
    pcapPusher_ =
        std::make_unique<PcapPushSubscriberAsyncClient>(std::move(chan));
    distributionServiceReady_.store(true);
  };
  pcapDistributionEventBase_.runInEventBaseThread(creation);
}

void SwSwitch::killDistributionProcess(){
  pcapPusher_->future_kill();
  LOG(INFO) << "KILLING DISTRIBUTION PROCESS FROM AGENT";
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

  // Stop tunMgr so we don't get any packets to process
  // in software that were sent to the switch ip or were
  // routed from kernel to the front panel tunnel interface.
  tunMgr_.reset();

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
  portRemediator_.reset();

  nUpdater_.reset();

  if (lldpManager_) {
    lldpManager_->stop();
  }

  if (lagManager_) {
    lagManager_.reset();
  }

  if (unresolvedNhopsProber_) {
    unresolvedNhopsProber_.reset();
  }

  // Need to destroy IPv6Handler as it is a state observer,
  // but we must do it after we've stopped TunManager.
  // Otherwise, we might attempt to call sendL3Packet which
  // calls ipv6_->sendNeighborSolicitaion which will then segfault
  ipv6_.reset();

  routeUpdateLogger_.reset();

  bgThreadHeartbeat_.reset();
  updThreadHeartbeat_.reset();
  packetTxThreadHeartbeat_.reset();
  lacpThreadHeartbeat_.reset();

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
  logSwitchRunStateChange(oldState, runState);
}

SwSwitch::SwitchRunState SwSwitch::getSwitchRunState() const {
  return runState_.load(std::memory_order_acquire);
}

void SwSwitch::gracefulExit() {
  if (isFullyInitialized()) {
    steady_clock::time_point begin = steady_clock::now();
    LOG(INFO) << "[Exit] Starting SwSwitch graceful exit";
    ipv6_->floodNeighborAdvertisements();
    arp_->floodGratuituousArp();
    steady_clock::time_point neighborFloodDone = steady_clock::now();
    LOG(INFO)
        << "[Exit] Neighbor flood time "
        << duration_cast<duration<float>>(neighborFloodDone - begin).count();
    // Stop handlers and threads before uninitializing h/w
    stop();
    steady_clock::time_point stopThreadsAndHandlersDone = steady_clock::now();
    LOG(INFO) << "[Exit] Stop thread and handlers time "
              << duration_cast<duration<float>>(
                     stopThreadsAndHandlersDone - neighborFloodDone)
                     .count();

    folly::dynamic switchState = folly::dynamic::object;
    // TODO - Serialize both desired and applied state to
    // file. Right now we just serialize applied state and
    // then rely on a route/FIB sync on warm boot to recover
    // desired state.
    switchState[kSwSwitch] = getAppliedState()->toFollyDynamic();
    steady_clock::time_point switchStateToFollyDone = steady_clock::now();
    LOG(INFO) << "[Exit] Switch state to folly dynamic "
              << duration_cast<duration<float>>(
                     switchStateToFollyDone - stopThreadsAndHandlersDone)
                     .count();
    // Cleanup if we ever initialized
    hw_->gracefulExit(switchState);
    LOG(INFO)
        << "[Exit] SwSwitch Graceful Exit time "
        << duration_cast<duration<float>>(steady_clock::now() - begin).count();
  }
}

void SwSwitch::getProductInfo(ProductInfo& productInfo) const {
  platform_->getProductInfo(productInfo);
}
void SwSwitch::updateStats(){
  try {
    getHw()->updateStats(stats());
  } catch (const std::exception& ex) {
    stats()->updateStatsException();
    LOG(ERROR) << "Error running updateStats: "
      << folly::exceptionStr(ex);
  }
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
  switchState[kSwSwitch] =  getAppliedState()->toFollyDynamic();
  switchState[kHwSwitch] = hw_->toFollyDynamic();
  if (!dumpStateToFile(platform_->getCrashSwitchStateFile(), switchState)) {
    LOG(ERROR) << "Unable to write switch state JSON to file";
  }
}

void SwSwitch::clearWarmBootCache() {
  hw_->clearWarmBootCache();
}

void SwSwitch::publishRxPacket(RxPacket* pkt, uint16_t ethertype){
  RxPacketData pubPkt;
  pubPkt.srcPort = pkt->getSrcPort();
  pubPkt.srcVlan = pkt->getSrcVlan();

  for (const auto& r : pkt->getReasons()) {
    RxReason reason;
    reason.bytes = r.bytes;
    reason.description = r.description;
    pubPkt.reasons.push_back(reason);
  }

  folly::IOBuf buf_copy;
  pkt->buf()->cloneInto(buf_copy);
  pubPkt.packetData = buf_copy.moveToFbString();
  auto onError = [&](std::runtime_error& /* unused */) {
    stats()->pcapDistFailure();
    FB_LOG_EVERY_MS(ERROR, 1000)
        << "Unable to push packet to distribution service\n";
  };
  pcapPusher_->future_receiveRxPacket(pubPkt, ethertype)
      .onError(std::move(onError));
}

void SwSwitch::publishTxPacket(TxPacket* pkt, uint16_t ethertype){
  TxPacketData pubPkt;
  folly::IOBuf copy_buf;
  pkt->buf()->cloneInto(copy_buf);
  pubPkt.packetData = copy_buf.moveToFbString();
  auto onError = [&](std::runtime_error& /* unused */) {
    stats()->pcapDistFailure();
    FB_LOG_EVERY_MS(ERROR, 1000)
        << "Unable to push packet to distribution service\n";
  };
  pcapPusher_->future_receiveTxPacket(pubPkt, ethertype)
      .onError(std::move(onError));
}

void SwSwitch::init(std::unique_ptr<TunManager> tunMgr, SwitchFlags flags) {
  auto begin = steady_clock::now();
  flags_ = flags;
  auto hwInitRet = hw_->init(this);
  auto initialState = hwInitRet.switchState;
  // for now, warmboot is not keeping failed routes, so keep the same state as
  // applied and desired.
  auto initialStateDesired = hwInitRet.switchState;
  bootType_ = hwInitRet.bootType;

  VLOG(0) << "hardware initialized in " <<
    hwInitRet.bootTime << " seconds; applying initial config";

  // Store the initial state
  initialState->publish();
  initialStateDesired->publish();
  setStateInternal(initialState, initialStateDesired);

  platform_->onHwInitialized(this);

  // Notify the state observers of the initial state
  updateEventBase_.runInEventBaseThread([initialStateDesired, this]() {
    notifyStateObservers(
        StateDelta(std::make_shared<SwitchState>(), initialStateDesired));
  });

  // In ALPM mode we need to make sure that the first route added is
  // the default route and that the route table always contains a default
  // route
  RouteUpdater updater(initialStateDesired->getRouteTables());
  updater.addRoute(RouterID(0), folly::IPAddressV4("0.0.0.0"), 0,
      StdClientIds2ClientID(StdClientIds::STATIC_ROUTE),
      RouteNextHopEntry(RouteForwardAction::DROP,
        AdminDistance::MAX_ADMIN_DISTANCE));
  updater.addRoute(RouterID(0), folly::IPAddressV6("::"), 0,
      StdClientIds2ClientID(StdClientIds::STATIC_ROUTE),
      RouteNextHopEntry(RouteForwardAction::DROP,
        AdminDistance::MAX_ADMIN_DISTANCE));
  auto newRt = updater.updateDone();
  if (newRt) {
    // If null default routes from above got added,
    // send a state update to h/w
    auto newState = initialStateDesired->clone();
    newState->resetRouteTables(std::move(newRt));
    newState->publish();

    updateEventBase_.runInEventBaseThread(
        [newState, initialStateDesired, this]() {
          applyUpdate(initialStateDesired, newState);
        });
  }

  if (flags & SwitchFlags::ENABLE_TUN) {
    if (tunMgr) {
      tunMgr_ = std::move(tunMgr);
    } else {
      tunMgr_ = std::make_unique<TunManager>(this, &packetTxEventBase_);
    }
  }

  startThreads();
  LOG(INFO)
      << "Time to init switch and start all threads "
      << duration_cast<duration<float>>(steady_clock::now() - begin).count();

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
    lldpManager_ = std::make_unique<LldpManager>(this);
  }

  if (flags & SwitchFlags::ENABLE_NHOPS_PROBER) {
    unresolvedNhopsProber_ = std::make_unique<UnresolvedNhopsProber>(this);
    // Feed initial state.
    unresolvedNhopsProber_->stateUpdated(
        StateDelta(std::make_shared<SwitchState>(), getState()));
  }

  if (flags & SwitchFlags::ENABLE_LACP) {
    lagManager_ = std::make_unique<LinkAggregationManager>(this);
  }

  auto bgHeartbeatStatsFunc = [this] (int delay, int backLog) {
    stats()->bgHeartbeatDelay(delay);
    stats()->bgEventBacklog(backLog);
  };
  bgThreadHeartbeat_ = std::make_unique<ThreadHeartbeat>(
    &backgroundEventBase_, "fbossBgThread", FLAGS_thread_heartbeat_ms,
    bgHeartbeatStatsFunc);

  auto updHeartbeatStatsFunc = [this] (int delay, int backLog) {
    stats()->updHeartbeatDelay(delay);
    stats()->updEventBacklog(backLog);
  };
  updThreadHeartbeat_ = std::make_unique<ThreadHeartbeat>(
    &updateEventBase_, "fbossUpdateThread", FLAGS_thread_heartbeat_ms,
    updHeartbeatStatsFunc);

  auto packetTxHeartbeatStatsFunc = [this](int delay, int backLog) {
    stats()->packetTxHeartbeatDelay(delay);
    stats()->packetTxEventBacklog(backLog);
  };
  packetTxThreadHeartbeat_ = std::make_unique<ThreadHeartbeat>(
      &packetTxEventBase_,
      "fbossPktTxThread",
      FLAGS_thread_heartbeat_ms,
      packetTxHeartbeatStatsFunc);

  auto updateLacpThreadHeartbeatStats = [this](int delay, int backLog) {
        stats()->lacpHeartbeatDelay(delay);
        stats()->lacpEventBacklog(backLog);
  };
  lacpThreadHeartbeat_ = std::make_unique<ThreadHeartbeat>(
      &lacpEventBase_,
      *folly::getThreadName(lacpThread_->get_id()),
      FLAGS_thread_heartbeat_ms,
      updateLacpThreadHeartbeatStats);

  portRemediator_->init();

  setSwitchRunState(SwitchRunState::INITIALIZED);

  constructPushClient(pcap_pubsub_constants::PCAP_PUBSUB_PORT());
}

void SwSwitch::initialConfigApplied(const steady_clock::time_point& startTime) {
  // notify the hw
  hw_->initialConfigApplied();
  setSwitchRunState(SwitchRunState::CONFIGURED);

  if (tunMgr_) {
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

    // Perform initial sync of interfaces
    tunMgr_->forceInitialSync();

  }

  if (lldpManager_) {
    lldpManager_->start();
  }

  if (flags_ & SwitchFlags::PUBLISH_STATS) {
    publishInitTimes("fboss.agent.switch_configured",
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
  updateEventBase_.runImmediatelyOrRunInEventBaseThreadAndWait([=]() {
      addStateObserver(observer, name);
  });
}

void SwSwitch::unregisterStateObserver(StateObserver* observer) {
  updateEventBase_.runImmediatelyOrRunInEventBaseThreadAndWait([=]() {
    removeStateObserver(observer);
  });
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

void SwSwitch::updateState(
    unique_ptr<StateUpdate> update) {
  {
    folly::SpinLockGuard guard(pendingUpdatesLock_);
    pendingUpdates_.push_back(*update.release());
  }

  // Signal the update thread that updates are pending.
  // We call runInEventBaseThread() with a static function pointer since this
  // is more efficient than having to allocate a new bound function object.
  updateEventBase_.runInEventBaseThread(handlePendingUpdatesHelper, this);
}

void SwSwitch::queueStateUpdateForGettingHwInSync(
    StringPiece name,
    StateUpdateFn fn) {
  auto update = make_unique<FunctionStateUpdate>(name, std::move(fn));
  {
    // Push the state update in front to preserver ordering.
    // This is not particularly necessary, since this state
    // update is freely coalesced with other state updates when
    // we come to processing pending updates
    folly::SpinLockGuard guard(pendingUpdatesLock_);
    pendingUpdates_.push_front(*update.release());
  }
  // Don't inform updateEventBase about this update being queued.
  // Rather let this update be processed with the next incoming update.
  // This laziness avoids busy loop that ensue otherwise - since nothing
  // in the h/w changed, our attempt to sync h/w is likely to fail again,
  // which would result in another h/w out of sync update to be queued and
  // so on. Instead we just wait next state update, this way we stand
  // a chance for that update to make changes to h/w to allow this update to
  // go through. Simplest example of this is when route addition fails due
  // to tables being full. Space for these routes is likely to be made only
  // when some routes get deleted (ignoring platform dependent h/w
  // optimizations).
}

void SwSwitch::updateState(
    StringPiece name,
    StateUpdateFn fn) {
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

  if (updates.size() == 1 &&
      updates.begin()->getName() == kOutOfSyncStateUpdate) {
    return;
  }

  // This function should never be called with valid updates while we are
  // not initialized yet
  DCHECK(isInitialized());

  std::shared_ptr<SwitchState> oldAppliedState;
  std::shared_ptr<SwitchState> oldDesiredState;
  // Call all of the update functions to prepare the new SwitchState
  std::tie(oldAppliedState, oldDesiredState) = getStates();
  bool oldOutOfSync = (oldAppliedState != oldDesiredState);
  // We start with the old applied state, and apply state updates one at a
  // time. The first state update applied is one from oldAppliedState ->
  // oldDesiredState. This is the one we always enqueue at the front of the
  // queue whenever applied and desired states diverge. After that, other
  // supplied state updates are applied (that were spliced above).
  auto newDesiredState = oldAppliedState;
  auto iter = updates.begin();
  while (iter != updates.end()) {
    StateUpdate* update = &(*iter);
    ++iter;

    shared_ptr<SwitchState> intermediateState;
    LOG(INFO) << "preparing state update " << update->getName();
    try {
      intermediateState = update->applyUpdate(newDesiredState);
    } catch (const std::exception& ex) {
      // Call the update's onError() function, and then immediately delete
      // it (therefore removing it from the intrusive list).  This way we won't
      // call it's onSuccess() function later.
      update->onError(ex);
      delete update;
    }
    // We have applied the update to software switch state, so call success
    // on the update.
    if (intermediateState) {
      // Call publish after applying each StateUpdate.  This guarantees that
      // the next StateUpdate function will have clone the SwitchState before
      // making any changes.  This ensures that if a StateUpdate function
      // ever fails partway through it can't have partially modified our
      // existing state, leaving it in an invalid state.
      intermediateState->publish();
      newDesiredState = intermediateState;
    }
  }

  // Now apply the update and notify subscribers
  if (newDesiredState != oldAppliedState) {
    // There was some change during these state updates
    auto newAppliedState = applyUpdate(oldAppliedState, newDesiredState);
    // Stick the initial applied->desired in the beginning
    bool newOutOfSync = (newAppliedState != newDesiredState);
    if (newOutOfSync) {
      // If we could not apply the whole delta successfully, put the difference
      // as a state update at the beginning
      queueStateUpdateForGettingHwInSync(
          kOutOfSyncStateUpdate,
          [newDesiredState, newAppliedState](
              const std::shared_ptr<SwitchState>& /*oldState*/) {
            // clone the newDesiredState and then inheritGeneration from
            // newAppliedState otherwise the return state has a smaller gen#
            // than the one of appliedState
            auto hwOutOfSyncState = newDesiredState->clone();
            hwOutOfSyncState->inheritGeneration(*newAppliedState);
            return hwOutOfSyncState;
          });
      if (!isExiting() && !oldOutOfSync) {
        stats()->setHwOutOfSync();
      }
    } else {
      if (!isExiting() && oldOutOfSync) {
        stats()->clearHwOutOfSync();
      }
    }
  }

  // Notify all of the updates of success, and delete them. Success is defined
  // as SwSwitch's attempt to apply them to hw, even though they might have not
  // actually been applied yet.
  while (!updates.empty()) {
    unique_ptr<StateUpdate> update(&updates.front());
    updates.pop_front();
    update->onSuccess();
  }
}

int SwSwitch::getHighresSamplers(HighresSamplerList* samplers,
                                 const set<CounterRequest>& counters) {
  int numCountersAdded = 0;

  // Group the counters by namespace to make it easier to get samplers
  map<string, set<CounterRequest>> groupedCounters;
  for (const auto& c : counters) {
    groupedCounters[c.namespaceName].insert(c);
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

void SwSwitch::setStateInternal(
    std::shared_ptr<SwitchState> newAppliedState,
    std::shared_ptr<SwitchState> newDesiredState) {
  // This is one of the only two places that should ever directly access
  // stateDontUseDirectly_.  (getState() being the other one.)
  CHECK(bool(newAppliedState));
  CHECK(bool(newDesiredState));
  CHECK(newAppliedState->isPublished());
  CHECK(newDesiredState->isPublished());
  folly::SpinLockGuard guard(stateLock_);
  appliedStateDontUseDirectly_.swap(newAppliedState);
  desiredStateDontUseDirectly_.swap(newDesiredState);
}

void SwSwitch::setDesiredState(std::shared_ptr<SwitchState> newDesiredState) {
  CHECK(bool(newDesiredState));
  CHECK(newDesiredState->isPublished());
  folly::SpinLockGuard guard(stateLock_);
  desiredStateDontUseDirectly_.swap(newDesiredState);
}

std::shared_ptr<SwitchState> SwSwitch::applyUpdate(
    const shared_ptr<SwitchState>& oldState,
    const shared_ptr<SwitchState>& newState) {
  // Check that we are starting from what has been already applied
  DCHECK_EQ(oldState, getAppliedState());

  auto start = std::chrono::steady_clock::now();
  LOG(INFO) << "Updating state: old_gen=" << oldState->getGeneration() <<
    " new_gen=" << newState->getGeneration();
  DCHECK_GT(newState->getGeneration(), oldState->getGeneration());

  StateDelta delta(oldState, newState);

  // If we are already exiting, abort the update
  if (isExiting()) {
    return oldState;
  }

  std::shared_ptr<SwitchState> newAppliedState;

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
    newAppliedState = hw_->stateChanged(delta);
  } catch (const std::exception& ex) {
    // Notify the hw_ of the crash so it can execute any device specific
    // tasks before we fatal. An example would be to dump the current hw state.
    //
    // Another thing we could try here is rolling back to the old state.
    hw_->exitFatal();
    LOG(FATAL) << "error applying state change to hardware: " <<
      folly::exceptionStr(ex);
  }

  setStateInternal(newAppliedState, newState);

  // Notifies all observers of the current state update. We notify them that
  // the state changed to "desired state", even if the whole state might not
  // have been applied yet. If an observer wants to know the applied state,
  // they can query the SwSwitch about it.
  notifyStateObservers(delta);

  auto end = std::chrono::steady_clock::now();
  auto duration =
    std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  stats()->stateUpdate(duration);
  VLOG(0) << "Update state took " << duration.count() << "us";
  return newAppliedState;
}

PortStats* SwSwitch::portStats(PortID portID) {
  auto portStats = stats()->port(portID);
  if (portStats) {
    return portStats;
  }
  auto portIf = getState()->getPorts()->getPortIf(portID);
  if (portIf) {
    // get portName from current state
    return stats()->createPortStats(
      portID, getState()->getPort(portID)->getName());
  } else {
    // only for port0 case
    VLOG(0) << "Port node doesn't exist, use default name=port" << portID;
    return stats()->createPortStats(portID, folly::to<string>("port", portID));
  }
}

PortStats* SwSwitch::portStats(const RxPacket* pkt) {
  return portStats(pkt->getSrcPort());
}

map<int32_t, PortStatus> SwSwitch::getPortStatus() {
  map<int32_t, PortStatus> statusMap;
  std::shared_ptr<PortMap> portMap = getState()->getPorts();
  for (const auto& p : *portMap) {
    statusMap[p->getID()] = fillInPortStatus(*p, this);
  }
  return statusMap;
}

PortStatus SwSwitch::getPortStatus(PortID portID) {
  std::shared_ptr<Port> port = getState()->getPort(portID);
  return fillInPortStatus(*port, this);
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
  portStats(port)->setPortStatus(up);
}

void SwSwitch::packetReceived(std::unique_ptr<RxPacket> pkt) noexcept {
  PortID port = pkt->getSrcPort();
  try {
    handlePacket(std::move(pkt));
  } catch (const std::exception& ex) {
    portStats(port)->pktError();
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
  portStats(port)->trappedPkt();

  pcapMgr_->packetReceived(pkt.get());

  // The minimum required frame length for ethernet is 64 bytes.
  // Abort processing early if the packet is too short.
  auto len = pkt->getLength();
  if (len < 64) {
    portStats(port)->pktBogus();
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

  if (distributionServiceReady_.load()) {
    publishRxPacket(pkt.get(), ethertype);
  }

  VLOG(5) << "trapped packet: src_port=" << pkt->getSrcPort() << " srcAggPort="
          << (pkt->isFromAggregatePort()
                  ? folly::to<string>(pkt->getSrcAggregatePort())
                  : "None")
          << " vlan=" << pkt->getSrcVlan() << " length=" << len
          << " src=" << srcMac << " dst=" << dstMac << " ethertype=0x"
          << std::hex << ethertype << " :: " << pkt->describeDetails();

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
  case LACPDU::EtherType::SLOW_PROTOCOLS:
  {
    // The only supported protocol in the Ethernet suite's "Slow Protocols"
    // is the Link Aggregation Control Protocol
    auto subtype = c.readBE<uint8_t>();
    if (subtype == LACPDU::EtherSubtype::LACP) {
      if (lagManager_) {
        lagManager_->handlePacket(std::move(pkt), c);
      } else {
        LOG_EVERY_N(WARNING, 60) << "Received LACP frame but LACP not enabled";
      }
    } else if (subtype == LACPDU::EtherSubtype::MARKER) {
      LOG(WARNING) << "Received Marker frame but Marker Protocol not supported";
    }
  }
    return;
  default:
    break;
  }

  // If we are still here, we don't know what to do with this packet.
  // Increment a counter and just drop the packet on the floor.
  portStats(port)->pktUnhandled();
}

void SwSwitch::linkStateChanged(PortID portId, bool up) {
  LOG(INFO) << "Link state changed: " << portId << "->" << (up ? "UP" : "DOWN");
  if (not isFullyInitialized()) {
    return;
  }

  // Schedule an update for port's operational status
  auto updateOperStateFn = [=](const std::shared_ptr<SwitchState>& state) {
    std::shared_ptr<SwitchState> newState(state);
    auto* port = newState->getPorts()->getPortIf(portId).get();
    if (not port) {
      throw FbossError("Port ", portId, " doesn't exists in SwitchState.");
    }

    port = port->modify(&newState);
    port->setOperState(up);

    return newState;
  };
  updateStateNoCoalescing(
      "Port OperState Update", std::move(updateOperStateFn));

  // Log event and update counters
  logLinkStateEvent(portId, up);
  setPortStatusCounter(portId, up);
  portStats(portId)->linkStateChange();
}

void SwSwitch::startThreads() {
  backgroundThread_.reset(new std::thread(
      [=] { this->threadLoop("fbossBgThread", &backgroundEventBase_); }));
  updateThread_.reset(new std::thread(
      [=] { this->threadLoop("fbossUpdateThread", &updateEventBase_); }));
  packetTxThread_.reset(new std::thread(
      [=] { this->threadLoop("fbossPktTxThread", &packetTxEventBase_); }));
  pcapDistributionThread_.reset(new std::thread([=] {
    this->threadLoop(
        "fbossPcapDistributionThread", &pcapDistributionEventBase_);
  }));
  qsfpCacheThread_.reset(new std::thread(
      [=] { this->threadLoop("fbossQsfpCacheThread", &qsfpCacheEventBase_); }));
  lacpThread_.reset(new std::thread(
      [=] { this->threadLoop("fbossLacpThread", &lacpEventBase_); }));
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
  if (packetTxThread_) {
    packetTxEventBase_.runInEventBaseThread(
        [this] { packetTxEventBase_.terminateLoopSoon(); });
  }
  if (pcapDistributionThread_) {
    pcapDistributionEventBase_.runInEventBaseThread(
        [this] { pcapDistributionEventBase_.terminateLoopSoon(); });
  }
  if (qsfpCacheThread_) {
    qsfpCacheEventBase_.runInEventBaseThread(
        [this] { qsfpCacheEventBase_.terminateLoopSoon(); });
  }
  if (lacpThread_) {
    lacpEventBase_.runInEventBaseThread(
        [this] { lacpEventBase_.terminateLoopSoon(); });
  }
  if (backgroundThread_) {
    backgroundThread_->join();
  }
  if (updateThread_) {
    updateThread_->join();
  }
  if (packetTxThread_) {
    packetTxThread_->join();
  }
  if (pcapDistributionThread_) {
    pcapDistributionThread_->join();
  }
  if (qsfpCacheThread_) {
    qsfpCacheThread_->join();
  }
  if (lacpThread_) {
    lacpThread_->join();
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

  Cursor c(pkt->buf());
  // unused to parse the ethertype correctly
  PktUtil::readMac(&c);
  PktUtil::readMac(&c);
  auto ethertype = c.readBE<uint16_t>();
  if (ethertype == 0x8100) {
    // 802.1Q
    c += 2; // Advance over the VLAN tag.  We ignore it for now
    ethertype = c.readBE<uint16_t>();
  }

  if (distributionServiceReady_.load()) {
    publishTxPacket(pkt.get(), ethertype);
  }

  if (!hw_->sendPacketOutOfPort(std::move(pkt), portID)) {
    // Just log an error for now.  There's not much the caller can do about
    // send failures--even on successful return from sendPacket*() the
    // send may ultimately fail since it occurs asynchronously in the
    // background.
    LOG(ERROR) << "failed to send packet out port " << portID;
  }
}

void SwSwitch::sendPacketOutOfPort(
    std::unique_ptr<TxPacket> pkt,
    AggregatePortID aggPortID) noexcept {
  auto aggPort = getState()->getAggregatePorts()->getAggregatePortIf(aggPortID);
  if (!aggPort) {
    LOG(ERROR) << "failed to send packet out aggregate port " << aggPortID
               << ": no aggregate port corresponding to identifier";
    return;
  }

  auto subportAndFwdStates = aggPort->subportAndFwdState();
  if (subportAndFwdStates.begin() == subportAndFwdStates.end()) {
    LOG(ERROR) << "failed to send packet out aggregate port " << aggPortID
               << ": aggregate port has no constituent physical ports";
    return;
  }

  // Ideally, we would select the same (physical) sub-port to send this
  // packet out of as the ASIC. This could be accomplished by mimicking the
  // hardware's logic for doing so, which would for the most part entail
  // computing a hash of the appropriate header fields. Considering the
  // small number of packets that will be forwarded via this path, the added
  // complexity hardly seems worth it.
  // Instead, we simply always choose to send the packet out of the first
  // (physical) sub-port belonging to the aggregate port at hand. This scheme
  // will avoid any issues related to packet reordering. Of course, this
  // will increase the load on the first physical sub-port of each aggregate
  // port, but this imbalance should be negligible.
  PortID subport;
  AggregatePort::Forwarding fwdState;
  for (auto elem : subportAndFwdStates) {
    std::tie(subport, fwdState) = elem;
    if (fwdState == AggregatePort::Forwarding::ENABLED) {
      sendPacketOutOfPort(std::move(pkt), subport);
      return;
    }
  }
  LOG(INFO) << "failed to send packet out aggregate port" << aggPortID
            << ": aggregate port has no enabled physical ports";
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
    std::unique_ptr<TxPacket> pkt,
    folly::Optional<InterfaceID> maybeIfID) noexcept {

  // Buffer should not be shared.
  folly::IOBuf *buf = pkt->buf();
  CHECK(!buf->isShared());

  // Add L2 header to L3 packet. Information doesn't need to be complete
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

  auto state = getState();

  // Get VlanID associated with interface
  VlanID vlanID = getCPUVlan();
  if (maybeIfID.hasValue()) {
    auto intf = state->getInterfaces()->getInterfaceIf(*maybeIfID);
    if (!intf) {
      LOG(ERROR) << "Interface " << *maybeIfID << " doesn't exists in state.";
      return;
    }

    // Extract primary Vlan associated with this interface
    vlanID = intf->getVlanID();
  }

  try {
    uint16_t protocol{0};
    folly::IPAddress dstAddr;

    // Parse L3 header to identify IP-Protocol and dstAddr
    folly::io::Cursor cursor(buf);
    uint8_t protoVersion = cursor.read<uint8_t>() >> 4;
    cursor.reset(buf);  // Make cursor point to beginning again
    if (protoVersion == 4) {
      protocol = IPv4Handler::ETHERTYPE_IPV4;
      IPv4Hdr ipHdr(cursor);
      dstAddr = ipHdr.dstAddr;
    } else if (protoVersion == 6) {
      protocol = IPv6Handler::ETHERTYPE_IPV6;
      IPv6Hdr ipHdr(cursor);
      dstAddr = ipHdr.dstAddr;
    } else {
      throw FbossError("Wrong version number ", static_cast<int>(protoVersion),
                       " in the L3 packet to send.");
    }

    // Extend IOBuf to make room for L2 header and satisfy minimum packet size
    buf->prepend(l2Len);
    if (tailRoom) {
      // padding with 0
      memset(buf->writableTail(), 0, tailRoom);
      buf->append(tailRoom);
    }

    // We always use our CPU's mac-address as source mac-address
    const folly::MacAddress srcMac = getPlatform()->getLocalMac();

    // Derive destination mac address
    folly::MacAddress dstMac{};
    if (dstAddr.isMulticast()) {
      // Multicast Case:
      // Derive destination mac-address based on destination ip-address
      if (dstAddr.isV4()) {
        dstMac = getMulticastMacAddr(dstAddr.asV4());
      } else {
        dstMac = getMulticastMacAddr(dstAddr.asV6());
      }
    } else if (dstAddr.isLinkLocal()) {
      // LinkLocal Case:
      // Resolve neighbor mac address for given destination address. If address
      // doesn't exists in NDP table then request neighbor solicitation for it.
      CHECK(dstAddr.isLinkLocal());
      auto vlan = state->getVlans()->getVlan(vlanID);
      if (dstAddr.isV4()) {
        // We do not consult ARP table to forward v4 link local addresses.
        // Reason explained below.
        //
        // XXX: In 6Pack/Galaxy ipv4 link-local addresses are routed
        // internally within FCs/LCs so they might not be reachable directly.
        //
        // For now let's make use of L3 table to forward these packets by
        // using dstMac as srcMac
        dstMac = srcMac;
      } else {
        const auto dstAddrV6 = dstAddr.asV6();
        try {
          auto entry = vlan->getNdpTable()->getEntry(dstAddrV6);
          dstMac = entry->getMac();
        } catch (...) {
          // We don't have dstAddr in our NDP table. Request solicitation for
          // it and let this packet be dropped.
          IPv6Handler::sendNeighborSolicitation(
              this, dstAddrV6, srcMac, vlanID);
          throw;
        } // try
      }
    } else {
      // Unicast Packet:
      // Ideally we can do routing in SW but it can consume some good ammount of
      // CPU. To avoid this we prefer to perform routing in hardware. Using
      // our CPU MacAddr as DestAddr we will trigger L3 lookup in hardware :)
      dstMac = srcMac;

      // Resolve the l2 address of the next hop if needed. These functions
      // will do the RIB lookup and then probe for any unresolved nexthops
      // of the route.
      if (dstAddr.isV6()) {
        ipv6_->sendNeighborSolicitations(PortID(0), dstAddr.asV6());
      } else {
        ipv4_->resolveMac(state.get(), PortID(0), dstAddr.asV4());
      }
    }

    // Write L2 header. NOTE that we pass specific VLAN and a dstMac on which
    // packet should be forwarded to.
    folly::io::RWPrivateCursor rwCursor(buf);
    TxPacket::writeEthHeader(&rwCursor, dstMac, srcMac, vlanID, protocol);

    // We can look up the vlan to make sure it exists. However, the vlan can
    // be just deleted after the lookup. So, in order to do it correctly, we
    // will need to lock the state update. Even with that, this still cannot
    // guarantee the vlan we are looking here is the vlan when the packet is
    // originated from the host. Because of that, we are just going to send
    // the packet out to the HW. The HW will drop the packet if the vlan is
    // deleted.
    stats()->pktFromHost(l3Len);
    sendPacketSwitched(std::move(pkt));
  } catch (const std::exception& ex) {
    LOG(ERROR) << "Failed to send out L3 packet :"
               << folly::exceptionStr(ex);
  }
}

bool SwSwitch::sendPacketToHost(
    InterfaceID dstIfID,
    std::unique_ptr<RxPacket> pkt) {
  if (tunMgr_) {
    return tunMgr_->sendPacketToHost(dstIfID, std::move(pkt));
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
        apache::thrift::SimpleJSONSerializer::deserialize<cfg::SwitchConfig>(
            curConfigStr_.c_str(), curConfig_);

        // Set oper status of interfaces in SwitchState
        auto& newState = rval.first;
        for (auto const& port : *newState->getPorts()) {
          port->setOperState(hw_->isPortUp(port->getID()));
        }

        return newState;
      });
}

bool SwSwitch::isValidStateUpdate(
    const StateDelta& delta) const {
  return hw_->isValidStateUpdate(delta);
}

std::string SwSwitch::switchRunStateStr(
  facebook::fboss::SwSwitch::SwitchRunState runState) const {
  switch (runState) {
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

AdminDistance SwSwitch::clientIdToAdminDistance(int clientId) const {
  auto distance = curConfig_.clientIdToAdminDistance.find(clientId);
  if (distance == curConfig_.clientIdToAdminDistance.end()) {
    // In case we get a client id we don't know about
    LOG(ERROR) << "No admin distance mapping available for client id "
               << clientId << ". Using default distance - MAX_ADMIN_DISTANCE";
    return AdminDistance::MAX_ADMIN_DISTANCE;
  }

  if (VLOG_IS_ON(3)) {
    auto clientName = folly::get_default(
        _StdClientIds_VALUES_TO_NAMES, StdClientIds(clientId),
        "UNKNOWN");
    auto distanceString = folly::get_default(
        _AdminDistance_VALUES_TO_NAMES,
        static_cast<AdminDistance>(distance->second),
        "UNKNOWN");
    VLOG(3) << "Mapping client id " << clientId << " (" << clientName
            << ") to admin distance " << distance->second << " ("
            << distanceString << ").";
  }

  return static_cast<AdminDistance>(distance->second);
}
}} // facebook::fboss
