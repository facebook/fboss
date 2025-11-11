/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/benchmarks/HwRouteScaleBenchmarkHelpers.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

#include <folly/Benchmark.h>
#include <folly/Synchronized.h>
#include <chrono>
#include <thread>

namespace facebook::fboss {

class InterfaceMapUpdateScheduler {
 public:
  struct InterfaceMapUpdate {
    std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Intfs;
  };

  explicit InterfaceMapUpdateScheduler(
      FbossEventBase* updateEvb,
      std::function<void(const InterfaceMapUpdate&)> applyUpdateFn)
      : updateEvb_(updateEvb), applyUpdateFn_(std::move(applyUpdateFn)) {}

  void queueInterfaceMapUpdate(InterfaceMapUpdate&& update) {
    bool needsScheduling = false;

    {
      auto nextUpdateWlock = nextUpdate_.wlock();
      needsScheduling = (*nextUpdateWlock == nullptr);
      *nextUpdateWlock =
          std::make_unique<InterfaceMapUpdate>(std::move(update));
    }

    if (needsScheduling) {
      updateEvb_->runInFbossEventBaseThread([this]() {
        InterfaceMapUpdate update;
        {
          auto nextUpdateWlock = nextUpdate_.wlock();
          if (*nextUpdateWlock == nullptr) {
            return;
          }
          update = std::move(**nextUpdateWlock);
          nextUpdateWlock->reset();
        }
        applyUpdateFn_(update);
      });
    }
  }

 private:
  FbossEventBase* updateEvb_;
  std::function<void(const InterfaceMapUpdate&)> applyUpdateFn_;
  folly::Synchronized<std::unique_ptr<InterfaceMapUpdate>> nextUpdate_;
};

class PerSwitchInterfaceMapScheduler {
 public:
  PerSwitchInterfaceMapScheduler(
      SwitchID switchId,
      std::shared_ptr<InterfaceMap> intfsWithNeighbor,
      std::shared_ptr<SystemPortMap> systemPorts,
      AgentEnsemble* ensemble,
      int nbrUpdateIntervalMs,
      int numNbrToUpdate)
      : switchId_(switchId),
        intfsWithNeighbor_(std::move(intfsWithNeighbor)),
        systemPorts_(std::move(systemPorts)),
        ensemble_(ensemble),
        nbrUpdateIntervalMs_(nbrUpdateIntervalMs),
        hasNeighbor_(false),
        eventBase_("IntfMapScheduler") {
    CHECK(numNbrToUpdate <= intfsWithNeighbor_->size());
    intfsWithoutNeighbor_ = std::make_shared<InterfaceMap>();
    for (const auto& [intfId, intf] : *intfsWithNeighbor_) {
      if (intfsWithoutNeighbor_->size() < numNbrToUpdate) {
        auto intfWithoutNbr = intf->clone();
        intfWithoutNbr->setNdpTable(std::make_shared<NdpTable>());
        intfsWithoutNeighbor_->insert(intfId, std::move(intfWithoutNbr));
      } else {
        intfsWithoutNeighbor_->insert(intfId, intf->clone());
      }
    }
  }

  void start() {
    evbThread_ = std::make_unique<std::thread>([this]() {
      auto applyUpdateFn =
          [this](
              const InterfaceMapUpdateScheduler::InterfaceMapUpdate& update) {
            std::map<SwitchID, std::shared_ptr<SystemPortMap>>
                switchId2SystemPorts;
            switchId2SystemPorts[switchId_] = systemPorts_;

            SwSwitch::StateUpdateFn updateDsfStateFn =
                [this, update, switchId2SystemPorts](
                    const std::shared_ptr<SwitchState>& in) {
                  return DsfStateUpdaterUtil::getUpdatedState(
                      in,
                      ensemble_->getSw()->getScopeResolver(),
                      ensemble_->getSw()->getRib(),
                      switchId2SystemPorts,
                      update.switchId2Intfs);
                };

            ensemble_->getSw()->getRib()->updateStateInRibThread(
                [this, updateDsfStateFn]() {
                  ensemble_->getSw()->updateStateWithHwFailureProtection(
                      folly::sformat(
                          "Update interface map for switch: {}",
                          static_cast<uint16_t>(switchId_)),
                      updateDsfStateFn);
                });
          };

      scheduler_ = std::make_unique<InterfaceMapUpdateScheduler>(
          &eventBase_, applyUpdateFn);

      eventBase_.runInFbossEventBaseThread([this]() { scheduleNextUpdate(); });

      eventBase_.loopForever();
    });
  }

