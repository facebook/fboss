// Copyright 2004-present Facebook.  All rights reserved.

#include <folly/Memory.h>
#include <folly/Random.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include "fboss/agent/thrift_packet_stream/PacketStreamClient.h"
#include "fboss/agent/thrift_packet_stream/PacketStreamService.h"
#include "fboss/lib/ThriftServiceUtils.h"

using namespace testing;
using namespace facebook::fboss;

static const std::string g_client = "testClient";
static const std::unordered_set<std::string> g_ports = {"eth0", "eth1", "lo"};
static const std::string g_pktCnt = "Content";
class DerivedPacketStreamService : public PacketStreamService {
 public:
  DerivedPacketStreamService(
      const std::string& serviceName,
      std::shared_ptr<folly::Baton<>> baton)
      : PacketStreamService(serviceName, true), baton_(baton) {}

  void clientConnected(const std::string& clientId) override {
    EXPECT_EQ(clientId, g_client);
  }
  void clientDisconnected(const std::string& clientId) override {
    EXPECT_EQ(clientId, g_client);
    if (baton_) {
      baton_->post();
    }
  }
  void addPort(const std::string& clientId, const std::string& l2Port)
      override {
    EXPECT_EQ(clientId, g_client);
    EXPECT_FALSE(g_ports.find(l2Port) == g_ports.end());
  }
  void removePort(const std::string& clientId, const std::string& l2Port)
      override {
    EXPECT_EQ(clientId, g_client);
    EXPECT_FALSE(g_ports.find(l2Port) == g_ports.end());
  }

  std::shared_ptr<folly::Baton<>> baton_;
};

class DerivedPacketStreamClient : public PacketStreamClient {
 public:
  DerivedPacketStreamClient(
      const std::string& clientId,
      folly::EventBase* evb,
      std::shared_ptr<folly::Baton<>> baton)
      : PacketStreamClient(clientId, evb), baton_(baton) {}

  void recvPacket(TPacket&& packet) override {
    {
      std::lock_guard<std::mutex> gd(mutex_);
      EXPECT_FALSE(g_ports.find(*packet.l2Port()) == g_ports.end());
      EXPECT_EQ(g_pktCnt, *packet.buf());
      pktCnt_[*packet.l2Port()]++;
    }
    if (baton_) {
      baton_->post();
    }
  }

  size_t getPckCnt(const std::string& port) {
    std::lock_guard<std::mutex> gd(mutex_);
    return pktCnt_[port].load();
  }

  size_t validatePctCnt(size_t value) {
    std::lock_guard<std::mutex> gd(mutex_);
    auto count = 0;
    for (auto& val : pktCnt_) {
      EXPECT_EQ(val.second.load(std::memory_order_relaxed), value);
      count++;
    }
    return count;
  }

  void setBaton(std::shared_ptr<folly::Baton<>> baton) {
    std::lock_guard<std::mutex> gd(mutex_);
    baton_ = baton;
  }

 private:
  std::unordered_map<std::string, std::atomic<size_t>> pktCnt_;
  std::shared_ptr<folly::Baton<>> baton_;
  std::mutex mutex_;
};

