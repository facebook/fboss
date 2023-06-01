/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestUtils.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/EncapIndexAllocator.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/hw/test/StaticL2ForNeighborHwSwitchUpdater.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

#include <folly/experimental/FunctionScheduler.h>
#include <folly/gen/Base.h>
#include <utility>

DEFINE_bool(
    setup_thrift,
    false,
    "Setup a thrift handler. Primarily useful for inspecting HW state,"
    "say for debugging things via a shell");

DEFINE_int32(
    thrift_port,
    5909,
    "Port for thrift server to use (use with --setup_thrift");

DEFINE_bool(mmu_lossless_mode, false, "Enable mmu lossless mode");
DEFINE_bool(
    qgroup_guarantee_enable,
    false,
    "Enable setting of unicast and multicast queue guaranteed buffer sizes");
DEFINE_bool(enable_exact_match, false, "enable init of exact match table");
DEFINE_bool(skip_buffer_reservation, false, "Enable skip reservation");

using namespace std::chrono_literals;

namespace facebook::fboss {

HwSwitchEnsemble::HwSwitchEnsemble(const Features& featuresDesired)
    : featuresDesired_(featuresDesired) {}

HwSwitchEnsemble::~HwSwitchEnsemble() {
  if (thriftThread_) {
    thriftThread_->join();
  }
  if (fs_) {
    fs_->shutdown();
  }
  if (platform_ && getHwSwitch() &&
      getHwSwitch()->getRunState() >= SwitchRunState::INITIALIZED) {
    if (platform_->getAsic()->isSupported(HwAsic::Feature::ROUTE_PROGRAMMING)) {
      auto minRouteState = getMinAlpmRouteState(getProgrammedState());
      applyNewState(minRouteState);
    }
    // Unregister callbacks before we start destroying hwSwitch
    getHwSwitch()->unregisterCallbacks();
  }
  // HwSwitch is about to go away, stop observers to let them finish any
  // in flight events.
  stopObservers();
}

uint32_t HwSwitchEnsemble::getHwSwitchFeatures() const {
  uint32_t features{0};
  for (auto feature : featuresDesired_) {
    switch (feature) {
      case LINKSCAN:
        features |= HwSwitch::LINKSCAN_DESIRED;
        break;
      case PACKET_RX:
        features |= HwSwitch::PACKET_RX_DESIRED;
        break;
      case TAM_NOTIFY:
        features |= HwSwitch::TAM_EVENT_NOTIFY_DESIRED;
        break;
      case STATS_COLLECTION:
        // No HwSwitch feture need to turned on.
        // Handled by HwSwitchEnsemble
        break;
    }
  }
  return features;
}

HwSwitch* HwSwitchEnsemble::getHwSwitch() {
  return platform_->getHwSwitch();
}

std::shared_ptr<SwitchState> HwSwitchEnsemble::getProgrammedState() const {
  CHECK(programmedState_->isPublished());
  /*
   * Acquire mutex to guard against picking up a stale programmed
   * state. A state update maybe in progress. A common pattern in
   * tests is to get the current programmedState, make changes to
   * it and then call applyNewState. If a state update is in
   * progress when you query programmedState_, you risk undoing
   * the changes of ongoing state update.
   */
  std::lock_guard<std::mutex> lk(updateStateMutex_);
  return programmedState_;
}

std::shared_ptr<SwitchState> HwSwitchEnsemble::applyNewConfig(
    const cfg::SwitchConfig& config) {
  // Mimic SwSwitch::applyConfig() to modifyTransceiverMap
  auto originalState = getProgrammedState();
  auto overrideTcvrInfos = platform_->getOverrideTransceiverInfos();
  auto qsfpCache = getPlatform()->getQsfpCache();
  if (overrideTcvrInfos || qsfpCache) {
    const auto& currentTcvrs = overrideTcvrInfos
        ? *overrideTcvrInfos
        : qsfpCache->getAllTransceivers();
    auto tempState = SwSwitch::modifyTransceivers(
        getProgrammedState(),
        currentTcvrs,
        getPlatform()->getPlatformMapping(),
        scopeResolver_.get());
    if (tempState) {
      originalState = tempState;
    }
  } else {
    XLOG(WARN) << "Current platform doesn't have QsfpCache and "
               << "OverrideTransceiverInfos. No need to build TransceiverMap";
  }

  if (routingInformationBase_) {
    auto routeUpdater = getRouteUpdater();
    applyNewState(applyThriftConfig(
        originalState,
        &config,
        getPlatform(),
        getPlatform()->getPlatformMapping(),
        hwAsicTable_.get(),
        &routeUpdater));
    routeUpdater.program();
    return getProgrammedState();
  }
  return applyNewState(applyThriftConfig(
      originalState,
      &config,
      getPlatform(),
      getPlatform()->getPlatformMapping(),
      hwAsicTable_.get()));
}

std::shared_ptr<SwitchState> HwSwitchEnsemble::updateEncapIndices(
    const std::shared_ptr<SwitchState>& in) const {
  // For wedge_agent we assign Encap indices on VOQ swtiches at
  // SwSwitch layer and later assert for their presence before
  // programming in HW.
  // For HW tests, since there is no SwSwitch layer we need to
  // explicitly assign and remove encap indices from neighbor entries
  StateDelta delta(programmedState_, in);
  return EncapIndexAllocator::updateEncapIndices(delta, *getAsic());
}

std::shared_ptr<SwitchState> HwSwitchEnsemble::applyNewStateImpl(
    const std::shared_ptr<SwitchState>& newState,
    bool transaction,
    bool disableAppliedStateVerification) {
  if (!newState) {
    return programmedState_;
  }

  newState->publish();
  auto toApply = updateEncapIndices(newState);
  toApply->publish();
  StateDelta delta(programmedState_, toApply);
  auto appliedState = toApply;
  bool applyUpdateSuccess = true;
  {
    std::lock_guard<std::mutex> lk(updateStateMutex_);
    applyUpdateSuccess = FLAGS_enable_state_oper_delta
        ? applyUpdate(delta.getOperDelta(), lk, transaction)
        : applyUpdate(delta, lk, transaction);
    // We are about to give up the lock, cache programmedState
    // applied by this function invocation
    appliedState = programmedState_;
  }
  StaticL2ForNeighborHwSwitchUpdater updater(this);
  updater.stateUpdated(delta);
  if (!disableAppliedStateVerification && !applyUpdateSuccess) {
    throw FbossHwUpdateError(toApply, appliedState);
  }
  return appliedState;
}

bool HwSwitchEnsemble::applyUpdate(
    const StateDelta& delta,
    const std::lock_guard<std::mutex>& /*lock*/,
    bool transaction) {
  programmedState_ = transaction ? getHwSwitch()->stateChangedTransaction(delta)
                                 : getHwSwitch()->stateChanged(delta);
  programmedState_->publish();
  return (delta.newState() == programmedState_);
}

bool HwSwitchEnsemble::applyUpdate(
    const fsdb::OperDelta& operDelta,
    const std::lock_guard<std::mutex>& /*lock*/,
    bool transaction) {
  auto resultOperDelta = transaction
      ? getHwSwitch()->stateChangedTransaction(operDelta)
      : getHwSwitch()->stateChanged(operDelta);
  programmedState_ = getHwSwitch()->getProgrammedState();
  programmedState_->publish();
  return resultOperDelta.changes()->empty();
}

void HwSwitchEnsemble::applyInitialConfig(const cfg::SwitchConfig& initCfg) {
  CHECK(haveFeature(HwSwitchEnsemble::LINKSCAN))
      << "Link scan feature must be enabled for exercising "
      << "applyInitialConfig";
  linkToggler_->applyInitialConfig(initCfg);
  switchRunStateChanged(SwitchRunState::CONFIGURED);
}

void HwSwitchEnsemble::linkStateChanged(
    PortID port,
    bool up,
    std::optional<phy::LinkFaultStatus> /* iPhyFaultStatus */) {
  if (getHwSwitch()->getRunState() < SwitchRunState::INITIALIZED) {
    return;
  }
  linkToggler_->linkStateChanged(port, up);
  auto hwEventObservers = hwEventObservers_.rlock();
  std::for_each(
      hwEventObservers->begin(),
      hwEventObservers->end(),
      [port, up](auto observer) { observer->changeLinkState(port, up); });
}

void HwSwitchEnsemble::packetReceived(std::unique_ptr<RxPacket> pkt) noexcept {
  auto hwEventObservers = hwEventObservers_.rlock();
  std::for_each(
      hwEventObservers->begin(),
      hwEventObservers->end(),
      [&pkt](auto observer) { observer->receivePacket(pkt.get()); });
}

void HwSwitchEnsemble::l2LearningUpdateReceived(
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  auto hwEventObservers = hwEventObservers_.rlock();
  std::for_each(
      hwEventObservers->begin(),
      hwEventObservers->end(),
      [l2Entry, l2EntryUpdateType](auto observer) {
        observer->updateL2EntryState(l2Entry, l2EntryUpdateType);
      });
}

void HwSwitchEnsemble::addHwEventObserver(HwSwitchEventObserverIf* observer) {
  auto hwEventObservers = hwEventObservers_.wlock();
  if (!hwEventObservers->insert(observer).second) {
    throw FbossError("Observer was already added");
  }
}

void HwSwitchEnsemble::removeHwEventObserver(
    HwSwitchEventObserverIf* observer) {
  auto hwEventObservers = hwEventObservers_.wlock();
  if (!hwEventObservers->erase(observer)) {
    throw FbossError("Observer erase failed");
  }
}

void HwSwitchEnsemble::createEqualDistributedUplinkDownlinks(
    const std::vector<PortID>& /*ports*/,
    std::vector<PortID>& /*uplinks*/,
    std::vector<PortID>& /*downlinks*/,
    std::vector<PortID>& /*disabled*/,
    const int /*totalLinkCount*/) {
  throw FbossError("Needs to be implemented by the derived class");
}

std::vector<SystemPortID> HwSwitchEnsemble::masterLogicalSysPortIds() const {
  std::vector<SystemPortID> sysPorts;
  if (getAsic()->getSwitchType() != cfg::SwitchType::VOQ) {
    return sysPorts;
  }
  auto switchId = getHwSwitch()->getSwitchId();
  CHECK(switchId.has_value());
  auto sysPortRange = getProgrammedState()
                          ->getDsfNodes()
                          ->getNodeIf(SwitchID(*switchId))
                          ->getSystemPortRange();
  CHECK(sysPortRange.has_value());
  for (auto port : masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})) {
    sysPorts.push_back(
        SystemPortID(*sysPortRange->minimum() + static_cast<int>(port)));
  }
  return sysPorts;
}

