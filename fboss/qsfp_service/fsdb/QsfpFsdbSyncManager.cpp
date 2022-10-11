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

namespace facebook {
namespace fboss {

QsfpFsdbSyncManager::QsfpFsdbSyncManager() {
  if (FLAGS_publish_state_to_fsdb) {
    stateSyncer_ =
        std::make_unique<fsdb::FsdbSyncManager2<state::QsfpServiceData>>(
            "qsfp_service", getStatePath(), false, true);
  }
}

void QsfpFsdbSyncManager::start() {
  if (FLAGS_publish_state_to_fsdb) {
    stateSyncer_->start();
  }
}
void QsfpFsdbSyncManager::stop() {
  if (FLAGS_publish_state_to_fsdb) {
    stateSyncer_->stop();
  }
}

void QsfpFsdbSyncManager::updateConfig(cfg::QsfpServiceConfig newConfig) {
  if (FLAGS_publish_state_to_fsdb) {
    stateSyncer_->updateState([newConfig =
                                   std::move(newConfig)](const auto& in) {
      auto out = in->clone();
      out->template modify<state::qsfp_state_tags::strings::config>();
      out->template ref<state::qsfp_state_tags::strings::config>()->fromThrift(
          newConfig);
      return out;
    });
  }
}

} // namespace fboss
} // namespace facebook
