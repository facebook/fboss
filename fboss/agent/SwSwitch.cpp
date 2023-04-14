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

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/FibHelpers.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/IPv4Handler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/LookupClassRouteUpdater.h"
#include "fboss/agent/LookupClassUpdater.h"
#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
#if FOLLY_HAS_COROUTINES
#include "fboss/agent/MKAServiceManager.h"
#endif
#include "fboss/agent/AclNexthopHandler.h"
#include "fboss/agent/DsfSubscriber.h"
#include "fboss/agent/FsdbSyncer.h"
#include "fboss/agent/MPLSHandler.h"
#include "fboss/agent/MacTableManager.h"
#include "fboss/agent/MirrorManager.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/PacketLogger.h"
#include "fboss/agent/PacketObserver.h"
#include "fboss/agent/PhySnapshotManager-defs.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/PortUpdateHandler.h"
#include "fboss/agent/ResolvedNexthopMonitor.h"
#include "fboss/agent/ResolvedNexthopProbeScheduler.h"
#include "fboss/agent/RestartTimeTracker.h"
#include "fboss/agent/RouteUpdateLogger.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/StaticL2ForNeighborObserver.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TeFlowNexthopHandler.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/capture/PcapPkt.h"
#include "fboss/agent/capture/PktCaptureManager.h"
#include "fboss/agent/gen-cpp2/switch_config_types_custom_protocol.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/MPLSHdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/platforms/common/PlatformMappingUtils.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/StateUpdateHelpers.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

#include <fb303/ServiceData.h>
#include <folly/Demangle.h>
#include <folly/FileUtil.h>
#include <folly/GLog.h>
#include <folly/MacAddress.h>
#include <folly/MapUtil.h>
#include <folly/SocketAddress.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <folly/system/ThreadName.h>
#include <glog/logging.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <chrono>
#include <condition_variable>
#include <exception>
#include <tuple>

using folly::EventBase;
using folly::SocketAddress;
using folly::StringPiece;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::exception;
using std::lock_guard;
using std::make_unique;
using std::map;
using std::mutex;
using std::set;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

using facebook::fboss::AgentConfig;
using facebook::fboss::cfg::SwitchConfig;
using facebook::fboss::cfg::SwitchInfo;
using facebook::fboss::cfg::SwitchType;
using facebook::fboss::DeltaFunctions::forEachChanged;

using namespace std::chrono;
using namespace apache::thrift;
using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;

DEFINE_int32(thread_heartbeat_ms, 1000, "Thread heartbeat interval (ms)");
DEFINE_int32(
    distribution_timeout_ms,
    1000,
    "Timeout for sending to distribution_service (ms)");

DEFINE_bool(
    log_all_fib_updates,
    false,
    "Flag to turn on logging of all updates to the FIB");

DEFINE_int32(
    minimum_ethernet_packet_length,
    64,
    "Expected minimum ethernet packet length");

DEFINE_int32(
    fsdbStatsStreamIntervalSeconds,
    5,
    "Interval at which stats subscriptions are served");

DEFINE_int32(
    update_phy_info_interval_s,
    10,
    "Update phy info interval in seconds");
namespace {

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
  bytes[3] = addrBytes[1] & 0x7f; // Take only 7 bits
  bytes[4] = addrBytes[2];
  bytes[5] = addrBytes[3];
  return folly::MacAddress::fromBinary(
      folly::ByteRange(bytes.begin(), bytes.end()));
}

facebook::fboss::PortStatus fillInPortStatus(
    const facebook::fboss::Port& port,
    const facebook::fboss::SwSwitch* sw) {
  facebook::fboss::PortStatus status;
  *status.enabled() = port.isEnabled();
  *status.up() = port.isUp();
  *status.speedMbps() = static_cast<int>(port.getSpeed());
  *status.profileID() = apache::thrift::util::enumName(port.getProfileID());
  *status.drained() = port.isDrained();

  try {
    status.transceiverIdx() =
        sw->getPlatform()->getPortMapping(port.getID(), port.getSpeed());
  } catch (const facebook::fboss::FbossError& err) {
    // No problem, we just don't set the other info
  }
  return status;
}

const std::map<int64_t, SwitchInfo> getSwitchInfoFromConfig(
    const SwitchConfig* config) {
  return *config->switchSettings()->switchIdToSwitchInfo();
}

const std::map<int64_t, SwitchInfo> getSwitchInfoFromConfig() {
  std::unique_ptr<AgentConfig> config;
  try {
    config = AgentConfig::fromDefaultFile();
  } catch (const exception& e) {
    // expected on devservers where no config file is available
    return std::map<int64_t, SwitchInfo>();
  }
  auto swConfig = config->thrift.sw();
  return getSwitchInfoFromConfig(&(swConfig.value()));
}

auto constexpr kHwUpdateFailures = "hw_update_failures";

} // anonymous namespace

namespace facebook::fboss {

SwSwitch::SwSwitch(std::unique_ptr<Platform> platform)
    : hw_(platform->getHwSwitch()),
      platform_(std::move(platform)),
      platformProductInfo_(
          std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath)),
      pktObservers_(new PacketObservers()),
      arp_(new ArpHandler(this)),
      ipv4_(new IPv4Handler(this)),
      ipv6_(new IPv6Handler(this)),
      nUpdater_(new NeighborUpdater(this)),
      pcapMgr_(new PktCaptureManager(
          platform_->getPersistentStateDir(),
          pktObservers_.get())),
      mirrorManager_(new MirrorManager(this)),
      mplsHandler_(new MPLSHandler(this)),
      packetLogger_(new PacketLogger(this)),
      routeUpdateLogger_(new RouteUpdateLogger(this)),
      resolvedNexthopMonitor_(new ResolvedNexthopMonitor(this)),
      resolvedNexthopProbeScheduler_(new ResolvedNexthopProbeScheduler(this)),
      portUpdateHandler_(new PortUpdateHandler(this)),
      lookupClassUpdater_(new LookupClassUpdater(this)),
      lookupClassRouteUpdater_(new LookupClassRouteUpdater(this)),
      staticL2ForNeighborObserver_(new StaticL2ForNeighborObserver(this)),
      macTableManager_(new MacTableManager(this)),
      phySnapshotManager_(
          new PhySnapshotManager<kIphySnapshotIntervalSeconds>()),
      aclNexthopHandler_(new AclNexthopHandler(this)),
      teFlowNextHopHandler_(new TeFlowNexthopHandler(this)),
      dsfSubscriber_(new DsfSubscriber(this)),
      switchInfoTable_(this, getSwitchInfoFromConfig()) {
  // Create the platform-specific state directories if they
  // don't exist already.
  utilCreateDir(platform_->getVolatileStateDir());
  utilCreateDir(platform_->getPersistentStateDir());
  try {
    platformProductInfo_->initialize();
    platformMapping_ =
        utility::initPlatformMapping(platformProductInfo_->getType());
    auto existingMapping = platform_->getPlatformMapping();
    // TODO - remove this later
    CHECK(existingMapping->toThrift() == platformMapping_->toThrift());
  } catch (const std::exception& ex) {
    // Expected when fruid file is not of a switch (eg: on devservers)
    XLOG(INFO) << "Couldn't initialize platform mapping " << ex.what();
  }
}

SwSwitch::SwSwitch(
    std::unique_ptr<Platform> platform,
    std::unique_ptr<PlatformMapping> platformMapping,
    cfg::SwitchConfig* config)
    : SwSwitch(std::move(platform)) {
  platformMapping_ = std::move(platformMapping);
  if (config) {
    switchInfoTable_ = SwitchInfoTable(this, getSwitchInfoFromConfig(config));
  }
}

SwSwitch::~SwSwitch() {
  if (getSwitchRunState() < SwitchRunState::EXITING) {
    // If we didn't already stop (say via gracefulExit call), begin
    // exit
    stop();
    restart_time::stop();
  }
}