bool HwSwitchEnsemble::ensureSendPacketSwitched(std::unique_ptr<TxPacket> pkt) {
  // lambda that returns HwPortStats for the given port(s)
  auto getPortStats =
      [&](const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
    return getLatestPortStats(portIds);
  };
  auto getSysPortStats = [&](const std::vector<SystemPortID>& portIds)
      -> std::map<SystemPortID, HwSysPortStats> {
    return getLatestSysPortStats(portIds);
  };

  return utility::ensureSendPacketSwitched(
      getHwSwitch(),
      std::move(pkt),
      masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}),
      getPortStats,
      masterLogicalSysPortIds(),
      getSysPortStats);
}

bool HwSwitchEnsemble::ensureSendPacketOutOfPort(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) {
  // lambda that returns HwPortStats for the given port(s)
  auto getPortStats =
      [&](const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
    return getLatestPortStats(portIds);
  };
  return utility::ensureSendPacketOutOfPort(
      getHwSwitch(),
      std::move(pkt),
      portID,
      masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}),
      getPortStats,
      queue);
}

bool HwSwitchEnsemble::waitPortStatsCondition(
    std::function<bool(const std::map<PortID, HwPortStats>&)> conditionFn,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) {
  // lambda that returns HwPortStats for the given port(s)
  auto getPortStatsFn =
      [&](const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
    return getLatestPortStats(portIds);
  };
  return utility::waitPortStatsCondition(
      conditionFn,
      masterLogicalPortIds(),
      retries,
      msBetweenRetry,
      getPortStatsFn);
}

