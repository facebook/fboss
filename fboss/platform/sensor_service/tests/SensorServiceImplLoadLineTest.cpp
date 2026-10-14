// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>

#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/SensorServiceImpl.h"

using namespace facebook::fboss::platform::sensor_service;
using namespace facebook::fboss::platform::sensor_config;

namespace facebook::fboss {

class SensorServiceImplLoadLineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    PmUnitSensors pmUnitSensors;
    pmUnitSensors.slotPath() = "/";
    pmUnitSensors.pmUnitName() = "MOCK";
    config_.pmUnitSensorsList() = {pmUnitSensors};
  }

  // Build a voltage PmSensor configured for load-line droop compensation.
  PmSensor makeLoadLineSensor(
      const std::string& name,
      const std::string& currentSensorName,
      int32_t uOhms) {
    PmSensor sensor;
    sensor.name() = name;
    sensor.sysfsPath() = "/tmp/dummy/" + name;
    sensor.type() = SensorType::VOLTAGE;
    sensor.loadLineCurrentSensor() = currentSensorName;
    sensor.loadLineuOhms() = uOhms;
    return sensor;
  }

  std::map<std::string, SensorData> createPolledData(
      const std::map<std::string, float>& sensorData) {
    std::map<std::string, SensorData> polledData;
    for (const auto& [sensorName, value] : sensorData) {
      SensorData sData;
      sData.name() = sensorName;
      sData.value() = value;
      sData.sensorType() = SensorType::VOLTAGE;
      sData.thresholds() = Thresholds();
      polledData.emplace(sensorName, sData);
    }
    return polledData;
  }

  // Add a polledData entry for a sensor whose read failed (no value set).
  void addReadFailure(
      std::map<std::string, SensorData>& polledData,
      const std::string& sensorName) {
    SensorData sData;
    sData.name() = sensorName;
    sData.sensorType() = SensorType::VOLTAGE;
    sData.thresholds() = Thresholds();
    polledData.emplace(sensorName, sData);
  }

  // Expected adjusted vout with load-line droops compensated
  float expectedVout(float rawVout, int32_t uOhms, float amps) {
    return static_cast<float>(
        static_cast<double>(rawVout) + uOhms * 1e-6 * amps);
  }

  // Build a full SensorConfig with a load-line VOUT sensor and its IOUT
  // companion, both backed by temp sysfs files, then write their raw
  // readings. compute "@/1000" is applied during fetchSensorData.
  void setupFetchConfig(
      const std::string& rawVout,
      const std::string& rawIout,
      int32_t uOhms) {
    auto voutPath = tmpDir_.path().string() + "/vout";
    auto ioutPath = tmpDir_.path().string() + "/iout";

    PmUnitSensors pmUnitSensors;
    pmUnitSensors.slotPath() = "/";
    pmUnitSensors.pmUnitName() = "MOCK";

    PmSensor vout;
    vout.name() = "VDD_CORE_VOUT";
    vout.sysfsPath() = voutPath;
    vout.type() = SensorType::VOLTAGE;
    vout.compute() = "@/1000";
    vout.loadLineCurrentSensor() = "VDD_CORE_IOUT";
    vout.loadLineuOhms() = uOhms;
    vout.thresholds() = Thresholds();

    PmSensor iout;
    iout.name() = "VDD_CORE_IOUT";
    iout.sysfsPath() = ioutPath;
    iout.type() = SensorType::CURRENT;
    iout.compute() = "@/1000";

    pmUnitSensors.sensors() = {std::move(vout), std::move(iout)};
    config_.pmUnitSensorsList() = {std::move(pmUnitSensors)};

    folly::writeFile(rawVout, voutPath.c_str());
    folly::writeFile(rawIout, ioutPath.c_str());
  }

  void createImpl() {
    impl_ = std::make_unique<SensorServiceImpl>(config_);
  }

  int64_t readValueCounter(const std::string& sensorName) {
    return fb303::fbData->getCounter(
        fmt::format(SensorServiceImpl::kReadValue, sensorName));
  }

  int64_t readFailureCounter(const std::string& sensorName) {
    return fb303::fbData->getCounter(
        fmt::format(SensorServiceImpl::kReadFailure, sensorName));
  }

  // tmpDir_ must outlive impl_ so its sysfs files remain readable while
  // fetchSensorData runs.
  folly::test::TemporaryDirectory tmpDir_;
  SensorConfig config_;
  std::unique_ptr<SensorServiceImpl> impl_;
};

