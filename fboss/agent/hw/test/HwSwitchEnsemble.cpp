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

#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/hw/test/StaticL2ForNeighborHwSwitchUpdater.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/experimental/FunctionScheduler.h>

DEFINE_bool(
    setup_thrift,
    false,
    "Setup a thrift handler. Primarily useful for inspecting HW state,"
    "say for debugging things via a shell");

DEFINE_int32(
    thrift_port,
    5909,
    "Port for thrift server to use (use with --setup_thrift");

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
    // don't touch programmedState_ unless init is done
    // ALPM requires that the default routes (always required to be
    // present for ALPM) be deleted last. When we destroy the HwSwitch
    // and the contained routeTable, there is no notion of a *order* of
    // destruction.
    // So blow away all routes except the min required for ALPM
    // We are going to reset HwSwith anyways, so deleting routes does not
    // matter here.
    // Blowing away all routes means, blowing away 2 tables
    // - Route tables
    // - Interface addresses - for platforms where trapping packets to CPU is
    // done via interfaceToMe routes. So blow away routes and interface
    // addresses.
    auto noRoutesState{getProgrammedState()->clone()};
    auto routeTables = noRoutesState->getRouteTables()->modify(&noRoutesState);
    routeTables->removeRouteTable(routeTables->getRouteTable(RouterID(0)));

    auto vlans = noRoutesState->getVlans()->modify(&noRoutesState);
    for (auto& vlan : *vlans) {
      vlan->modify(&noRoutesState);
      vlan->setArpTable(std::make_shared<ArpTable>());
      vlan->setNdpTable(std::make_shared<NdpTable>());
    }

    auto newIntfMap = noRoutesState->getInterfaces()->clone();
    for (auto& interface : *newIntfMap) {
      auto newIntf = interface->clone();
      newIntf->setAddresses(Interface::Addresses{});
      newIntfMap->updateNode(newIntf);
    }
    noRoutesState->resetIntfs(newIntfMap);
    applyNewState(setupAlpmState(noRoutesState));
    // Unregister callbacks before we start destroying hwSwitch
    getHwSwitch()->unregisterCallbacks();
  }
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

std::shared_ptr<SwitchState> HwSwitchEnsemble::applyNewState(
    std::shared_ptr<SwitchState> newState) {
  if (!newState) {
    return programmedState_;
  }
  newState->publish();
  auto appliedState = newState;
  StateDelta delta(programmedState_, newState);
  {
    std::lock_guard<std::mutex> lk(updateStateMutex_);
    programmedState_ = getHwSwitch()->stateChanged(delta);
    programmedState_->publish();
    // We are about to give up the lock, cache programmedState
    // applied by this function invocation
    appliedState = programmedState_;
  }
  if (!allowPartialStateApplication_) {
    // Assert that our desired state was applied exactly
    CHECK_EQ(newState, appliedState);
  }
  StaticL2ForNeighborHwSwitchUpdater updater(this);
  updater.stateUpdated(StateDelta(delta.oldState(), appliedState));
  return appliedState;
}

void HwSwitchEnsemble::applyInitialConfig(const cfg::SwitchConfig& initCfg) {
  CHECK(haveFeature(HwSwitchEnsemble::LINKSCAN))
      << "Link scan feature must be enabled for exercising "
      << "applyInitialConfig";
  linkToggler_->applyInitialConfig(
      getProgrammedState(), getPlatform(), initCfg);
  switchRunStateChanged(SwitchRunState::CONFIGURED);
}

void HwSwitchEnsemble::linkStateChanged(PortID port, bool up) {
  linkToggler_->linkStateChanged(port, up);
  auto hwEventObservers = hwEventObservers_.rlock();
  std::for_each(
      hwEventObservers->begin(),
      hwEventObservers->end(),
      [port, up](auto observer) { observer->linkStateChanged(port, up); });
}

void HwSwitchEnsemble::packetReceived(std::unique_ptr<RxPacket> pkt) noexcept {
  auto hwEventObservers = hwEventObservers_.rlock();
  std::for_each(
      hwEventObservers->begin(),
      hwEventObservers->end(),
      [&pkt](auto observer) { observer->packetReceived(pkt.get()); });
}

