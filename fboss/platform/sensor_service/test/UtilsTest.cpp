// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/Utils.h"

using namespace ::testing;
namespace facebook::fboss::platform::sensor_service {
class MockPmUnitInfoFetcher : public PmUnitInfoFetcher {
 public:
  explicit MockPmUnitInfoFetcher() : PmUnitInfoFetcher() {}
  MOCK_METHOD(
      (std::optional<std::array<int16_t, 3>>),
      fetch,
      (const std::string&),
      (const));
};

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
  bool isEqual(VersionedPmSensor s1, VersionedPmSensor s2) {
    return s1.productProductionState() == s2.productProductionState() &&
        s1.productVersion() == s2.productVersion() &&
        s1.productSubVersion() == s2.productSubVersion();
  }
  MockPmUnitInfoFetcher fetcher_;
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

TEST_F(UtilsTests, PmUnitInfoFetcherTest) {
  std::optional<VersionedPmSensor> resolvedVersionedSensor;
  // Case-0: Empty version config
  EXPECT_EQ(
      Utils().resolveVersionedSensors(fetcher_, slotPath_, {}), std::nullopt);
  // Case-1: Non existence PmUnitInfo (e.g no IDPROM)
  EXPECT_CALL(fetcher_, fetch(_)).WillOnce(Return(std::nullopt));
  EXPECT_EQ(
      Utils().resolveVersionedSensors(
          fetcher_, slotPath_, {createVersionedPmSensor(1, 1, 2)}),
      std::nullopt);
  // Case-2: Non-matching VersionedPmSensor
  EXPECT_CALL(fetcher_, fetch(_))
      .WillOnce(Return(std::array<int16_t, 3>{1, 0, 20}));
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      fetcher_, slotPath_, {createVersionedPmSensor(1, 1, 2)});
  EXPECT_EQ(resolvedVersionedSensor, std::nullopt);
  // Case-3a: Matching Single VersionedPmSensors
  EXPECT_CALL(fetcher_, fetch(_))
      .WillOnce(Return(std::array<int16_t, 3>{1, 1, 20}));
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      fetcher_, slotPath_, {createVersionedPmSensor(1, 1, 2)});
  EXPECT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_TRUE(
      isEqual(*resolvedVersionedSensor, createVersionedPmSensor(1, 1, 2)));
  // Case-3b: Matching Multiple VersionedPmSensors
  EXPECT_CALL(fetcher_, fetch(_))
      .WillOnce(Return(std::array<int16_t, 3>{1, 1, 20}));
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      fetcher_,
      slotPath_,
      {createVersionedPmSensor(1, 1, 2), createVersionedPmSensor(1, 1, 4)});
  EXPECT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_TRUE(
      isEqual(*resolvedVersionedSensor, createVersionedPmSensor(1, 1, 4)));
  // Case-4: Matching Unordered VersionedPmSensors
  EXPECT_CALL(fetcher_, fetch(_))
      .WillOnce(Return(std::array<int16_t, 3>{3, 1, 20}));
  resolvedVersionedSensor = Utils().resolveVersionedSensors(
      fetcher_,
      slotPath_,
      {createVersionedPmSensor(3, 1, 20),
       createVersionedPmSensor(2, 1, 20),
       createVersionedPmSensor(2, 3, 20)});
  EXPECT_NE(resolvedVersionedSensor, std::nullopt);
  EXPECT_TRUE(
      isEqual(*resolvedVersionedSensor, createVersionedPmSensor(3, 1, 20)));
}
} // namespace facebook::fboss::platform::sensor_service
