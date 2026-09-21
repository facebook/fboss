// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>

#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/SensorServiceImpl.h"

using namespace facebook::fboss::platform::sensor_service;
using namespace facebook::fboss::platform::sensor_config;
namespace pm = facebook::fboss::platform::platform_manager;

namespace facebook::fboss {

inline pm::PmUnitInfo makePmUnitInfo(bool isPresent) {
  pm::PresenceInfo presenceInfo;
  presenceInfo.isPresent() = isPresent;
  pm::PmUnitInfo pmUnitInfo;
  pmUnitInfo.name() = "FAKE";
  pmUnitInfo.presenceInfo() = presenceInfo;
  return pmUnitInfo;
}

class SensorServiceImplInputVoltageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    PmUnitSensors pmUnitSensors;
    pmUnitSensors.slotPath() = "/";
    pmUnitSensors.pmUnitName() = "MOCK_PSU";
    config_.pmUnitSensorsList() = {pmUnitSensors};

    PowerConfig powerConfig;
    powerConfig.perSlotPowerConfigs() = {};
    powerConfig.otherPowerSensorNames() = {};
    powerConfig.powerDelta() = 0.0;
    powerConfig.inputVoltageSensors() = {};
    config_.powerConfig() = powerConfig;
  }

  void addInputVoltageSensors(const std::vector<std::string>& sensorNames) {
    config_.powerConfig()->inputVoltageSensors() = sensorNames;
    for (const auto& name : sensorNames) {
      PmSensor sensor;
      sensor.name() = name;
      sensor.sysfsPath() = "/tmp/dummy/" + name;
      config_.pmUnitSensorsList()[0].sensors()->push_back(sensor);
    }
  }

  // Append a PmUnitSensors entry. Defaults to a PSU. Pass pmUnitName
  // explicitly ("PEM", "HSC", etc.) for non-PSU slots. publishPsuPemCounters
  // counts physical slots from pmUnitSensorsList where pmUnitName ==
  // "PSU" || "PEM".
  void addPmUnit(
      const std::string& slotPath,
      const std::string& pmUnitName = "PSU") {
    PmUnitSensors pmUnitSensors;
    pmUnitSensors.slotPath() = slotPath;
    pmUnitSensors.pmUnitName() = pmUnitName;
    config_.pmUnitSensorsList()->push_back(pmUnitSensors);
  }

  // Build a pmUnitInfoMap from {slotPath → presence}. Uses std::nullopt
  // for slots whose presence is "unknown" (RPC failed). For "missing
  // presenceInfo" cases, construct the map manually.
  std::map<std::string, std::optional<pm::PmUnitInfo>> makePmUnitInfoMap(
      std::initializer_list<std::pair<std::string, bool>> entries) {
    std::map<std::string, std::optional<pm::PmUnitInfo>> map;
    for (const auto& [slotPath, present] : entries) {
      map[slotPath] = makePmUnitInfo(present);
    }
    return map;
  }

  std::map<std::string, SensorData> createPolledData(
      const std::map<std::string, float>& sensorData,
      const std::map<std::string, std::string>& slotPaths = {}) {
    std::map<std::string, SensorData> polledData;
    for (const auto& [sensorName, value] : sensorData) {
      SensorData sData;
      sData.value() = value;
      auto it = slotPaths.find(sensorName);
      if (it != slotPaths.end()) {
        sData.slotPath() = it->second;
      }
      polledData.emplace(sensorName, sData);
    }
    return polledData;
  }

  void runInputVoltageTest(std::map<std::string, SensorData>& polledData) {
    impl_->processInputVoltage(polledData, *config_.powerConfig());
  }

  void createImpl() {
    impl_ = std::make_unique<SensorServiceImpl>(config_);
  }

  void expectMaxVoltageStats(int expectedValue, int expectedFailures = 0) {
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(
                SensorServiceImpl::kDerivedValue,
                SensorServiceImpl::kMaxInputVoltage)),
        expectedValue);
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(
                SensorServiceImpl::kDerivedFailure,
                SensorServiceImpl::kMaxInputVoltage)),
        expectedFailures);
  }

  void expectInputPowerType(int expectedType) {
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(
                SensorServiceImpl::kDerivedValue,
                SensorServiceImpl::kInputPowerType)),
        expectedType);
  }

  void expectTotalNumPresentPsu(int expectedValue) {
    EXPECT_EQ(
        fb303::fbData->getCounter(SensorServiceImpl::kTotalNumPresentPsu),
        expectedValue);
  }

  void expectNoTotalNumPresentPsuCounter() {
    EXPECT_THROW(
        fb303::fbData->getCounter(SensorServiceImpl::kTotalNumPresentPsu),
        std::invalid_argument);
  }

  void expectUnexpectedNumPresentPsu(int expectedValue) {
    EXPECT_EQ(
        fb303::fbData->getCounter(SensorServiceImpl::kUnexpectedNumPresentPsu),
        expectedValue);
  }

  void expectNoUnexpectedNumPresentPsuCounter() {
    EXPECT_THROW(
        fb303::fbData->getCounter(SensorServiceImpl::kUnexpectedNumPresentPsu),
        std::invalid_argument);
  }

  SensorConfig config_;
  std::unique_ptr<SensorServiceImpl> impl_;
};