void HwSwitchEnsemble::l2LearningUpdateReceived(
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  auto hwEventObservers = hwEventObservers_.rlock();
  std::for_each(
      hwEventObservers->begin(),
      hwEventObservers->end(),
      [l2Entry, l2EntryUpdateType](auto observer) {
        observer->l2LearningUpdateReceived(l2Entry, l2EntryUpdateType);
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

bool HwSwitchEnsemble::ensureSendPacketSwitched(std::unique_ptr<TxPacket> pkt) {
  auto originalPortStats = getLatestPortStats(masterLogicalPortIds());
  bool result = getHwSwitch()->sendPacketSwitchedSync(std::move(pkt));
  return result && waitForAnyPorAndQueutOutBytesIncrement(originalPortStats);
}

bool HwSwitchEnsemble::ensureSendPacketOutOfPort(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) {
  auto originalPortStats = getLatestPortStats(masterLogicalPortIds());
  bool result =
      getHwSwitch()->sendPacketOutOfPortSync(std::move(pkt), portID, queue);
  return result && waitForAnyPorAndQueutOutBytesIncrement(originalPortStats);
}

bool HwSwitchEnsemble::waitPortStatsCondition(
    std::function<bool(const std::map<PortID, HwPortStats>&)> conditionFn,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) {
  auto newPortStats = getLatestPortStats(masterLogicalPortIds());
  while (retries--) {
    // TODO(borisb): exponential backoff!
    if (conditionFn(newPortStats)) {
      return true;
    }
    std::this_thread::sleep_for(msBetweenRetry);
    newPortStats = getLatestPortStats(masterLogicalPortIds());
  }
  XLOG(DBG3) << "Awaited port stats condition was never satisfied";
  return false;
}

HwPortStats HwSwitchEnsemble::getLatestPortStats(PortID port) {
  return getLatestPortStats(std::vector<PortID>{port})[port];
}

bool HwSwitchEnsemble::waitForAnyPorAndQueutOutBytesIncrement(
    const std::map<PortID, HwPortStats>& originalPortStats) {
  auto queueStatsSupported =
      platform_->getAsic()->isSupported(HwAsic::Feature::L3_QOS);
  auto conditionFn = [&originalPortStats,
                      queueStatsSupported](const auto& newPortStats) {
    for (const auto& [portId, portStat] : originalPortStats) {
      auto newPortStatItr = newPortStats.find(portId);
      if (newPortStatItr != newPortStats.end()) {
        if (*newPortStatItr->second.outBytes__ref() >
            portStat.outBytes__ref()) {
          // Wait for queue stat increment if queues are supported
          // on this platform
          if (!queueStatsSupported ||
              std::any_of(
                  portStat.queueOutBytes__ref()->begin(),
                  portStat.queueOutBytes__ref()->end(),
                  [newPortStatItr](auto queueAndBytes) {
                    auto [qid, oldQbytes] = queueAndBytes;
                    const auto newQueueStats =
                        newPortStatItr->second.queueOutBytes__ref();
                    return newQueueStats->find(qid)->second > oldQbytes;
                  })) {
            return true;
          }
        }
      }
    }
    XLOG(DBG3) << "No port stats increased yet";
    return false;
  };
  return waitPortStatsCondition(conditionFn);
}

void HwSwitchEnsemble::setupEnsemble(
    std::unique_ptr<Platform> platform,
    std::unique_ptr<HwLinkStateToggler> linkToggler,
    std::unique_ptr<std::thread> thriftThread) {
  platform_ = std::move(platform);
  linkToggler_ = std::move(linkToggler);

  programmedState_ = getHwSwitch()->init(this).switchState;
  // HwSwitch::init() returns an unpublished programmedState_.  SwSwitch is
  // normally responsible for publishing it.  Go ahead and call publish now.
  // This will catch errors if test cases accidentally try to modify this
  // programmedState_ without first cloning it.
  programmedState_->publish();
  StaticL2ForNeighborHwSwitchUpdater updater(this);
  updater.stateUpdated(
      StateDelta(std::make_shared<SwitchState>(), programmedState_));

  routingInformationBase_ = std::make_unique<rib::RoutingInformationBase>();

  // Handle ALPM state. ALPM requires that default routes be programmed
  // before any other routes. We handle that setup here. Similarly ALPM
  // requires that default routes be deleted last. That aspect is handled
  // in TearDown
  auto alpmState = setupAlpmState(programmedState_);
  if (alpmState) {
    applyNewState(alpmState);
  }

  thriftThread_ = std::move(thriftThread);
  switchRunStateChanged(SwitchRunState::INITIALIZED);
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

void HwSwitchEnsemble::gracefulExit() {
  if (thriftThread_) {
    // Join thrif thread. Thrift calls will fail post
    // warm boot exit sequence initiated below
    thriftThread_->join();
  }
  if (fs_) {
    fs_->shutdown();
  }
  // Initiate warm boot
  folly::dynamic switchState = folly::dynamic::object;
  getHwSwitch()->unregisterCallbacks();
  switchState[kSwSwitch] = getProgrammedState()->toFollyDynamic();
  getHwSwitch()->gracefulExit(switchState);
}

void HwSwitchEnsemble::waitForLineRateOnPort(PortID port) {
  auto portSpeedBps =
      static_cast<uint64_t>(programmedState_->getPort(port)->getSpeed()) *
      1000 * 1000;
  auto retries = 5;
  while (retries--) {
    const auto prevPortStats = getLatestPortStats(port);
    auto prevPortBytes = *prevPortStats.outBytes__ref();
    auto prevPortPackets =
        (*prevPortStats.outUnicastPkts__ref() +
         *prevPortStats.outMulticastPkts__ref() +
         *prevPortStats.outBroadcastPkts__ref());
    sleep(1);
    const auto curPortStats = getLatestPortStats(port);
    auto curPortPackets =
        (*curPortStats.outUnicastPkts__ref() +
         *curPortStats.outMulticastPkts__ref() +
         *curPortStats.outBroadcastPkts__ref());
    // 20 bytes are consumed by ethernet preamble, start of frame and
    // interpacket gap. Account for that in linerate.
    auto packetPaddingBytes = (curPortPackets - prevPortPackets) * 20;
    auto curPortBytes = *curPortStats.outBytes__ref() + packetPaddingBytes;
    if (((curPortBytes - prevPortBytes) * 8) >= portSpeedBps) {
      return;
    }
  }
  throw FbossError("Line rate was never reached");
}

HwSwitchEnsemble::Features HwSwitchEnsemble::getAllFeatures() {
  return {HwSwitchEnsemble::LINKSCAN,
          HwSwitchEnsemble::PACKET_RX,
          HwSwitchEnsemble::STATS_COLLECTION};
}

void HwSwitchEnsemble::ensureThrift() {
  if (!thriftThread_) {
    thriftThread_ = setupThrift();
  }
}
} // namespace facebook::fboss