class PacketStreamTest : public Test {
 public:
  void tryConnect(
      std::shared_ptr<folly::Baton<>> baton,
      DerivedPacketStreamClient& streamClient) {
    streamClient.connectToServer("::1", server_->getPort());
    auto retry = 15;
    EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(50)));
    while (!streamClient.isConnectedToServer() && retry > 0) {
      baton->reset();
      EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(50)));
      retry--;
    }
    baton->reset();
    EXPECT_TRUE(streamClient.isConnectedToServer());
  }

  void clientReset(std::unique_ptr<DerivedPacketStreamClient> streamClient) {
    streamClient.reset();
    baton_->reset();
    baton_->try_wait_for(std::chrono::milliseconds(500));
  }

  void SetUp() override {
    baton_ = std::make_shared<folly::Baton<>>();
    handler_ = std::make_shared<DerivedPacketStreamService>(
        "PacketStreamServiceTest", baton_);
    server_ = std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
        handler_,
        facebook::fboss::ThriftServiceUtils::createThriftServerConfig());
  }

  void TearDown() override {
    handler_.reset();
    baton_->reset();
    baton_->try_wait_for(std::chrono::milliseconds(100));
  }

  void sendPkt(const std::string& port) {
    EXPECT_TRUE(handler_->isClientConnected(g_client));
    EXPECT_TRUE(handler_->isPortRegistered(g_client, port));
    TPacket pkt;
    *pkt.l2Port() = port;
    *pkt.buf() = g_pktCnt;
    EXPECT_NO_THROW(handler_->send(g_client, std::move(pkt)));
  }
  std::shared_ptr<folly::Baton<>> baton_;
  std::shared_ptr<DerivedPacketStreamService> handler_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> server_;
  folly::ScopedEventBaseThread clientThread_;
};
#if FOLLY_HAS_COROUTINES
TEST_F(PacketStreamTest, Connect) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient.get());
  baton.reset();
  EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, PacketSend) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);
  EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  auto packetCnt = streamClient->getPckCnt(port);
  baton->reset();
  sendPkt(port);
  EXPECT_TRUE(baton->try_wait_for(std::chrono::milliseconds(50)));
  EXPECT_EQ(streamClient->getPckCnt(port), packetCnt + 1);
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, PacketSendMultiple) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);
  EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  for (auto i = 0; i < 1000; i++) {
    auto packetCnt = streamClient->getPckCnt(port);
    baton->reset();
    sendPkt(port);
    EXPECT_TRUE(baton->try_wait_for(std::chrono::milliseconds(100)));
    EXPECT_EQ(streamClient->getPckCnt(port), packetCnt + 1);
  }
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, UnregisterPortToServerFail) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);
  EXPECT_TRUE(handler_->isClientConnected(g_client));
  EXPECT_FALSE(handler_->isPortRegistered(g_client, port));
  TPacket pkt;
  *pkt.l2Port() = port;
  EXPECT_THROW(handler_->send(g_client, std::move(pkt)), TPacketException);
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, clearPortFromServerFail) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);

  EXPECT_TRUE(handler_->isClientConnected(g_client));
  EXPECT_FALSE(handler_->isPortRegistered(g_client, port));
  EXPECT_THROW(streamClient->clearPortFromServer(port), std::exception);
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, clearPortFromServerSendFail) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);
  EXPECT_TRUE(handler_->isClientConnected(g_client));
  EXPECT_FALSE(handler_->isPortRegistered(g_client, port));

  EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  baton->reset();
  sendPkt(port);
  EXPECT_TRUE(baton->try_wait_for(std::chrono::milliseconds(50)));
  EXPECT_TRUE(handler_->isPortRegistered(g_client, port));

  EXPECT_NO_THROW(streamClient->clearPortFromServer(port));
  TPacket pkt;
  *pkt.l2Port() = port;
  EXPECT_THROW(handler_->send(g_client, std::move(pkt)), std::exception);
  EXPECT_FALSE(handler_->isPortRegistered(g_client, port));
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, PacketSendMultiPort) {
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);

  tryConnect(baton, *streamClient);
  for (auto& port : g_ports) {
    EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  }
  streamClient->setBaton(nullptr);
  for (auto& port : g_ports) {
    sendPkt(port);
  }
  baton->reset();
  EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(50)));
  size_t count = streamClient->validatePctCnt(1);
  EXPECT_EQ(count, g_ports.size());
  streamClient->cancel();
  // streamClient.reset();
  baton_->reset();
  baton_->try_wait_for(std::chrono::milliseconds(500));
  // clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, PacketSendMultiPortPauseSend) {
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);

  for (auto& port : g_ports) {
    EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  }
  streamClient->setBaton(nullptr);
  for (auto& port : g_ports) {
    for (auto i = 0; i < 100; i++) {
      baton->reset();
      EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(5)));
      sendPkt(port);
    }
  }
  baton->reset();
  EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(50)));
  size_t count = streamClient->validatePctCnt(100);
  EXPECT_EQ(count, g_ports.size());
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, PacketSendMultiPortclearPortFromServer) {
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);

  for (auto& port : g_ports) {
    EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  }
  streamClient->setBaton(nullptr);
  for (auto& port : g_ports) {
    for (auto i = 0; i < 100; i++) {
      baton->reset();
      EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(5)));
      sendPkt(port);
    }
    EXPECT_NO_THROW(streamClient->clearPortFromServer(port));
  }
  baton->reset();
  EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(50)));
  size_t count = 0;
  for (auto& port : g_ports) {
    TPacket pkt;
    *pkt.l2Port() = port;
    *pkt.buf() = g_pktCnt;
    EXPECT_THROW(handler_->send(g_client, std::move(pkt)), TPacketException);
  }
  count = streamClient->validatePctCnt(100);
  EXPECT_EQ(count, g_ports.size());
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, registerPortToServerTest) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);
  EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  auto rPort = std::make_unique<std::string>("test");
  EXPECT_THROW(handler_->registerPort({}, std::move(rPort)), TPacketException);
  EXPECT_THROW(handler_->registerPort({}, {}), TPacketException);
  EXPECT_THROW(
      handler_->registerPort(
          std::make_unique<std::string>("test123"),
          std::make_unique<std::string>()),
      TPacketException);
  EXPECT_FALSE(handler_->isPortRegistered("test123", port));
  EXPECT_FALSE(handler_->isPortRegistered(g_client, "newport"));
  EXPECT_THROW(
      handler_->disconnect(std::make_unique<std::string>()), TPacketException);
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, clearPortFromServerTest) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  std::unique_ptr<DerivedPacketStreamClient> streamClient =
      std::make_unique<DerivedPacketStreamClient>(
          g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);
  EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  EXPECT_NO_THROW(streamClient->clearPortFromServer(port));
  EXPECT_THROW(streamClient->clearPortFromServer(port), TPacketException);
  auto rPort = std::make_unique<std::string>("test");
  EXPECT_THROW(handler_->clearPort({}, std::move(rPort)), TPacketException);
  EXPECT_THROW(
      handler_->clearPort(
          std::make_unique<std::string>("test"), std::move(rPort)),
      TPacketException);
  EXPECT_FALSE(handler_->isClientConnected("test"));
  EXPECT_THROW(handler_->clearPort({}, {}), TPacketException);
  EXPECT_THROW(
      handler_->clearPort(
          std::make_unique<std::string>("test123"),
          std::make_unique<std::string>("test123")),
      TPacketException);
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, SendHandlerFailTest) {
  TPacket pkt;
  *pkt.l2Port() = "testRandom";
  EXPECT_THROW(handler_->send("test123", std::move(pkt)), TPacketException);
  TPacket newpkt;
  *newpkt.l2Port() = "testRandom";
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  EXPECT_THROW(streamClient->registerPortToServer(port), std::exception);
  tryConnect(baton, *streamClient);
  EXPECT_THROW(handler_->send(g_client, std::move(newpkt)), TPacketException);
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, ServerDisconnected) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  auto streamClient = std::make_unique<DerivedPacketStreamClient>(
      g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);
  EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  auto packetCnt = streamClient->getPckCnt(port);
  baton->reset();
  sendPkt(port);
  EXPECT_TRUE(baton->try_wait_for(std::chrono::milliseconds(50)));
  EXPECT_EQ(streamClient->getPckCnt(port), packetCnt + 1);
  server_.reset();
  handler_.reset();
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, ServerNotStarted) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  auto serverPort = server_->getPort();
  server_.reset();
  handler_.reset();

  std::unique_ptr<DerivedPacketStreamClient> streamClient =
      std::make_unique<DerivedPacketStreamClient>(
          g_client, clientThread_.getEventBase(), baton);
  streamClient->connectToServer("::1", serverPort);

  auto retry = 3;
  EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(50)));
  while (!streamClient->isConnectedToServer() && retry > 0) {
    baton->reset();
    EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(50)));
    retry--;
  }
  baton->reset();
  EXPECT_FALSE(streamClient->isConnectedToServer());
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, ClientDisconnect) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  std::unique_ptr<DerivedPacketStreamClient> streamClient =
      std::make_unique<DerivedPacketStreamClient>(
          g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);
  EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  auto packetCnt = streamClient->getPckCnt(port);
  baton->reset();
  sendPkt(port);
  EXPECT_TRUE(baton->try_wait_for(std::chrono::milliseconds(50)));
  EXPECT_EQ(streamClient->getPckCnt(port), packetCnt + 1);
  clientReset(std::move(streamClient));
  EXPECT_FALSE(handler_->isClientConnected(g_client));
}

