// Copyright 2004-present Facebook.  All rights reserved.

#include "fboss/agent/thrift_packet_stream/BidirectionalPacketStream.h"
#include <folly/Memory.h>
#include <folly/Random.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/logging/xlog.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include "folly/synchronization/Baton.h"

#include "fboss/lib/CommonUtils.h"

using namespace testing;
using namespace facebook::fboss;

static const std::string g_client = "testClient";
static const std::vector<std::string> g_ports = {"eth0", "eth1", "lo"};
static const double g_connect_timer = 100;
static const std::string g_mkaTofboss = "FromMkaToFboss";

class PacketRecvAcceptor : public facebook::fboss::BidirectionalPacketAcceptor {
 public:
  explicit PacketRecvAcceptor(std::shared_ptr<folly::Baton<>> baton)
      : baton_(baton) {
    if (baton_) {
      baton_->reset();
    }
  }
  void recvPacket(TPacket&& packet) override {
    packets_.emplace_back(std::move(packet));
    if (++n_ == exp_) {
      baton_->post();
    }
  }
  void setExpectedPackets(size_t n) {
    exp_ = n;
    n_ = 0;
  }
  std::vector<TPacket> packets_;
  std::shared_ptr<folly::Baton<>> baton_;
  size_t exp_{0};
  size_t n_{0};
};

class PacketAcceptor
    : public facebook::fboss::AsyncPacketTransport::ReadCallback {
 public:
  explicit PacketAcceptor(std::shared_ptr<folly::Baton<>> baton)
      : baton_(baton) {
    if (baton_) {
      baton_->reset();
    }
  }

  void onReadError(
      const folly::AsyncSocketException& /*ex*/) noexcept override {
    exception_ = true;
    baton_->post();
  }

  void onReadClosed() noexcept override {
    n_ = 0;
  }

  void onDataAvailable(std::unique_ptr<folly::IOBuf> data) noexcept override {
    rxIOBufs_.emplace_back(std::move(data));
    if (++n_ == exp_) {
      baton_->post();
    }
  }

  void setExpectedPackets(size_t n) {
    exp_ = n;
    n_ = 0;
  }
  std::vector<std::unique_ptr<folly::IOBuf>> rxIOBufs_;
  std::shared_ptr<folly::Baton<>> baton_;
  size_t exp_{0};
  size_t n_{0};
  bool exception_{false};
};

class BidirectionalPacketStreamTest : public Test {
 public:
  BidirectionalPacketStreamTest() = default;

