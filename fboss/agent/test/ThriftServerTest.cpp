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
#include <thrift/lib/cpp2/async/ServerStreamMultiPublisher.h>
#include "common/network/if/gen-cpp2/Address_types.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestPacketFactory.h"
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

    mockMultiSwitchHandler_ =
        std::make_shared<MultiSwitchThriftHandlerMock>(sw_);

    // Setup test server
    swSwitchTestServer_ =
        std::make_unique<MultiSwitchTestServer>(sw_, mockMultiSwitchHandler_);

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
  std::shared_ptr<MultiSwitchThriftHandlerMock> mockMultiSwitchHandler_;
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

  folly::coro::Task<apache::thrift::ServerStream<fsdb::OperDelta>>
  co_getStateUpdates(int64_t switchId) override {
    co_return multiPublisher_.addStream([switchId] {
      XLOG(DBG2) << "Disconnecting stream for switchId " << switchId;
    });
  }

  void sendOperDelta(fsdb::OperDelta operDelta) {
    multiPublisher_.next(operDelta);
  }

 private:
  folly::coro::UnboundedQueue<multiswitch::RxPacket, true, true>
      receivedPktsQueue_;
  apache::thrift::ServerStreamMultiPublisher<fsdb::OperDelta> multiPublisher_;
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

CO_TEST_F(ThriftServerTest, setPortStateSink) {
  // setup server and clients
  setupServerAndClients();

  const PortID port5{5};
  auto result = co_await multiSwitchClient_->co_notifyLinkEvent(100);
  auto verifyOperState = [this](const PortID& portId, bool up) {
    WITH_RETRIES({
      auto port = this->sw_->getState()->getPorts()->getNodeIf(portId);
      if (up) {
        EXPECT_EVENTUALLY_TRUE(port->getOperState() == Port::OperState::UP);
      } else {
        EXPECT_EVENTUALLY_TRUE(port->getOperState() == Port::OperState::DOWN);
      }
    });
  };
  auto ret = co_await result.sink(
      [&]() -> folly::coro::AsyncGenerator<multiswitch::LinkEvent&&> {
        multiswitch::LinkEvent upEvent;
        upEvent.port() = port5;
        upEvent.up() = true;
        co_yield std::move(upEvent);
        verifyOperState(port5, true);

        // set port oper state down
        multiswitch::LinkEvent downEvent;
        downEvent.port() = port5;
        downEvent.up() = false;
        co_yield std::move(downEvent);
        verifyOperState(port5, false);
      }());
  EXPECT_TRUE(ret);
}

CO_TEST_F(ThriftServerTest, fdbEventTest) {
  // setup server and clients
  setupServerAndClients();

  auto result = co_await multiSwitchClient_->co_notifyFdbEvent(100);
  auto verifyMacState = [this](
                            const VlanID vlanId, std::string mac, bool added) {
    WITH_RETRIES({
      auto vlan = this->sw_->getState()->getVlans()->getNodeIf(vlanId);
      auto macTable = vlan->getMacTable();
      auto node = macTable->getMacIf(folly::MacAddress(mac));
      if (added) {
        EXPECT_EVENTUALLY_NE(nullptr, node);
      } else {
        EXPECT_EVENTUALLY_EQ(nullptr, node);
      }
    });
  };

  auto createFdbEntry = [](L2EntryUpdateType updateType) {
    multiswitch::FdbEvent fdbEvent;
    PortID port5{5};
    fdbEvent.entry()->port() = port5;
    fdbEvent.entry()->vlanID() = 1;
    fdbEvent.entry()->mac() = "00:01:02:03:04:05";
    fdbEvent.entry()->l2EntryType() = L2EntryType::L2_ENTRY_TYPE_VALIDATED;
    fdbEvent.updateType() = updateType;
    return fdbEvent;
  };
  auto ret = co_await result.sink(
      [&]() -> folly::coro::AsyncGenerator<multiswitch::FdbEvent&&> {
        // add a new mac entry to the table
        auto fdbEvent =
            createFdbEntry(L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD);
        co_yield std::move(fdbEvent);
        verifyMacState(VlanID(1), "00:01:02:03:04:05", true);

        // delete the mac entry
        auto fdbEvent2 =
            createFdbEntry(L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE);
        co_yield std::move(fdbEvent2);
        verifyMacState(VlanID(1), "00:01:02:03:04:05", false);
      }());
  EXPECT_TRUE(ret);
}

