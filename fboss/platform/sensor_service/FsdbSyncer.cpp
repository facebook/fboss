// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/FsdbSyncer.h"
#include <folly/logging/xlog.h>
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <optional>

namespace facebook::fboss {
FsdbSyncer::FsdbSyncer()
    : fsdbPubSubMgr_(
          std::make_unique<fsdb::FsdbPubSubManager>("sensor_service")) {
  if (FLAGS_publish_stats_to_fsdb) {
    fsdbPubSubMgr_->createStatPathPublisher(
        getSensorServiceStatsPath(), [this](auto oldState, auto newState) {
          fsdbStatPublisherStateChanged(oldState, newState);
        });
  }
}

FsdbSyncer::~FsdbSyncer() {
  CHECK(!readyForStatPublishing_.load());
}

void FsdbSyncer::stop() {
  readyForStatPublishing_.store(false);
  fsdbPubSubMgr_.reset();
}

void FsdbSyncer::statsUpdated(const stats::SensorServiceStats& stats) {
  if (!readyForStatPublishing_.load()) {
    return;
  }
  fsdb::OperState stateUnit;
  stateUnit.contents() =
      apache::thrift::BinarySerializer::serialize<std::string>(stats);
  stateUnit.protocol() = fsdb::OperProtocol::BINARY;
  fsdbPubSubMgr_->publishStat(std::move(stateUnit));
}

void FsdbSyncer::fsdbStatPublisherStateChanged(
    fsdb::FsdbStreamClient::State oldState,
    fsdb::FsdbStreamClient::State newState) {
  CHECK(oldState != newState);
  if (newState == fsdb::FsdbStreamClient::State::CONNECTED) {
    // Stats sync at regular intervals, so let the sync
    // happen in that sequence after a connection.
    XLOG(INFO) << "FSDB Connection Established! Ready for Stat Publishing";
    readyForStatPublishing_.store(true);
  } else {
    XLOG(INFO) << "FSDB Disconnected";
    readyForStatPublishing_.store(false);
  }
}
} // namespace facebook::fboss
