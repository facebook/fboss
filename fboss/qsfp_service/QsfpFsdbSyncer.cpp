/*
 *  Copyright (c) 2021-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "folly/Singleton.h"

#include "fboss/qsfp_service/TransceiverManager.h"

#include "fboss/qsfp_service/QsfpFsdbSyncer.h"

namespace {

struct SingletonTag {};

} // anonymous namespace

namespace facebook {
namespace fboss {

static folly::Singleton<QsfpFsdbSyncer, SingletonTag> qsfpFsdbSyncer;

QsfpFsdbSyncer::QsfpFsdbSyncer()
    : fsdbPubSubMgr_(
          std::make_unique<fsdb::FsdbPubSubManager>("qsfp_service")) {
  if (FLAGS_publish_state_to_fsdb) {
    fsdbPubSubMgr_->createStateDeltaPublisher(
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

std::shared_ptr<QsfpFsdbSyncer> QsfpFsdbSyncer::getInstance() {
  if (FLAGS_publish_state_to_fsdb || FLAGS_publish_stats_to_fsdb) {
    return qsfpFsdbSyncer.try_get();
  } else {
    return nullptr;
  }
}

template <typename Node>
fsdb::OperDeltaUnit QsfpFsdbSyncer::createDelta(
    const std::vector<std::string>& path,
    const Node* oldState,
    const Node* newState) {
  fsdb::OperDeltaUnit deltaUnit;
  deltaUnit.path().ensure().raw() = path;
  if (oldState) {
    deltaUnit.oldState() =
        apache::thrift::BinarySerializer::serialize<std::string>(*oldState);
  }
  if (newState) {
    deltaUnit.newState() =
        apache::thrift::BinarySerializer::serialize<std::string>(*newState);
  }
  return deltaUnit;
}

fsdb::OperDeltaUnit QsfpFsdbSyncer::createConfigDelta(
    const cfg::QsfpServiceConfig* oldState,
    const cfg::QsfpServiceConfig* newState) {
  return createDelta(QsfpFsdbSyncer::getConfigPath(), oldState, newState);
}

QsfpFsdbSyncer& QsfpFsdbSyncer::operator<<(
    const std::vector<fsdb::OperDeltaUnit>& deltaUnits) {
  fsdb::OperDelta delta;
  delta.changes() = deltaUnits;
  delta.protocol() = fsdb::OperProtocol::BINARY;
  fsdbPubSubMgr_->publishState(std::move(delta));
  return *this;
}

QsfpFsdbSyncer& QsfpFsdbSyncer::operator<<(
    const fsdb::OperDeltaUnit& deltaUnit) {
  fsdb::OperDelta delta;
  delta.changes().ensure().emplace_back(deltaUnit);
  delta.protocol() = fsdb::OperProtocol::BINARY;
  fsdbPubSubMgr_->publishState(std::move(delta));
  return *this;
}

void QsfpFsdbSyncer::handleStatePublisherStateChanged(
    fsdb::FsdbStreamClient::State /* oldState */,
    fsdb::FsdbStreamClient::State newState) {
  statePublisherConnected_ =
      newState == fsdb::FsdbStreamClient::State::CONNECTED;
  // TODO: ask transceiver manager to republish everything
}

void QsfpFsdbSyncer::handleStatsPublisherStateChanged(
    fsdb::FsdbStreamClient::State /* oldState */,
    fsdb::FsdbStreamClient::State newState) {
  statsPublisherConnected_ =
      newState == fsdb::FsdbStreamClient::State::CONNECTED;
}

} // namespace fboss
} // namespace facebook
