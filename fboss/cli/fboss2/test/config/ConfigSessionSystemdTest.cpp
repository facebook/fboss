/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/session/FbossServiceUtil.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/test/config/MockFbossServiceUtil.h"
#include "fboss/cli/fboss2/test/config/MockSystemdInterface.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using ::testing::_; // NOLINT(bugprone-reserved-identifier)
using ::testing::InSequence;
using ::testing::Return;
using ::testing::Throw;

namespace facebook::fboss {

namespace {

// Build a switch info map with a single switch at index 0
std::map<int64_t, cfg::SwitchInfo> makeSingleSwitchInfoMap() {
  cfg::SwitchInfo info;
  info.switchIndex() = 0;
  return {{0, info}};
}

// Build an empty switch info map (monolithic mode has no split switch info)
std::map<int64_t, cfg::SwitchInfo> makeEmptySwitchInfoMap() {
  return {};
}

} // namespace

// Test: isSplitMode() returns true when multi_switch flag is set
TEST(FbossServiceUtilTest, IsSplitMode_ReturnsTrueWhenMultiSwitchSet) {
  auto mockSystemd = std::make_unique<MockSystemdInterface>();

  FbossServiceUtil util(
      makeSingleSwitchInfoMap(), /*multiSwitch=*/true, std::move(mockSystemd));
  EXPECT_TRUE(util.isSplitMode());
}

// Test: isSplitMode() returns false when multi_switch flag is not set
TEST(FbossServiceUtilTest, IsSplitMode_ReturnsFalseWhenMultiSwitchNotSet) {
  auto mockSystemd = std::make_unique<MockSystemdInterface>();

  FbossServiceUtil util(
      makeEmptySwitchInfoMap(), /*multiSwitch=*/false, std::move(mockSystemd));
  EXPECT_FALSE(util.isSplitMode());
}

// Test: Monolithic mode warmboot restart
TEST(FbossServiceUtilTest, RestartService_MonolithicMode_Warmboot) {
  auto mockSystemd = std::make_unique<MockSystemdInterface>();

  EXPECT_CALL(*mockSystemd, restartService("wedge_agent")).Times(1);
  EXPECT_CALL(*mockSystemd, waitForServiceActive("wedge_agent", _, _)).Times(1);

  FbossServiceUtil util(
      makeEmptySwitchInfoMap(), /*multiSwitch=*/false, std::move(mockSystemd));

  auto services = util.restartService(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);

  EXPECT_EQ(services.size(), 1);
  EXPECT_EQ(services[0], "wedge_agent");
}

// Test: Monolithic mode coldboot restart
TEST(FbossServiceUtilTest, RestartService_MonolithicMode_Coldboot) {
  auto mockSystemd = std::make_unique<MockSystemdInterface>();

  // Expect marker creation, then restart -> wait for coldboot
  InSequence seq;
  EXPECT_CALL(*mockSystemd, restartService("wedge_agent")).Times(1);
  EXPECT_CALL(*mockSystemd, waitForServiceActive("wedge_agent", _, _)).Times(1);

  FbossServiceUtil util(
      makeEmptySwitchInfoMap(), /*multiSwitch=*/false, std::move(mockSystemd));

  auto services = util.restartService(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_COLDBOOT);

  EXPECT_EQ(services.size(), 1);
  EXPECT_EQ(services[0], "wedge_agent");
}

// Test: Split mode warmboot restart (single hw_agent)
// hw_agent must be restarted before sw_agent
TEST(FbossServiceUtilTest, RestartService_SplitMode_Warmboot_SingleHwAgent) {
  auto mockSystemd = std::make_unique<MockSystemdInterface>();

  // Each service: restart -> wait, in hw-before-sw order
  InSequence seq;
  EXPECT_CALL(*mockSystemd, restartService("fboss_hw_agent@0")).Times(1);
  EXPECT_CALL(*mockSystemd, waitForServiceActive("fboss_hw_agent@0", _, _))
      .Times(1);
  EXPECT_CALL(*mockSystemd, restartService("fboss_sw_agent")).Times(1);
  EXPECT_CALL(*mockSystemd, waitForServiceActive("fboss_sw_agent", _, _))
      .Times(1);

  FbossServiceUtil util(
      makeSingleSwitchInfoMap(), /*multiSwitch=*/true, std::move(mockSystemd));

  auto services = util.restartService(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);

  EXPECT_EQ(services.size(), 2);
  EXPECT_EQ(services[0], "fboss_hw_agent@0");
  EXPECT_EQ(services[1], "fboss_sw_agent");
}

// Test: Split mode coldboot restart (single hw_agent)
// hw_agent must be fully coldbooted before sw_agent
TEST(FbossServiceUtilTest, RestartService_SplitMode_Coldboot_SingleHwAgent) {
  auto mockSystemd = std::make_unique<MockSystemdInterface>();

  // Expect sequential coldboot: create marker -> restart -> wait for each
  // service
  InSequence seq;
  EXPECT_CALL(*mockSystemd, restartService("fboss_hw_agent@0")).Times(1);
  EXPECT_CALL(*mockSystemd, waitForServiceActive("fboss_hw_agent@0", _, _))
      .Times(1);
  EXPECT_CALL(*mockSystemd, restartService("fboss_sw_agent")).Times(1);
  EXPECT_CALL(*mockSystemd, waitForServiceActive("fboss_sw_agent", _, _))
      .Times(1);

  FbossServiceUtil util(
      makeSingleSwitchInfoMap(), /*multiSwitch=*/true, std::move(mockSystemd));

  auto services = util.restartService(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_COLDBOOT);

  EXPECT_EQ(services.size(), 2);
  EXPECT_EQ(services[0], "fboss_hw_agent@0");
  EXPECT_EQ(services[1], "fboss_sw_agent");
}

// Test: Split mode warmboot restart (multiple hw_agents)
// All hw_agents must be restarted before sw_agent
TEST(FbossServiceUtilTest, RestartService_SplitMode_Warmboot_MultipleHwAgents) {
  auto mockSystemd = std::make_unique<MockSystemdInterface>();

  // Each service: restart -> wait, in hw-before-sw order
  InSequence seq;
  EXPECT_CALL(*mockSystemd, restartService("fboss_hw_agent@0")).Times(1);
  EXPECT_CALL(*mockSystemd, waitForServiceActive("fboss_hw_agent@0", _, _))
      .Times(1);
  EXPECT_CALL(*mockSystemd, restartService("fboss_hw_agent@1")).Times(1);
  EXPECT_CALL(*mockSystemd, waitForServiceActive("fboss_hw_agent@1", _, _))
      .Times(1);
  EXPECT_CALL(*mockSystemd, restartService("fboss_sw_agent")).Times(1);
  EXPECT_CALL(*mockSystemd, waitForServiceActive("fboss_sw_agent", _, _))
      .Times(1);

  cfg::SwitchInfo info0;
  info0.switchIndex() = 0;
  cfg::SwitchInfo info1;
  info1.switchIndex() = 1;
  std::map<int64_t, cfg::SwitchInfo> switchInfoMap = {{0, info0}, {1, info1}};

  FbossServiceUtil util(
      std::move(switchInfoMap), /*multiSwitch=*/true, std::move(mockSystemd));

  auto services = util.restartService(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);

  EXPECT_EQ(services.size(), 3);
  EXPECT_EQ(services[0], "fboss_hw_agent@0");
  EXPECT_EQ(services[1], "fboss_hw_agent@1");
  EXPECT_EQ(services[2], "fboss_sw_agent");
}

// Test: Service restart failure is propagated
TEST(FbossServiceUtilTest, RestartService_PropagatesFailure) {
  auto mockSystemd = std::make_unique<MockSystemdInterface>();

  EXPECT_CALL(*mockSystemd, restartService("wedge_agent"))
      .WillOnce(Throw(std::runtime_error("Failed to restart wedge_agent")));

  FbossServiceUtil util(
      makeEmptySwitchInfoMap(), /*multiSwitch=*/false, std::move(mockSystemd));

  EXPECT_THROW(
      util.restartService(
          cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT),
      std::runtime_error);
}

// Test: Service fails to become active within timeout
TEST(FbossServiceUtilTest, RestartService_ServiceFailsToStart) {
  auto mockSystemd = std::make_unique<MockSystemdInterface>();

  EXPECT_CALL(*mockSystemd, restartService("wedge_agent")).Times(1);
  EXPECT_CALL(*mockSystemd, waitForServiceActive("wedge_agent", _, _))
      .WillOnce(Throw(
          std::runtime_error(
              "wedge_agent did not become active within 60 seconds")));

  FbossServiceUtil util(
      makeEmptySwitchInfoMap(), /*multiSwitch=*/false, std::move(mockSystemd));

  EXPECT_THROW(
      util.restartService(
          cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT),
      std::runtime_error);
}

// ============================================================
// ConfigSession integration tests using MockFbossServiceUtil
// These verify that ConfigSession::applyServiceActions() correctly
// delegates to fbossServiceUtil_ without touching real systemd or thrift.
// ============================================================

// Test: applyServiceActions() delegates AGENT_WARMBOOT to
// FbossServiceUtil::restartService()
TEST(
    ConfigSessionServiceTest,
    ApplyServiceActions_Warmboot_DelegatesToRestartService) {
  auto mock = std::make_unique<MockFbossServiceUtil>();
  auto* mockPtr = mock.get();

  HostInfo hostInfo(
      "localhost", "localhost-oob", folly::IPAddress("127.0.0.1"));

  EXPECT_CALL(
      *mockPtr,
      restartService(
          cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT))
      .WillOnce(::testing::Return(std::vector<std::string>{"wedge_agent"}));

  TestableConfigSession session(
      "/tmp/test_session", "/tmp/test_system", std::move(mock));

  std::map<cli::ServiceType, cli::ConfigActionLevel> actions = {
      {cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT}};
  auto serviceNames = session.applyServiceActions(actions, hostInfo);

  ASSERT_EQ(serviceNames[cli::ServiceType::AGENT].size(), 1);
  EXPECT_EQ(serviceNames[cli::ServiceType::AGENT][0], "wedge_agent");
}

// Test: applyServiceActions() delegates AGENT_COLDBOOT to
// FbossServiceUtil::restartService()
TEST(
    ConfigSessionServiceTest,
    ApplyServiceActions_Coldboot_DelegatesToRestartService) {
  auto mock = std::make_unique<MockFbossServiceUtil>();
  auto* mockPtr = mock.get();

  HostInfo hostInfo(
      "localhost", "localhost-oob", folly::IPAddress("127.0.0.1"));

  EXPECT_CALL(
      *mockPtr,
      restartService(
          cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_COLDBOOT))
      .WillOnce(
          ::testing::Return(
              std::vector<std::string>{"fboss_hw_agent@0", "fboss_sw_agent"}));

  TestableConfigSession session(
      "/tmp/test_session", "/tmp/test_system", std::move(mock));

  std::map<cli::ServiceType, cli::ConfigActionLevel> actions = {
      {cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_COLDBOOT}};
  auto serviceNames = session.applyServiceActions(actions, hostInfo);

  ASSERT_EQ(serviceNames[cli::ServiceType::AGENT].size(), 2);
  EXPECT_EQ(serviceNames[cli::ServiceType::AGENT][0], "fboss_hw_agent@0");
  EXPECT_EQ(serviceNames[cli::ServiceType::AGENT][1], "fboss_sw_agent");
}

// Test: ConfigSession::applyServiceActions() delegates HITLESS to
// FbossServiceUtil::reloadConfig() (monolithic mode)
TEST(
    ConfigSessionServiceTest,
    ApplyServiceActions_Hitless_DelegatesToReloadConfig_Monolithic) {
  auto mock = std::make_unique<MockFbossServiceUtil>();
  auto* mockPtr = mock.get();

  HostInfo hostInfo(
      "localhost", "localhost-oob", folly::IPAddress("127.0.0.1"));

  EXPECT_CALL(*mockPtr, reloadConfig(cli::ServiceType::AGENT, ::testing::_))
      .WillOnce(::testing::Return(std::vector<std::string>{"wedge_agent"}));

  TestableConfigSession session(
      "/tmp/test_session", "/tmp/test_system", std::move(mock));

  std::map<cli::ServiceType, cli::ConfigActionLevel> actions = {
      {cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS}};

  auto serviceNames = session.applyServiceActions(actions, hostInfo);

  ASSERT_EQ(serviceNames.size(), 1);
  ASSERT_EQ(serviceNames[cli::ServiceType::AGENT].size(), 1);
  EXPECT_EQ(serviceNames[cli::ServiceType::AGENT][0], "wedge_agent");
}

// Test: ConfigSession::applyServiceActions() delegates HITLESS to
// FbossServiceUtil::reloadConfig() (split mode)
TEST(
    ConfigSessionServiceTest,
    ApplyServiceActions_Hitless_DelegatesToReloadConfig_SplitMode) {
  auto mock = std::make_unique<MockFbossServiceUtil>();
  auto* mockPtr = mock.get();

  HostInfo hostInfo(
      "localhost", "localhost-oob", folly::IPAddress("127.0.0.1"));

  EXPECT_CALL(*mockPtr, reloadConfig(cli::ServiceType::AGENT, ::testing::_))
      .WillOnce(::testing::Return(std::vector<std::string>{"fboss_sw_agent"}));

  TestableConfigSession session(
      "/tmp/test_session", "/tmp/test_system", std::move(mock));

  std::map<cli::ServiceType, cli::ConfigActionLevel> actions = {
      {cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS}};

  auto serviceNames = session.applyServiceActions(actions, hostInfo);

  ASSERT_EQ(serviceNames.size(), 1);
  ASSERT_EQ(serviceNames[cli::ServiceType::AGENT].size(), 1);
  EXPECT_EQ(serviceNames[cli::ServiceType::AGENT][0], "fboss_sw_agent");
}

} // namespace facebook::fboss