void SwSwitch::stop(bool revertToMinAlpmState) {
  setSwitchRunState(SwitchRunState::EXITING);

  XLOG(DBG2) << "Stopping SwSwitch...";
  // Stop DSF subscriber to let us unsubscribe gracefully before stoppping
  // packet TX/RX functionality
  dsfSubscriber_.reset();

  // First tell the hw to stop sending us events by unregistering the callback
  // After this we should no longer receive packets or link state changed events
  // while we are destroying ourselves
  hw_->unregisterCallbacks();

  // Stop tunMgr so we don't get any packets to process
  // in software that were sent to the switch ip or were
  // routed from kernel to the front panel tunnel interface.
  if (tunMgr_) {
    tunMgr_->stopProcessing();
  }

  resolvedNexthopMonitor_.reset();
  resolvedNexthopProbeScheduler_.reset();
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
  nUpdater_.reset();

  if (lldpManager_) {
    lldpManager_->stop();
  }

  if (lagManager_) {
    lagManager_.reset();
  }

  // Need to destroy IPv6Handler as it is a state observer,
  // but we must do it after we've stopped TunManager.
  // Otherwise, we might attempt to call sendL3Packet which
  // calls ipv6_->sendNeighborSolicitation which will then segfault
  ipv6_.reset();

  packetLogger_.reset();
  routeUpdateLogger_.reset();

  heartbeatWatchdog_->stop();
  heartbeatWatchdog_.reset();
  bgThreadHeartbeat_.reset();
  updThreadHeartbeat_.reset();
  packetTxThreadHeartbeat_.reset();
  lacpThreadHeartbeat_.reset();
  neighborCacheThreadHeartbeat_.reset();
  if (rib_) {
    rib_->stop();
  }

  lookupClassUpdater_.reset();
  lookupClassRouteUpdater_.reset();
  macTableManager_.reset();
#if FOLLY_HAS_COROUTINES
  mkaServiceManager_.reset();
#endif
  phySnapshotManager_.reset();

  if (fsdbSyncer_) {
    fsdbSyncer_->stop();
  }
  // stops the background and update threads.
  stopThreads();

  // reset explicitly since it uses observer. Make sure to reset after bg thread
  // is stopped, else we'll race with bg thread sending packets such as route
  // advertisements
  pcapMgr_.reset();
  pktObservers_.reset();

  fsdbSyncer_.reset();
  // reset tunnel manager only after pkt thread is stopped
  // as there could be state updates in progress which will
  // access entries in tunnel manager
  tunMgr_.reset();

  // ALPM requires default routes be deleted at last. Thus,
  // blow away all routes except the min required for ALPM,
  // so as to properly destroy hw switch. This is part of
  // exit logic, but updateStateBlocking() won't work in EXITING
  // state. Thus, directly calling underlying hw_->stateChanged()
  if (revertToMinAlpmState) {
    XLOG(DBG3) << "setup min ALPM state";
    hw_->stateChanged(StateDelta(getState(), getMinAlpmRouteState(getState())));
  }
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

bool SwSwitch::isExiting() const {
  return getSwitchRunState() >= SwitchRunState::EXITING;
}

void SwSwitch::setSwitchRunState(SwitchRunState runState) {
  auto oldState = runState_.exchange(runState, std::memory_order_acq_rel);
  CHECK(oldState <= runState);
  onSwitchRunStateChange(runState);
  logSwitchRunStateChange(oldState, runState);
}

void SwSwitch::onSwitchRunStateChange(SwitchRunState newState) {
  if (newState == SwitchRunState::INITIALIZED) {
    restart_time::mark(RestartEvent::INITIALIZED);
  } else if (newState == SwitchRunState::CONFIGURED) {
    restart_time::mark(RestartEvent::CONFIGURED);
  }
  getHw()->switchRunStateChanged(newState);
}

SwitchRunState SwSwitch::getSwitchRunState() const {
  return runState_.load(std::memory_order_acquire);
}

void SwSwitch::setFibSyncTimeForClient(ClientID clientId) {
  switch (clientId) {
    case ClientID::BGPD:
      restart_time::mark(RestartEvent::FIB_SYNCED_BGPD);
      break;
    case ClientID::OPENR:
      restart_time::mark(RestartEvent::FIB_SYNCED_OPENR);
      break;
    default:
      // no time tracking for remaining clients
      break;
  }
}

std::tuple<folly::dynamic, state::WarmbootState> SwSwitch::gracefulExitState()
    const {
  folly::dynamic follySwitchState = folly::dynamic::object;
  state::WarmbootState thriftSwitchState;
  if (rib_) {
    // For RIB we employ a optmization to serialize only unresolved routes
    // and recover others from FIB
    thriftSwitchState.routeTables() = rib_->warmBootState();
    // TODO(pshaikh): delete rib's folly dynamic
    follySwitchState[kRib] = rib_->unresolvedRoutesFollyDynamic();
  }
  *thriftSwitchState.swSwitchState() = getAppliedState()->toThrift();
  return std::make_tuple(follySwitchState, thriftSwitchState);
}

void SwSwitch::gracefulExit() {
  if (isFullyInitialized()) {
    steady_clock::time_point begin = steady_clock::now();
    XLOG(DBG2) << "[Exit] Starting SwSwitch graceful exit";
    ipv6_->floodNeighborAdvertisements();
    arp_->floodGratuituousArp();
    steady_clock::time_point neighborFloodDone = steady_clock::now();
    XLOG(DBG2)
        << "[Exit] Neighbor flood time "
        << duration_cast<duration<float>>(neighborFloodDone - begin).count();
    // Stop handlers and threads before uninitializing h/w
    stop();
    steady_clock::time_point stopThreadsAndHandlersDone = steady_clock::now();
    XLOG(DBG2) << "[Exit] Stop thread and handlers time "
               << duration_cast<duration<float>>(
                      stopThreadsAndHandlersDone - neighborFloodDone)
                      .count();

    auto [follySwitchState, thriftSwitchState] = gracefulExitState();

    steady_clock::time_point switchStateToFollyDone = steady_clock::now();
    XLOG(DBG2) << "[Exit] Switch state to folly dynamic "
               << duration_cast<duration<float>>(
                      switchStateToFollyDone - stopThreadsAndHandlersDone)
                      .count();
    // Cleanup if we ever initialized
    hw_->gracefulExit(follySwitchState, thriftSwitchState);
    XLOG(DBG2)
        << "[Exit] SwSwitch Graceful Exit time "
        << duration_cast<duration<float>>(steady_clock::now() - begin).count();
  }
}

void SwSwitch::getProductInfo(ProductInfo& productInfo) const {
  platform_->getProductInfo(productInfo);
}

void SwSwitch::publishPhyInfoSnapshots(PortID portID) const {
  phySnapshotManager_->publishSnapshots(portID);
}

void SwSwitch::updateLldpStats() {
  if (!lldpManager_) {
    return;
  }

  // Use number of entries left after pruning expired entries
  stats()->LldpNeighborsSize(lldpManager_->getDB()->pruneExpiredNeighbors());
}

void SwSwitch::updateStats() {
  SCOPE_EXIT {
    if (FLAGS_publish_stats_to_fsdb) {
      auto now = std::chrono::steady_clock::now();
      if (!publishedStatsToFsdbAt_ ||
          std::chrono::duration_cast<std::chrono::seconds>(
              now - *publishedStatsToFsdbAt_)
                  .count() > FLAGS_fsdbStatsStreamIntervalSeconds) {
        AgentStats agentStats;
        agentStats.hwPortStats() = getHw()->getPortStats();
        agentStats.sysPortStats() = getHw()->getSysPortStats();

        agentStats.hwAsicErrors() =
            getHw()->getSwitchStats()->getHwAsicErrors();
        agentStats.teFlowStats() = getTeFlowStats();
        stats()->fillAgentStats(agentStats);
        agentStats.bufferPoolStats() = getBufferPoolStats();
        fsdbSyncer_->statsUpdated(std::move(agentStats));
        publishedStatsToFsdbAt_ = now;
      }
    }
  };
  updateRouteStats();
  updatePortInfo();
  updateLldpStats();
  updateTeFlowStats();
  try {
    getHw()->updateStats(stats());
  } catch (const std::exception& ex) {
    stats()->updateStatsException();
    XLOG(ERR) << "Error running updateStats: " << folly::exceptionStr(ex);
  }
  // Determine if collect phy info
  auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  if (now - phyInfoUpdateTime_ >= FLAGS_update_phy_info_interval_s) {
    phyInfoUpdateTime_ = now;
    try {
      phySnapshotManager_->updatePhyInfos(getHw()->updateAllPhyInfo());
    } catch (const std::exception& ex) {
      stats()->updateStatsException();
      XLOG(ERR) << "Error running updatePhyInfos: " << folly::exceptionStr(ex);
    }
  }
}

TeFlowStats SwSwitch::getTeFlowStats() {
  TeFlowStats teFlowStats;
  std::map<std::string, HwTeFlowStats> hwTeFlowStats;
  auto statMap = facebook::fb303::fbData->getStatMap();
  for (const auto& [flowStr, flowEntry] :
       std::as_const(*getState()->getTeFlowTable())) {
    std::ignore = flowStr;
    if (const auto& counter = flowEntry->getCounterID()) {
      auto statName = folly::to<std::string>(counter->toThrift(), ".bytes");
      // returns default stat if statName does not exists
      auto statPtr = statMap->getStatPtrNoExport(statName);
      auto lockedStatPtr = statPtr->lock();
      auto numLevels = lockedStatPtr->numLevels();
      // Cumulative (ALLTIME) counters are at (numLevels - 1)
      HwTeFlowStats flowStat;
      flowStat.bytes() = lockedStatPtr->sum(numLevels - 1);
      hwTeFlowStats.emplace(counter->toThrift(), std::move(flowStat));
    }
  }
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  teFlowStats.timestamp() = now.count();
  teFlowStats.hwTeFlowStats() = std::move(hwTeFlowStats);
  return teFlowStats;
}

HwBufferPoolStats SwSwitch::getBufferPoolStats() const {
  HwBufferPoolStats stats;
  stats.deviceWatermarkBytes() = getHw()->getDeviceWatermarkBytes();
  return stats;
}

void SwSwitch::registerNeighborListener(
    std::function<void(
        const std::vector<std::string>& added,
        const std::vector<std::string>& deleted)> callback) {
  XLOG(DBG2) << "Registering neighbor listener";
  lock_guard<mutex> g(neighborListenerMutex_);
  neighborListener_ = std::move(callback);
}

void SwSwitch::invokeNeighborListener(
    const std::vector<std::string>& added,
    const std::vector<std::string>& removed) {
  lock_guard<mutex> g(neighborListenerMutex_);
  if (neighborListener_ && !isExiting()) {
    neighborListener_(added, removed);
  }
}

bool SwSwitch::getAndClearNeighborHit(RouterID vrf, folly::IPAddress ip) {
  return hw_->getAndClearNeighborHit(vrf, ip);
}

void SwSwitch::exitFatal() const noexcept {
  folly::dynamic switchState = folly::dynamic::object;
  switchState[kHwSwitch] = hw_->toFollyDynamic();
  state::WarmbootState thriftSwitchState;
  *thriftSwitchState.swSwitchState() = getAppliedState()->toThrift();
  if (!dumpStateToFile(platform_->getCrashSwitchStateFile(), switchState) ||
      !dumpBinaryThriftToFile(
          platform_->getCrashThriftSwitchStateFile(), thriftSwitchState)) {
    XLOG(ERR) << "Unable to write switch state JSON or Thrift to file";
  }
}

void SwSwitch::publishRxPacket(RxPacket* pkt, uint16_t ethertype) {
  RxPacketData pubPkt;
  pubPkt.srcPort = pkt->getSrcPort();
  pubPkt.srcVlan = getVlanIDHelper(pkt->getSrcVlanIf());

  for (const auto& r : pkt->getReasons()) {
    RxReason reason;
    reason.bytes = r.bytes;
    reason.description = r.description;
    pubPkt.reasons.push_back(reason);
  }

  folly::IOBuf buf_copy;
  pkt->buf()->cloneInto(buf_copy);
  pubPkt.packetData = buf_copy.moveToFbString();
}

void SwSwitch::publishTxPacket(TxPacket* pkt, uint16_t ethertype) {
  TxPacketData pubPkt;
  folly::IOBuf copy_buf;
  pkt->buf()->cloneInto(copy_buf);
  pubPkt.packetData = copy_buf.moveToFbString();
}

void SwSwitch::init(
    HwSwitch::Callback* callback,
    std::unique_ptr<TunManager> tunMgr,
    SwitchFlags flags) {
  auto begin = steady_clock::now();
  flags_ = flags;
  auto hwInitRet = hw_->init(
      callback,
      false /*failHwCallsOnWarmboot*/,
      platform_->getAsic()->getSwitchType(),
      platform_->getAsic()->getSwitchId());
  auto initialState = hwInitRet.switchState;
  bootType_ = hwInitRet.bootType;
  rib_ = std::move(hwInitRet.rib);
  fb303::fbData->setCounter(kHwUpdateFailures, 0);

  XLOG(DBG0) << "hardware initialized in " << hwInitRet.bootTime
             << " seconds; applying initial config";

  restart_time::init(
      platform_->getWarmBootDir(), bootType_ == BootType::WARM_BOOT);

  // Store the initial state
  initialState->publish();
  setStateInternal(initialState);

  // start LACP thread
  lacpThread_.reset(new std::thread(
      [=] { this->threadLoop("fbossLacpThread", &lacpEventBase_); }));

  // start lagMananger
  if (flags & SwitchFlags::ENABLE_LACP) {
    lagManager_ = std::make_unique<LinkAggregationManager>(this);
  }

  if (flags & SwitchFlags::ENABLE_TUN) {
    if (tunMgr) {
      tunMgr_ = std::move(tunMgr);
    } else {
      tunMgr_ = std::make_unique<TunManager>(this, &packetTxEventBase_);
    }
    tunMgr_->probe();
  }
  platform_->onHwInitialized(this);

  // Notify the state observers of the initial state
  updateEventBase_.runInEventBaseThread([initialState, this]() {
    notifyStateObservers(
        StateDelta(std::make_shared<SwitchState>(), initialState));
  });

  startThreads();
  XLOG(DBG2)
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

  auto bgHeartbeatStatsFunc = [this](int delay, int backLog) {
    stats()->bgHeartbeatDelay(delay);
    stats()->bgEventBacklog(backLog);
  };
  bgThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &backgroundEventBase_,
      "fbossBgThread",
      FLAGS_thread_heartbeat_ms,
      bgHeartbeatStatsFunc);

  auto updHeartbeatStatsFunc = [this](int delay, int backLog) {
    stats()->updHeartbeatDelay(delay);
    stats()->updEventBacklog(backLog);
  };
  updThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &updateEventBase_,
      "fbossUpdateThread",
      FLAGS_thread_heartbeat_ms,
      updHeartbeatStatsFunc);

  auto packetTxHeartbeatStatsFunc = [this](int delay, int backLog) {
    stats()->packetTxHeartbeatDelay(delay);
    stats()->packetTxEventBacklog(backLog);
  };
  packetTxThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &packetTxEventBase_,
      "fbossPktTxThread",
      FLAGS_thread_heartbeat_ms,
      packetTxHeartbeatStatsFunc);

  auto updateLacpThreadHeartbeatStats = [this](int delay, int backLog) {
    stats()->lacpHeartbeatDelay(delay);
    stats()->lacpEventBacklog(backLog);
  };
  lacpThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &lacpEventBase_,
      *folly::getThreadName(lacpThread_->get_id()),
      FLAGS_thread_heartbeat_ms,
      updateLacpThreadHeartbeatStats);

  neighborCacheThreadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &neighborCacheEventBase_,
      *folly::getThreadName(neighborCacheThread_->get_id()),
      FLAGS_thread_heartbeat_ms,
      [this](int delay, int backlog) {
        stats()->neighborCacheHeartbeatDelay(delay);
        stats()->neighborCacheEventBacklog(backlog);
      });

  heartbeatWatchdog_ = std::make_unique<ThreadHeartbeatWatchdog>(
      std::chrono::milliseconds(FLAGS_thread_heartbeat_ms * 10),
      [this]() { stats()->ThreadHeartbeatMissCount(); });
  heartbeatWatchdog_->startMonitoringHeartbeat(bgThreadHeartbeat_);
  heartbeatWatchdog_->startMonitoringHeartbeat(packetTxThreadHeartbeat_);
  heartbeatWatchdog_->startMonitoringHeartbeat(updThreadHeartbeat_);
  heartbeatWatchdog_->startMonitoringHeartbeat(lacpThreadHeartbeat_);
  heartbeatWatchdog_->startMonitoringHeartbeat(neighborCacheThreadHeartbeat_);
  heartbeatWatchdog_->start();

  setSwitchRunState(SwitchRunState::INITIALIZED);
  if (platform_->getAsic()->isSupported(HwAsic::Feature::ROUTE_PROGRAMMING)) {
    SwSwitchRouteUpdateWrapper(this, rib_.get()).programMinAlpmState();
  }
  if (FLAGS_log_all_fib_updates) {
    constexpr auto kAllFibUpdates = "all_fib_updates";
    logRouteUpdates("::", 0, kAllFibUpdates);
    logRouteUpdates("0.0.0.0", 0, kAllFibUpdates);
  }
}