CO_TEST_F(ThriftServerTest, receivePktHandler) {
  // setup server and clients
  setupServerAndClients();

  CounterCache counters(sw_);
  // Send packets to server using sink
  auto result = co_await multiSwitchClient_->co_notifyRxPacket(100);
  auto ret = co_await result.sink(
      [&]() -> folly::coro::AsyncGenerator<multiswitch::RxPacket&&> {
        multiswitch::RxPacket rxPkt;
        rxPkt.data() = std::make_unique<folly::IOBuf>(createV4Packet(
            folly::IPAddressV4("10.0.0.2"),
            folly::IPAddressV4("10.0.0.1"),
            MockPlatform::getMockLocalMac(),
            MockPlatform::getMockLocalMac()));
        rxPkt.port() = 1;
        rxPkt.vlan() = 1;
        co_yield std::move(rxPkt);
      }());
  EXPECT_TRUE(ret);
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.mine.sum", 1);
}

CO_TEST_F(ThriftServerTest, transmitPktHandler) {
  // setup server and clients
  setupServerAndClients();

  std::string payloadPad(9216 * 2, 'f'); // jumbo pkt
  auto pkt = createV4Packet(
      folly::IPAddressV4("10.0.0.2"),
      folly::IPAddressV4("10.0.0.1"),
      MockPlatform::getMockLocalMac(),
      MockPlatform::getMockLocalMac(),
      payloadPad);

  auto txPkt = createTxPacket(sw_, pkt);
  auto origPktSize = txPkt->buf()->length();

  auto gen =
      (co_await multiSwitchClient_->co_getTxPackets(100)).toAsyncGenerator();

  sw_->sendPacketOutViaThriftStream(std::move(txPkt), SwitchID(100), PortID(5));

  const auto& val = co_await gen.next();
  // got packet
  EXPECT_EQ(5, *val->port());
  EXPECT_EQ(origPktSize, (*val->data())->length());
}

CO_TEST_F(ThriftServerTest, operDeltaStream) {
  // setup server and clients
  setupServerWithMockAndClients();

  // create another client to get operDelta
  auto evbThreadSecond =
      std::make_shared<folly::ScopedEventBaseThread>("AnotherTestClient");
  auto multiSwitchClientSecond =
      setupSwSwitchClient<apache::thrift::Client<multiswitch::MultiSwitchCtrl>>(
          "AnotherTestClient", evbThreadSecond);

  auto generatorClient1 =
      (co_await multiSwitchClient_->co_getStateUpdates(100)).toAsyncGenerator();
  auto generatorClient2 =
      (co_await multiSwitchClientSecond->co_getStateUpdates(101))
          .toAsyncGenerator();

  fsdb::OperDelta delta;
  delta.protocol() = fsdb::OperProtocol::BINARY;
  mockMultiSwitchHandler_->sendOperDelta(delta);

  auto gen1 =
      folly::coro::co_invoke([&generatorClient1]() -> folly::coro::Task<bool> {
        const auto& val = co_await generatorClient1.next();
        EXPECT_EQ(val->protocol().value(), fsdb::OperProtocol::BINARY);
        co_return true;
      });
  auto gen2 =
      folly::coro::co_invoke([&generatorClient2]() -> folly::coro::Task<bool> {
        const auto& val = co_await generatorClient2.next();
        EXPECT_EQ(val->protocol().value(), fsdb::OperProtocol::BINARY);
        co_return true;
      });
  EXPECT_TRUE((co_await folly::coro::co_awaitTry(std::move(gen1))).value());
  EXPECT_TRUE((co_await folly::coro::co_awaitTry(std::move(gen2))).value());
}