TEST_F(PacketStreamTest, ServerSendClientDisconnected) {
  std::string port(*g_ports.begin());
  auto baton = std::make_shared<folly::Baton<>>();
  std::unique_ptr<DerivedPacketStreamClient> streamClient =
      std::make_unique<DerivedPacketStreamClient>(
          g_client, clientThread_.getEventBase(), baton);
  tryConnect(baton, *streamClient);
  EXPECT_NO_THROW(streamClient->registerPortToServer(port));
  auto packetCnt = streamClient->getPckCnt(port);
  baton->reset();
  sendPkt(port);
  EXPECT_TRUE(baton->try_wait_for(std::chrono::milliseconds(50)));
  EXPECT_EQ(streamClient->getPckCnt(port), packetCnt + 1);
  EXPECT_TRUE(handler_->isClientConnected(g_client));
  EXPECT_TRUE(handler_->isPortRegistered(g_client, port));
  streamClient.reset();
  TPacket pkt;
  *pkt.l2Port() = port;
  *pkt.buf() = g_pktCnt;
  EXPECT_THROW(handler_->send(g_client, std::move(pkt)), TPacketException);
  clientReset(std::move(streamClient));
}

TEST_F(PacketStreamTest, WaitForDisconnectedServer) {
  auto baton = std::make_shared<folly::Baton<>>();
  std::unique_ptr<DerivedPacketStreamClient> streamClient =
      std::make_unique<DerivedPacketStreamClient>(
          g_client, clientThread_.getEventBase(), baton);
  auto port = server_->getPort();
  // disconnect server
  handler_.reset();
  server_.reset();
  // connect client
  streamClient->connectToServer("::1", port);
  auto retry = 50;
  EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(50)));
  while (!streamClient->isConnectedToServer() && retry > 0) {
    streamClient->connectToServer("::1", port);
    baton->reset();
    EXPECT_FALSE(baton->try_wait_for(std::chrono::milliseconds(50)));
    retry--;
  }
  baton->reset();
  EXPECT_FALSE(streamClient->isConnectedToServer());
}
// --- Tests for PacketStreamClient::send() (client-to-server via sink) ---