void SwSwitch::init(std::unique_ptr<TunManager> tunMgr, SwitchFlags flags) {
  this->init(this, std::move(tunMgr), flags);
}

void SwSwitch::initialConfigApplied(const steady_clock::time_point& startTime) {
  // notify the hw
  platform_->onInitialConfigApplied(this);
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
    publishInitTimes(
        "fboss.agent.switch_configured",
        duration_cast<duration<float>>(steady_clock::now() - startTime)
            .count());
  }
#if FOLLY_HAS_COROUTINES
  if (flags_ & SwitchFlags::ENABLE_MACSEC) {
    mkaServiceManager_ = std::make_unique<MKAServiceManager>(this);
  }
#endif
  // Start FSDB state syncer post initial config applied. FSDB state
  // syncer will do a full sync upon connection establishment to FSDB
  fsdbSyncer_ = std::make_unique<FsdbSyncer>(this);
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

void SwSwitch::stopLoggingRouteUpdates(const std::string& identifier) {
  routeUpdateLogger_->stopLoggingForIdentifier(identifier);
}

void SwSwitch::registerStateObserver(
    StateObserver* observer,
    const string name) {
  XLOG(DBG2) << "Registering state observer: " << name;
  updateEventBase_.runImmediatelyOrRunInEventBaseThreadAndWait(
      [=]() { addStateObserver(observer, name); });
}

