// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Subprocess.h>
#include <gtest/gtest.h>

#include "fboss/lib/CommonUtils.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fan_service/ControlLogic.h"
#include "fboss/platform/helpers/Init.h"

DEFINE_bool(run_forever, false, "run the test forever");

using namespace facebook::fboss::platform::fan_service;
using namespace facebook::fboss::platform;

namespace {

const std::vector<std::string> kStopFsdb = {
    "/bin/systemctl",
    "stop",
    "fsdb_service_for_testing"};

const std::vector<std::string> kStartFsdb = {
    "/bin/systemctl",
    "start",
    "fsdb_service_for_testing"};

const std::vector<std::string> kRestartFsdb = {
    "/bin/systemctl",
    "restart",
    "fsdb_service_for_testing"};

const std::vector<std::string> kRestartSensorSvc = {
    "/bin/systemctl",
    "restart",
    "sensor_service_for_testing"};

void stopFsdbService() {
  XLOG(DBG2) << "Stopping FSDB Service";
  folly::Subprocess(kStopFsdb).waitChecked();
}

void startFsdbService() {
  XLOG(DBG2) << "Starting FSDB Service";
  folly::Subprocess(kStartFsdb).waitChecked();
}

void restartFsdbService() {
  XLOG(DBG2) << "Restarting FSDB Service";
  folly::Subprocess(kRestartFsdb).waitChecked();
}

void restartSensorService() {
  XLOG(DBG2) << "Restarting Sensor Service";
  folly::Subprocess(kRestartSensorSvc).waitChecked();
}

class FanSensorFsdbIntegrationTests : public ::testing::Test {
 public:
  void SetUp() override {
    std::string fanServiceConfJson = ConfigLib().getFanServiceConfig();
    auto config =
        apache::thrift::SimpleJSONSerializer::deserialize<FanServiceConfig>(
            fanServiceConfJson);
    XLOG(INFO) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
        config);
    auto pBsp = std::make_shared<Bsp>(config);
    controlLogic_ = std::make_unique<ControlLogic>(config, pBsp);
  }

  void TearDown() override {
    if (FLAGS_run_forever) {
      while (true) {
        /* sleep override */ sleep(1);
      }
    }
    // Restart Sensor and FSDB to bring it back to healthy state for the next
    // test
    restartSensorService();
    restartFsdbService();
  }

  std::unique_ptr<ControlLogic> controlLogic_{nullptr};
};

} // namespace