  void tryConnect() {
    mkaServerStream_->connectClient(fbossAgent_->getPort());
    fbossAgentStream_->connectClient(mkaServer_->getPort());

    EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(500)));
    WITH_RETRIES(
        { ASSERT_EVENTUALLY_TRUE(mkaServerStream_->isConnectedToServer()); });
    WITH_RETRIES({
      ASSERT_EVENTUALLY_TRUE(
          mkaServerStream_->isClientConnected("fboss_agent"));
    });

    WITH_RETRIES(
        { ASSERT_EVENTUALLY_TRUE(fbossAgentStream_->isConnectedToServer()); });
    WITH_RETRIES({
      ASSERT_EVENTUALLY_TRUE(
          fbossAgentStream_->isClientConnected("mka_server"));
    });
  }

  void sendFbossToMka(
      size_t numPkts,
      const std::string& port,
      std::shared_ptr<AsyncPacketTransport> transport,
      std::shared_ptr<folly::Baton<>> baton = {}) {
    XLOG(DBG2) << "Send FbossToMka: port: " << port << " numPkts:" << numPkts;
    if (!baton) {
      baton = std::make_shared<folly::Baton<>>();
    }
    PacketAcceptor acceptor(baton);
    acceptor.setExpectedPackets(numPkts);
    transport->setReadCallback(&acceptor);
    SCOPE_EXIT {
      transport->setReadCallback(nullptr);
    };
    baton->reset();
    WITH_RETRIES(
        { ASSERT_EVENTUALLY_TRUE(fbossAgentStream_->isPortRegistered(port)); });
    std::string pktString = "FromFbossToMka";
    for (auto i = 0; i < numPkts; i++) {
      TPacket packet;
      *packet.l2Port() = port;
      *packet.buf() = pktString;
      // send TPacket from fboss -> mka
      EXPECT_EQ(pktString.size(), fbossAgentStream_->send(std::move(packet)));
    }

    EXPECT_TRUE(baton->try_wait_for(std::chrono::milliseconds(500)));
    // verify the packets received on other end.
    EXPECT_EQ(acceptor.rxIOBufs_.size(), numPkts);
    for (auto& buf : acceptor.rxIOBufs_) {
      auto bufStr = buf->to<std::string>();
      EXPECT_EQ(bufStr, pktString);
    }

    XLOG(DBG2) << "Completed SendFbossToMKA for port:" << port;
  }

  void sendMkaToFboss(
      size_t numPkts,
      const std::string& port,
      std::shared_ptr<AsyncPacketTransport> transport,
      std::shared_ptr<folly::Baton<>> baton = {},
      bool validatePackets = true) {
    XLOG(DBG2) << "Send MKAToFboss: port: " << port << " numPkts:" << numPkts;
    if (!baton) {
      baton = std::make_shared<folly::Baton<>>();
    }
    PacketRecvAcceptor rcvAcceptor(baton);
    if (validatePackets) {
      rcvAcceptor.setExpectedPackets(numPkts);
      fbossAgentStream_->setPacketAcceptor(&rcvAcceptor);
      baton->reset();
    }
    CHECK_EQ(transport->iface(), port);
    WITH_RETRIES(
        { ASSERT_EVENTUALLY_TRUE(fbossAgentStream_->isPortRegistered(port)); });
    for (auto i = 0; i < numPkts; i++) {
      auto mkpdu = folly::IOBuf::copyBuffer(g_mkaTofboss);
      // send from mka -> fboss.
      EXPECT_EQ(g_mkaTofboss.size(), transport->send(std::move(mkpdu)));
    }
    if (validatePackets) {
      EXPECT_TRUE(baton->try_wait_for(std::chrono::milliseconds(500)));
      fbossAgentStream_->setPacketAcceptor(nullptr);
      // verify the packets
      EXPECT_EQ(rcvAcceptor.packets_.size(), numPkts);
      for (auto& pkt : rcvAcceptor.packets_) {
        EXPECT_EQ(g_mkaTofboss, *pkt.buf());
      }
    }
    XLOG(DBG2) << "Completed SendMKAToFboss for port:" << port;
  }

  void sendParallelMultiplePkts(size_t numPkts = 100) {
    auto port = "eth0";
    tryConnect();
    auto transport = mkaServerStream_->listen(port);

    auto fbossThread =
        std::thread([&, this]() { sendFbossToMka(numPkts, port, transport); });

    auto mkaThread =
        std::thread([&, this]() { sendMkaToFboss(numPkts, port, transport); });

    fbossThread.join();
    mkaThread.join();
  }

  void sendParallelMultiplePktsMultiplePorts(size_t numPkts = 100) {
    tryConnect();
    std::vector<std::shared_ptr<AsyncPacketTransport>> transportList;
    std::vector<std::shared_ptr<std::thread>> threadList;
    PacketRecvAcceptor rcvAcceptor(baton_);

    rcvAcceptor.setExpectedPackets(numPkts * g_ports.size());
    fbossAgentStream_->setPacketAcceptor(&rcvAcceptor);
    baton_->reset();
    auto index = 0;
    for (auto& port : g_ports) {
      auto transport = mkaServerStream_->listen(port);
      transportList.emplace_back(transport);
    }

    for (auto& port : g_ports) {
      auto transport = transportList[index++];
      auto fbossThread = std::make_shared<std::thread>(
          [&, transport, this]() { sendFbossToMka(numPkts, port, transport); });

      auto mkaThread = std::make_shared<std::thread>([&, transport, this]() {
        sendMkaToFboss(numPkts, port, transport, baton_, false);
      });
      threadList.emplace_back(fbossThread);
      threadList.emplace_back(mkaThread);
    }
    for (auto& thread : threadList) {
      thread->join();
    }

    EXPECT_TRUE(baton_->try_wait_for(std::chrono::milliseconds(500)));
    // validate packets received in fboss
    fbossAgentStream_->setPacketAcceptor(nullptr);
    // verify the packets
    EXPECT_EQ(rcvAcceptor.packets_.size(), numPkts * g_ports.size());
    for (auto& pkt : rcvAcceptor.packets_) {
      EXPECT_EQ(g_mkaTofboss, *pkt.buf());
    }
    baton_->reset();
    EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(200)));
  }
  void mkaServerReconnect(
      uint16_t fbossAgentPort,
      const std::string& listeningPort) {
    // Now stop the fboss Server.
    fbossAgent_.reset();
    fbossAgentStream_->stopClient();
    fbossAgentStream_.reset();
    fbossClientThread_.reset();

    fbossClientThread_ = std::make_unique<folly::ScopedEventBaseThread>();

    // Lets try to connect the mkaServer. This should fail because the fboss
    // server is not running.
    mkaServerStream_->connectClient(fbossAgentPort);
    auto transport = mkaServerStream_->listen(listeningPort);

    EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(500)));
    EXPECT_FALSE(mkaServerStream_->isConnectedToServer());
    EXPECT_FALSE(mkaServerStream_->isClientConnected("fboss_agent"));
    // ensure send fails
    auto mkpdu = folly::IOBuf::copyBuffer(g_mkaTofboss);
    EXPECT_EQ(-1, transport->send(std::move(mkpdu)));

    // Now lets start the fboss Server and mka should automatically connect.
    baton_->reset();
    fbossAgentStream_ = std::make_shared<BidirectionalPacketStream>(
        "fboss_agent",
        fbossClientThread_->getEventBase(),
        &evb_,
        g_connect_timer);

    fbossAgent_ = std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
        fbossAgentStream_, "::1", fbossAgentPort);
    fbossAgentStream_->connectClient(mkaServer_->getPort());

    EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(500)));
    EXPECT_TRUE(mkaServerStream_->isConnectedToServer());
    EXPECT_TRUE(mkaServerStream_->isClientConnected("fboss_agent"));

    EXPECT_TRUE(fbossAgentStream_->isConnectedToServer());
    EXPECT_TRUE(fbossAgentStream_->isClientConnected("mka_server"));

    // we can try to send packets.
    sendMkaToFboss(1, listeningPort, transport);
    sendFbossToMka(1, listeningPort, transport);
  }

  void fbossAgentReconnect(
      uint16_t serverPort,
      const std::string& listeningPort) {
    // Now stop the fboss Server.
    mkaServer_.reset();
    mkaServerStream_->stopClient();
    mkaServerStream_.reset();
    mkaClientThread_.reset();
    mkaClientThread_ = std::make_unique<folly::ScopedEventBaseThread>();

    // Lets try to connect the mkaServer. This should fail because the fboss
    // server is not running.
    fbossAgentStream_->connectClient(serverPort);
    baton_->reset();
    EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(500)));
    EXPECT_FALSE(fbossAgentStream_->isConnectedToServer());
    EXPECT_FALSE(fbossAgentStream_->isClientConnected("mka_server"));

    // Now lets start the fboss Server and mka should automatically connect.
    baton_->reset();
    mkaServerStream_ = std::make_shared<BidirectionalPacketStream>(
        "mka_server", mkaClientThread_->getEventBase(), &evb_, g_connect_timer);

    mkaServer_ = std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
        mkaServerStream_, "::1", serverPort);
    mkaServerStream_->connectClient(fbossAgent_->getPort());

    EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(500)));
    EXPECT_TRUE(fbossAgentStream_->isConnectedToServer());
    EXPECT_TRUE(fbossAgentStream_->isClientConnected("mka_server"));

    EXPECT_TRUE(mkaServerStream_->isConnectedToServer());
    EXPECT_TRUE(mkaServerStream_->isClientConnected("fboss_agent"));
    // we can try to send packets.
    auto transport = mkaServerStream_->listen(listeningPort);

    sendMkaToFboss(1, listeningPort, transport);
    sendFbossToMka(1, listeningPort, transport);
  }

  void SetUp() override {
    mkaClientThread_ =
        std::make_unique<folly::ScopedEventBaseThread>("mkaClient");
    fbossClientThread_ =
        std::make_unique<folly::ScopedEventBaseThread>("fbossClient");

    timeThread_ =
        std::make_unique<std::thread>([this]() { evb_.loopForever(); });
    baton_ = std::make_shared<folly::Baton<>>();

    mkaServerStream_ = std::make_shared<BidirectionalPacketStream>(
        "mka_server", mkaClientThread_->getEventBase(), &evb_, g_connect_timer);
    mkaServer_ = std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
        mkaServerStream_);

    fbossAgentStream_ = std::make_shared<BidirectionalPacketStream>(
        "fboss_agent",
        fbossClientThread_->getEventBase(),
        &evb_,
        g_connect_timer);
    fbossAgent_ = std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
        fbossAgentStream_);
  }

  void TearDown() override {
    mkaServerStream_->stopClient();
    fbossAgentStream_->stopClient();
    baton_->reset();
    EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(500)));

    evb_.terminateLoopSoon();
    timeThread_->join();
    timeThread_.reset();
    baton_->reset();
    EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(100)));
  }

  std::shared_ptr<folly::Baton<>> baton_;

  std::unique_ptr<folly::ScopedEventBaseThread> mkaClientThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> fbossClientThread_;

  std::shared_ptr<BidirectionalPacketStream> mkaServerStream_;
  std::shared_ptr<BidirectionalPacketStream> fbossAgentStream_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> mkaServer_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> fbossAgent_;
  folly::EventBase evb_;
  std::unique_ptr<std::thread> timeThread_;
};