void SwSwitch::unregisterStateObserver(StateObserver* observer) {
  updateEventBase_.runImmediatelyOrRunInEventBaseThreadAndWait(
      [=]() { removeStateObserver(observer); });
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
      XLOG(FATAL) << "error notifying " << observerName.second
                  << " of update: " << folly::exceptionStr(ex);
    }
  }
}

bool SwSwitch::updateState(unique_ptr<StateUpdate> update) {
  if (isExiting()) {
    XLOG(DBG2) << " Skipped queuing update: " << update->getName()
               << " since exit already started";
    return false;
  }
  {
    std::unique_lock guard(pendingUpdatesLock_);
    pendingUpdates_.push_back(*update.release());
  }

  // Signal the update thread that updates are pending.
  // We call runInEventBaseThread() with a static function pointer since this
  // is more efficient than having to allocate a new bound function object.
  updateEventBase_.runInEventBaseThread(handlePendingUpdatesHelper, this);
  return true;
}

bool SwSwitch::updateState(StringPiece name, StateUpdateFn fn) {
  auto update = make_unique<FunctionStateUpdate>(name, std::move(fn));
  return updateState(std::move(update));
}

void SwSwitch::updateStateNoCoalescing(StringPiece name, StateUpdateFn fn) {
  auto update = make_unique<FunctionStateUpdate>(
      name,
      std::move(fn),
      static_cast<int>(StateUpdate::BehaviorFlags::NON_COALESCING));
  updateState(std::move(update));
}

void SwSwitch::updateStateBlocking(folly::StringPiece name, StateUpdateFn fn) {
  auto behaviorFlags = static_cast<int>(StateUpdate::BehaviorFlags::NONE);
  updateStateBlockingImpl(name, fn, behaviorFlags);
}

void SwSwitch::updateStateWithHwFailureProtection(
    folly::StringPiece name,
    StateUpdateFn fn) {
  int stateUpdateBehavior =
      static_cast<int>(StateUpdate::BehaviorFlags::NON_COALESCING) |
      static_cast<int>(StateUpdate::BehaviorFlags::HW_FAILURE_PROTECTION);

  updateStateBlockingImpl(name, fn, stateUpdateBehavior);
}

