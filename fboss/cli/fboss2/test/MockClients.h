// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"


namespace facebook::fboss {
using namespace ::testing;

extern std::vector<facebook::fboss::ArpEntryThrift> createArpEntries();

// Mock Thrift client FbossCtrlAsyncClient
class MockFbossCtrlAsyncClient : public facebook::fboss::FbossCtrlAsyncClient {
 public:
  using facebook::fboss::FbossCtrlAsyncClient::FbossCtrlAsyncClient;
  MockFbossCtrlAsyncClient() : FbossCtrlAsyncClient(getDummyProtocol()) {
    ON_CALL(*this, sync_getArpTable(_)).WillByDefault(
      Invoke([](std::vector<facebook::fboss::ArpEntryThrift>& entries) {
        entries = createArpEntries();
      }));
  }

  MOCK_METHOD(
    void,
    sync_getArpTable,
    (std::vector<facebook::fboss::ArpEntryThrift>&)
  );

 private:
  static FbossCtrlAsyncClient::channel_ptr getDummyProtocol() {
    /* library-local */ static folly::EventBase eventBase;
    auto transport = folly::AsyncSocket::newSocket(&eventBase);
    return apache::thrift::HeaderClientChannel::newChannel(
        std::move(transport));
  }
};

} // namespace facebook::fboss
