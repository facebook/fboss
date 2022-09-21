/*
 *  Copyright (c) 2021-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/fsdb/QsfpConfigFsdbSyncer.h"

#include "fboss/qsfp_service/fsdb/QsfpFsdbSyncManager.h"

namespace facebook::fboss {

QsfpConfigFsdbSyncer::QsfpConfigFsdbSyncer(folly::EventBase* evb)
    : fsdb::FsdbStateComponentSyncer<cfg::QsfpServiceConfig>(
          evb,
          QsfpFsdbSyncManager::getConfigPath()) {}

cfg::QsfpServiceConfig QsfpConfigFsdbSyncer::getCurrentState() {
  // Return the config that's supposed to be published for initial sync instead
  // of the latest pending config. Then let the pending config be a delta sent
  // after the initial sync. This keeps the case where new configs are coming in
  // during initial sync simple.
  return publishedConfig_.withRLock([](auto& configPtr) {
    CHECK(configPtr);
    return *configPtr;
  });
}

void QsfpConfigFsdbSyncer::configUpdated(
    const cfg::QsfpServiceConfig& newConfig) {
  // Deposit the new config as pending immediately. Then schedule to publish it
  // on EVB later.
  //
  // During initial sync, the sync was run on EVB. It calls getCurrentState()
  // and publish the published config. If new configs come in via this function
  // while the publishing is happening, new configs will just get stored as
  // pendingConfig. Then published as delta on the EVB after the initial sync.
  // Multiple updates will get coalesced into the same pendingConfig with one
  // delta. Duplicated delta publishing are not scheduled.
  auto pendingConfig = pendingConfig_.wlock();
  auto alreadyPending = (bool)*pendingConfig;
  *pendingConfig = std::make_unique<cfg::QsfpServiceConfig>(newConfig);
  if (alreadyPending) {
    return;
  }

  folly::via(evb_).then(
      [this](auto&& /* unused */) { publishPendingConfig(); });
}

void QsfpConfigFsdbSyncer::publishPendingConfig() {
  // If ready, use publishedConfig and pendingConfig to publish a delta.
  // Then set publishedConfig to what we just published. If not ready, don't
  // publish. But still set it to publishedConfig. Later when we're
  // connected and ready, it will just pick up the publishedConfig for
  // initial sync.
  std::optional<cfg::QsfpServiceConfig> oldConfig;
  std::optional<cfg::QsfpServiceConfig> newConfig;
  {
    auto pendingConfig = pendingConfig_.wlock();
    auto publishedConfig = publishedConfig_.wlock();

    if (!*pendingConfig) {
      // If multiple config updates come in quickly, a previous update might
      // have already published the pending config. Just move on.
      return;
    }

    if (isReady()) {
      CHECK(*publishedConfig);
      oldConfig = std::make_optional(**publishedConfig);
      newConfig = std::make_optional(**pendingConfig);
    }

    *publishedConfig = std::move(*pendingConfig);
  }

  if (!dataReady_.isFulfilled()) {
    dataReady_.setValue();
  }

  if (oldConfig && newConfig) {
    auto deltaUnit = createDeltaUnit(getBasePath(), oldConfig, newConfig);
    publishDelta(createDelta({deltaUnit}));
  }
}

folly::SemiFuture<folly::Unit> QsfpConfigFsdbSyncer::dataReady() {
  return dataReady_.getSemiFuture();
}

} // namespace facebook::fboss