void SwSwitch::updateStateBlockingImpl(
    folly::StringPiece name,
    StateUpdateFn fn,
    int stateUpdateBehavior) {
  auto result = std::make_shared<BlockingUpdateResult>();
  auto update = make_unique<BlockingStateUpdate>(
      name, std::move(fn), result, stateUpdateBehavior);
  if (updateState(std::move(update))) {
    result->wait();
  }
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
    std::unique_lock guard(pendingUpdatesLock_);
    // When deciding how many elements to pull off the pendingUpdates_
    // list, we pull as many as we can, subject to the following conditions
    // - Non coalescing updates are executed by themselves
    auto iter = pendingUpdates_.begin();
    while (iter != pendingUpdates_.end()) {
      StateUpdate* update = &(*iter);
      if (update->isNonCoalescing()) {
        if (iter == pendingUpdates_.begin()) {
          // First update is non coalescing, splice it onto the updates list
          // and apply transaction by itself
          ++iter;
          break;
        } else {
          // Splice all updates upto this non coalescing update, we will
          // get the non coalescing update in the next round
          break;
        }
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

  // Non coalescing updates should be applied individually
  bool isNonCoalescing = updates.begin()->isNonCoalescing();
  if (isNonCoalescing) {
    CHECK_EQ(updates.size(), 1)
        << " Non coalescing updates should be applied individually";
  }
  if (updates.begin()->hwFailureProtected()) {
    CHECK(isNonCoalescing)
        << " Hw Failure protected updates should be non coalescing";
  }

  // This function should never be called with valid updates while we are
  // not initialized yet
  DCHECK(isInitialized());

  // Call all of the update functions to prepare the new SwitchState
  auto oldAppliedState = getState();
  // We start with the old state, and apply state updates one at a time.
  auto newDesiredState = oldAppliedState;
  auto iter = updates.begin();
  while (iter != updates.end()) {
    StateUpdate* update = &(*iter);
    ++iter;

    shared_ptr<SwitchState> intermediateState;
    XLOG(DBG2) << "preparing state update " << update->getName();
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
  // Start newAppliedState as equal to newDesiredState unless
  // we learn otherwise
  auto newAppliedState = newDesiredState;
  // Now apply the update and notify subscribers
  if (newDesiredState != oldAppliedState) {
    auto isTransaction = updates.begin()->hwFailureProtected() &&
        getHw()->transactionsSupported();
    // There was some change during these state updates
    newAppliedState =
        applyUpdate(oldAppliedState, newDesiredState, isTransaction);
    if (newDesiredState != newAppliedState) {
      if (isExiting()) {
        /*
         * If we started exit, applyUpdate will reject updates leading
         * to a mismatch b/w applied and desired states. Log, but otherwise
         * ignore this error. Ideally, we should throw this error back to
         * the callers, but during exit with threads in various state of
         * stoppage, it becomes hard to manage. Since we require a resync
         * of state post restart anyways (through WB state replay, config
         * application and FIB sync) this should not cause problems. Note
         * that this sync is not optional, but required for external clients
         * post restart - agent could COLD boot, or there could be a mismatch
         * with the HW state.
         * If we ever want to relax this requirement - viz. only require a
         * resync from external clients on COLD boot, we will need to get
         * more rigorous here.
         */
        XLOG(DBG2) << " Failed to apply updates to HW since SwSwtich already "
                      "started exit";
      } else if (updates.size() == 1 && updates.begin()->hwFailureProtected()) {
        fb303::fbData->incrementCounter(kHwUpdateFailures);
        unique_ptr<StateUpdate> update(&updates.front());
        try {
          throw FbossHwUpdateError(
              newDesiredState,
              newAppliedState,
              "Update : ",
              update->getName(),
              " application to HW failed");

        } catch (const std::exception& ex) {
          update->onError(ex);
        }
        return;
      } else {
        XLOG(FATAL)
            << " Failed to apply update to HW and the update is not marked for "
               "HW failure protection";
      }
    }
  }
  updatePtpTcCounter();
  // Notify all of the updates of success and delete them.
  while (!updates.empty()) {
    unique_ptr<StateUpdate> update(&updates.front());
    updates.pop_front();
    update->onSuccess();
  }
}

void SwSwitch::updatePtpTcCounter() {
  // update fb303 counter to reflect current state of PTP
  // should be invoked post update
  auto switchSettings = getState()->getSwitchSettings();
  fb303::fbData->setCounter(
      SwitchStats::kCounterPrefix + "ptp_tc_enabled",
      switchSettings->isPtpTcEnable() ? 1 : 0);
}

void SwSwitch::setStateInternal(std::shared_ptr<SwitchState> newAppliedState) {
  // This is one of the only two places that should ever directly access
  // stateDontUseDirectly_.  (getState() being the other one.)
  CHECK(bool(newAppliedState));
  CHECK(newAppliedState->isPublished());
  std::unique_lock guard(stateLock_);
  appliedStateDontUseDirectly_.swap(newAppliedState);
}

std::shared_ptr<SwitchState> SwSwitch::applyUpdate(
    const shared_ptr<SwitchState>& oldState,
    const shared_ptr<SwitchState>& newState,
    bool isTransaction) {
  // Check that we are starting from what has been already applied
  DCHECK_EQ(oldState, getAppliedState());

  auto start = std::chrono::steady_clock::now();
  XLOG(DBG2) << "Updating state: old_gen=" << oldState->getGeneration()
             << " new_gen=" << newState->getGeneration();
  DCHECK_GT(newState->getGeneration(), oldState->getGeneration());

  StateDelta delta(oldState, newState);

  // If we are already exiting, abort the update
  if (isExiting()) {
    XLOG(DBG2) << " Agent exiting before all updates could be applied";
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
    newAppliedState = isTransaction ? hw_->stateChangedTransaction(delta)
                                    : hw_->stateChanged(delta);
  } catch (const std::exception& ex) {
    // Notify the hw_ of the crash so it can execute any device specific
    // tasks before we fatal. An example would be to dump the current hw state.
    //
    // Another thing we could try here is rolling back to the old state.
    XLOG(ERR) << "error applying state change to hardware: "
              << folly::exceptionStr(ex);
    hw_->exitFatal();

    dumpBadStateUpdate(oldState, newState);
    XLOG(FATAL) << "encountered a fatal error: " << folly::exceptionStr(ex);
  }

  setStateInternal(newAppliedState);

  // Notifies all observers of the current state update.
  notifyStateObservers(StateDelta(oldState, newAppliedState));

  auto end = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  stats()->stateUpdate(duration);

  XLOG(DBG0) << "Update state took " << duration.count() << "us";
  return newAppliedState;
}

void SwSwitch::dumpBadStateUpdate(
    const std::shared_ptr<SwitchState>& oldState,
    const std::shared_ptr<SwitchState>& newState) const {
  // dump the previous state and target state to understand what led to the
  // crash
  utilCreateDir(platform_->getCrashBadStateUpdateDir());
  if (!dumpBinaryThriftToFile(
          platform_->getCrashBadStateUpdateOldStateFile(),
          oldState->toThrift())) {
    XLOG(ERR) << "Unable to write old switch state thrift to "
              << platform_->getCrashBadStateUpdateOldStateFile();
  }
  if (!dumpBinaryThriftToFile(
          platform_->getCrashBadStateUpdateNewStateFile(),
          newState->toThrift())) {
    XLOG(ERR) << "Unable to write new switch state thrift to "
              << platform_->getCrashBadStateUpdateNewStateFile();
  }
}

std::map<PortID, const phy::PhyInfo> SwSwitch::getIPhyInfo(
    std::vector<PortID>& portIDs) {
  return phySnapshotManager_->getPhyInfos(portIDs);
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
    XLOG(DBG0) << "Port node doesn't exist, use default name=port" << portID;
    return stats()->createPortStats(portID, folly::to<string>("port", portID));
  }
}

PortStats* SwSwitch::portStats(const RxPacket* pkt) {
  return portStats(pkt->getSrcPort());
}

InterfaceStats* SwSwitch::interfaceStats(InterfaceID intfID) {
  auto intfStats = stats()->intf(intfID);
  if (intfStats) {
    return intfStats;
  }
  auto interfaceIf = getState()->getInterfaces()->getInterfaceIf(intfID);
  if (!interfaceIf) {
    XLOG(DBG0) << "Interface node doesn't exist, use default name=intf"
               << intfID;
    return stats()->createInterfaceStats(
        intfID, folly::to<string>("intf", intfID));
  }
  return stats()->createInterfaceStats(intfID, interfaceIf->getName());
}

map<int32_t, PortStatus> SwSwitch::getPortStatus() {
  map<int32_t, PortStatus> statusMap;
  std::shared_ptr<PortMap> portMap = getState()->getPorts();
  for (const auto& p : std::as_const(*portMap)) {
    statusMap[p.second->getID()] = fillInPortStatus(*p.second, this);
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
    XLOG(ERR) << "error processing trapped packet: " << folly::exceptionStr(ex);
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
    XLOG(DBG3) << "Dropping received packets received on UNINITIALIZED switch";
    return;
  }
  PortID port = pkt->getSrcPort();
  portStats(port)->trappedPkt();

  pktObservers_->packetReceived(pkt.get());

  /*
   * The minimum required frame length for ethernet is 64 bytes.
   * Abort processing early if the packet is too short.
   * Adding an option to override this minimum length in case
   * wedge_agent is used with simulator. In this case, wedge_agent
   * using simulator can be connected to a peer linux_agent running
   * in a container with a GRE tunnel interconnecting them. An ARP
   * request/response from linux_agent will not be padded to be
   * 64B coz of the GRE header added, but as GRE tunnel is decaped
   * before the ARP packet is handed over to wedge_agent, the
   * received frame size as seen by wedge_agent will be <64B.
   */
  auto len = pkt->getLength();
  if (len < FLAGS_minimum_ethernet_packet_length) {
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

  std::stringstream ss;
  ss << "trapped packet: src_port=" << pkt->getSrcPort() << " srcAggPort="
     << (pkt->isFromAggregatePort()
             ? folly::to<string>(pkt->getSrcAggregatePort())
             : "None")
     << " vlan=" << pkt->getSrcVlan() << " length=" << len << " src=" << srcMac
     << " dst=" << dstMac << " ethertype=0x" << std::hex << ethertype
     << " :: " << pkt->describeDetails();
  XLOG(DBG5) << ss.str();
  XLOG_EVERY_N(DBG2, 10000) << "sampled " << ss.str();

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
#if FOLLY_HAS_COROUTINES
    case MKAServiceManager::ETHERTYPE_EAPOL:
      if (mkaServiceManager_) {
        portStats(port)->MkPduRecvdPkt();
        mkaServiceManager_->handlePacket(std::move(pkt));
        return;
      }
      break;
#endif
    case IPv4Handler::ETHERTYPE_IPV4:
      ipv4_->handlePacket(std::move(pkt), dstMac, srcMac, c);
      return;
    case IPv6Handler::ETHERTYPE_IPV6:
      ipv6_->handlePacket(std::move(pkt), dstMac, srcMac, c);
      return;
    case LACPDU::EtherType::SLOW_PROTOCOLS: {
      // The only supported protocol in the Ethernet suite's "Slow Protocols"
      // is the Link Aggregation Control Protocol
      auto subtype = c.readBE<uint8_t>();
      if (subtype == LACPDU::EtherSubtype::LACP) {
        if (lagManager_) {
          lagManager_->handlePacket(std::move(pkt), c);
        } else {
          LOG_EVERY_N(WARNING, 60)
              << "Received LACP frame but LACP not enabled";
        }
      } else if (subtype == LACPDU::EtherSubtype::MARKER) {
        XLOG(WARNING)
            << "Received Marker frame but Marker Protocol not supported";
      }
    }
      return;
    case (uint16_t)ETHERTYPE::ETHERTYPE_MPLS: {
      // updates the cursor
      auto hdr = MPLSHdr(&c);
      mplsHandler_->handlePacket(std::move(pkt), hdr, c);
      break;
    }
    default:
      break;
  }

  // If we are still here, we don't know what to do with this packet.
  // Increment a counter and just drop the packet on the floor.
  portStats(port)->pktUnhandled();
}

void SwSwitch::pfcWatchdogStateChanged(
    const PortID& portId,
    const bool deadlockDetected) {
  if (!isFullyInitialized()) {
    XLOG(ERR)
        << "Ignore PFC watchdog event changes before we are fully initialized...";
    return;
  }
  auto state = getState();
  if (!state) {
    // Make sure the state actually exists, this could be an issue if
    // called during initialization
    XLOG(ERR)
        << "Ignore PFC watchdog event changes, as switch state doesn't exist yet";
    return;
  }
  if (deadlockDetected) {
    portStats(portId)->pfcDeadlockDetectionCount();
  } else {
    portStats(portId)->pfcDeadlockRecoveryCount();
  }
}

void SwSwitch::linkStateChanged(
    PortID portId,
    bool up,
    std::optional<phy::LinkFaultStatus> iPhyFaultStatus) {
  if (!isFullyInitialized()) {
    XLOG(ERR)
        << "Ignore link state change event before we are fully initialized...";
    return;
  }

  // Schedule an update for port's operational status
  auto updateOperStateFn = [=](const std::shared_ptr<SwitchState>& state) {
    std::shared_ptr<SwitchState> newState(state);
    auto* port = newState->getPorts()->getPortIf(portId).get();

    if (port) {
      if (port->isUp() != up) {
        XLOG(DBG2) << "SW Link state changed: " << port->getName() << " ["
                   << (port->isUp() ? "UP" : "DOWN") << "->"
                   << (up ? "UP" : "DOWN") << "]";
        port = port->modify(&newState);
        port->setOperState(up);
        if (iPhyFaultStatus) {
          port->setIPhyLinkFaultStatus(*iPhyFaultStatus);
        }
        // Log event and update counters if there is a change
        logLinkStateEvent(portId, up);
        setPortStatusCounter(portId, up);
        portStats(portId)->linkStateChange(up);
      }
    }

    return newState;
  };
  updateStateNoCoalescing(
      "Port OperState Update", std::move(updateOperStateFn));
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
  neighborCacheThread_.reset(new std::thread([=] {
    this->threadLoop("fbossNeighborCacheThread", &neighborCacheEventBase_);
  }));
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
  if (lacpThread_) {
    lacpEventBase_.runInEventBaseThread(
        [this] { lacpEventBase_.terminateLoopSoon(); });
  }
  if (neighborCacheThread_) {
    neighborCacheEventBase_.runInEventBaseThread(
        [this] { neighborCacheEventBase_.terminateLoopSoon(); });
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
  if (lacpThread_) {
    lacpThread_->join();
  }
  if (neighborCacheThread_) {
    neighborCacheThread_->join();
  }
  // Drain any pending updates by calling handlePendingUpdates. Since
  // we already set state to EXITING, handlePendingUpdates will simply
  // signal the updates and not apply them to HW.
  bool updatesDrained = false;
  do {
    handlePendingUpdates();
    {
      std::unique_lock guard(pendingUpdatesLock_);
      updatesDrained = pendingUpdates_.empty();
    }
  } while (!updatesDrained);

  platform_->stop();
}

void SwSwitch::threadLoop(StringPiece name, EventBase* eventBase) {
  initThread(name);
  eventBase->loopForever();
}

uint32_t SwSwitch::getEthernetHeaderSize() const {
  // VOQ/Fabric switches require that the packets are not VLAN tagged.
  return getSwitchInfoTable().vlansSupported() ? EthHdr::SIZE
                                               : EthHdr::UNTAGGED_PKT_SIZE;
}

std::unique_ptr<TxPacket> SwSwitch::allocatePacket(uint32_t size) {
  return hw_->allocatePacket(size);
}

std::unique_ptr<TxPacket> SwSwitch::allocateL3TxPacket(uint32_t l3Len) {
  const uint32_t l2Len = getEthernetHeaderSize();
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

void SwSwitch::sendNetworkControlPacketAsync(
    std::unique_ptr<TxPacket> pkt,
    std::optional<PortDescriptor> port) noexcept {
  if (port) {
    // TODO(joseph5wu): Control this by distinguishing the highest priority
    // queue from the config.
    static const uint8_t kNCStrictPriorityQueue = 7;
    auto portVal = *port;
    switch (portVal.type()) {
      case PortDescriptor::PortType::PHYSICAL:
        sendPacketOutOfPortAsync(
            std::move(pkt), portVal.phyPortID(), kNCStrictPriorityQueue);
        break;
      case PortDescriptor::PortType::AGGREGATE:
        sendPacketOutOfPortAsync(
            std::move(pkt), portVal.aggPortID(), kNCStrictPriorityQueue);
        break;
      case PortDescriptor::PortType::SYSTEM_PORT:
        XLOG(FATAL) << " Packet send over system ports not handled yet";
        break;
    };
  } else {
    this->sendPacketSwitchedAsync(std::move(pkt));
  }
}

void SwSwitch::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  auto state = getState();
  if (!state->getPorts()->getPortIf(portID)) {
    XLOG(ERR) << "SendPacketOutOfPortAsync: dropping packet to unexpected port "
              << portID;
    stats()->pktDropped();
    return;
  }

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

  if (!hw_->sendPacketOutOfPortAsync(std::move(pkt), portID, queue)) {
    // Just log an error for now.  There's not much the caller can do about
    // send failures--even on successful return from sendPacket*() the
    // send may ultimately fail since it occurs asynchronously in the
    // background.
    XLOG(ERR) << "failed to send packet out port " << portID;
  }
}

void SwSwitch::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    AggregatePortID aggPortID,
    std::optional<uint8_t> queue) noexcept {
  auto aggPort = getState()->getAggregatePorts()->getAggregatePortIf(aggPortID);
  if (!aggPort) {
    XLOG(ERR) << "failed to send packet out aggregate port " << aggPortID
              << ": no aggregate port corresponding to identifier";
    return;
  }

  auto subportAndFwdStates = aggPort->subportAndFwdState();
  if (subportAndFwdStates.begin() == subportAndFwdStates.end()) {
    XLOG(ERR) << "failed to send packet out aggregate port " << aggPortID
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
      sendPacketOutOfPortAsync(std::move(pkt), subport, queue);
      return;
    }
  }
  XLOG(DBG2) << "failed to send packet out aggregate port" << aggPortID
             << ": aggregate port has no enabled physical ports";
}

void SwSwitch::sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept {
  pcapMgr_->packetSent(pkt.get());
  if (!hw_->sendPacketSwitchedAsync(std::move(pkt))) {
    // Just log an error for now.  There's not much the caller can do about
    // send failures--even on successful return from sendPacketSwitchedAsync()
    // the send may ultimately fail since it occurs asynchronously in the
    // background.
    XLOG(ERR) << "failed to send L2 switched packet";
  }
}

void SwSwitch::sendL3Packet(
    std::unique_ptr<TxPacket> pkt,
    std::optional<InterfaceID> maybeIfID) noexcept {
  if (!isFullyInitialized()) {
    XLOG(DBG2) << " Dropping L3 packet since device not yet initialized";
    stats()->pktDropped();
    return;
  }

  // Buffer should not be shared.
  folly::IOBuf* buf = pkt->buf();
  CHECK(!buf->isShared());

  // Add L2 header to L3 packet. Information doesn't need to be complete
  // make sure the packet has enough headroom for L2 header and large enough
  // for the minimum size packet.
  const uint32_t l2Len = getEthernetHeaderSize();
  const uint32_t l3Len = buf->length();
  const uint32_t minLen = 68;
  uint32_t tailRoom = (l2Len + l3Len >= minLen) ? 0 : minLen - l2Len - l3Len;
  if (buf->headroom() < l2Len || buf->tailroom() < tailRoom) {
    XLOG(ERR) << "Packet is not big enough headroom=" << buf->headroom()
              << " required=" << l2Len << ", tailroom=" << buf->tailroom()
              << " required=" << tailRoom;
    stats()->pktError();
    return;
  }

  auto state = getState();

  // Get VlanID associated with interface
  auto vlanID = getCPUVlan();
  if (maybeIfID.has_value()) {
    auto intf = state->getInterfaces()->getInterfaceIf(*maybeIfID);
    if (!intf) {
      XLOG(ERR) << "Interface " << *maybeIfID << " doesn't exists in state.";
      stats()->pktDropped();
      return;
    }

    // Extract primary Vlan associated with this interface, if any
    vlanID = intf->getVlanIDIf();
  }

  try {
    uint16_t protocol{0};
    folly::IPAddress dstAddr;

    // Parse L3 header to identify IP-Protocol and dstAddr
    folly::io::Cursor cursor(buf);
    uint8_t protoVersion = cursor.read<uint8_t>() >> 4;
    cursor.reset(buf); // Make cursor point to beginning again
    if (protoVersion == 4) {
      protocol = IPv4Handler::ETHERTYPE_IPV4;
      IPv4Hdr ipHdr(cursor);
      dstAddr = ipHdr.dstAddr;
    } else if (protoVersion == 6) {
      protocol = IPv6Handler::ETHERTYPE_IPV6;
      IPv6Hdr ipHdr(cursor);
      dstAddr = ipHdr.dstAddr;
    } else {
      throw FbossError(
          "Wrong version number ",
          static_cast<int>(protoVersion),
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
      auto vlan = state->getVlans()->getVlan(getVlanIDHelper(vlanID));
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
          IPv6Handler::sendMulticastNeighborSolicitation(
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
        ipv6_->sendMulticastNeighborSolicitations(PortID(0), dstAddr.asV6());
      } else {
        // TODO(skhare) Support optional VLAN arg to resolveMac
        ipv4_->resolveMac(state, PortID(0), dstAddr.asV4(), vlanID.value());
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
    sendPacketSwitchedAsync(std::move(pkt));
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to send out L3 packet :" << folly::exceptionStr(ex);
  }
}

bool SwSwitch::sendPacketToHost(
    InterfaceID dstIfID,
    std::unique_ptr<RxPacket> pkt) {
  return tunMgr_ && tunMgr_->sendPacketToHost(dstIfID, std::move(pkt));
}

void SwSwitch::applyConfig(const std::string& reason, bool reload) {
  auto target = reload ? platform_->reloadConfig() : platform_->config();
  const auto& newConfig = *target->thrift.sw();
  applyConfig(reason, newConfig);
  target->dumpConfig(platform_->getRunningConfigDumpFile());
}

void SwSwitch::applyConfig(
    const std::string& reason,
    const cfg::SwitchConfig& newConfig) {
  // We don't need to hold a lock here. updateStateBlocking() does that for us.
  auto routeUpdater = getRouteUpdater();
  auto oldConfig = getConfig();
  updateStateBlocking(
      reason,
      [&](const shared_ptr<SwitchState>& state) -> shared_ptr<SwitchState> {
        auto originalState = state;
        // Eventually we'll allow qsfp_service to call programInternalPhyPorts
        // with specific TransceiverInfo, and then build the TransceiverMap from
        // there. Before we can enable qsfp_service to do that, we need to have
        // wedge_agent use TrransceiverMap to build Port::profileConfig and
        // pinConfigs. To do so, we need to manually build this TransceiverMap
        // by using QsfpCache to fetch all transceiver infos.
        auto qsfpCache = getPlatform()->getQsfpCache();
        if (qsfpCache) {
          const auto& currentTcvrs = qsfpCache->getAllTransceivers();
          auto tempState = SwitchState::modifyTransceivers(state, currentTcvrs);
          if (tempState) {
            originalState = tempState;
          }
        } else {
          XLOG(WARN) << "Current platform doesn't have QsfpCache. "
                     << "No need to build TransceiverMap";
        }
        auto newState = rib_ ? applyThriftConfig(
                                   originalState,
                                   &newConfig,
                                   getPlatform(),
                                   &routeUpdater,
                                   aclNexthopHandler_.get())
                             : applyThriftConfig(
                                   originalState,
                                   &newConfig,
                                   getPlatform(),
                                   (RoutingInformationBase*)nullptr,
                                   aclNexthopHandler_.get());

        if (newState && !isValidStateUpdate(StateDelta(state, newState))) {
          throw FbossError("Invalid config passed in, skipping");
        }

        // Update config cached in SwSwitch. Update this even if the config did
        // not change (as this might be during warmboot).
        curConfig_ = newConfig;
        curConfigStr_ =
            apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                newConfig);

        if (!newState) {
          // if config is not updated, the new state will return null
          // in such a case return here, to prevent possible crash.
          XLOG(WARNING) << "Applying config did not cause state change";
          return nullptr;
        }
        return newState;
      });
  // Since we're using blocking state update, once we reach here, the new config
  // should be already applied and programmed into hardware.
  updateConfigAppliedInfo();

  /*
   * For RIB always make route programming go through the routeUpdater wrapper
   * abstraction. For rib when routes are applied 2 things happen
   *  - Work is scheduled on rib thread
   *  - Fib update callback is called to program routes. This in turn (for
   *  SwSwitch) *may* schedule updates on SwSwitch::update thread.
   *
   * If we don't use RouteUpdateWrapper flow for config application becomes
   * - schedule a blocking update on SwSwitch::update thread
   *   - schedule work on RIB thread.
   *
   * This is classic lock/blocking work inversion, and creates a deadlock
   * scenario b/w route programming (route add, del or route classID update) and
   * applyConfig. So ensure programming allways goes through the route update
   * wrapper abstraction
   */

  routeUpdater.program();
  if (fsdbSyncer_) {
    // TODO - figure out a way to send full agent config
    fsdbSyncer_->cfgUpdated(oldConfig, newConfig);
  }
}

void SwSwitch::updateConfigAppliedInfo() {
  auto lockedConfigAppliedInfo = configAppliedInfo_.wlock();
  auto currentInMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());

  lockedConfigAppliedInfo->lastAppliedInMs() = currentInMs.count();
  // Only need to update `lastColdbootAppliedInMs` once if there's a coldboot
  // since the recent agent restarts
  if (!lockedConfigAppliedInfo->lastColdbootAppliedInMs() &&
      bootType_ == BootType::COLD_BOOT) {
    lockedConfigAppliedInfo->lastColdbootAppliedInMs() = currentInMs.count();
  }

  XLOG(DBG2) << "Finished applied config, lastConfigAppliedInMs="
             << *lockedConfigAppliedInfo->lastAppliedInMs()
             << ", coldboot lastConfigAppliedInMs="
             << (lockedConfigAppliedInfo->lastColdbootAppliedInMs()
                     ? *lockedConfigAppliedInfo->lastColdbootAppliedInMs()
                     : 0);
}

bool SwSwitch::isValidStateUpdate(const StateDelta& delta) const {
  bool isValid = true;

  forEachChanged(
      delta.getAclsDelta(),
      [&](const shared_ptr<AclEntry>& /* oldAcl */,
          const shared_ptr<AclEntry>& newAcl) {
        isValid = isValid && newAcl->hasMatcher();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<AclEntry>& addAcl) {
        isValid = isValid && addAcl->hasMatcher();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<AclEntry>& /* delAcl */) {});

  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& /* oldport */,
          const shared_ptr<Port>& newport) {
        isValid = isValid && newport->hasValidPortQueues();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<Port>& addport) {
        isValid = isValid && addport->hasValidPortQueues();
        return isValid ? LoopAction::CONTINUE : LoopAction::BREAK;
      },
      [&](const shared_ptr<Port>& /* delport */) {});

  return isValid && hw_->isValidStateUpdate(delta);
}