namespace facebook::fboss::platform::fan_service {

TEST_F(FanSensorFsdbIntegrationTests, sensorUpdate) {
  SensorData prevSensorData;
  uint64_t beforeLastFetchTime;

  // Get the sensor data separately from sensor service via thrift. Expect the
  // same sensors to be available in the fan service cache later. This would
  // confirm that the sensor service publishes its sensors to fsdb and the fan
  // service correctly subscribes to those sensors from fsdb
  std::shared_ptr<SensorData> thriftSensorData = std::make_shared<SensorData>();
  controlLogic_->getSensorDataThrift(thriftSensorData);
  // Expect non-zero sensors
  ASSERT_TRUE(thriftSensorData->getSensorEntries().size());

  WITH_RETRIES_N_TIMED(6, std::chrono::seconds(10), {
    // Kick off the control fan logic, which will try to fetch the sensor
    // data from sensor_service
    controlLogic_->controlFan();
    beforeLastFetchTime = controlLogic_->lastSensorFetchTimeSec();
    prevSensorData = controlLogic_->sensorData();
    // Confirm that the fan service received the same sensors from fsdb as
    // returned by sensor service via thrift
    ASSERT_EVENTUALLY_TRUE(
        prevSensorData.getSensorEntries().size() >=
        thriftSensorData->getSensorEntries().size());
    for (const auto& [sensorName, sensorEntry] :
         thriftSensorData->getSensorEntries()) {
      ASSERT_TRUE(prevSensorData.getSensorEntry(sensorName));
    }
  });

  // Fetch the sensor data again and expect the timestamps to advance
  WITH_RETRIES_N_TIMED(6, std::chrono::seconds(10), {
    controlLogic_->controlFan();
    auto currSensorData = controlLogic_->sensorData();
    auto afterLastFetchTime = controlLogic_->lastSensorFetchTimeSec();
    ASSERT_EVENTUALLY_TRUE(afterLastFetchTime > beforeLastFetchTime);
    ASSERT_EVENTUALLY_TRUE(
        currSensorData.getSensorEntries().size() ==
        prevSensorData.getSensorEntries().size());
    for (const auto& [sensorName, prevSensorEntry] :
         prevSensorData.getSensorEntries()) {
      auto currSensorEntry = currSensorData.getSensorEntry(sensorName);
      ASSERT_TRUE(currSensorEntry);
      XLOG(INFO) << "Sensor: " << sensorName
                 << ". Previous timestamp: " << prevSensorEntry.lastUpdated
                 << ", Current timestamp: " << currSensorEntry->lastUpdated;
      // The timestamps should advance. Sometimes the timestamps are 0 for
      // some sensors returned by sensor data, so add a special check for
      // that too
      ASSERT_EVENTUALLY_TRUE(
          (currSensorEntry->lastUpdated > prevSensorEntry.lastUpdated) ||
          (currSensorEntry->lastUpdated == 0 &&
           prevSensorEntry.lastUpdated == 0));
    }
  });
}

// Verifies sensor data is synced correctly after a fsdb restart
TEST_F(FanSensorFsdbIntegrationTests, fsdbRestart) {
  SensorData prevSensorData;
  uint64_t prevLastFetchTime;

  // Fetch the sensor data from sensor_service over thrift. This way we know
  // which sensors were explicitly published by sensor service
  std::shared_ptr<SensorData> thriftSensorData = std::make_shared<SensorData>();
  controlLogic_->getSensorDataThrift(thriftSensorData);
  // Expect non-zero sensors
  ASSERT_TRUE(thriftSensorData->getSensorEntries().size());

  // Allow time for fan_service to warm up and sync all the sensor data from
  // fsdb. We should expect to sync all the sensors that were received from
  // thrift earlier
  WITH_RETRIES_N_TIMED(6, std::chrono::seconds(10), {
    controlLogic_->controlFan();
    prevLastFetchTime = controlLogic_->lastSensorFetchTimeSec();
    prevSensorData = controlLogic_->sensorData();
    // Confirm that the fan service received the same sensors from fsdb as
    // returned by sensor service via thrift
    ASSERT_EVENTUALLY_TRUE(
        prevSensorData.getSensorEntries().size() >=
        thriftSensorData->getSensorEntries().size());
    for (const auto& [sensorName, sensorEntry] :
         thriftSensorData->getSensorEntries()) {
      ASSERT_TRUE(prevSensorData.getSensorEntry(sensorName));
    }
  });

  // Stop FSDB
  stopFsdbService();

  // With FSDB stopped, we shouldn't receive any new sensor updates. Fetch the
  // sensor data for two SensorFetchFrequency intervals and confirm that the
  // sensor timestamps don't advance
  auto fetchFrequencyInSec = controlLogic_->getSensorFetchFrequency();
  XLOG(INFO) << "Verifying that there are no sensor updates for "
             << (2 * fetchFrequencyInSec + 10) << " seconds";
  /* sleep override */
  sleep(2 * fetchFrequencyInSec + 10);
  controlLogic_->controlFan();
  auto currSensorData = controlLogic_->sensorData();
  for (const auto& [sensorName, prevSensorEntry] :
       prevSensorData.getSensorEntries()) {
    auto currSensorEntry = currSensorData.getSensorEntry(sensorName);
    ASSERT_TRUE(currSensorEntry);
    ASSERT_EQ(currSensorEntry->lastUpdated, prevSensorEntry.lastUpdated);
  }

  // Start FSDB
  startFsdbService();

  // Expect the lastFetchTime to advance and number of sensors to be same as
  // last time
  WITH_RETRIES_N_TIMED(6, std::chrono::seconds(10), {
    controlLogic_->controlFan();
    auto currSensorData = controlLogic_->sensorData();
    auto afterLastFetchTime = controlLogic_->lastSensorFetchTimeSec();
    ASSERT_EVENTUALLY_TRUE(afterLastFetchTime > prevLastFetchTime);
    ASSERT_EVENTUALLY_TRUE(
        currSensorData.getSensorEntries().size() ==
        prevSensorData.getSensorEntries().size());
    for (const auto& [sensorName, prevSensorEntry] :
         prevSensorData.getSensorEntries()) {
      auto currSensorEntry = currSensorData.getSensorEntry(sensorName);
      ASSERT_TRUE(currSensorEntry);
      XLOG(INFO) << "Sensor: " << sensorName
                 << ". Previous timestamp: " << prevSensorEntry.lastUpdated
                 << ", Current timestamp: " << currSensorEntry->lastUpdated;
      // The timestamps should advance. Sometimes the timestamps are 0 for
      // some sensors returned by sensor data, so add a special check for
      // that too
      ASSERT_EVENTUALLY_TRUE(
          (currSensorEntry->lastUpdated > prevSensorEntry.lastUpdated) ||
          (currSensorEntry->lastUpdated == 0 &&
           prevSensorEntry.lastUpdated == 0));
    }
  });
}

TEST_F(FanSensorFsdbIntegrationTests, qsfpSync) {
  std::unordered_map<
      std::string,
      std::pair<uint64_t /* timestamp */, int /* count */>>
      lastSyncTimeAndCountMap;

  WITH_RETRIES_N_TIMED(9, std::chrono::seconds(10), {
    // Kick off the control fan logic, which will try to process the optics data
    // synced from fsdb
    controlLogic_->controlFan();
    SensorData sensorData = controlLogic_->sensorData();
    // Ensure that the optic entry is not empty
    ASSERT_EVENTUALLY_TRUE(sensorData.getOpticEntries().size() > 0);
    for (const auto& [opticName, opticEntry] : sensorData.getOpticEntries()) {
      if (lastSyncTimeAndCountMap.find(opticName) ==
          lastSyncTimeAndCountMap.end()) {
        lastSyncTimeAndCountMap[opticName] = {0, 0};
      }
      // For every optic sensor that is synced, this timestamp should advance.
      auto& [timestamp, count] = lastSyncTimeAndCountMap[opticName];
      if (opticEntry.dataProcessTimeStamp > timestamp) {
        count++;
        timestamp = opticEntry.dataProcessTimeStamp;
      }
      // Let the test verify at least 2 syncs
      ASSERT_EVENTUALLY_TRUE(count >= 2);
    }
  });
}

} // namespace facebook::fboss::platform::fan_service

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