#if FOLLY_HAS_COROUTINES
TEST_F(BidirectionalPacketStreamTest, InvalidTimerTest) {
  EXPECT_THROW(
      std::make_shared<BidirectionalPacketStream>(
          "mka_server",
          mkaClientThread_->getEventBase(),
          nullptr,
          g_connect_timer),
      std::exception);
}

TEST_F(BidirectionalPacketStreamTest, InvalidClientTest) {
  EXPECT_THROW(mkaServerStream_->connectClient(0), std::exception);
}

TEST_F(BidirectionalPacketStreamTest, MKAServerAutoReconnectTest) {
  // create a random port in ephemeral port range.
  auto fbossAgentPort = fbossAgent_->getPort();
  // lets listen on some ports before connecting.
  auto listeningPort = "eth0";
  mkaServerReconnect(fbossAgentPort, listeningPort);
}

TEST_F(BidirectionalPacketStreamTest, FbossAgentAutoReconnectTest) {
  // create a random port in ephemeral port range.
  auto serverPort = mkaServer_->getPort();
  auto listeningPort = "eth0";
  fbossAgentReconnect(serverPort, listeningPort);
}

TEST_F(BidirectionalPacketStreamTest, MKAServerDisconnectReconnectTest) {
  // create a random port in ephemeral port range.
  auto fbossAgentPort = fbossAgent_->getPort();
  // lets listen on some ports before connecting.
  auto listeningPort = "eth0";
  mkaServerReconnect(fbossAgentPort, listeningPort);
  auto transport = mkaServerStream_->listen(listeningPort);

  // now lets disconnect Fboss Agent and see if mkaserver would
  // automatically connect when fboss agent becomes active.

  // Now stop the fboss Server.
  fbossAgent_.reset();
  fbossAgentStream_.reset();
  fbossClientThread_ = std::make_unique<folly::ScopedEventBaseThread>();

  baton_->reset();
  EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(200)));
  EXPECT_FALSE(mkaServerStream_->isConnectedToServer());
  EXPECT_FALSE(mkaServerStream_->isClientConnected("fboss_agent"));

  // ensure send fails
  auto mkpdu = folly::IOBuf::copyBuffer(g_mkaTofboss);
  EXPECT_EQ(-1, transport->send(std::move(mkpdu)));

  // Now lets start the fboss Server and mka should automatically connect.
  baton_->reset();
  fbossAgentStream_ = std::make_shared<BidirectionalPacketStream>(
      "fboss_agent",
      fbossClientThread_->getEventBase(),
      &evb_,
      g_connect_timer);

  fbossAgent_ = std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
      fbossAgentStream_, "::1", fbossAgentPort);
  fbossAgentStream_->connectClient(mkaServer_->getPort());

  EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(200)));
  EXPECT_TRUE(mkaServerStream_->isConnectedToServer());
  EXPECT_TRUE(mkaServerStream_->isClientConnected("fboss_agent"));

  EXPECT_TRUE(fbossAgentStream_->isConnectedToServer());
  EXPECT_TRUE(fbossAgentStream_->isClientConnected("mka_server"));
  // we have not re-registered the port, this should automatically allow
  // sending to ports.
  // we can try to send packets.
  sendMkaToFboss(1, listeningPort, transport);
  sendFbossToMka(1, listeningPort, transport);
}