AdminDistance SwSwitch::clientIdToAdminDistance(int clientId) const {
  auto distance = curConfig_.clientIdToAdminDistance()->find(clientId);
  if (distance == curConfig_.clientIdToAdminDistance()->end()) {
    // In case we get a client id we don't know about
    XLOG(ERR) << "No admin distance mapping available for client id "
              << clientId << ". Using default distance - MAX_ADMIN_DISTANCE";
    return AdminDistance::MAX_ADMIN_DISTANCE;
  }

  if (XLOG_IS_ON(DBG3)) {
    auto clientName = apache::thrift::util::enumNameSafe(ClientID(clientId));
    auto distanceString = apache::thrift::util::enumNameSafe(
        static_cast<AdminDistance>(distance->second));
    XLOG(DBG3) << "Mapping client id " << clientId << " (" << clientName
               << ") to admin distance " << distance->second << " ("
               << distanceString << ").";
  }

  return static_cast<AdminDistance>(distance->second);
}

void SwSwitch::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  getHw()->clearPortStats(ports);
}

std::vector<PrbsLaneStats> SwSwitch::getPortAsicPrbsStats(int32_t portId) {
  return getHw()->getPortAsicPrbsStats(portId);
}

void SwSwitch::clearPortAsicPrbsStats(int32_t portId) {
  getHw()->clearPortAsicPrbsStats(portId);
}

