/*
 *  Copyright (c) 2022-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/fsdb/client/FsdbStreamClient.h"

namespace facebook {
namespace fboss {

namespace fsdb {
class FsdbPubSubManager;
}
class TransceiverManager;

class QsfpFsdbSyncer {
 public:
  explicit QsfpFsdbSyncer(TransceiverManager* transceiverMgr);
  ~QsfpFsdbSyncer();

  fsdb::FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_.get();
  }

 private:
  std::vector<std::string> getStatePath() const;
  std::vector<std::string> getStatsPath() const;

  void handleStatePublisherStateChanged(
      fsdb::FsdbStreamClient::State oldState,
      fsdb::FsdbStreamClient::State newState);
  void handleStatsPublisherStateChanged(
      fsdb::FsdbStreamClient::State oldState,
      fsdb::FsdbStreamClient::State newState);

  TransceiverManager* transceiverMgr_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::atomic<bool> statePublisherConnected_{false};
  std::atomic<bool> statsPublisherConnected_{false};
};
} // namespace fboss
} // namespace facebook
