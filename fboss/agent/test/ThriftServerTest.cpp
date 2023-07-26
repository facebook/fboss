/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include "common/network/if/gen-cpp2/Address_types.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/facebook/test/MultiSwitchTestServer.h"

#include <folly/experimental/coro/GtestHelpers.h>
#include <folly/experimental/coro/Timeout.h>
#include <folly/experimental/coro/UnboundedQueue.h>
#include <folly/portability/GTest.h>

#include "common/thrift/cpp/server/code_frameworks/testing/Server.h"

using namespace facebook::fboss;
using namespace facebook::stats;
using facebook::network::toBinaryAddress;
using facebook::network::thrift::BinaryAddress;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::StringPiece;
using std::shared_ptr;
using std::unique_ptr;
using ::testing::_;
using ::testing::Return;

class MultiSwitchThriftHandlerMock;

class ThriftServerTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
  }

  void setupServerAndClients() {
    XLOG(DBG2) << "Initializing thrift server";

    // Setup test server
    swSwitchTestServer_ = std::make_unique<MultiSwitchTestServer>(sw_);

    // setup clients
    multiSwitchClient_ = setupSwSwitchClient<
        apache::thrift::Client<multiswitch::MultiSwitchCtrl>>(
        "testClient1", evbThread1_);
    fbossCtlClient_ = setupSwSwitchClient<apache::thrift::Client<FbossCtrl>>(
        "testClient2", evbThread2_);
  }

  void setupServerWithMockAndClients() {
    XLOG(DBG2) << "Initializing thrift server";

    // Setup test server
    swSwitchTestServer_ = std::make_unique<MultiSwitchTestServer>(
        sw_, std::make_shared<MultiSwitchThriftHandlerMock>(sw_));

    // setup clients
    multiSwitchClient_ = setupSwSwitchClient<
        apache::thrift::Client<multiswitch::MultiSwitchCtrl>>(
        "testClient1", evbThread1_);
    fbossCtlClient_ = setupSwSwitchClient<apache::thrift::Client<FbossCtrl>>(
        "testClient2", evbThread2_);
  }

  template <typename clientT>
  std::unique_ptr<clientT> setupSwSwitchClient(
      std::string clientName,
      std::shared_ptr<folly::ScopedEventBaseThread>& evbThread) {
    evbThread = std::make_shared<folly::ScopedEventBaseThread>(clientName);
    auto channel = apache::thrift::PooledRequestChannel::newChannel(
        evbThread->getEventBase(),
        evbThread,
        [this](folly::EventBase& evb) mutable {
          return apache::thrift::RocketClientChannel::newChannel(
              folly::AsyncSocket::UniquePtr(new folly::AsyncSocket(
                  &evb, "::1", swSwitchTestServer_->getPort())));
        });
    return std::make_unique<clientT>(std::move(channel));
  }

 protected:
  std::unique_ptr<MultiSwitchTestServer> swSwitchTestServer_;
  std::unique_ptr<apache::thrift::Client<FbossCtrl>> fbossCtlClient_;
  std::unique_ptr<apache::thrift::Client<multiswitch::MultiSwitchCtrl>>
      multiSwitchClient_;
  std::shared_ptr<folly::ScopedEventBaseThread> evbThread2_;
  std::shared_ptr<folly::ScopedEventBaseThread> evbThread1_;
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

class MultiSwitchThriftHandlerMock : public MultiSwitchThriftHandler {
 public:
  explicit MultiSwitchThriftHandlerMock(SwSwitch* sw)
      : MultiSwitchThriftHandler(sw) {}

  folly::coro::Task<apache::thrift::ServerStream<multiswitch::TxPacket>>
  co_getTxPackets(int64_t /*switchId*/) override {
    co_return folly::coro::co_invoke(
        [this]() mutable
        -> folly::coro::AsyncGenerator<multiswitch::TxPacket&&> {
          while (receivedPktsQueue_.size()) {
            multiswitch::TxPacket txPkt;
            multiswitch::RxPacket rxPkt;
            co_await folly::coro::timeout(
                receivedPktsQueue_.dequeue(rxPkt), std::chrono::seconds(1));
            txPkt.port() = rxPkt.port().value();
            co_yield std::move(txPkt);
          }
        });
  }

  folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::RxPacket, bool>>
  co_notifyRxPacket(int64_t /*switchId*/) override {
    co_return apache::thrift::SinkConsumer<multiswitch::RxPacket, bool>{
        [this](folly::coro::AsyncGenerator<multiswitch::RxPacket&&> gen)
            -> folly::coro::Task<bool> {
          while (auto item = co_await gen.next()) {
            receivedPktsQueue_.enqueue(std::move(*item));
          }
          co_return true;
        },
        10 /* buffer size */
    };
  }

 private:
  folly::coro::UnboundedQueue<multiswitch::RxPacket, true, true>
      receivedPktsQueue_;
};

TEST_F(ThriftServerTest, setPortStateBlocking) {
  // setup server and clients
  setupServerAndClients();

  const PortID port5{5};
  folly::coro::blockingWait(fbossCtlClient_->co_setPortState(port5, true));
  waitForStateUpdates(this->sw_);

  auto port = this->sw_->getState()->getPorts()->getNodeIf(port5);
  EXPECT_TRUE(port->isEnabled());

  folly::coro::blockingWait(fbossCtlClient_->co_setPortState(port5, false));
  waitForStateUpdates(this->sw_);

  port = this->sw_->getState()->getPorts()->getNodeIf(port5);
  EXPECT_FALSE(port->isEnabled());
}

CO_TEST_F(ThriftServerTest, packetStreamAndSink) {
  // setup server and clients
  setupServerWithMockAndClients();

  int rxPktCount = 10;
  // Send packets to server using sink
  auto result = co_await multiSwitchClient_->co_notifyRxPacket(100);
  auto ret = co_await result.sink(
      [&]() -> folly::coro::AsyncGenerator<multiswitch::RxPacket&&> {
        for (int i = 0; i < rxPktCount; i++) {
          multiswitch::RxPacket pkt;
          pkt.port() = i;
          co_yield std::move(pkt);
        }
      }());
  EXPECT_TRUE(ret);

  // Get packets from server using stream and verify
  int expectedPort = 0;
  int txPktCount = 0;
  try {
    auto gen =
        (co_await multiSwitchClient_->co_getTxPackets(100)).toAsyncGenerator();
    while (const auto& val = co_await gen.next()) {
      // got value
      EXPECT_EQ(expectedPort++, *val->port());
      txPktCount++;
    }
    // server completed the stream
  } catch (const std::exception& e) {
    // server sent an exception, or there was a connection error
    XLOG(DBG2) << "Unable to connect " << e.what();
  }
  EXPECT_EQ(rxPktCount, txPktCount);
}