std::vector<prbs::PrbsPolynomial> SwSwitch::getPortPrbsPolynomials(
    int32_t portId) {
  return getHw()->getPortPrbsPolynomials(portId);
}

prbs::InterfacePrbsState SwSwitch::getPortPrbsState(PortID portId) {
  return getHw()->getPortPrbsState(portId);
}

std::vector<PrbsLaneStats> SwSwitch::getPortGearboxPrbsStats(
    int32_t portId,
    phy::Side side) {
  return getHw()->getPortGearboxPrbsStats(portId, side);
}

void SwSwitch::clearPortGearboxPrbsStats(int32_t portId, phy::Side side) {
  getHw()->clearPortGearboxPrbsStats(portId, side);
}

template <typename AddressT>
std::shared_ptr<Route<AddressT>> SwSwitch::longestMatch(
    std::shared_ptr<SwitchState> state,
    const AddressT& address,
    RouterID vrf) {
  return findLongestMatchRoute(getRib(), vrf, address, state);
}

template std::shared_ptr<Route<folly::IPAddressV4>> SwSwitch::longestMatch(
    std::shared_ptr<SwitchState> state,
    const folly::IPAddressV4& address,
    RouterID vrf);
template std::shared_ptr<Route<folly::IPAddressV6>> SwSwitch::longestMatch(
    std::shared_ptr<SwitchState> state,
    const folly::IPAddressV6& address,
    RouterID vrf);

