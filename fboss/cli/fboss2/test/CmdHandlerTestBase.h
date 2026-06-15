// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>
#include <thrift/lib/cpp2/server/ThriftServer.h> // NOLINT(misc-include-cleaner)
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include <memory>

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/test/MockClients.h"
#include "fboss/cli/fboss2/test/bgp/MockBgpClient.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/lib/ThriftServiceUtils.h"

namespace facebook::fboss {

class CmdHandlerTestBase : public ::testing::Test {
 public:
  void SetUp() override {
    mockedAgent_ = std::make_shared<MockFbossCtrlAgent>();
    mockedQsfpService_ = std::make_shared<MockFbossQsfpService>();
    // mockedBgpService_ is created in setupMockedBgpServer() when needed
    mockedFsdb_ = std::make_shared<MockFsdb>();
    localHost_ = std::make_unique<HostInfo>(
        "test.host", "test-oob.host", folly::IPAddressV6("::1"));
  }

  // Helper to configure server with epoll backend to avoid libevent2
  // performance issues in OSS builds that cause multi-second RPC latencies.
  static auto createFastMockServerConfig() {
    return ThriftServiceUtils::createThriftServerConfig();
  }

  void setupMockedAgentServer() {
    mockedAgentServer_ =
        std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
            mockedAgent_, "::1", 0, createFastMockServerConfig());
    // set global agent thrift port to the fake server created on localhost
    CmdGlobalOptions::getInstance()->setAgentThriftPort(
        mockedAgentServer_->getAddress().getPort());

    mockedQsfpServer_ =
        std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
            mockedQsfpService_, "::1", 0, createFastMockServerConfig());
    // set global agent thrift port to the fake server created on localhost
    CmdGlobalOptions::getInstance()->setQsfpThriftPort(
        mockedQsfpServer_->getAddress().getPort());
  }

  void setupMockedFsdbServer() {
    mockedFsdbServer_ =
        std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
            mockedFsdb_, createFastMockServerConfig());
    CmdGlobalOptions::getInstance()->setFsdbThriftPort(
        mockedFsdbServer_->getAddress().getPort());
  }

  void setupMockedBgpServer() {
    // Create a fresh mock service to avoid stale expectations between tests
    mockedBgpService_ = std::make_shared<MockBgpClient>();
    mockedBgpServer_ =
        std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
            mockedBgpService_, "::1", 0, createFastMockServerConfig());
    CmdGlobalOptions::getInstance()->setBgpThriftPort(
        mockedBgpServer_->getAddress().getPort());
  }

  void TearDown() override {
    // stop agent and BGP servers
    mockedAgentServer_.reset();
    mockedFsdbServer_.reset();
    mockedBgpServer_.reset();
    // Reset BGP mnemonic caches to ensure fresh data for next test
    resetBgpMnemonicCaches();
  }

  auto& getMockAgent() {
    return *mockedAgent_;
  }

  auto& getQsfpService() {
    return *mockedQsfpService_;
  }

  auto& getBgpService() {
    return *mockedBgpService_;
  }

  auto& getMockFsdb() {
    return *mockedFsdb_;
  }

  // Alias for getBgpService() used by BGP tests
  auto& getMockBgp() {
    return *mockedBgpService_;
  }

  const auto& localhost() {
    return *localHost_;
  }

  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread>
      mockedAgentServer_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread>
      mockedQsfpServer_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> mockedBgpServer_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread>
      mockedFsdbServer_;

 private:
  std::shared_ptr<MockFbossCtrlAgent> mockedAgent_;

  std::shared_ptr<MockFbossQsfpService> mockedQsfpService_;

  std::shared_ptr<MockBgpClient> mockedBgpService_;

  std::shared_ptr<MockFsdb> mockedFsdb_;

  std::unique_ptr<HostInfo> localHost_;
};

// Helper macro to compare vectors of Thrift structs element by element
// EXPECT_THRIFT_EQ doesn't work with vectors, only individual structs
#define EXPECT_THRIFT_EQ_VECTOR(actual, expected)            \
  do {                                                       \
    ASSERT_EQ((actual).size(), (expected).size())            \
        << "Vector sizes differ: actual=" << (actual).size() \
        << " expected=" << (expected).size();                \
    for (size_t i = 0; i < (expected).size(); ++i) {         \
      EXPECT_THRIFT_EQ((actual)[i], (expected)[i])           \
          << "Mismatch at index " << i;                      \
    }                                                        \
  } while (0)

} // namespace facebook::fboss
