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
#include "fboss/agent/IPv4Handler.h"
#include "fboss/agent/IPv6Handler.h"
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
#include "fboss/agent/SfpMap.h"
#include "fboss/agent/SfpModule.h"
#include "fboss/agent/SfpImpl.h"
#include "fboss/agent/LldpManager.h"
#include "common/stats/ServiceData.h"
#include <folly/FileUtil.h>
#include <folly/MacAddress.h>
#include <folly/String.h>
#include <chrono>
#include <condition_variable>

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
using std::unique_lock;
using std::unique_ptr;

namespace {
  facebook::fboss::PortStatus fillInPortStatus(
      const facebook::fboss::Port& port,
      const facebook::fboss::SwSwitch* sw) {
    facebook::fboss::PortStatus status;
    status.enabled = (port.getState() ==
        facebook::fboss::cfg::PortState::UP ? true : false);
    status.up = sw->isPortUp(port.getID());
    return status;
  }
}

namespace facebook { namespace fboss {

SwSwitch::SwSwitch(std::unique_ptr<Platform> platform)
  : hw_(platform->getHwSwitch()),
    platform_(std::move(platform)),
    arp_(new ArpHandler(this)),
    ipv4_(new IPv4Handler(this)),
    ipv6_(new IPv6Handler(this)),
    nUpdater_(new NeighborUpdater(this)),
    pcapMgr_(new PktCaptureManager(this)),
    sfpMap_(new SfpMap()) {
  // Create the platform-specific state directories if they
  // don't exist already.
  utilCreateDir(platform_->getVolatileStateDir());
  utilCreateDir(platform_->getPersistentStateDir());
}

SwSwitch::~SwSwitch() {
    stop();
}

void SwSwitch::stop() {
  {
    lock_guard<mutex> g(hwMutex_);

    setSwitchRunState(SwitchRunState::EXITING);

    // Several member variables are performing operations in the background
    // thread.  Ask them to stop, before we shut down the background thread.
    ipv6_.reset();
    nUpdater_.reset();
    if (lldpManager_) {
      lldpManager_->stop();
    }

    // Stop tunMgr so we don't get any packets to process
    // in software that were sent to the switch ip or were
    // routed from kernel to the front panel tunnel interface.
    tunMgr_.reset();
  }

  // This needs to be run without holding hwMutex_ because the update
  // thread may be waiting on the mutex in applyUpdate
  stopThreads();
}

bool SwSwitch::isFullyInitialized() const {
  return getSwitchRunState() >= SwitchRunState::INITIALIZED;
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
}

SwSwitch::SwitchRunState SwSwitch::getSwitchRunState() const {
  return runState_.load(std::memory_order_acquire);
}

void SwSwitch::gracefulExit() {
  if (isFullyInitialized()) {
    dumpStateToFile(platform_->getWarmBootSwitchStateFile());
    ipv6_->floodNeighborAdvertisements();
    arp_->floodGratuituousArp();
    // Stop handlers and threads before uninitializing h/w
    stop();
    // Cleanup if we ever initialized
    hw_->gracefulExit();
  }
  exit(0);
}

void SwSwitch::dumpStateToFile(const string& filename) const {
  bool success = folly::writeFile(
    toPrettyJson(getState()->toFollyDynamic()).toStdString(),
    filename.c_str());
  if (!success) {
    LOG(ERROR) << "Unable to dump switch state to " << filename;
  }
}

bool SwSwitch::isPortUp(PortID port) const {
   if (getState()->getPort(port)->getState() == cfg::PortState::UP) {
     return hw_->isPortUp(port);
   }
   // Port not enabled
   return false;
}

void SwSwitch::registerPortStatusListener(
    std::function<void(PortID, const PortStatus)> callback) {
  lock_guard<mutex> g(portListenerMutex_);
  portListener_ = callback;
}

bool SwSwitch::neighborEntryHit(RouterID vrf, folly::IPAddress& ip) {
  lock_guard<mutex> g(hwMutex_);
  return hw_->neighborEntryHit(vrf, ip);
}

void SwSwitch::exitFatal() const noexcept {
  dumpStateToFile(platform_->getCrashSwitchStateFile());
}

void SwSwitch::clearWarmBootCache() {
  lock_guard<mutex> g(hwMutex_);
  hw_->clearWarmBootCache();
}

void SwSwitch::init(SwitchFlags flags) {
  lock_guard<mutex> g(hwMutex_);
  auto start = std::chrono::steady_clock::now();
  auto stateAndBootType = hw_->init(this);
  auto initialState = stateAndBootType.first;
  bootType_ = stateAndBootType.second;
  auto end = std::chrono::steady_clock::now();
  auto initDuration =
    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  VLOG(0) << "hardware initialized in " <<
    (initDuration.count() / 1000.0) << " seconds; applying initial config";

  // Store the initial state
  initialState->publish();
  setStateInternal(initialState);

  platform_->onHwInitialized(this);

  if (flags & SwitchFlags::ENABLE_TUN) {
    tunMgr_ = folly::make_unique<TunManager>(this, &backgroundEventBase_);
    tunMgr_->startProbe();
  }

  startThreads();

  if (flags & SwitchFlags::PUBLISH_BOOTTYPE) {
    publishBootType();
  }

  if (flags & SwitchFlags::ENABLE_LLDP) {
      lldpManager_ = folly::make_unique<LldpManager>(this);
  }
  setSwitchRunState(SwitchRunState::INITIALIZED);
}

void SwSwitch::initialConfigApplied() {
  {
    lock_guard<mutex> g(hwMutex_);
    hw_->initialConfigApplied();
  }
  setSwitchRunState(SwitchRunState::CONFIGURED);
  syncTunInterfaces();
  if (lldpManager_) {
      lldpManager_->start();
  }
}

void SwSwitch::fibSynced() {
  setSwitchRunState(SwitchRunState::FIB_SYNCED);
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

void SwSwitch::updateStateBlocking(folly::StringPiece name, StateUpdateFn fn) {
  BlockingUpdateResult result;
  auto update = make_unique<BlockingStateUpdate>(name, std::move(fn), &result);
  updateState(std::move(update));
  result.wait();
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
    pendingUpdates_.swap(updates);
  }

  // handlePendingUpdates() is invoked once for each update, but a previous
  // call might have already processed everything.  If we don't have anything
  // to do just return early.
  if (updates.empty()) {
    return;
  }

  // Call all of the update functions to prepare the new SwitchState
  auto origState = getState();
  auto state = origState;
  auto iter = updates.begin();
  while (iter != updates.end()) {
    StateUpdate* update = &(*iter);
    ++iter;

    shared_ptr<SwitchState> newState;
    VLOG(3) << "preparing state update " << update->getName();
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

void SwSwitch::syncTunInterfaces() {
  CHECK(isConfigured());
  if (!tunMgr_) {
    return;
  }

  try {
    tunMgr_->startSync(getState()->getInterfaces());
  } catch (const std::exception& ex) {
    // TODO: Figure out the best way to handle errors here.
    LOG(FATAL) << "error applying interfaces change to system: " <<
      folly::exceptionStr(ex);
  }
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

  // Hold the hwMutex_ wihle applying the updates.
  // We are currently holding this for a bit longer than necessary, just to
  // be conservative.  It should be possible to only hold this around
  // HwSwitch::stateChanged()  (or eventually just make
  // HwSwitch::stateChanged() responsible for providing its own locking).
  lock_guard<mutex> hwGuard(hwMutex_);

  // If we are already exiting, abort the update
  if (isExiting()) {
    return;
  }

  // Publish the configuration as our active state.
  setStateInternal(newState);

  // Inform the IPv6Handler of the change.
  // TODO: We need a more generic mechanism for listeners to register to
  // receive state change events.
  ipv6_->stateChanged(delta);

  // Inform the NeighborUpdater of the change.
  nUpdater_->stateChanged(delta);

  // sync the new interface info to the host
  if (isConfigured()) {
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
    syncTunInterfaces();
  }

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

PortStatus SwSwitch::getPortStatus(PortID port) {
  return fillInPortStatus(*getState()->getPort(port), this);
}

SfpModule* SwSwitch::getSfp(PortID portID) const {
  return sfpMap_->sfpModule(portID);
}

map<int32_t, SfpDom> SwSwitch::getSfpDoms() const {
  map<int32_t, SfpDom> domInfos;
  for (const auto& sfp : *sfpMap_) {
    SfpDom domInfo;
    sfp.second->getSfpDom(domInfo);
    domInfos[sfp.first] = domInfo;
  }
  return domInfos;
}

SfpDom SwSwitch::getSfpDom(PortID port) const {
  SfpDom domInfo;
  getSfp(port)->getSfpDom(domInfo);
  return domInfo;
}

void SwSwitch::createSfp(PortID portID, std::unique_ptr<SfpImpl>& sfpImpl) {
  std::unique_ptr<SfpModule> sfpModule =
                            folly::make_unique<SfpModule>(sfpImpl);
  sfpMap_->createSfp(portID, sfpModule);
}

void SwSwitch::detectSfp() {
  for (auto it = sfpMap_->begin(); it != sfpMap_->end(); ++it) {
    it->second.get()->detectSfp();
  }
}

void SwSwitch::updateSfpDomFields() {
  for (auto it = sfpMap_->begin(); it != sfpMap_->end(); it++) {
    it->second.get()->updateSfpDomFields();
  }
}

SwitchStats* SwSwitch::createSwitchStats() {
  SwitchStats* s = new SwitchStats();
  stats_.reset(s);
  return s;
}

void SwSwitch::packetReceived(std::unique_ptr<RxPacket> pkt) noexcept{
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

void SwSwitch::handlePacket(std::unique_ptr<RxPacket> pkt) {
  // If we are already exiting, don't handle any more packets
  // since the individual handlers, h/w sdk data structures
  // may already be (partially) destroyed
  if (isExiting()) {
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
    " ethertype=0x" << std::hex << ethertype;

  switch (ethertype) {
  case ArpHandler::ETHERTYPE_ARP:
    arp_->handlePacket(std::move(pkt), dstMac, srcMac, c);
    return;
  case LldpManager::ETHERTYPE_LLDP:
    lldpManager_->handlePacket(std::move(pkt), dstMac, srcMac, c);
    return;
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

void SwSwitch::linkStateChanged(PortID port, bool up) noexcept {
  LOG(INFO) << "link state changed: " << port << " enabled = " << up;
  lock_guard<mutex> g(portListenerMutex_);
  if (!portListener_) {
    return;
  }
  // It seems that this function can get called before the state is fully
  // initialized. Don't do anything if we have a null state.
  auto state = getState();
  if (state != nullptr) {
    portListener_(port, fillInPortStatus(*state->getPort(port), this));
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

  auto stopThread = [=](void* arg) {
    auto* evb = static_cast<folly::EventBase*>(arg);
    evb->terminateLoopSoon();
  };

  if (backgroundThread_) {
    backgroundEventBase_.runInEventBaseThread(stopThread,
                                              &backgroundEventBase_);
  }
  if (updateThread_) {
    updateEventBase_.runInEventBaseThread(stopThread, &updateEventBase_);
  }
  if (backgroundThread_) {
    backgroundThread_->join();
  }
  if (updateThread_) {
    updateThread_->join();
  }
}

void SwSwitch::threadLoop(StringPiece name, EventBase* eventBase) {
  // We need a null-terminated string to pass to pthread_setname_np().
  // The pthread name can be at most 15 bytes long, so truncate it if necessary
  // when creating the string.
  size_t pthreadLength = std::min(name.size(), (size_t)15);
  char pthreadName[pthreadLength + 1];
  memcpy(pthreadName, name.begin(), pthreadLength);
  pthreadName[pthreadLength] = '\0';
  pthread_setname_np(pthread_self(), pthreadName);

#ifdef FACEBOOK
  // Set the name for glog
  google::setThreadName(name.str());
#endif

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
  return std::move(pkt);
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
  pcapMgr_->packetSentToHost(pkt.get());
  if (tunMgr_) {
    return tunMgr_->sendPacketToHost(std::move(pkt));
  } else {
    return false;
  }
}

}} // facebook::fboss
