/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <boost/filesystem.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

#include "fboss/cli/fboss2/commands/config/qos/buffer_pool/CmdConfigQosBufferPool.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/utils/PortMap.h"

namespace fs = std::filesystem;

namespace facebook::fboss {

class CmdConfigQosBufferPoolTestFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directories
    auto tempBase = fs::temp_directory_path();
    auto uniquePath =
        boost::filesystem::unique_path("fboss_bp_test_%%%%-%%%%-%%%%-%%%%");
    testHomeDir_ = tempBase / (uniquePath.string() + "_home");
    testEtcDir_ = tempBase / (uniquePath.string() + "_etc");

    std::error_code ec;
    if (fs::exists(testHomeDir_)) {
      fs::remove_all(testHomeDir_, ec);
    }
    if (fs::exists(testEtcDir_)) {
      fs::remove_all(testEtcDir_, ec);
    }

    // Create test directories
    fs::create_directories(testHomeDir_);
    fs::create_directories(testEtcDir_ / "coop");
    fs::create_directories(testEtcDir_ / "coop" / "cli");

    // Set environment variables
    setenv("HOME", testHomeDir_.c_str(), 1);
    setenv("USER", "testuser", 1);

    // Create a test system config file
    fs::path initialRevision = testEtcDir_ / "coop" / "cli" / "agent-r1.conf";
    createTestConfig(initialRevision, R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000
      }
    ]
  }
})");

    // Create symlink
    systemConfigPath_ = testEtcDir_ / "coop" / "agent.conf";
    fs::create_symlink(initialRevision, systemConfigPath_);

    // Create session config path
    sessionConfigPath_ = testHomeDir_ / ".fboss2" / "agent.conf";
    cliConfigDir_ = testEtcDir_ / "coop" / "cli";
  }

  void TearDown() override {
    // Reset the singleton to ensure tests don't interfere with each other
    TestableConfigSession::setInstance(nullptr);

    std::error_code ec;
    if (fs::exists(testHomeDir_)) {
      fs::remove_all(testHomeDir_, ec);
    }
    if (fs::exists(testEtcDir_)) {
      fs::remove_all(testEtcDir_, ec);
    }
    CmdHandlerTestBase::TearDown();
  }

 protected:
  void createTestConfig(const fs::path& path, const std::string& content) {
    std::ofstream file(path);
    file << content;
    file.close();
  }

  std::string readFile(const fs::path& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }

  fs::path testHomeDir_;
  fs::path testEtcDir_;
  fs::path systemConfigPath_;
  fs::path sessionConfigPath_;
  fs::path cliConfigDir_;
};

// Test BufferPoolConfig argument validation
TEST_F(CmdConfigQosBufferPoolTestFixture, bufferPoolConfigValidation) {
  // Valid config with one attribute
  EXPECT_NO_THROW(BufferPoolConfig({"ingress_pool", "shared-bytes", "1000"}));
  EXPECT_NO_THROW(
      BufferPoolConfig({"egress-lossy-pool", "headroom-bytes", "2000"}));
  EXPECT_NO_THROW(BufferPoolConfig({"Pool1", "reserved-bytes", "3000"}));

  // Valid config with multiple attributes
  EXPECT_NO_THROW(BufferPoolConfig(
      {"test_pool", "shared-bytes", "1000", "headroom-bytes", "2000"}));
  EXPECT_NO_THROW(BufferPoolConfig(
      {"test_pool",
       "shared-bytes",
       "1000",
       "headroom-bytes",
       "2000",
       "reserved-bytes",
       "3000"}));

  // Empty should throw
  EXPECT_THROW(BufferPoolConfig({}), std::invalid_argument);

  // Name only (no attributes) should throw
  EXPECT_THROW(BufferPoolConfig({"pool1"}), std::invalid_argument);

  // Name with only one arg (odd number after name) should throw
  EXPECT_THROW(
      BufferPoolConfig({"pool1", "shared-bytes"}), std::invalid_argument);

  // Invalid pool names - must start with letter
  EXPECT_THROW(
      BufferPoolConfig({"123pool", "shared-bytes", "1000"}),
      std::invalid_argument);
  EXPECT_THROW(
      BufferPoolConfig({"_pool", "shared-bytes", "1000"}),
      std::invalid_argument);

  // Invalid attribute name
  EXPECT_THROW(
      BufferPoolConfig({"pool1", "invalid-attr", "1000"}),
      std::invalid_argument);

  // Note: Invalid bytes values (negative, non-numeric) are validated in
  // queryClient, not in the constructor. This allows the command to be
  // constructed and provide better error messages at execution time.
}

// Test BufferPoolConfig getters
TEST_F(CmdConfigQosBufferPoolTestFixture, bufferPoolConfigGetters) {
  BufferPoolConfig config(
      {"test_pool", "shared-bytes", "1000", "headroom-bytes", "2000"});

  EXPECT_EQ(config.getName(), "test_pool");

  const auto& attrs = config.getAttributes();
  ASSERT_EQ(attrs.size(), 2);
  EXPECT_EQ(attrs[0].first, "shared-bytes");
  EXPECT_EQ(attrs[0].second, "1000");
  EXPECT_EQ(attrs[1].first, "headroom-bytes");
  EXPECT_EQ(attrs[1].second, "2000");
}