TEST_F(SensorServiceImplInputVoltageTest, MaxInputVoltageWithMultipleSensors) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();
  auto polledData =
      createPolledData({{"PSU1_VIN", 220.5}, {"PSU2_VIN", 215.3}});
  runInputVoltageTest(polledData);

  expectMaxVoltageStats(220);
}

TEST_F(SensorServiceImplInputVoltageTest, MaxInputVoltageWithMissingSensor) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 225.0}});
  runInputVoltageTest(polledData);

  expectMaxVoltageStats(225);
}

TEST_F(SensorServiceImplInputVoltageTest, EmptyInputVoltageSensors) {
  createImpl();
  auto polledData = createPolledData({});
  runInputVoltageTest(polledData);
  // Verify no crash when no sensors are configured
  SUCCEED();
}

TEST_F(SensorServiceImplInputVoltageTest, NoSensorData) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();
  auto polledData = createPolledData({});
  runInputVoltageTest(polledData);

  expectMaxVoltageStats(0, 1);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeUnknown);
}

TEST_F(SensorServiceImplInputVoltageTest, MaxInputVoltageWithZeroValue) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 0.0}, {"PSU2_VIN", 220.0}});
  runInputVoltageTest(polledData);

  expectMaxVoltageStats(220);
}

TEST_F(
    SensorServiceImplInputVoltageTest,
    InputPowerTypePersistsAcrossReadings) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();

  auto polledData1 =
      createPolledData({{"PSU1_VIN", 220.0}, {"PSU2_VIN", 215.0}});
  runInputVoltageTest(polledData1);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeAC);

  auto polledData2 = createPolledData({{"PSU1_VIN", 48.0}, {"PSU2_VIN", 50.0}});
  runInputVoltageTest(polledData2);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeAC);
}

TEST_F(SensorServiceImplInputVoltageTest, DCPowerTypeEstablished) {
  addInputVoltageSensors({"PSU1_VIN"});
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 50.0f}});
  runInputVoltageTest(polledData);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeDC);
}

TEST_F(SensorServiceImplInputVoltageTest, UnknownPowerTypeLowVoltage) {
  addInputVoltageSensors({"PSU1_VIN"});
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 5.0f}});
  runInputVoltageTest(polledData);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeUnknown);
}

TEST_F(SensorServiceImplInputVoltageTest, UnknownPowerTypeGapVoltage) {
  addInputVoltageSensors({"PSU1_VIN"});
  createImpl();
  // 75V is between dcVoltageMax (64) and acVoltageMin (90)
  auto polledData = createPolledData({{"PSU1_VIN", 75.0f}});
  runInputVoltageTest(polledData);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeUnknown);
  // No thresholds should be assigned when power type is unknown
  EXPECT_FALSE(
      polledData["PSU1_VIN"].thresholds()->lowerCriticalVal().has_value());
  EXPECT_FALSE(
      polledData["PSU1_VIN"].thresholds()->upperCriticalVal().has_value());
}

