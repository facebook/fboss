// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fan_service/hw_test/integration_tests/FanSensorFsdbIntegrationTests.h"

#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "fboss/lib/CommonUtils.h"
#include "fboss/platform/fan_service/HelperFunction.h"
#include "fboss/platform/helpers/Init.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"

DEFINE_bool(run_forever, false, "run the test forever");

namespace {
const std::vector<std::string> kStopFsdb = {
    "/bin/systemctl",
    "stop",
    "fsdb_service_for_testing"};

const std::vector<std::string> kStartFsdb = {
    "/bin/systemctl",
    "start",
    "fsdb_service_for_testing"};
} // namespace

namespace facebook::fboss::platform {

void FanSensorFsdbIntegrationTests::SetUp() {
  // Define service and handler for testing.
  std::tie(thriftServer_, thriftHandler_) = setupThrift();
  thriftHandler_->getFanService()->kickstart();
  // Finally, if this is for Meta, start service
  fsTestSetUp(thriftServer_, thriftHandler_.get());
}

void FanSensorFsdbIntegrationTests::TearDown() {
  if (FLAGS_run_forever) {
    while (true) {
      /* sleep override */ sleep(1);
    }
  }
  fsTestTearDown(thriftServer_, thriftHandler_.get());
}

FanService* FanSensorFsdbIntegrationTests::getFanService() {
  return thriftHandler_->getFanService();
}

void FanSensorFsdbIntegrationTests::stopFsdbService() const {
  XLOG(DBG2) << "Stopping FSDB Service";
  folly::Subprocess(kStopFsdb).waitChecked();
}

void FanSensorFsdbIntegrationTests::startFsdbService() const {
  XLOG(DBG2) << "Starting FSDB Service";
  folly::Subprocess(kStartFsdb).waitChecked();
}

TEST_F(FanSensorFsdbIntegrationTests, sensorUpdate) {
  SensorData prevSensorData;
  uint64_t beforeLastFetchTime;

  // Get the sensor data separately from sensor service via thrift. Expect the
  // same sensors to be available in the fan service cache later. This would
  // confirm that the sensor service publishes its sensors to fsdb and the fan
  // service correctly subscribes to those sensors from fsdb
  std::shared_ptr<SensorData> thriftSensorData = std::make_shared<SensorData>();
  getFanService()->getSensorDataThrift(thriftSensorData);
  // Expect non-zero sensors
  ASSERT_TRUE(thriftSensorData->size());

  WITH_RETRIES_N_TIMED(
      {
        // Kick off the control fan logic, which will try to fetch the sensor
        // data from sensor_service
        getFanService()->controlFan();
        beforeLastFetchTime = getFanService()->lastSensorFetchTimeSec();
        prevSensorData = getFanService()->sensorData();
        // Confirm that the fan service received the same sensors from fsdb as
        // returned by sensor service via thrift
        ASSERT_EVENTUALLY_TRUE(
            prevSensorData.size() >= thriftSensorData->size());
        for (auto it = thriftSensorData->begin(); it != thriftSensorData->end();
             it++) {
          ASSERT_EVENTUALLY_TRUE(prevSensorData.checkIfEntryExists(it->first));
        }
      },
      6,
      std::chrono::seconds(10));

  // Fetch the sensor data again and expect the timestamps to advance
  WITH_RETRIES_N_TIMED(
      {
        getFanService()->controlFan();
        auto currSensorData = getFanService()->sensorData();
        auto afterLastFetchTime = getFanService()->lastSensorFetchTimeSec();
        ASSERT_EVENTUALLY_TRUE(afterLastFetchTime > beforeLastFetchTime);
        ASSERT_EVENTUALLY_TRUE(currSensorData.size() == prevSensorData.size());
        for (auto it = thriftSensorData->begin(); it != thriftSensorData->end();
             it++) {
          ASSERT_EVENTUALLY_TRUE(currSensorData.checkIfEntryExists(it->first));
          XLOG(INFO) << "Sensor: " << it->first << ". Previous timestamp: "
                     << prevSensorData.getLastUpdated(it->first)
                     << ", Current timestamp: "
                     << currSensorData.getLastUpdated(it->first);
          // The timestamps should advance. Sometimes the timestamps are 0 for
          // some sensors returned by sensor data, so add a special check for
          // that too
          ASSERT_EVENTUALLY_TRUE(
              (currSensorData.getLastUpdated(it->first) >
               prevSensorData.getLastUpdated(it->first)) ||
              (currSensorData.getLastUpdated(it->first) == 0 &&
               prevSensorData.getLastUpdated(it->first) == 0));
        }
      },
      6,
      std::chrono::seconds(10));
}

// Verifies sensor data is synced correctly after a fsdb restart
TEST_F(FanSensorFsdbIntegrationTests, fsdbRestart) {
  SensorData prevSensorData;
  uint64_t prevLastFetchTime;

  // Fetch the sensor data from sensor_service over thrift. This way we know
  // which sensors were explicitly published by sensor service
  std::shared_ptr<SensorData> thriftSensorData = std::make_shared<SensorData>();
  getFanService()->getSensorDataThrift(thriftSensorData);
  // Expect non-zero sensors
  ASSERT_TRUE(thriftSensorData->size());

  // Allow time for fan_service to warm up and sync all the sensor data from
  // fsdb. We should expect to sync all the sensors that were received from
  // thrift earlier
  WITH_RETRIES_N_TIMED(
      {
        getFanService()->controlFan();
        prevLastFetchTime = getFanService()->lastSensorFetchTimeSec();
        prevSensorData = getFanService()->sensorData();
        // Confirm that the fan service received the same sensors from fsdb as
        // returned by sensor service via thrift
        ASSERT_EVENTUALLY_TRUE(
            prevSensorData.size() >= thriftSensorData->size());
        for (auto it = thriftSensorData->begin(); it != thriftSensorData->end();
             it++) {
          ASSERT_EVENTUALLY_TRUE(prevSensorData.checkIfEntryExists(it->first));
        }
      },
      6,
      std::chrono::seconds(10));

  // Stop FSDB
  stopFsdbService();

  // With FSDB stopped, we shouldn't receive any new sensor updates. Fetch the
  // sensor data for two SensorFetchFrequency intervals and confirm that the
  // sensor timestamps don't advance
  auto fetchFrequencyInSec = getFanService()->getSensorFetchFrequency();
  XLOG(INFO) << "Verifying that there are no sensor updates for "
             << (2 * fetchFrequencyInSec + 10) << " seconds";
  /* sleep override */
  sleep(2 * fetchFrequencyInSec + 10);
  getFanService()->controlFan();
  auto currSensorData = getFanService()->sensorData();
  for (auto it = thriftSensorData->begin(); it != thriftSensorData->end();
       it++) {
    ASSERT_TRUE(currSensorData.checkIfEntryExists(it->first));
    ASSERT_EQ(
        currSensorData.getLastUpdated(it->first),
        prevSensorData.getLastUpdated(it->first));
  }

  // Start FSDB
  startFsdbService();

  // Expect the lastFetchTime to advance and number of sensors to be same as
  // last time
  WITH_RETRIES_N_TIMED(
      {
        getFanService()->controlFan();
        auto currSensorData = getFanService()->sensorData();
        auto afterLastFetchTime = getFanService()->lastSensorFetchTimeSec();
        ASSERT_EVENTUALLY_TRUE(afterLastFetchTime > prevLastFetchTime);
        ASSERT_EVENTUALLY_TRUE(currSensorData.size() == prevSensorData.size());
        for (auto it = thriftSensorData->begin(); it != thriftSensorData->end();
             it++) {
          ASSERT_EVENTUALLY_TRUE(currSensorData.checkIfEntryExists(it->first));
          XLOG(INFO) << "Sensor: " << it->first << ". Previous timestamp: "
                     << prevSensorData.getLastUpdated(it->first)
                     << ", Current timestamp: "
                     << currSensorData.getLastUpdated(it->first);
          // The timestamps should advance. Sometimes the timestamps are 0 for
          // some sensors returned by sensor data, so add a special check for
          // that too
          ASSERT_EVENTUALLY_TRUE(
              (currSensorData.getLastUpdated(it->first) >
               prevSensorData.getLastUpdated(it->first)) ||
              (currSensorData.getLastUpdated(it->first) == 0 &&
               prevSensorData.getLastUpdated(it->first) == 0));
        }
      },
      6,
      std::chrono::seconds(10));
}

} // namespace facebook::fboss::platform
