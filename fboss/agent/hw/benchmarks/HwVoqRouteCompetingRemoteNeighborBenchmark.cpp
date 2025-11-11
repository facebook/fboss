/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwRouteScaleBenchmarkHelpers.h"

#include <folly/Benchmark.h>
#include <folly/Synchronized.h>

namespace {
constexpr auto kEcmpWidth = 2048;
}

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

BENCHMARK(HwVoqRouteCompetingRemoteNeighborBenchmark) {
  folly::BenchmarkSuspender suspender;

  auto ensemble = setupForVoqRouteScale(kEcmpWidth);

  std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2IntfsWithNeighbor;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>>
      switchId2IntfsWithoutNeighbor;

  const auto& config = ensemble->getSw()->getConfig();
  const auto useEncapIndex =
      ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE);

  utility::populateRemoteIntfAndSysPorts(
      switchId2SystemPorts, switchId2IntfsWithNeighbor, config, useEncapIndex);
  utility::populateRemoteIntfAndSysPorts(
      switchId2SystemPorts,
      switchId2IntfsWithoutNeighbor,
      config,
      useEncapIndex,
      false /*addNeighbor*/);
}
} // namespace facebook::fboss
