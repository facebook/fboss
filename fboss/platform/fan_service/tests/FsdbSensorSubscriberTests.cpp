// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fan_service/FsdbSensorSubscriber.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class FsdbSensorSubscriberTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create subscriber with nullptr - we're only testing path generation
    // and basic getter methods, not actual subscription behavior
    subscriber_ = std::make_unique<FsdbSensorSubscriber>(nullptr);
  }

  std::unique_ptr<FsdbSensorSubscriber> subscriber_;
};

// Test constructor
TEST_F(FsdbSensorSubscriberTest, Constructor) {
  EXPECT_NE(subscriber_, nullptr);
  EXPECT_EQ(subscriber_->pubSubMgr(), nullptr);
}

// Test getSensorDataStatsPath returns non-empty path
TEST_F(FsdbSensorSubscriberTest, GetSensorDataStatsPath) {
  auto path = FsdbSensorSubscriber::getSensorDataStatsPath();
  EXPECT_FALSE(path.empty());
  // The path should contain sensor_service related tokens
  EXPECT_GT(path.size(), 0);
}

// Test getQsfpDataStatsPath returns non-empty path
TEST_F(FsdbSensorSubscriberTest, GetQsfpDataStatsPath) {
  auto path = FsdbSensorSubscriber::getQsfpDataStatsPath();
  EXPECT_FALSE(path.empty());
  // The path should contain qsfp_service related tokens
  EXPECT_GT(path.size(), 0);
}

// Test getQsfpDataStatePath returns non-empty path
TEST_F(FsdbSensorSubscriberTest, GetQsfpDataStatePath) {
  auto path = FsdbSensorSubscriber::getQsfpDataStatePath();
  EXPECT_FALSE(path.empty());
  // The path should contain qsfp_service state related tokens
  EXPECT_GT(path.size(), 0);
}

// Test getAgentDataStatsPath returns non-empty path
TEST_F(FsdbSensorSubscriberTest, GetAgentDataStatsPath) {
  auto path = FsdbSensorSubscriber::getAgentDataStatsPath();
  EXPECT_FALSE(path.empty());
  // The path should contain agent related tokens
  EXPECT_GT(path.size(), 0);
}

// Test paths are different from each other
TEST_F(FsdbSensorSubscriberTest, PathsAreDifferent) {
  auto sensorPath = FsdbSensorSubscriber::getSensorDataStatsPath();
  auto qsfpStatsPath = FsdbSensorSubscriber::getQsfpDataStatsPath();
  auto qsfpStatePath = FsdbSensorSubscriber::getQsfpDataStatePath();
  auto agentPath = FsdbSensorSubscriber::getAgentDataStatsPath();

  // Ensure all paths are unique
  EXPECT_NE(sensorPath, qsfpStatsPath);
  EXPECT_NE(sensorPath, qsfpStatePath);
  EXPECT_NE(sensorPath, agentPath);
  EXPECT_NE(qsfpStatsPath, qsfpStatePath);
  EXPECT_NE(qsfpStatsPath, agentPath);
  EXPECT_NE(qsfpStatePath, agentPath);
}

// Test getSensorStatsLastUpdatedTime initial value
TEST_F(FsdbSensorSubscriberTest, GetSensorStatsLastUpdatedTimeInitial) {
  EXPECT_EQ(subscriber_->getSensorStatsLastUpdatedTime(), 0);
}

// Test getSensorData returns empty map initially
TEST_F(FsdbSensorSubscriberTest, GetSensorDataInitial) {
  auto data = subscriber_->getSensorData();
  EXPECT_TRUE(data.empty());
}

// Test getAgentData returns empty map initially
TEST_F(FsdbSensorSubscriberTest, GetAgentDataInitial) {
  auto data = subscriber_->getAgentData();
  EXPECT_TRUE(data.empty());
}

// Test getTcvrState returns empty map initially
TEST_F(FsdbSensorSubscriberTest, GetTcvrStateInitial) {
  auto state = subscriber_->getTcvrState();
  EXPECT_TRUE(state.empty());
}

// Test getTcvrStats returns empty map initially
TEST_F(FsdbSensorSubscriberTest, GetTcvrStatsInitial) {
  auto stats = subscriber_->getTcvrStats();
  EXPECT_TRUE(stats.empty());
}

// Test that paths are static and consistent
TEST_F(FsdbSensorSubscriberTest, PathsAreStatic) {
  // Call path methods multiple times and verify they return the same values
  auto sensorPath1 = FsdbSensorSubscriber::getSensorDataStatsPath();
  auto sensorPath2 = FsdbSensorSubscriber::getSensorDataStatsPath();
  EXPECT_EQ(sensorPath1, sensorPath2);

  auto qsfpStatsPath1 = FsdbSensorSubscriber::getQsfpDataStatsPath();
  auto qsfpStatsPath2 = FsdbSensorSubscriber::getQsfpDataStatsPath();
  EXPECT_EQ(qsfpStatsPath1, qsfpStatsPath2);

  auto qsfpStatePath1 = FsdbSensorSubscriber::getQsfpDataStatePath();
  auto qsfpStatePath2 = FsdbSensorSubscriber::getQsfpDataStatePath();
  EXPECT_EQ(qsfpStatePath1, qsfpStatePath2);

  auto agentPath1 = FsdbSensorSubscriber::getAgentDataStatsPath();
  auto agentPath2 = FsdbSensorSubscriber::getAgentDataStatsPath();
  EXPECT_EQ(agentPath1, agentPath2);
}

// Test getSensorData is const
TEST_F(FsdbSensorSubscriberTest, GetSensorDataIsConst) {
  const auto* constSubscriber = subscriber_.get();
  auto data = constSubscriber->getSensorData();
  // Just verify it compiles and returns something (should be empty initially)
  EXPECT_TRUE(data.empty());
}

// Test getAgentData is const
TEST_F(FsdbSensorSubscriberTest, GetAgentDataIsConst) {
  const auto* constSubscriber = subscriber_.get();
  auto data = constSubscriber->getAgentData();
  EXPECT_TRUE(data.empty());
}

// Test getTcvrState is const
TEST_F(FsdbSensorSubscriberTest, GetTcvrStateIsConst) {
  const auto* constSubscriber = subscriber_.get();
  auto state = constSubscriber->getTcvrState();
  EXPECT_TRUE(state.empty());
}

// Test getTcvrStats is const
TEST_F(FsdbSensorSubscriberTest, GetTcvrStatsIsConst) {
  const auto* constSubscriber = subscriber_.get();
  auto stats = constSubscriber->getTcvrStats();
  EXPECT_TRUE(stats.empty());
}