// Test shared-bytes command creates buffer pool config
TEST_F(CmdConfigQosBufferPoolTestFixture, sharedBytesCreatesBufferPool) {
  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigPath_.string(),
          systemConfigPath_.string(),
          cliConfigDir_.string()));

  auto cmd = CmdConfigQosBufferPool();
  BufferPoolConfig config({"test_pool", "shared-bytes", "50000"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, ::testing::HasSubstr("Successfully configured"));
  EXPECT_THAT(result, ::testing::HasSubstr("test_pool"));

  // Verify the config was actually modified
  auto& agentConfig = ConfigSession::getInstance().getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  ASSERT_TRUE(switchConfig.bufferPoolConfigs().has_value());

  auto& bufferPoolConfigs = *switchConfig.bufferPoolConfigs();
  auto it = bufferPoolConfigs.find("test_pool");
  ASSERT_NE(it, bufferPoolConfigs.end());
  EXPECT_EQ(*it->second.sharedBytes(), 50000);
}

// Test headroom-bytes command creates buffer pool config
TEST_F(CmdConfigQosBufferPoolTestFixture, headroomBytesCreatesBufferPool) {
  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigPath_.string(),
          systemConfigPath_.string(),
          cliConfigDir_.string()));

  auto cmd = CmdConfigQosBufferPool();
  BufferPoolConfig config({"headroom_pool", "headroom-bytes", "10000"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, ::testing::HasSubstr("Successfully configured"));
  EXPECT_THAT(result, ::testing::HasSubstr("headroom_pool"));

  // Verify the config was actually modified
  auto& agentConfig = ConfigSession::getInstance().getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  ASSERT_TRUE(switchConfig.bufferPoolConfigs().has_value());

  auto& bufferPoolConfigs = *switchConfig.bufferPoolConfigs();
  auto it = bufferPoolConfigs.find("headroom_pool");
  ASSERT_NE(it, bufferPoolConfigs.end());
  EXPECT_EQ(*it->second.sharedBytes(), 0); // Default value
  ASSERT_TRUE(it->second.headroomBytes().has_value());
  EXPECT_EQ(*it->second.headroomBytes(), 10000);
}

// Test reserved-bytes command creates buffer pool config
TEST_F(CmdConfigQosBufferPoolTestFixture, reservedBytesCreatesBufferPool) {
  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigPath_.string(),
          systemConfigPath_.string(),
          cliConfigDir_.string()));

  auto cmd = CmdConfigQosBufferPool();
  BufferPoolConfig config({"reserved_pool", "reserved-bytes", "20000"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, ::testing::HasSubstr("Successfully configured"));
  EXPECT_THAT(result, ::testing::HasSubstr("reserved_pool"));

  // Verify the config was actually modified
  auto& agentConfig = ConfigSession::getInstance().getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  ASSERT_TRUE(switchConfig.bufferPoolConfigs().has_value());

  auto& bufferPoolConfigs = *switchConfig.bufferPoolConfigs();
  auto it = bufferPoolConfigs.find("reserved_pool");
  ASSERT_NE(it, bufferPoolConfigs.end());
  EXPECT_EQ(*it->second.sharedBytes(), 0); // Default value
  ASSERT_TRUE(it->second.reservedBytes().has_value());
  EXPECT_EQ(*it->second.reservedBytes(), 20000);
}

// Test updating an existing buffer pool with multiple attributes
TEST_F(CmdConfigQosBufferPoolTestFixture, updateExistingBufferPool) {
  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigPath_.string(),
          systemConfigPath_.string(),
          cliConfigDir_.string()));

  auto cmd = CmdConfigQosBufferPool();

  // Set all attributes in one command
  BufferPoolConfig config(
      {"existing_pool",
       "shared-bytes",
       "30000",
       "headroom-bytes",
       "5000",
       "reserved-bytes",
       "2000"});
  cmd.queryClient(localhost(), config);

  // Verify all values are set correctly
  auto& agentConfig = ConfigSession::getInstance().getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  ASSERT_TRUE(switchConfig.bufferPoolConfigs().has_value());

  auto& bufferPoolConfigs = *switchConfig.bufferPoolConfigs();
  auto it = bufferPoolConfigs.find("existing_pool");
  ASSERT_NE(it, bufferPoolConfigs.end());
  EXPECT_EQ(*it->second.sharedBytes(), 30000);
  ASSERT_TRUE(it->second.headroomBytes().has_value());
  EXPECT_EQ(*it->second.headroomBytes(), 5000);
  ASSERT_TRUE(it->second.reservedBytes().has_value());
  EXPECT_EQ(*it->second.reservedBytes(), 2000);
}

// Test printOutput for buffer-pool command
TEST_F(CmdConfigQosBufferPoolTestFixture, printOutputBufferPool) {
  auto cmd = CmdConfigQosBufferPool();
  std::string successMessage = "Successfully configured buffer-pool 'my_pool'";

  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
  cmd.printOutput(successMessage);
  std::cout.rdbuf(old);

  EXPECT_EQ(buffer.str(), successMessage + "\n");
}

} // namespace facebook::fboss
