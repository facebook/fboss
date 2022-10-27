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

} // namespace fboss
} // namespace facebook