TEST_F(SensorServiceImplInputVoltageTest, ThresholdsSetOnPowerTypeEstablished) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();

  auto polledData =
      createPolledData({{"PSU1_VIN", 220.0}, {"PSU2_VIN", 215.0}});

  EXPECT_FALSE(
      polledData["PSU1_VIN"].thresholds()->lowerCriticalVal().has_value());
  EXPECT_FALSE(
      polledData["PSU1_VIN"].thresholds()->upperCriticalVal().has_value());

  runInputVoltageTest(polledData);

  // Thresholds come from thrift PowerConfig defaults
  // (acVoltageMin=90, acVoltageMax=305)
  Thresholds expectedThresholds;
  expectedThresholds.lowerCriticalVal() = 90.0;
  expectedThresholds.upperCriticalVal() = 305.0;
  EXPECT_EQ(*polledData["PSU1_VIN"].thresholds(), expectedThresholds);
  EXPECT_EQ(*polledData["PSU2_VIN"].thresholds(), expectedThresholds);
}

TEST_F(SensorServiceImplInputVoltageTest, ThresholdsSetForDC) {
  addInputVoltageSensors({"PSU1_VIN"});
  createImpl();

  auto polledData = createPolledData({{"PSU1_VIN", 50.0}});
  runInputVoltageTest(polledData);

  // Thresholds come from thrift PowerConfig defaults
  // (dcVoltageMin=9, dcVoltageMax=64)
  Thresholds expectedThresholds;
  expectedThresholds.lowerCriticalVal() = 9.0;
  expectedThresholds.upperCriticalVal() = 64.0;
  EXPECT_EQ(*polledData["PSU1_VIN"].thresholds(), expectedThresholds);
}

TEST_F(
    SensorServiceImplInputVoltageTest,
    ThresholdsNotOverwrittenIfAlreadySet) {
  addInputVoltageSensors({"PSU1_VIN"});
  createImpl();

  auto polledData = createPolledData({{"PSU1_VIN", 220.0}});
  polledData["PSU1_VIN"].thresholds()->lowerCriticalVal() = 100.0f;
  polledData["PSU1_VIN"].thresholds()->upperCriticalVal() = 300.0f;

  runInputVoltageTest(polledData);

  Thresholds expectedThresholds;
  expectedThresholds.lowerCriticalVal() = 100.0f;
  expectedThresholds.upperCriticalVal() = 300.0f;
  EXPECT_EQ(*polledData["PSU1_VIN"].thresholds(), expectedThresholds);
}

TEST_F(SensorServiceImplInputVoltageTest, TotalNumPresentPsuAllPresent) {
  // Both physical PSU slots present → presentCount = 2. minAcPsuCount=2
  // satisfied → no alert.
  addInputVoltageSensors({"PSU1_VIN"});
  config_.powerConfig()->minAcPsuCount() = 2;
  // Make voltage sensor read so AC power type gets resolved.
  addPmUnit("/PSU_SLOT@0");
  addPmUnit("/PSU_SLOT@1");
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 220.0}});
  impl_->processInputVoltage(polledData, *config_.powerConfig());
  impl_->publishPsuPemCounters(
      makePmUnitInfoMap({{"/PSU_SLOT@0", true}, {"/PSU_SLOT@1", true}}),
      *config_.powerConfig());

  expectTotalNumPresentPsu(2);
  expectUnexpectedNumPresentPsu(0);
}

TEST_F(
    SensorServiceImplInputVoltageTest,
    UnexpectedNumPresentPsuFiresWhenAbsent) {
  // PSU2 absent → presentCount = 1. minAcPsuCount=2 → alert fires.
  addInputVoltageSensors({"PSU1_VIN"});
  config_.powerConfig()->minAcPsuCount() = 2;
  addPmUnit("/PSU_SLOT@0");
  addPmUnit("/PSU_SLOT@1");
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 220.0}});
  impl_->processInputVoltage(polledData, *config_.powerConfig());
  impl_->publishPsuPemCounters(
      makePmUnitInfoMap({{"/PSU_SLOT@0", true}, {"/PSU_SLOT@1", false}}),
      *config_.powerConfig());

  expectTotalNumPresentPsu(1);
  expectUnexpectedNumPresentPsu(1);
}