// Service that tracks packets received via the sink (processReceivedPacket).
// Uses portRegistration=false since the sink path doesn't use port
// registration.
class SinkTrackingService : public PacketStreamService {
 public:
  explicit SinkTrackingService(const std::string& serviceName)
      : PacketStreamService(serviceName, false /* portRegistration */) {}

  void clientConnected(const std::string& /*clientId*/) override {}
  void clientDisconnected(const std::string& /*clientId*/) override {
    disconnectBaton_.post();
  }
  void addPort(const std::string& /*clientId*/, const std::string& /*l2Port*/)
      override {}
  void removePort(
      const std::string& /*clientId*/,
      const std::string& /*l2Port*/) override {}

  void processReceivedPacket(const std::string& clientId, TPacket&& packet)
      override {
    std::lock_guard<std::mutex> lk(mutex_);
    receivedPackets_.push_back(std::move(packet));
    lastClientId_ = clientId;
  }

  size_t receivedCount() {
    std::lock_guard<std::mutex> lk(mutex_);
    return receivedPackets_.size();
  }

  TPacket getReceivedPacket(size_t index) {
    std::lock_guard<std::mutex> lk(mutex_);
    return receivedPackets_.at(index);
  }

  void waitForDisconnect() {
    disconnectBaton_.try_wait_for(std::chrono::milliseconds(500));
    disconnectBaton_.reset();
  }

 private:
  std::vector<TPacket> receivedPackets_;
  std::string lastClientId_;
  std::mutex mutex_;
  folly::Baton<> disconnectBaton_;
};

