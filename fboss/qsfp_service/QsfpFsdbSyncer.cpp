/*
 *  Copyright (c) 2021-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/fsdb/Flags.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include "fboss/qsfp_service/TransceiverManager.h"

#include "fboss/qsfp_service/QsfpFsdbSyncer.h"

namespace facebook {
namespace fboss {

QsfpFsdbSyncer::QsfpFsdbSyncer(TransceiverManager* transceiverMgr)
    : transceiverMgr_(transceiverMgr),
      fsdbPubSubMgr_(
          std::make_unique<fsdb::FsdbPubSubManager>("qsfp_service")) {
  if (FLAGS_publish_state_to_fsdb) {
    fsdbPubSubMgr_->createStatePathPublisher(
        getStatePath(), [this](auto oldState, auto newState) {
          handleStatePublisherStateChanged(oldState, newState);
        });
  }
  if (FLAGS_publish_stats_to_fsdb) {
    fsdbPubSubMgr_->createStatPathPublisher(
        getStatsPath(), [this](auto oldState, auto newState) {
          handleStatsPublisherStateChanged(oldState, newState);
        });
  }
}

// Prevent inlining to make unique_ptr to incomplete type happy
QsfpFsdbSyncer::~QsfpFsdbSyncer() = default;

void QsfpFsdbSyncer::handleStatePublisherStateChanged(
    fsdb::FsdbStreamClient::State /* oldState */,
    fsdb::FsdbStreamClient::State newState) {
  statePublisherConnected_ =
      newState == fsdb::FsdbStreamClient::State::CONNECTED;
}

void QsfpFsdbSyncer::handleStatsPublisherStateChanged(
    fsdb::FsdbStreamClient::State /* oldState */,
    fsdb::FsdbStreamClient::State newState) {
  statsPublisherConnected_ =
      newState == fsdb::FsdbStreamClient::State::CONNECTED;
}

} // namespace fboss
} // namespace facebook