bool HwSwitchEnsemble::waitStatsCondition(
    const std::function<bool()>& conditionFn,
    const std::function<void()>& updateStatsFn,
    uint32_t retries,
    const std::chrono::duration<uint32_t, std::milli> msBetweenRetry) {
  return utility::waitStatsCondition(
      conditionFn, updateStatsFn, retries, msBetweenRetry);
}

HwPortStats HwSwitchEnsemble::getLatestPortStats(PortID port) {
  return getLatestPortStats(std::vector<PortID>{port})[port];
}

std::map<PortID, HwPortStats> HwSwitchEnsemble::getLatestPortStats(
    const std::vector<PortID>& ports) {
  std::map<PortID, HwPortStats> portIdStatsMap;
  SwitchStats dummy{};
  getHwSwitch()->updateStats(&dummy);

  auto swState = getProgrammedState();
  auto stats = getHwSwitch()->getPortStats();
  for (auto [portName, stats] : stats) {
    auto portId = swState->getPorts()->getPort(portName)->getID();
    if (std::find(ports.begin(), ports.end(), (PortID)portId) == ports.end()) {
      continue;
    }
    portIdStatsMap.emplace((PortID)portId, stats);
  }
  return portIdStatsMap;
}

HwSysPortStats HwSwitchEnsemble::getLatestSysPortStats(SystemPortID port) {
  return getLatestSysPortStats(std::vector<SystemPortID>{port})[port];
}