// Minimal client that just counts received packets (from server stream).
class SimpleRecvClient : public PacketStreamClient {
 public:
  SimpleRecvClient(const std::string& clientId, folly::EventBase* evb)
      : PacketStreamClient(clientId, evb) {}

  void recvPacket(TPacket&& /*packet*/) override {
    recvCount_.fetch_add(1);
  }

 private:
  std::atomic<size_t> recvCount_{0};
};

class PacketStreamClientSendTest : public Test {
 public:
  void SetUp() override {
    sinkHandler_ = std::make_shared<SinkTrackingService>("SinkTestService");
    sinkServer_ = std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
        sinkHandler_,
        facebook::fboss::ThriftServiceUtils::createThriftServerConfig());
  }

  void TearDown() override {
    sendClient_.reset();
    sinkHandler_->waitForDisconnect();
  }

  void connectSendClient() {
    sendClientEvb_ =
        std::make_unique<folly::ScopedEventBaseThread>("sink_send_test_client");
    sendClient_ = std::make_unique<SimpleRecvClient>(
        g_client, sendClientEvb_->getEventBase());
    sendClient_->connectToServer("::1", sinkServer_->getPort());

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!sendClient_->isConnectedToServer() &&
           std::chrono::steady_clock::now() < deadline) {
      /* sleep override */ std::this_thread::sleep_for(
          std::chrono::milliseconds(50));
    }
    ASSERT_TRUE(sendClient_->isConnectedToServer());

    ASSERT_NO_THROW(sendClient_->createSink());

    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!sendClient_->isSinkReady() &&
           std::chrono::steady_clock::now() < deadline) {
      /* sleep override */ std::this_thread::sleep_for(
          std::chrono::milliseconds(50));
    }
    ASSERT_TRUE(sendClient_->isSinkReady());
  }

  bool waitForSinkRecv(size_t count, int timeoutMs = 5000) {
    auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (sinkHandler_->receivedCount() < count &&
           std::chrono::steady_clock::now() < deadline) {
      /* sleep override */ std::this_thread::sleep_for(
          std::chrono::milliseconds(50));
    }
    return sinkHandler_->receivedCount() >= count;
  }

  std::shared_ptr<SinkTrackingService> sinkHandler_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> sinkServer_;
  std::unique_ptr<folly::ScopedEventBaseThread> sendClientEvb_;
  std::unique_ptr<SimpleRecvClient> sendClient_;
};

TEST_F(PacketStreamClientSendTest, SinkReadyAfterCreateSink) {
  sendClientEvb_ =
      std::make_unique<folly::ScopedEventBaseThread>("sink_send_test_client");
  sendClient_ = std::make_unique<SimpleRecvClient>(
      g_client, sendClientEvb_->getEventBase());

  EXPECT_FALSE(sendClient_->isSinkReady());

  sendClient_->connectToServer("::1", sinkServer_->getPort());
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!sendClient_->isConnectedToServer() &&
         std::chrono::steady_clock::now() < deadline) {
    /* sleep override */ std::this_thread::sleep_for(
        std::chrono::milliseconds(50));
  }
  ASSERT_TRUE(sendClient_->isConnectedToServer());
  EXPECT_FALSE(sendClient_->isSinkReady());

  EXPECT_NO_THROW(sendClient_->createSink());

  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!sendClient_->isSinkReady() &&
         std::chrono::steady_clock::now() < deadline) {
    /* sleep override */ std::this_thread::sleep_for(
        std::chrono::milliseconds(50));
  }
  EXPECT_TRUE(sendClient_->isSinkReady());
}

TEST_F(PacketStreamClientSendTest, SendSinglePacket) {
  connectSendClient();

  TPacket pkt;
  pkt.l2Port() = "eth0";
  pkt.buf() = "hello";
  EXPECT_TRUE(sendClient_->send(std::move(pkt)));

  ASSERT_TRUE(waitForSinkRecv(1));
  auto received = sinkHandler_->getReceivedPacket(0);
  EXPECT_EQ(*received.l2Port(), "eth0");
  EXPECT_EQ(*received.buf(), "hello");
}

