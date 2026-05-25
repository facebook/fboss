// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/Utils.h"

namespace facebook::fboss::platform::sensor_service {

class UtilsTests : public testing::Test {
 public:
  VersionedPmSensor createVersionedPmSensor(
      uint productProductionState,
      uint productVersion,
      uint productSubVersion) {
    VersionedPmSensor versionedPmSensor;
    versionedPmSensor.productProductionState() = productProductionState;
    versionedPmSensor.productVersion() = productVersion;
    versionedPmSensor.productSubVersion() = productSubVersion;
    return versionedPmSensor;
  }
  platform_manager::PmUnitInfo
  createPmUnitInfo(int16_t pps, int16_t pv, int16_t psv) {
    platform_manager::PmUnitInfo info;
    info.name() = "TestUnit";
    platform_manager::PmUnitVersion version;
    version.productProductionState() = pps;
    version.productVersion() = pv;
    version.productSubVersion() = psv;
    info.version() = version;
    return info;
  }
  bool isEqual(VersionedPmSensor s1, VersionedPmSensor s2) {
    return s1.productProductionState() == s2.productProductionState() &&
        s1.productVersion() == s2.productVersion() &&
        s1.productSubVersion() == s2.productSubVersion();
  }
  std::string slotPath_;
};

TEST_F(UtilsTests, Equal) {
  EXPECT_FLOAT_EQ(Utils::computeExpression("x * 0.1", 10.0), 1.0);
  EXPECT_FLOAT_EQ(Utils::computeExpression("x * 0.1/100", 10.0), 0.01);
  EXPECT_FLOAT_EQ(
      Utils::computeExpression("(x * 0.1+5)/1000", 100.0, "x"), 0.015);
  EXPECT_FLOAT_EQ(
      Utils::computeExpression("(y / 0.1+300)/ (1000*10 + 5)", 30.0, "y"),
      0.05997);
  EXPECT_FLOAT_EQ(
      Utils::computeExpression("(@ / 0.1+300)/ (1000*10 + 5)", 30.0, "x"),
      0.05997);
  EXPECT_FLOAT_EQ(
      Utils::computeExpression(
          "(@ / 0.1+300)/ (1000*10 + @ * 10000)", 30.0, "x"),
      0.0019354839);
}

TEST_F(UtilsTests, ResolveVersionedSensors) {
  std::optional<VersionedPmSensor> resolvedVersionedSensor;
  // Case-0: Empty version config
  EXPECT_EQ(
      Utils().resolveVersionedSensors(std::nullopt, slotPath_, {}),
      std::nullopt);
  // Case-1: Fail to fetch PmUnitInfo (RPC error)
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      std::nullopt,
      slotPath_,
      {createVersionedPmSensor(1, 1, 2), createVersionedPmSensor(2, 0, 1)});
  EXPECT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_TRUE(
      isEqual(*resolvedVersionedSensor, createVersionedPmSensor(2, 0, 1)));
  // Case-1b: PmUnitInfo returned but no version (no IDPROM)
  {
    platform_manager::PmUnitInfo infoNoVersion;
    infoNoVersion.name() = "TestUnit";
    resolvedVersionedSensor = Utils().resolveVersionedSensors(
        infoNoVersion,
        slotPath_,
        {createVersionedPmSensor(1, 1, 2), createVersionedPmSensor(2, 0, 1)});
    EXPECT_NE(resolvedVersionedSensor, std::nullopt);
    EXPECT_TRUE(
        isEqual(*resolvedVersionedSensor, createVersionedPmSensor(2, 0, 1)));
  }
  // Case-2: Non-matching VersionedPmSensor
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfo(1, 0, 20),
      slotPath_,
      {createVersionedPmSensor(1, 1, 2)});
  EXPECT_EQ(resolvedVersionedSensor, std::nullopt);
  // Case-3a: Matching Single VersionedPmSensors
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfo(1, 1, 20),
      slotPath_,
      {createVersionedPmSensor(1, 1, 2)});
  EXPECT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_TRUE(
      isEqual(*resolvedVersionedSensor, createVersionedPmSensor(1, 1, 2)));
  // Case-3b: Matching Multiple VersionedPmSensors
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfo(1, 1, 20),
      slotPath_,
      {createVersionedPmSensor(1, 1, 2), createVersionedPmSensor(1, 1, 4)});
  EXPECT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_TRUE(
      isEqual(*resolvedVersionedSensor, createVersionedPmSensor(1, 1, 4)));
  // Case-4: Matching Unordered VersionedPmSensors
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfo(2, 4, 10),
      slotPath_,
      {createVersionedPmSensor(3, 1, 20),
       createVersionedPmSensor(2, 1, 20),
       createVersionedPmSensor(2, 3, 20)});
  EXPECT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_TRUE(
      isEqual(*resolvedVersionedSensor, createVersionedPmSensor(2, 3, 20)));
}