TEST_F(BidirectionalPacketStreamTest, FbossAgentDisconnectReconnectTest) {
  // create a random port in ephemeral port range.
  auto serverPort = mkaServer_->getPort();
  auto listeningPort = "eth0";
  fbossAgentReconnect(serverPort, listeningPort);
  // lets disconnect and try again.
  fbossAgentReconnect(serverPort, listeningPort);
}

TEST_F(BidirectionalPacketStreamTest, ConnectTest) {
  tryConnect();
}
TEST_F(BidirectionalPacketStreamTest, EmptyTransport) {
  tryConnect();
  std::string port;
  auto transport = mkaServerStream_->listen(port);
  EXPECT_EQ(transport, nullptr);
}

TEST_F(BidirectionalPacketStreamTest, ListenTestFromFboss) {
  tryConnect();
  auto port = "eth0";
  auto transport = mkaServerStream_->listen(port);
  sendFbossToMka(1, port, transport);
}

TEST_F(BidirectionalPacketStreamTest, ListenTestFromMka) {
  tryConnect();
  auto port = "eth0";
  auto transport = mkaServerStream_->listen(port);
  sendMkaToFboss(1, port, transport);
}

TEST_F(BidirectionalPacketStreamTest, StartFromFboss) {
  tryConnect();
  auto port = "eth0";
  auto transport = mkaServerStream_->listen(port);
  sendFbossToMka(1, port, transport);
  sendMkaToFboss(1, port, transport);
}

