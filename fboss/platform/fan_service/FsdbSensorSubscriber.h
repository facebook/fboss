// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <cstdint>

namespace facebook::fboss {
namespace fsdb {
class FsdbPubSubManager;
}

class FsdbSensorSubscriber {
 public:
  explicit FsdbSensorSubscriber(fsdb::FsdbPubSubManager* pubSubMgr)
      : fsdbPubSubMgr_(pubSubMgr) {}

  fsdb::FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_;
  }

  void subscribeToSensorServiceStat();
  void subscribeToQsfpServiceState();
  void subscribeToQsfpServiceStat();
  void subscribeToAgentStat();

  // Paths
  static std::vector<std::string> getSensorDataStatsPath();
  static std::vector<std::string> getQsfpDataStatsPath();
  static std::vector<std::string> getQsfpDataStatePath();
  static std::vector<std::string> getAgentDataStatsPath();

  uint64_t getSensorStatsLastUpdatedTime() const {
    return sensorStatsLastUpdatedTime.load();
  }

  std::map<std::string, fboss::platform::sensor_service::SensorData>
  getSensorData() const;
  std::map<std::string, fboss::platform::sensor_service::SensorData>
  getAgentData() const;
  std::map<int32_t, TcvrState> getTcvrState() const;
  std::map<int32_t, TcvrStats> getTcvrStats() const;

 private:
  template <typename T>
  void subscribeToStatsOrState(
      std::vector<std::string> path,
      folly::Synchronized<T>& storage,
      bool stats,
      std::atomic<uint64_t>& lastUpdateTime);
  void checkDataFreshness(
      const std::atomic<uint64_t>& lastUpdatedTime,
      const std::string& contentName,
      const std::string& staleCounterName) const;
  fsdb::FsdbPubSubManager* fsdbPubSubMgr_;
  std::atomic<uint64_t> sensorStatsLastUpdatedTime{0};
  std::atomic<uint64_t> agentLastUpdatedTime{0};
  std::atomic<uint64_t> qsfpStatsLastUpdatedTime{0};
  std::atomic<uint64_t> qsfpStateLastUpdatedTime{0};
  folly::Synchronized<std::map<
      std::string /* sensor name */,
      fboss::platform::sensor_service::SensorData>>
      sensorSvcData;
  folly::Synchronized<std::map<
      std::string /* ASIC temp sensor as asic_temp_x_y,
                   where x is ASIC instance and y is ASIC sensor number
                   from SDK */
      ,
      fboss::platform::sensor_service::SensorData>>
      agentData;
  folly::Synchronized<std::map<int32_t /* tcvrId */, TcvrState>> tcvrState;
  folly::Synchronized<std::map<int32_t /* tcvrId */, TcvrStats>> tcvrStats;
};

} // namespace facebook::fboss
