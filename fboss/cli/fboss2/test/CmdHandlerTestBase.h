// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include <memory>
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/test/MockClients.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

#pragma once

namespace facebook::fboss {

class CmdHandlerTestBase : public ::testing::Test {
 public:
  void SetUp() override {
    mockedAgent_ = std::make_shared<MockFbossCtrlAgent>();
    mockedQsfpService_ = std::make_shared<MockFbossQsfpService>();
    mockedBgpService_ = std::make_shared<MockFbossBgpService>();
    localHost_ = std::make_unique<HostInfo>(
        "test.host", "test-oob.host", folly::IPAddressV6("::1"));
  }

  // Helper to create server config with short idle timeout to avoid 5-7 second
  // delays in test responses. Without this, the default idle timeout causes
  // the server to wait before sending responses back to the client.
  static auto createFastMockServerConfig() {
    return [](apache::thrift::ThriftServer& server) {
      server.setIdleTimeout(std::chrono::milliseconds(50));
    };
  }

  void setupMockedAgentServer() {
    mockedAgentServer_ =
        std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
            mockedAgent_, createFastMockServerConfig());
    // set global agent thrift port to the fake server created on localhost
    CmdGlobalOptions::getInstance()->setAgentThriftPort(
        mockedAgentServer_->getAddress().getPort());

    mockedQsfpServer_ =
        std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
            mockedQsfpService_, createFastMockServerConfig());
    // set global agent thrift port to the fake server created on localhost
    CmdGlobalOptions::getInstance()->setQsfpThriftPort(
        mockedQsfpServer_->getAddress().getPort());
  }

  void setupMockedBgpServer() {
#ifndef IS_OSS
    mockedBgpServer_ =
        std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
            mockedBgpService_, createFastMockServerConfig());
    CmdGlobalOptions::getInstance()->setBgpThriftPort(
        mockedBgpServer_->getAddress().getPort());
#endif
  }

  void TearDown() override {
    // stop agent servers
    mockedAgentServer_.reset();
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

  const auto& localhost() {
    return *localHost_;
  }

  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread>
      mockedAgentServer_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread>
      mockedQsfpServer_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> mockedBgpServer_;

 private:
  std::shared_ptr<MockFbossCtrlAgent> mockedAgent_;

  std::shared_ptr<MockFbossQsfpService> mockedQsfpService_;

  std::shared_ptr<MockFbossBgpService> mockedBgpService_;

  std::unique_ptr<HostInfo> localHost_;
};
} // namespace facebook::fboss