TEST_F(BidirectionalPacketStreamTest, StartFromMka) {
  tryConnect();
  auto port = "eth0";
  auto transport = mkaServerStream_->listen(port);
  sendFbossToMka(1, port, transport);
  sendMkaToFboss(1, port, transport);
}

TEST_F(BidirectionalPacketStreamTest, TestMultiplePkts) {
  tryConnect();
  auto port = "eth0";
  auto transport = mkaServerStream_->listen(port);

  sendMkaToFboss(100, port, transport);
  sendFbossToMka(100, port, transport);
}

TEST_F(BidirectionalPacketStreamTest, SendParallelMultiplePkts) {
  sendParallelMultiplePkts();
}

TEST_F(BidirectionalPacketStreamTest, sendParallelMultiplePktsMultiplePorts) {
  sendParallelMultiplePktsMultiplePorts();
}

TEST_F(
    BidirectionalPacketStreamTest,
    sendParallelMultiplePktsMultiplePortsNoOrder) {
  tryConnect();
  size_t numPkts = 100;
  std::vector<std::shared_ptr<AsyncPacketTransport>> transportList;
  std::vector<std::shared_ptr<std::thread>> threadList;
  PacketRecvAcceptor rcvAcceptor(baton_);

  rcvAcceptor.setExpectedPackets(numPkts * g_ports.size());
  fbossAgentStream_->setPacketAcceptor(&rcvAcceptor);
  baton_->reset();
  auto index = 0;
  for (auto& port : g_ports) {
    auto transport = mkaServerStream_->listen(port);
    transportList.emplace_back(transport);
  }

  for (auto& port : g_ports) {
    // order reversed
    auto transport = transportList[index++];
    auto mkaThread = std::make_shared<std::thread>([&, transport, this]() {
      sendMkaToFboss(numPkts, port, transport, baton_, false);
    });
    auto fbossThread = std::make_shared<std::thread>(
        [&, transport, this]() { sendFbossToMka(numPkts, port, transport); });

    threadList.emplace_back(fbossThread);
    threadList.emplace_back(mkaThread);
  }
  for (auto& thread : threadList) {
    thread->join();
  }

  EXPECT_TRUE(baton_->try_wait_for(std::chrono::milliseconds(500)));
  // validate packets received in fboss
  fbossAgentStream_->setPacketAcceptor(nullptr);
  // verify the packets
  EXPECT_EQ(rcvAcceptor.packets_.size(), numPkts * g_ports.size());
  for (auto& pkt : rcvAcceptor.packets_) {
    EXPECT_EQ(g_mkaTofboss, *pkt.buf());
  }
}

