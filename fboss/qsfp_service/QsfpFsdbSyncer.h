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
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"

namespace facebook {
namespace fboss {

namespace fsdb {
class FsdbPubSubManager;
}
class TransceiverManager;

class QsfpFsdbSyncer {
 public:
  explicit QsfpFsdbSyncer();
  ~QsfpFsdbSyncer();

  static std::shared_ptr<QsfpFsdbSyncer> getInstance();

  static const std::vector<std::string>& getStatePath();
  static const std::vector<std::string>& getStatsPath();
  static const std::vector<std::string>& getConfigPath();

  void setTransceiverManager(TransceiverManager* transceiverMgr) {
    transceiverMgr_ = transceiverMgr;
  }

  fsdb::FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_.get();
  }

  bool statePublisherConnected() {
    return statePublisherConnected_;
  }

  bool statsPublisherConnected() {
    return statsPublisherConnected_;
  }

  template <typename Node>
  static fsdb::OperDeltaUnit createDelta(
      const std::vector<std::string>& path,
      const Node* oldState,
      const Node* newState);

  static fsdb::OperDeltaUnit createConfigDelta(
      const cfg::QsfpServiceConfig* oldState,
      const cfg::QsfpServiceConfig* newState);

  QsfpFsdbSyncer& operator<<(
      const std::vector<fsdb::OperDeltaUnit>& deltaUnits);
  QsfpFsdbSyncer& operator<<(const fsdb::OperDeltaUnit& deltaUnit);

 private:
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