std::map<SystemPortID, HwSysPortStats> HwSwitchEnsemble::getLatestSysPortStats(
    const std::vector<SystemPortID>& ports) {
  std::map<SystemPortID, HwSysPortStats> portIdStatsMap;
  SwitchStats dummy{};
  getHwSwitch()->updateStats(&dummy);

  auto swState = getProgrammedState();
  auto stats = getHwSwitch()->getSysPortStats();
  for (auto [portName, stats] : stats) {
    auto portId = swState->getSystemPorts()->getSystemPort(portName)->getID();
    if (std::find(ports.begin(), ports.end(), portId) == ports.end()) {
      continue;
    }
    portIdStatsMap.emplace(portId, stats);
  }
  return portIdStatsMap;
}

HwTrunkStats HwSwitchEnsemble::getLatestAggregatePortStats(
    AggregatePortID aggregatePort) {
  return getLatestAggregatePortStats(
      std::vector<AggregatePortID>{aggregatePort})[aggregatePort];
}

void HwSwitchEnsemble::setupEnsemble(
    std::unique_ptr<Platform> platform,
    std::unique_ptr<HwLinkStateToggler> linkToggler,
    std::unique_ptr<std::thread> thriftThread,
    const HwSwitchEnsembleInitInfo& initInfo) {
  platform_ = std::move(platform);
  linkToggler_ = std::move(linkToggler);
  auto asic = platform_->getAsic();
  cfg::SwitchInfo switchInfo;
  switchInfo.switchType() = asic->getSwitchType();
  switchInfo.asicType() = asic->getAsicType();
  cfg::Range64 portIdRange;
  portIdRange.minimum() =
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN();
  portIdRange.maximum() =
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX();
  switchInfo.portIdRange() = portIdRange;
  auto switchIdToSwitchInfo = std::map<int64_t, cfg::SwitchInfo>(
      {{asic->getSwitchId() ? *asic->getSwitchId() : 0, switchInfo}});
  hwAsicTable_ = std::make_unique<HwAsicTable>(switchIdToSwitchInfo);

  auto hwInitResult = getHwSwitch()->init(
      this,
      true /*failHwCallsOnWarmboot*/,
      platform_->getAsic()->getSwitchType(),
      platform_->getAsic()->getSwitchId());

  programmedState_ = hwInitResult.switchState;
  programmedState_ = programmedState_->clone();
  auto settings = util::getFirstNodeIf(programmedState_->getSwitchSettings());
  auto newSettings = settings->modify(&programmedState_);
  newSettings->setSwitchIdToSwitchInfo(switchIdToSwitchInfo);
  routingInformationBase_ = std::move(hwInitResult.rib);
  // HwSwitch::init() returns an unpublished programmedState_.  SwSwitch is
  // normally responsible for publishing it.  Go ahead and call publish now.
  // This will catch errors if test cases accidentally try to modify this
  // programmedState_ without first cloning it.
  programmedState_->publish();
  scopeResolver_ =
      std::make_unique<SwitchIdScopeResolver>(switchIdToSwitchInfo);
  StaticL2ForNeighborHwSwitchUpdater updater(this);
  updater.stateUpdated(
      StateDelta(std::make_shared<SwitchState>(), programmedState_));

  if (platform_->getAsic()->isSupported(HwAsic::Feature::ROUTE_PROGRAMMING)) {
    // ALPM requires that default routes be programmed
    // before any other routes. We handle that setup here. Similarly ALPM
    // requires that default routes be deleted last. That aspect is handled
    // in TearDown
    getRouteUpdater().programMinAlpmState();
  }

  thriftThread_ = std::move(thriftThread);
  switchRunStateChanged(SwitchRunState::INITIALIZED);
  if (routingInformationBase_) {
    auto curProgrammedState = programmedState_;
    // Unless there is mismatched state in RIB (i.e.
    // mismatched from FIB). A empty update should not
    // change switchState. Assert that post init. Most
    // interesting case here is of state diverging post WB
    getRouteUpdater().program();
    CHECK_EQ(curProgrammedState, getProgrammedState());
  }
  // Set ConfigFactory port to default profile id map
  utility::setPortToDefaultProfileIDMap(
      getProgrammedState()->getPorts(), getPlatform());
}