TEST_F(SensorServiceImplLoadLineTest, AdjustsVoutByLoadLine) {
  createImpl();
  auto polledData =
      createPolledData({{"VDD_CORE_VOUT", 0.766f}, {"VDD_CORE_IOUT", 172.75f}});
  std::vector<PmSensor> loadLineSensors = {
      makeLoadLineSensor("VDD_CORE_VOUT", "VDD_CORE_IOUT", 59)};
  impl_->processLoadLineDroopAdjust(polledData, loadLineSensors);

  EXPECT_FLOAT_EQ(
      *polledData["VDD_CORE_VOUT"].value(), expectedVout(0.766f, 59, 172.75f));
  EXPECT_EQ(readFailureCounter("VDD_CORE_VOUT"), 0);
}

TEST_F(SensorServiceImplLoadLineTest, ZeroCurrentLeavesVoutUnchanged) {
  createImpl();
  auto polledData =
      createPolledData({{"VDD_CORE_VOUT", 0.766f}, {"VDD_CORE_IOUT", 0.0f}});
  std::vector<PmSensor> loadLineSensors = {
      makeLoadLineSensor("VDD_CORE_VOUT", "VDD_CORE_IOUT", 59)};
  impl_->processLoadLineDroopAdjust(polledData, loadLineSensors);

  EXPECT_FLOAT_EQ(*polledData["VDD_CORE_VOUT"].value(), 0.766f);
}

TEST_F(SensorServiceImplLoadLineTest, MissingCurrentSensorLeavesVoutUnchanged) {
  createImpl();
  auto polledData = createPolledData({{"VDD_CORE_VOUT", 0.766f}});
  std::vector<PmSensor> loadLineSensors = {
      makeLoadLineSensor("VDD_CORE_VOUT", "VDD_CORE_IOUT", 59)};
  impl_->processLoadLineDroopAdjust(polledData, loadLineSensors);

  EXPECT_FLOAT_EQ(*polledData["VDD_CORE_VOUT"].value(), 0.766f);
  EXPECT_EQ(readFailureCounter("VDD_CORE_VOUT"), 0);
}

TEST_F(SensorServiceImplLoadLineTest, ReadFailureVoutStillPublished) {
  createImpl();
  auto polledData = createPolledData({{"FAIL_IOUT", 172.75f}});
  addReadFailure(polledData, "FAIL_VOUT");
  std::vector<PmSensor> loadLineSensors = {
      makeLoadLineSensor("FAIL_VOUT", "FAIL_IOUT", 59)};
  impl_->processLoadLineDroopAdjust(polledData, loadLineSensors);

  EXPECT_FALSE(polledData["FAIL_VOUT"].value().has_value());
  EXPECT_EQ(readFailureCounter("FAIL_VOUT"), 1);
}

TEST_F(SensorServiceImplLoadLineTest, MissingVoltageSensorSkipped) {
  createImpl();
  auto polledData = createPolledData({{"ABSENT_IOUT", 172.75f}});
  std::vector<PmSensor> loadLineSensors = {
      makeLoadLineSensor("ABSENT_VOUT", "ABSENT_IOUT", 59)};
  impl_->processLoadLineDroopAdjust(polledData, loadLineSensors);

  EXPECT_THROW(readValueCounter("ABSENT_VOUT"), std::invalid_argument);
}

TEST_F(SensorServiceImplLoadLineTest, FetchSensorDataAppliesLoadLine) {
  setupFetchConfig("766", "172750", 59);
  createImpl();
  impl_->fetchSensorData();

  auto all = impl_->getAllSensorData();
  ASSERT_TRUE(all.find("VDD_CORE_VOUT") != all.end());
  EXPECT_FLOAT_EQ(
      *all["VDD_CORE_VOUT"].value(), expectedVout(0.766f, 59, 172.75f));
}

} // namespace facebook::fboss