TEST_F(SensorServiceImplInputVoltageTest, TotalNumPresentPsuExcludesHsc) {
  // HSC PmUnitSensors entries (pmUnitName not in {"PSU","PEM"}) must not
  // count toward psu.total_num_present, even when present.
  addInputVoltageSensors({"PSU1_VIN"});
  config_.powerConfig()->minAcPsuCount() = 1;
  addPmUnit("/PSU_SLOT@0");
  addPmUnit("/HSC_SLOT@0", "HSC");
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 220.0}});
  impl_->processInputVoltage(polledData, *config_.powerConfig());
  impl_->publishPsuPemCounters(
      makePmUnitInfoMap({{"/PSU_SLOT@0", true}, {"/HSC_SLOT@0", true}}),
      *config_.powerConfig());

  expectTotalNumPresentPsu(1);
  expectUnexpectedNumPresentPsu(0);
}

TEST_F(
    SensorServiceImplInputVoltageTest,
    TotalNumPresentPsuWhenPresenceUnknown) {
  // platform_manager fetch returned nullopt for the slot (RPC failure or
  // absent presenceInfo). Treated as "presence unknown" — slot is not
  // counted as present, but a transient platform_manager outage must not
  // silently hide real PSU data: minAcPsuCount=1 → alert still fires from
  // presentCount=0.
  addInputVoltageSensors({"PSU1_VIN"});
  config_.powerConfig()->minAcPsuCount() = 1;
  addPmUnit("/PSU_SLOT@0");
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 220.0}});
  impl_->processInputVoltage(polledData, *config_.powerConfig());
  std::map<std::string, std::optional<pm::PmUnitInfo>> map;
  map["/PSU_SLOT@0"] = std::nullopt;
  impl_->publishPsuPemCounters(map, *config_.powerConfig());

  expectTotalNumPresentPsu(0);
  expectUnexpectedNumPresentPsu(1);
}

TEST_F(
    SensorServiceImplInputVoltageTest,
    UnexpectedNumPresentPsuSkippedWhenMinZero) {
  // PSU slot present but minAcPsuCount/minDcPsuCount left at 0
  // (defensive — validator should reject this). The count is still
  // published; the boolean is skipped because no threshold to compare.
  addInputVoltageSensors({"PSU1_VIN"});
  addPmUnit("/PSU_SLOT@0");
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 220.0}});
  impl_->processInputVoltage(polledData, *config_.powerConfig());
  impl_->publishPsuPemCounters(
      makePmUnitInfoMap({{"/PSU_SLOT@0", true}}), *config_.powerConfig());

  expectTotalNumPresentPsu(1);
  expectNoUnexpectedNumPresentPsuCounter();
}

TEST_F(
    SensorServiceImplInputVoltageTest,
    UnexpectedNumPresentPsuSkippedWhenPowerTypeUnknown) {
  // Voltage 75V falls between DC max (64) and AC min (90) → power type
  // can't be determined → boolean is skipped. Count is still published.
  addInputVoltageSensors({"PSU1_VIN"});
  config_.powerConfig()->minAcPsuCount() = 1;
  config_.powerConfig()->minDcPsuCount() = 1;
  addPmUnit("/PSU_SLOT@0");
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 75.0}});
  impl_->processInputVoltage(polledData, *config_.powerConfig());
  impl_->publishPsuPemCounters(
      makePmUnitInfoMap({{"/PSU_SLOT@0", true}}), *config_.powerConfig());

  expectInputPowerType(SensorServiceImpl::kInputPowerTypeUnknown);
  expectTotalNumPresentPsu(1);
  expectNoUnexpectedNumPresentPsuCounter();
}