void HwSwitchEnsemble::switchRunStateChanged(SwitchRunState switchState) {
  getHwSwitch()->switchRunStateChanged(switchState);
  if (switchState == SwitchRunState::CONFIGURED &&
      haveFeature(STATS_COLLECTION)) {
    fs_ = std::make_unique<folly::FunctionScheduler>();
    fs_->setThreadName("UpdateStatsThread");
    auto statsCollect = [this] {
      SwitchStats dummy;
      getHwSwitch()->updateStats(&dummy);
    };
    auto timeInterval = std::chrono::seconds(1);
    fs_->addFunction(statsCollect, timeInterval, "updateStats");
    fs_->start();
  }
}

std::tuple<folly::dynamic, state::WarmbootState>
HwSwitchEnsemble::gracefulExitState() const {
  folly::dynamic follySwitchState = folly::dynamic::object;
  state::WarmbootState thriftSwitchState;

  // For RIB we employ a optmization to serialize only unresolved routes
  // and recover others from FIB
  if (routingInformationBase_) {
    // For RIB we employ a optmization to serialize only unresolved routes
    // and recover others from FIB
    thriftSwitchState.routeTables() = routingInformationBase_->warmBootState();
  }
  *thriftSwitchState.swSwitchState() = getProgrammedState()->toThrift();
  return std::make_tuple(follySwitchState, thriftSwitchState);
}

void HwSwitchEnsemble::stopObservers() {
  auto hwEventObservers = hwEventObservers_.rlock();
  std::for_each(
      hwEventObservers->begin(), hwEventObservers->end(), [](auto observer) {
        observer->stopObserving();
      });
}

void HwSwitchEnsemble::gracefulExit() {
  if (thriftThread_) {
    // Join thrift thread. Thrift calls will fail post
    // warm boot exit sequence initiated below
    thriftThread_->join();
  }
  if (fs_) {
    fs_->shutdown();
  }
  // Initiate warm boot
  getHwSwitch()->unregisterCallbacks();
  stopObservers();
  auto [follySwitchState, thriftSwitchState] = gracefulExitState();
  getHwSwitch()->gracefulExit(follySwitchState, thriftSwitchState);
}

/*
 * Wait for traffic on port to reach specified rate. If the
 * specified rate is reached, return true, else false.
 */
bool HwSwitchEnsemble::waitForRateOnPort(
    PortID port,
    const uint64_t desiredBps,
    int secondsToWaitPerIteration) {
  // Need to wait for atleast one second
  if (secondsToWaitPerIteration < 1) {
    secondsToWaitPerIteration = 1;
    XLOG(WARNING) << "Setting wait time to 1 second for tests!";
  }

  const auto portSpeedBps =
      static_cast<uint64_t>(
          programmedState_->getPorts()->getNodeIf(port)->getSpeed()) *
      1000 * 1000;
  if (desiredBps > portSpeedBps) {
    // Cannot achieve higher than line rate
    XLOG(ERR) << "Desired rate " << desiredBps << " bps is > port rate "
              << portSpeedBps << " bps!!";
    return false;
  }

  auto retries = 5;
  while (retries--) {
    const auto prevPortStats = getLatestPortStats(port);
    auto prevPortBytes = *prevPortStats.outBytes_();
    auto prevPortPackets =
        (*prevPortStats.outUnicastPkts_() + *prevPortStats.outMulticastPkts_() +
         *prevPortStats.outBroadcastPkts_());

    sleep(secondsToWaitPerIteration);
    const auto curPortStats = getLatestPortStats(port);
    auto curPortPackets =
        (*curPortStats.outUnicastPkts_() + *curPortStats.outMulticastPkts_() +
         *curPortStats.outBroadcastPkts_());

    // 20 bytes are consumed by ethernet preamble, start of frame and
    // interpacket gap. Account for that in linerate.
    auto packetPaddingBytes = (curPortPackets - prevPortPackets) * 20;
    auto curPortBytes = *curPortStats.outBytes_() + packetPaddingBytes;
    auto rate = static_cast<uint64_t>((curPortBytes - prevPortBytes) * 8) /
        secondsToWaitPerIteration;
    if (rate >= desiredBps) {
      XLOG(DBG0) << ": Current rate " << rate << " bps!";
      return true;
    } else {
      XLOG(WARNING) << ": Current rate " << rate << " bps < expected rate "
                    << desiredBps << " bps";
    }
  }
  return false;
}

