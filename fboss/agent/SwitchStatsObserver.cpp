// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/SwitchStatsObserver.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/DeltaFunctions.h"

namespace facebook::fboss {

SwitchStatsObserver::SwitchStatsObserver(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "SwitchStatsObserver");
}

SwitchStatsObserver::~SwitchStatsObserver() {
  sw_->unregisterStateObserver(this);
}

void SwitchStatsObserver::updateSystemPortCounters(
    const StateDelta& stateDelta) {
  auto getSysPortDelta = [](const auto& delta) {
    int cntDelta = 0;
    DeltaFunctions::forEachChanged(
        delta,
        [&](const auto& /*oldNode*/,
            const auto& /*newNode*/) { /* Counter not changed */ },
        [&](const auto& /*added*/) { cntDelta++; },
        [&](const auto& /*removed*/) { cntDelta--; });
    return cntDelta;
  };
  sw_->stats()->localSystemPort(
      getSysPortDelta(stateDelta.getSystemPortsDelta()));
  sw_->stats()->remoteSystemPort(
      getSysPortDelta(stateDelta.getRemoteSystemPortsDelta()));
}

template <typename NTableT>
int SwitchStatsObserver::getResolvedNeighborCount(const NTableT& ntable) {
  auto count = 0;
  for (const auto& [_, nbrEntry] : std::as_const(*ntable)) {
    if (nbrEntry->isReachable()) {
      count++;
    }
  }
  return count;
}

template <typename IntfDeltaT>
std::tuple<int, int, int> SwitchStatsObserver::getUpdatedInterfaceCounters(
    const IntfDeltaT& intfDelta) {
  int intfCountDelta = 0;
  int ndpCountDelta = 0;
  int arpCountDelta = 0;
  DeltaFunctions::forEachChanged(
      intfDelta,
      [&](const auto& oldNode, const auto& newNode) {
        ndpCountDelta += getResolvedNeighborCount(newNode->getNdpTable()) -
            getResolvedNeighborCount(oldNode->getNdpTable());
        arpCountDelta += getResolvedNeighborCount(newNode->getArpTable()) -
            getResolvedNeighborCount(oldNode->getArpTable());
      },
      [&](const auto& added) {
        intfCountDelta++;
        ndpCountDelta += getResolvedNeighborCount(added->getNdpTable());
        arpCountDelta += getResolvedNeighborCount(added->getArpTable());
      },
      [&](const auto& removed) {
        intfCountDelta--;
        ndpCountDelta -= getResolvedNeighborCount(removed->getNdpTable());
        arpCountDelta -= getResolvedNeighborCount(removed->getArpTable());
      });
  return std::make_tuple(intfCountDelta, ndpCountDelta, arpCountDelta);
}

void SwitchStatsObserver::updateInterfaceCounters(
    const StateDelta& stateDelta) {
  auto [localRif, localNdp, localArp] =
      getUpdatedInterfaceCounters(stateDelta.getIntfsDelta());
  sw_->stats()->localRifs(localRif);
  sw_->stats()->localResolvedNdp(localNdp);
  sw_->stats()->localResolvedArp(localArp);

  auto [remoteRif, remoteNdp, remoteArp] =
      getUpdatedInterfaceCounters(stateDelta.getRemoteIntfsDelta());
  sw_->stats()->remoteRifs(remoteRif);
  sw_->stats()->remoteResolvedNdp(remoteNdp);
  sw_->stats()->remoteResolvedArp(remoteArp);
}

void SwitchStatsObserver::stateUpdated(const StateDelta& stateDelta) {
  updateSystemPortCounters(stateDelta);
  updateInterfaceCounters(stateDelta);
}

} // namespace facebook::fboss