  void stop() {
    if (eventBase_.isRunning()) {
      eventBase_.terminateLoopSoon();
    }
    if (evbThread_ && evbThread_->joinable()) {
      evbThread_->join();
    }
  }

  ~PerSwitchInterfaceMapScheduler() {
    stop();
  }

 private:
  void scheduleNextUpdate() {
    InterfaceMapUpdateScheduler::InterfaceMapUpdate update;
    update.switchId2Intfs[switchId_] =
        hasNeighbor_ ? intfsWithoutNeighbor_ : intfsWithNeighbor_;

    hasNeighbor_ = !hasNeighbor_;

    scheduler_->queueInterfaceMapUpdate(std::move(update));

    eventBase_.scheduleAt(
        [this]() { scheduleNextUpdate(); },
        std::chrono::steady_clock::now() +
            std::chrono::milliseconds(nbrUpdateIntervalMs_));
  }

  SwitchID switchId_;
  std::shared_ptr<InterfaceMap> intfsWithNeighbor_;
  std::shared_ptr<InterfaceMap> intfsWithoutNeighbor_;
  std::shared_ptr<SystemPortMap> systemPorts_;
  AgentEnsemble* ensemble_;
  int nbrUpdateIntervalMs_;
  bool hasNeighbor_;
  FbossEventBase eventBase_;
  std::unique_ptr<std::thread> evbThread_;
  std::unique_ptr<InterfaceMapUpdateScheduler> scheduler_;
};

inline void voqRouteCompetingRemoteNeighborBenchmark(
    int ecmpGroup,
    int ecmpWidth,
    int nbrUpdateIntervalMs,
    int numRemoteNode,
    int numNbrToUpdate) {
  folly::BenchmarkSuspender suspender;

  auto ensemble = setupForVoqRouteScale(ecmpWidth);
  std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2IntfsWithNeighbor;

  const auto& config = ensemble->getSw()->getConfig();
  const auto useEncapIndex =
      ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE);

  utility::populateRemoteIntfAndSysPorts(
      switchId2SystemPorts, switchId2IntfsWithNeighbor, config, useEncapIndex);

  std::vector<std::unique_ptr<PerSwitchInterfaceMapScheduler>> schedulers;
  for (const auto& [switchId, systemPorts] : switchId2SystemPorts) {
    if (schedulers.size() >= numRemoteNode) {
      break;
    }

    auto scheduler = std::make_unique<PerSwitchInterfaceMapScheduler>(
        switchId,
        switchId2IntfsWithNeighbor[switchId],
        systemPorts,
        ensemble.get(),
        nbrUpdateIntervalMs,
        numNbrToUpdate);
    scheduler->start();
    schedulers.push_back(std::move(scheduler));
  }

  utility::EcmpSetupTargetedPorts6 ecmpHelper(
      ensemble->getProgrammedState(),
      ensemble->getSw()->needL2EntryForNeighbor());
  auto remoteNhops = utility::resolveRemoteNhops(ensemble.get(), ecmpHelper);
  auto nhopSets = getVoqRouteNextHopSets(remoteNhops, ecmpGroup, ecmpWidth);
  auto prefixes = getVoqRoutePrefixes(ecmpGroup);

  suspender.dismiss();
  auto updater = ensemble->getSw()->getRouteUpdater();
  ecmpHelper.programRoutes(&updater, nhopSets, prefixes);
  suspender.rehire();

  for (auto& scheduler : schedulers) {
    scheduler->stop();
  }
}
} // namespace facebook::fboss