void HwSwitchEnsemble::waitForLineRateOnPort(PortID port) {
  const auto portSpeedBps =
      static_cast<uint64_t>(
          programmedState_->getPorts()->getNodeIf(port)->getSpeed()) *
      1000 * 1000;
  if (waitForRateOnPort(port, portSpeedBps)) {
    // Traffic on port reached line rate!
    return;
  }
  throw FbossError("Line rate was never reached");
}

void HwSwitchEnsemble::waitForSpecificRateOnPort(
    PortID port,
    const uint64_t desiredBps,
    int secondsToWaitPerIteration) {
  if (waitForRateOnPort(port, desiredBps, secondsToWaitPerIteration)) {
    // Traffic on port reached desired rate!
    return;
  }

  throw FbossError("Desired rate ", desiredBps, " bps was never reached");
}

HwSwitchEnsemble::Features HwSwitchEnsemble::getAllFeatures() {
  return {
      HwSwitchEnsemble::LINKSCAN,
      HwSwitchEnsemble::PACKET_RX,
      HwSwitchEnsemble::STATS_COLLECTION};
}

void HwSwitchEnsemble::ensureThrift() {
  if (!thriftThread_) {
    thriftThread_ = setupThrift();
  }
}

size_t HwSwitchEnsemble::getMinPktsForLineRate(const PortID& port) {
  auto portSpeed = programmedState_->getPorts()->getNodeIf(port)->getSpeed();
  return (portSpeed > cfg::PortSpeed::HUNDREDG ? 1000 : 100);
}

void HwSwitchEnsemble::addOrUpdateCounter(
    const PortID& port,
    const bool deadlock) {
  auto& watchdogCounterMap =
      deadlock ? watchdogDeadlockCounter_ : watchdogRecoveryCounter_;
  auto iter = watchdogCounterMap.find(port);
  if (iter != watchdogCounterMap.end()) {
    (*iter).second += 1;
  } else {
    watchdogCounterMap.insert({port, 1});
  }
}

void HwSwitchEnsemble::pfcWatchdogStateChanged(
    const PortID& port,
    const bool deadlock) {
  addOrUpdateCounter(port, deadlock);
}

int HwSwitchEnsemble::readPfcDeadlockDetectionCounter(const PortID& port) {
  return readPfcWatchdogCounter(port, true);
}

int HwSwitchEnsemble::readPfcDeadlockRecoveryCounter(const PortID& port) {
  return readPfcWatchdogCounter(port, false);
}

int HwSwitchEnsemble::readPfcWatchdogCounter(
    const PortID& port,
    const bool deadlock) {
  auto& watchdogCounterMap =
      deadlock ? watchdogDeadlockCounter_ : watchdogRecoveryCounter_;
  auto iter = watchdogCounterMap.find(port);
  if (iter != watchdogCounterMap.end()) {
    return (*iter).second;
  }
  return 0;
}

void HwSwitchEnsemble::clearPfcDeadlockRecoveryCounter(const PortID& port) {
  clearPfcWatchdogCounter(port, false);
}

void HwSwitchEnsemble::clearPfcDeadlockDetectionCounter(const PortID& port) {
  clearPfcWatchdogCounter(port, true);
}

void HwSwitchEnsemble::clearPfcWatchdogCounter(
    const PortID& port,
    const bool deadlock) {
  auto& watchdogCounterMap =
      deadlock ? watchdogDeadlockCounter_ : watchdogRecoveryCounter_;
  auto iter = watchdogCounterMap.find(port);
  if (iter != watchdogCounterMap.end()) {
    watchdogCounterMap[port] = 0;
  }
}

const SwitchIdScopeResolver& HwSwitchEnsemble::scopeResolver() const {
  CHECK(scopeResolver_);
  return *scopeResolver_;
}
} // namespace facebook::fboss