void SwSwitch::l2LearningUpdateReceived(
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  macTableManager_->handleL2LearningUpdate(l2Entry, l2EntryUpdateType);
}

bool SwSwitch::sendArpRequestHelper(
    std::shared_ptr<Interface> intf,
    std::shared_ptr<SwitchState> state,
    folly::IPAddressV4 source,
    folly::IPAddressV4 target) {
  bool sent = false;
  auto vlanID = intf->getVlanID();
  auto vlan = state->getVlans()->getVlanIf(vlanID);
  if (vlan) {
    auto entry = vlan->getArpTable()->getEntryIf(target);
    if (entry == nullptr) {
      // No entry in ARP table, send ARP request
      auto mac = intf->getMac();
      ArpHandler::sendArpRequest(this, vlanID, mac, source, target);

      // Notify the updater that we sent an arp request
      getNeighborUpdater()->sentArpRequest(vlanID, target);
      sent = true;
    } else {
      XLOG(DBG4) << "not sending arp for " << target.str() << ", "
                 << ((entry->isPending()) ? "pending " : "")
                 << "entry already exists";
    }
  }

  return sent;
}

bool SwSwitch::sendNdpSolicitationHelper(
    std::shared_ptr<Interface> intf,
    std::shared_ptr<SwitchState> state,
    const folly::IPAddressV6& target) {
  bool sent = false;
  auto vlanID = intf->getVlanID();
  auto vlan = state->getVlans()->getVlanIf(vlanID);
  if (vlan) {
    auto entry = vlan->getNdpTable()->getEntryIf(target);
    if (entry == nullptr) {
      // No entry in NDP table, create a neighbor solicitation packet
      IPv6Handler::sendMulticastNeighborSolicitation(
          this, target, intf->getMac(), vlan->getID());

      // Notify the updater that we sent a solicitation out
      getNeighborUpdater()->sentNeighborSolicitation(vlanID, target);
      sent = true;
    } else {
      XLOG(DBG5) << "not sending neighbor solicitation for " << target.str()
                 << ", " << ((entry->isPending()) ? "pending" : "")
                 << " entry already exists";
    }
  }

  return sent;
}

VlanID SwSwitch::getVlanIDHelper(std::optional<VlanID> vlanID) const {
  // if vlanID does not have value, it must be VOQ or FABRIC switch
  CHECK(vlanID.has_value() || !getSwitchInfoTable().vlansSupported());

  // TODO(skhare)
  // VOQ/Fabric switches require that the packets are not tagged with any
  // VLAN. We are gradually enhancing wedge_agent to handle tagged as well as
  // untagged packets. During this transition, we will use VlanID 0 to
  // populate SwitchState/Neighbor cache etc. data structures. Once the
  // wedge_agent changes are complete, we will no longer need this function.
  return vlanID.has_value() ? vlanID.value() : VlanID(0);
}

std::optional<VlanID> SwSwitch::getVlanIDForPkt(VlanID vlanID) const {
  // TODO(skhare)
  // VOQ/Fabric switches require that the packets are not tagged with any
  // VLAN. We are gradually enhancing wedge_agent to handle tagged as well as
  // untagged packets. During this transition, we will use VlanID 0 to
  // populate SwitchState/Neighbor cache etc. data structures.
  // However, the packets on wire must not carry VLANs for VOQ/Fabric switches.
  // Once the wedge_agent changes are complete, we will no longer need this
  // function.

  if (!getSwitchInfoTable().vlansSupported()) {
    CHECK_EQ(vlanID, VlanID(0));
    return std::nullopt;
  } else {
    return vlanID;
  }
}

std::optional<VlanID> SwSwitch::getCPUVlan() const {
  return getSwitchInfoTable().vlansSupported()
      ? std::make_optional(VlanID(4095))
      : std::nullopt;
}

InterfaceID SwSwitch::getInterfaceIDForPort(PortID portID) const {
  auto port = getState()->getPorts()->getPortIf(portID);
  CHECK(port);
  // On VOQ/Fabric switches, port and interface have 1:1 relation.
  // For non VOQ/Fabric switches, in practice, a port is always part of a
  // single VLAN (and thus single interface).
  CHECK_EQ(port->getInterfaceIDs()->size(), 1);
  return InterfaceID(port->getInterfaceIDs()->at(0)->cref());
}

} // namespace facebook::fboss