TEST_F(UtilsTests, ResolveVersionedSensorsWithProductName) {
  std::optional<VersionedPmSensor> resolvedVersionedSensor;

  auto createVersionedPmSensorWithProductName =
      [this](uint pps, uint pv, uint psv, const std::string& productName) {
        auto vs = createVersionedPmSensor(pps, pv, psv);
        vs.productName() = productName;
        return vs;
      };
  auto createPmUnitInfoWithEepromProductName =
      [this](
          int16_t pps,
          int16_t pv,
          int16_t psv,
          const std::string& eepromProductName) {
        auto info = createPmUnitInfo(pps, pv, psv);
        info.eepromProductName() = eepromProductName;
        return info;
      };

  // Case-5: productName matches eepromProductName — use matching entry
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfoWithEepromProductName(1, 1, 20, "DC3K12V_M_L"),
      slotPath_,
      {createVersionedPmSensorWithProductName(1, 1, 2, "DC3K12V_M_L"),
       createVersionedPmSensorWithProductName(1, 1, 2, "AC3K12V_M_L")});
  ASSERT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_EQ(*resolvedVersionedSensor->productName(), "DC3K12V_M_L");

  // Case-6: productName does not match — no universal fallback → nullopt
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfoWithEepromProductName(1, 1, 20, "UNKNOWN_PSU"),
      slotPath_,
      {createVersionedPmSensorWithProductName(1, 1, 2, "DC3K12V_M_L"),
       createVersionedPmSensorWithProductName(1, 1, 2, "AC3K12V_M_L")});
  EXPECT_EQ(resolvedVersionedSensor, std::nullopt);

  // Case-7: productName not set on entry → universal fallback when no match
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfoWithEepromProductName(1, 1, 20, "UNKNOWN_PSU"),
      slotPath_,
      {createVersionedPmSensorWithProductName(1, 1, 2, "DC3K12V_M_L"),
       createVersionedPmSensor(1, 1, 2)});
  ASSERT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_FALSE(resolvedVersionedSensor->productName().has_value());

  // Case-8: productName matches — prefer over universal entry
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfoWithEepromProductName(1, 1, 20, "DC3K12V_M_L"),
      slotPath_,
      {createVersionedPmSensorWithProductName(1, 1, 2, "DC3K12V_M_L"),
       createVersionedPmSensor(1, 1, 4)});
  ASSERT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_EQ(*resolvedVersionedSensor->productName(), "DC3K12V_M_L");

  // Case-9: No eepromProductName on hardware — use universal entries only
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfo(1, 1, 20),
      slotPath_,
      {createVersionedPmSensorWithProductName(1, 1, 2, "DC3K12V_M_L"),
       createVersionedPmSensor(1, 1, 2)});
  ASSERT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_FALSE(resolvedVersionedSensor->productName().has_value());

  // Case-10: Multiple matching productName entries — pick highest version
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfoWithEepromProductName(2, 4, 10, "AC3K12V_M_L"),
      slotPath_,
      {createVersionedPmSensorWithProductName(2, 1, 0, "AC3K12V_M_L"),
       createVersionedPmSensorWithProductName(2, 3, 0, "AC3K12V_M_L"),
       createVersionedPmSensorWithProductName(2, 4, 0, "DC3K12V_M_L")});
  ASSERT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_TRUE(
      isEqual(*resolvedVersionedSensor, createVersionedPmSensor(2, 3, 0)));
  EXPECT_EQ(*resolvedVersionedSensor->productName(), "AC3K12V_M_L");

  // Case-11: productName-specific candidates were selected → universal entry
  // is not reconsidered, even when version doesn't satisfy → nullopt.
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      createPmUnitInfoWithEepromProductName(1, 0, 0, "DC3K12V_M_L"),
      slotPath_,
      {createVersionedPmSensorWithProductName(2, 0, 0, "DC3K12V_M_L"),
       createVersionedPmSensor(1, 0, 0)});
  EXPECT_EQ(resolvedVersionedSensor, std::nullopt);
}
} // namespace facebook::fboss::platform::sensor_service
