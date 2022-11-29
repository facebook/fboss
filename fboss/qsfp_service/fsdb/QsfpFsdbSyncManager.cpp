/*
 *  Copyright (c) 2021-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/fsdb/QsfpFsdbSyncManager.h"

#include "fboss/fsdb/common/Flags.h"
#include "fboss/lib/phy/gen-cpp2/phy_fatal_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_state_fatal_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_stats_fatal_types.h"

namespace facebook {
namespace fboss {

QsfpFsdbSyncManager::QsfpFsdbSyncManager() {
  if (FLAGS_publish_state_to_fsdb) {
    stateSyncer_ =
        std::make_unique<fsdb::FsdbSyncManager<state::QsfpServiceData>>(
            "qsfp_service", getStatePath(), false, true);
  }
  if (FLAGS_publish_stats_to_fsdb) {
    statsSyncer_ = std::make_unique<fsdb::FsdbSyncManager<stats::QsfpStats>>(
        "qsfp_service", getStatsPath(), true, false);
  }
}

void QsfpFsdbSyncManager::start() {
  if (FLAGS_publish_state_to_fsdb) {
    stateSyncer_->start();
  }
  if (FLAGS_publish_stats_to_fsdb) {
    statsSyncer_->start();
  }
}

void QsfpFsdbSyncManager::stop() {
  if (FLAGS_publish_state_to_fsdb) {
    stateSyncer_->stop();
  }
  if (FLAGS_publish_stats_to_fsdb) {
    statsSyncer_->stop();
  }
}

void QsfpFsdbSyncManager::updateConfig(cfg::QsfpServiceConfig newConfig) {
  if (!FLAGS_publish_state_to_fsdb) {
    return;
  }

  stateSyncer_->updateState([newConfig = std::move(newConfig)](const auto& in) {
    auto out = in->clone();
    out->template modify<state::qsfp_state_tags::strings::config>();
    out->template ref<state::qsfp_state_tags::strings::config>()->fromThrift(
        newConfig);
    return out;
  });
}

void QsfpFsdbSyncManager::updateTcvrState(
    int32_t tcvrId,
    TcvrState&& newState) {
  if (!FLAGS_publish_state_to_fsdb) {
    return;
  }

  stateSyncer_->updateState([tcvrId,
                             newState = std::move(newState)](const auto& in) {
    auto out = in->clone();
    out->template modify<state::qsfp_state_tags::strings::state>();
    auto& state = out->template ref<state::qsfp_state_tags::strings::state>();
    state->template modify<state::qsfp_state_tags::strings::tcvrStates>();
    auto& tcvrStates =
        state->template ref<state::qsfp_state_tags::strings::tcvrStates>();
    tcvrStates->modify(folly::to<std::string>(tcvrId));
    tcvrStates->ref(tcvrId)->fromThrift(newState);
    return out;
  });
}

void QsfpFsdbSyncManager::updateTcvrStats(std::map<int, TcvrStats>&& stats) {
  if (!FLAGS_publish_stats_to_fsdb) {
    return;
  }

  statsSyncer_->updateState([stats = std::move(stats)](const auto& in) {
    auto out = in->clone();
    out->template modify<stats::qsfp_stats_tags::strings::tcvrStats>();
    out->template ref<stats::qsfp_stats_tags::strings::tcvrStats>()->fromThrift(
        stats);
    return out;
  });
}

void QsfpFsdbSyncManager::updatePhyState(
    std::string&& portName,
    std::optional<phy::PhyState>&& newState) {
  if (!FLAGS_publish_state_to_fsdb) {
    return;
  }

  stateSyncer_->updateState([this,
                             portName = std::move(portName),
                             newState = std::move(newState)](const auto& in) {
    auto out = in->clone();
    out->template modify<state::qsfp_state_tags::strings::state>();
    auto& state = out->template ref<state::qsfp_state_tags::strings::state>();
    state->template modify<state::qsfp_state_tags::strings::phyStates>();
    auto& phyStates =
        state->template ref<state::qsfp_state_tags::strings::phyStates>();

    if (newState.has_value()) {
      phyStates->modify(portName);
      phyStates->ref(portName)->fromThrift(newState.value());
    } else {
      phyStates->remove(portName);
      if (phyStates->size() == 0) {
        // Special case. If size is 0, we won't have any more stats update per
        // port. But we still need to publish the empty stats.
        pendingPhyStats_.clear();
        updatePhyStats(std::move(pendingPhyStats_));
      }
    }

    return out;
  });
}

void QsfpFsdbSyncManager::updatePhyStats(PhyStatsMap&& stats) {
  if (!FLAGS_publish_stats_to_fsdb) {
    return;
  }

  statsSyncer_->updateState([stats = std::move(stats)](const auto& in) {
    auto out = in->clone();
    out->template modify<stats::qsfp_stats_tags::strings::phyStats>();
    out->template ref<stats::qsfp_stats_tags::strings::phyStats>()->fromThrift(
        stats);
    return out;
  });
}

void QsfpFsdbSyncManager::updatePhyStat(
    std::string&& portName,
    phy::PhyStats&& stat) {
  pendingPhyStats_[portName] = stat;

  // If we have stats for all ports with state, publish all accumulated stats.
  const auto& qsfpData = stateSyncer_->getState();
  const auto& state =
      qsfpData->template cref<state::qsfp_state_tags::strings::state>();
  const auto& phyStates =
      state->template cref<state::qsfp_state_tags::strings::phyStates>();

  // No need to check keys if we have less keys. More keys is fine. Maybe some
  // port got deleted.
  if (pendingPhyStats_.size() < phyStates->size()) {
    return;
  }

  // Make sure we have stats for every port with a state
  for (const auto& portState : std::as_const(*phyStates)) {
    if (!pendingPhyStats_.count(portState.first)) {
      return;
    }
  }

  // We have stats for all ports. Publish pending stats. The move should wipe
  // out our stats and we're ready to start over.
  updatePhyStats(std::move(pendingPhyStats_));
}

} // namespace fboss
} // namespace facebook