TEST_F(SensorServiceImplInputVoltageTest, PresentPsuSkippedForHscOnlyPlatform) {
  // No PSU/PEM pmUnitSensorsList entries → counters are not published.
  addInputVoltageSensors({"HSC_VIN"});
  addPmUnit("/HSC_SLOT@0", "HSC");
  createImpl();
  auto polledData = createPolledData({{"HSC_VIN", 220.0}});
  impl_->processInputVoltage(polledData, *config_.powerConfig());
  impl_->publishPsuPemCounters(
      makePmUnitInfoMap({{"/HSC_SLOT@0", true}}), *config_.powerConfig());

  expectNoTotalNumPresentPsuCounter();
  expectNoUnexpectedNumPresentPsuCounter();
}

TEST_F(
    SensorServiceImplInputVoltageTest,
    DarwinSinglePhysicalPemMultipleLogicalEntries) {
  // Darwin topology: one physical PEM at /PEM_SLOT@0, but two logical
  // PerSlotPowerConfig entries (PEM1, PEM2) both at /PEM_SLOT@0 — used for
  // PIN = VIN×CH1 + VIN×CH2 calculation. Counter must report PHYSICAL = 1,
  // not logical = 2.
  addInputVoltageSensors({"PEM_VIN"});
  config_.powerConfig()->minAcPsuCount() = 1;
  config_.powerConfig()->minDcPsuCount() = 1;
  addPmUnit("/PEM_SLOT@0", "PEM"); // 1 physical PEM
  // 2 logical PerSlotPowerConfigs at the same slot
  for (const auto& name : {"PEM1", "PEM2"}) {
    PerSlotPowerConfig pem;
    pem.name() = name;
    pem.voltageSensorName() = "PEM_VIN";
    pem.currentSensorName() = std::string(name) + "_IIN";
    pem.slotPath() = "/PEM_SLOT@0";
    config_.powerConfig()->perSlotPowerConfigs()->push_back(pem);
  }
  createImpl();
  auto polledData = createPolledData({{"PEM_VIN", 220.0}});
  impl_->processInputVoltage(polledData, *config_.powerConfig());
  impl_->publishPsuPemCounters(
      makePmUnitInfoMap({{"/PEM_SLOT@0", true}}), *config_.powerConfig());

  expectTotalNumPresentPsu(1); // physical, NOT 2
  expectUnexpectedNumPresentPsu(0);
}

TEST_F(SensorServiceImplInputVoltageTest, ProcessPowerHonorsAbsentPsuSlot) {
  // processPower must skip per-slot _POWER publication for PSU/PEM slots
  // that pmUnitInfoMap reports as absent. Distinguishes "skipped via
  // absent" from "processed-but-value-missing" — for skip, neither
  // .value nor .failure should be published.
  addInputVoltageSensors({"PSU1_VIN"});
  config_.powerConfig()->minAcPsuCount() = 1;
  for (const auto& [name, path] :
       std::vector<std::pair<std::string, std::string>>{
           {"PSU1", "/PSU_SLOT@0"}, {"PSU2", "/PSU_SLOT@1"}}) {
    PerSlotPowerConfig psu;
    psu.name() = name;
    psu.powerSensorName() = name + "_PWR";
    psu.slotPath() = path;
    config_.powerConfig()->perSlotPowerConfigs()->push_back(psu);
  }
  createImpl();
  auto polledData = createPolledData(
      {{"PSU1_VIN", 220.0}, {"PSU1_PWR", 500.0}, {"PSU2_PWR", 500.0}});
  impl_->processInputVoltage(polledData, *config_.powerConfig());
  impl_->processPower(
      polledData,
      *config_.powerConfig(),
      makePmUnitInfoMap({{"/PSU_SLOT@0", true}, {"/PSU_SLOT@1", false}}));

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU1_POWER")),
      500.0);
  // Skipped — neither .value nor .failure should exist.
  EXPECT_THROW(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU2_POWER")),
      std::invalid_argument);
  EXPECT_THROW(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU2_POWER")),
      std::invalid_argument);
}

} // namespace facebook::fboss
