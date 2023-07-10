// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fan_service/FsdbSensorSubscriber.h"

#include <optional>

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "common/time/Time.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_types.h"

namespace {
auto constexpr kFsdbSensorDataStale = "fsdb_sensor_service_data_stale";
auto constexpr kFsdbSyncTimeoutThresholdInSec = 3 * 60; // 3 minutes
} // namespace

namespace facebook::fboss {

template <typename T>
void FsdbSensorSubscriber::subscribeToStatsOrState(
    std::vector<std::string> path,
    folly::Synchronized<T>& storage,
    bool stats) {
  auto stateCb = [](fsdb::FsdbStreamClient::State /*old*/,
                    fsdb::FsdbStreamClient::State /*new*/) {};
  auto dataCb = [&](fsdb::OperState&& state) {
    storage.withWLock([&](auto& locked) {
      if (auto metadata = state.metadata()) {
        if (auto lastConfirmedAt = metadata->lastConfirmedAt()) {
          sensorStatsLastUpdatedTime.store(*lastConfirmedAt);
        }
      }
      if (auto contents = state.contents()) {
        locked = apache::thrift::BinarySerializer::deserialize<T>(*contents);
      } else {
        locked = {};
      }
    });
  };
  if (stats) {
    pubSubMgr()->addStatPathSubscription(path, stateCb, dataCb);
  } else {
    pubSubMgr()->addStatePathSubscription(path, stateCb, dataCb);
  }
}

void FsdbSensorSubscriber::subscribeToQsfpServiceStat() {
  subscribeToStatsOrState(getQsfpDataStatsPath(), tcvrStats, true /* stats */);
}

void FsdbSensorSubscriber::subscribeToQsfpServiceState() {
  subscribeToStatsOrState(getQsfpDataStatePath(), tcvrState, false /* state */);
}

void FsdbSensorSubscriber::subscribeToSensorServiceStat() {
  subscribeToStatsOrState(
      getSensorDataStatsPath(), sensorSvcData, true /* stats */);
}

std::map<std::string, fboss::platform::sensor_service::SensorData>
FsdbSensorSubscriber::getSensorData() const {
  auto timeSinceLastUpdate = facebook::WallClockUtil::NowInSecFast() -
      sensorStatsLastUpdatedTime.load();
  if (timeSinceLastUpdate > kFsdbSyncTimeoutThresholdInSec) {
    XLOG(ERR) << "Warning! Sensor data hasn't been synced since last "
              << timeSinceLastUpdate << " seconds";
    fb303::fbData->setCounter(kFsdbSensorDataStale, 1);
  } else {
    fb303::fbData->setCounter(kFsdbSensorDataStale, 0);
  }
  return sensorSvcData.copy();
};

std::map<int32_t, TcvrState> FsdbSensorSubscriber::getTcvrState() const {
  return tcvrState.copy();
};

std::map<int32_t, TcvrStats> FsdbSensorSubscriber::getTcvrStats() const {
  return tcvrStats.copy();
};
} // namespace facebook::fboss
