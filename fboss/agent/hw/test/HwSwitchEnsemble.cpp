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
#include "fboss/agent/state/SwitchState.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

#include <folly/experimental/FunctionScheduler.h>
#include <folly/gen/Base.h>

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
    auto tempState =
        SwitchState::modifyTransceivers(getProgrammedState(), currentTcvrs);
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
        originalState, &config, getPlatform(), &routeUpdater));
    routeUpdater.program();
    return getProgrammedState();
  }
  return applyNewState(
      applyThriftConfig(originalState, &config, getPlatform()));
}

std::shared_ptr<SwitchState> HwSwitchEnsemble::updateEncapIndices(
    const std::shared_ptr<SwitchState>& in) const {
  // For wedge_agent we assign Encap indices on VOQ swtiches at
  // SwSwitch layer and later assert for their presence before
  // programming in HW.
  // For HW tests, since there is no SwSwitch layer we need to
  // explicitly assign and remove encap indices from neighbor entries
  // This function's charter is
  // - Assign encap index to reachable nbr entries (if one is not already
  //  assigned)
  // - Clear encap index from unreachable/pending entries
  auto newState = in->clone();
  StateDelta delta(programmedState_, in);
  if (getAsic()->isSupported(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
    auto handleNewNbr = [this, &newState](auto newNbr) {
      if (!newNbr->isPending() && !newNbr->getEncapIndex()) {
        auto nbr = newNbr->clone();
        nbr->setEncapIndex(EncapIndexAllocator::getNextAvailableEncapIdx(
            newState, *getAsic()));
        return nbr;
      }
      if (newNbr->isPending() && newNbr->getEncapIndex()) {
        auto nbr = newNbr->clone();
        nbr->setEncapIndex(std::nullopt);
        return nbr;
      }
      return std::shared_ptr<std::remove_reference_t<decltype(*newNbr)>>();
    };
    for (const auto& intfDelta : delta.getIntfsDelta()) {
      auto updateIntf =
          [&](auto newNbr, const auto& nbrTable, InterfaceID intfId) {
            auto intfMap = newState->getInterfaces()->modify(&newState);
            auto intf = intfMap->getInterface(intfId);
            if (intf->getType() != cfg::InterfaceType::SYSTEM_PORT) {
              return;
            }
            auto updatedNbr = handleNewNbr(newNbr);
            if (updatedNbr) {
              auto intfNew = intf->clone();
              auto updatedNbrTable = nbrTable->toThrift();
              if (!nbrTable->getEntry(newNbr->getIP())) {
                updatedNbrTable.insert(
                    {newNbr->getIP().str(), updatedNbr->toThrift()});
              } else {
                updatedNbrTable.erase(newNbr->getIP().str());
                updatedNbrTable.insert(
                    {newNbr->getIP().str(), updatedNbr->toThrift()});
              }
              if (newNbr->getIP().version() == 6) {
                intfNew->setNdpTable(updatedNbrTable);
              } else {
                intfNew->setArpTable(updatedNbrTable);
              }
              intfMap->updateNode(intfNew);
            }
          };
      DeltaFunctions::forEachChanged(
          intfDelta.getArpEntriesDelta(),
          [&](auto /*oldNbr*/, auto newNbr) {
            updateIntf(
                newNbr,
                intfDelta.getNew()->getArpTable(),
                intfDelta.getNew()->getID());
          },
          [&](auto newNbr) {
            updateIntf(
                newNbr,
                intfDelta.getNew()->getArpTable(),
                intfDelta.getNew()->getID());
          },
          [&](auto /*rmNbr*/) {});

      DeltaFunctions::forEachChanged(
          intfDelta.getNdpEntriesDelta(),
          [&](auto /*oldNbr*/, auto newNbr) {
            updateIntf(
                newNbr,
                intfDelta.getNew()->getNdpTable(),
                intfDelta.getNew()->getID());
          },
          [&](auto newNbr) {
            updateIntf(
                newNbr,
                intfDelta.getNew()->getNdpTable(),
                intfDelta.getNew()->getID());
          },
          [&](auto /*rmNbr*/) {});
    }
  }
  return newState;
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
  {
    std::lock_guard<std::mutex> lk(updateStateMutex_);
    programmedState_ = transaction
        ? getHwSwitch()->stateChangedTransaction(delta)
        : getHwSwitch()->stateChanged(delta);
    programmedState_->publish();
    // We are about to give up the lock, cache programmedState
    // applied by this function invocation
    appliedState = programmedState_;
  }
  StaticL2ForNeighborHwSwitchUpdater updater(this);
  updater.stateUpdated(StateDelta(delta.oldState(), appliedState));
  if (!disableAppliedStateVerification && toApply != appliedState) {
    throw FbossHwUpdateError(toApply, appliedState);
  }
  return appliedState;
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

bool HwSwitchEnsemble::ensureSendPacketSwitched(std::unique_ptr<TxPacket> pkt) {
  // lambda that returns HwPortStats for the given port(s)
  auto getPortStats =
      [&](const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
    return getLatestPortStats(portIds);
  };

  return utility::ensureSendPacketSwitched(
      getHwSwitch(), std::move(pkt), masterLogicalPortIds(), getPortStats);
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
      masterLogicalPortIds(),
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
    if (std::find(ports.begin(), ports.end(), (SystemPortID)portId) ==
        ports.end()) {
      continue;
    }
    portIdStatsMap.emplace((SystemPortID)portId, stats);
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

  auto hwInitResult = getHwSwitch()->init(
      this,
      true /*failHwCallsOnWarmboot*/,
      platform_->getAsic()->getSwitchType(),
      platform_->getAsic()->getSwitchId());

  programmedState_ = hwInitResult.switchState;
  routingInformationBase_ = std::move(hwInitResult.rib);
  // HwSwitch::init() returns an unpublished programmedState_.  SwSwitch is
  // normally responsible for publishing it.  Go ahead and call publish now.
  // This will catch errors if test cases accidentally try to modify this
  // programmedState_ without first cloning it.
  programmedState_->publish();
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
  // For RIB we employ a optmization to serialize only unresolved routes
  // and recover others from FIB
  if (routingInformationBase_) {
    // For RIB we employ a optmization to serialize only unresolved routes
    // and recover others from FIB
    follySwitchState[kRib] =
        routingInformationBase_->unresolvedRoutesFollyDynamic();
  }
  // Only dump swSwitchState in thrift
  state::WarmbootState thriftSwitchState;
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
      static_cast<uint64_t>(programmedState_->getPort(port)->getSpeed()) *
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
      static_cast<uint64_t>(programmedState_->getPort(port)->getSpeed()) *
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
  auto portSpeed = programmedState_->getPort(port)->getSpeed();
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

std::vector<PortID> HwSwitchEnsemble::filterByPortTypes(
    const std::set<cfg::PortType>& filter,
    const std::vector<PortID>& portIDs) const {
  std::vector<PortID> filteredPortIDs;

  folly::gen::from(portIDs) |
      folly::gen::filter([this, filter](const auto& portID) {
        if (filter.empty()) {
          // if no filter is requested, allow all
          return true;
        }
        auto portType =
            getProgrammedState()->getPorts()->getPort(portID)->getPortType();

        return filter.find(portType) != filter.end();
      }) |
      folly::gen::appendTo(filteredPortIDs);

  return filteredPortIDs;
}

} // namespace facebook::fboss