TEST_F(BidirectionalPacketStreamTest, ValidateLazyPortRegister) {
  auto port = "eth0";
  auto transport = mkaServerStream_->listen(port);

  tryConnect();

  sendMkaToFboss(100, port, transport);
  sendFbossToMka(100, port, transport);
}

TEST_F(BidirectionalPacketStreamTest, CloseSendFailed) {
  auto port = "eth0";

  tryConnect();
  auto transport = mkaServerStream_->listen(port);

  sendMkaToFboss(100, port, transport);
  sendFbossToMka(100, port, transport);
  mkaServerStream_->close(port);
  // check send failed.
  PacketAcceptor acceptor(baton_);
  acceptor.setExpectedPackets(1);
  transport->setReadCallback(&acceptor);
  baton_->reset();
  std::string pktString = "FromFbossToMka";
  TPacket packet;
  *packet.l2Port() = port;
  *packet.buf() = pktString;
  // send TPacket from fboss -> mka
  EXPECT_EQ(-1, fbossAgentStream_->send(std::move(packet)));
  EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(50)));
}

TEST_F(BidirectionalPacketStreamTest, TransportCloseSendFailed) {
  auto port = "eth0";

  tryConnect();
  auto transport = mkaServerStream_->listen(port);

  sendMkaToFboss(100, port, transport);
  sendFbossToMka(100, port, transport);
  transport->close();
  // check send failed.
  PacketAcceptor acceptor(baton_);
  acceptor.setExpectedPackets(1);
  transport->setReadCallback(&acceptor);
  baton_->reset();
  std::string pktString = "FromFbossToMka";
  TPacket packet;
  *packet.l2Port() = port;
  *packet.buf() = pktString;
  // send TPacket from fboss -> mka
  EXPECT_EQ(-1, fbossAgentStream_->send(std::move(packet)));
  EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(50)));
}

TEST_F(BidirectionalPacketStreamTest, SendWithOutPort) {
  auto port = "eth0";

  tryConnect();
  auto transport = mkaServerStream_->listen(port);

  sendMkaToFboss(100, port, transport);
  sendFbossToMka(100, port, transport);
  // check send failed.
  PacketAcceptor acceptor(baton_);
  acceptor.setExpectedPackets(1);
  transport->setReadCallback(&acceptor);
  baton_->reset();
  std::string pktString = "FromFbossToMka";
  TPacket packet;
  *packet.buf() = pktString;
  // send TPacket from fboss -> mka
  EXPECT_EQ(-1, fbossAgentStream_->send(std::move(packet)));
  EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(50)));
}

TEST_F(BidirectionalPacketStreamTest, MKADisconnect) {
  auto port = "eth0";

  tryConnect();
  auto transport = mkaServerStream_->listen(port);

  sendMkaToFboss(100, port, transport);
  sendFbossToMka(100, port, transport);
  // check send failed.
  PacketAcceptor acceptor(baton_);
  acceptor.setExpectedPackets(1);
  transport->setReadCallback(&acceptor);
  baton_->reset();
  std::string pktString = "FromFbossToMka";
  TPacket packet;
  *packet.buf() = pktString;
  // send TPacket from fboss -> mka
  EXPECT_EQ(-1, fbossAgentStream_->send(std::move(packet)));
  EXPECT_FALSE(baton_->try_wait_for(std::chrono::milliseconds(50)));
}

TEST_F(BidirectionalPacketStreamTest, WaitForDisconnectedServer) {
  auto port = fbossAgent_->getPort();
  fbossAgent_.reset();
  auto timeout = 50;
  mkaServerStream_ = std::make_shared<BidirectionalPacketStream>(
      "mka_server", mkaClientThread_->getEventBase(), &evb_, timeout);
  auto baton = folly::Baton<>();
  LOG(INFO) << "Starting the test";
  auto retry = 50;
  mkaServerStream_->connectClient(port);
  while (!mkaServerStream_->isConnectedToServer() && --retry >= 0) {
    EXPECT_FALSE(baton.try_wait_for(std::chrono::milliseconds(50)));
    baton.reset();
  }
  EXPECT_FALSE(mkaServerStream_->isConnectedToServer());
}

#endif