TEST_F(PacketStreamClientSendTest, SendMultiplePackets) {
  connectSendClient();

  constexpr int kCount = 100;
  for (int i = 0; i < kCount; i++) {
    TPacket pkt;
    pkt.l2Port() = "eth0";
    pkt.buf() = "pkt_" + std::to_string(i);
    sendClient_->send(std::move(pkt));
  }

  ASSERT_TRUE(waitForSinkRecv(kCount, 10000));
  for (int i = 0; i < kCount; i++) {
    auto received = sinkHandler_->getReceivedPacket(i);
    EXPECT_EQ(*received.buf(), "pkt_" + std::to_string(i));
  }
}

TEST_F(PacketStreamClientSendTest, SendDropsBeforeSinkReady) {
  sendClientEvb_ =
      std::make_unique<folly::ScopedEventBaseThread>("sink_send_test_client");
  sendClient_ = std::make_unique<SimpleRecvClient>(
      g_client, sendClientEvb_->getEventBase());

  // Send before createSink — should return false (dropped)
  TPacket pkt;
  pkt.l2Port() = "eth0";
  pkt.buf() = "dropped";
  EXPECT_FALSE(sendClient_->send(std::move(pkt)));

  // Connect and create sink
  sendClient_->connectToServer("::1", sinkServer_->getPort());
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!sendClient_->isConnectedToServer() &&
         std::chrono::steady_clock::now() < deadline) {
    /* sleep override */ std::this_thread::sleep_for(
        std::chrono::milliseconds(50));
  }
  ASSERT_TRUE(sendClient_->isConnectedToServer());
  sendClient_->createSink();

  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!sendClient_->isSinkReady() &&
         std::chrono::steady_clock::now() < deadline) {
    /* sleep override */ std::this_thread::sleep_for(
        std::chrono::milliseconds(50));
  }
  ASSERT_TRUE(sendClient_->isSinkReady());

  // Now send a real packet — should return true
  TPacket realPkt;
  realPkt.l2Port() = "eth0";
  realPkt.buf() = "real";
  EXPECT_TRUE(sendClient_->send(std::move(realPkt)));

  ASSERT_TRUE(waitForSinkRecv(1));
  // Only the post-sink packet should arrive, not the dropped one
  EXPECT_EQ(sinkHandler_->receivedCount(), 1);
  auto received = sinkHandler_->getReceivedPacket(0);
  EXPECT_EQ(*received.buf(), "real");
}

TEST_F(PacketStreamClientSendTest, CancelStopsSinkLoop) {
  connectSendClient();

  TPacket pkt;
  pkt.l2Port() = "eth0";
  pkt.buf() = "before_cancel";
  sendClient_->send(std::move(pkt));

  ASSERT_TRUE(waitForSinkRecv(1));
  EXPECT_TRUE(sendClient_->isSinkReady());

  sendClient_->cancel();
  EXPECT_FALSE(sendClient_->isSinkReady());
}

TEST_F(PacketStreamClientSendTest, ConcurrentSend) {
  connectSendClient();

  constexpr int kThreads = 4;
  constexpr int kPerThread = 50;
  constexpr int kTotal = kThreads * kPerThread;

  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; t++) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < kPerThread; i++) {
        TPacket pkt;
        pkt.l2Port() = "eth0";
        pkt.buf() = "t" + std::to_string(t) + "_p" + std::to_string(i);
        sendClient_->send(std::move(pkt));
      }
    });
  }
  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(waitForSinkRecv(kTotal, 10000));
}

TEST_F(PacketStreamClientSendTest, SendMultiplePorts) {
  connectSendClient();

  std::vector<std::string> ports = {"eth0", "eth1", "eth2"};
  for (const auto& port : ports) {
    TPacket pkt;
    pkt.l2Port() = port;
    pkt.buf() = "payload_" + port;
    sendClient_->send(std::move(pkt));
  }

  ASSERT_TRUE(waitForSinkRecv(ports.size()));
  for (size_t i = 0; i < ports.size(); i++) {
    auto received = sinkHandler_->getReceivedPacket(i);
    EXPECT_EQ(*received.l2Port(), ports[i]);
    EXPECT_EQ(*received.buf(), "payload_" + ports[i]);
  }
}

#endif
