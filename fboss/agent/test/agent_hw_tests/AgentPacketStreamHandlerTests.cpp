// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/PacketStreamHandler.h"
#include "fboss/agent/SwAgentInitializer.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/thrift_packet_stream/PacketStreamClient.h"
#include "fboss/lib/CommonUtils.h"

#include <fb303/ServiceData.h>
#include <folly/io/async/ScopedEventBaseThread.h>

#if FOLLY_HAS_COROUTINES

DECLARE_int32(port);

using namespace std::chrono_literals;

namespace facebook::fboss {

namespace {

// Test client that receives packets from the PacketStreamHandler's stream.
class TestPacketStreamClient : public PacketStreamClient {
 public:
  TestPacketStreamClient(const std::string& clientId, folly::EventBase* evb)
      : PacketStreamClient(clientId, evb) {}

  void recvPacket(TPacket&& packet) override {
    std::lock_guard<std::mutex> lk(mutex_);
    receivedPackets_.push_back(std::move(packet));
  }

  int receivedCount() {
    std::lock_guard<std::mutex> lk(mutex_);
    return receivedPackets_.size();
  }

 private:
  std::vector<TPacket> receivedPackets_;
  std::mutex mutex_;
};

} // namespace

class AgentPacketStreamHandlerTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::CPU_RX_TX,
        ProductionFeature::AIFM_PACKET_STREAM_HANDLER};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto asic = checkSameAndGetAsicForTesting(ensemble.getL3Asics());
    auto cfg = AgentHwTest::initialConfig(ensemble);
    utility::setDefaultCpuTrafficPolicyConfig(cfg, {asic}, ensemble.isSai());
    utility::addCpuQueueConfig(cfg, {asic}, ensemble.isSai());
    addAifmCtrlAcl(cfg, asic, ensemble.isSai());
    return cfg;
  }

  void addAifmCtrlAcl(cfg::SwitchConfig& config, const HwAsic* asic, bool isSai)
      const {
    cfg::AclEntry acl;
    acl.name() = "cpuPolicing-aifm-packet-stream";
    if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      acl.etherType() =
          static_cast<cfg::EtherType>(PacketStreamHandler::ETHERTYPE_AIFM_CTRL);
    } else {
      acl.dstMac() = "00:11:22:33:44:55";
    }
    utility::addAcl(&config, acl, cfg::AclStage::INGRESS);

    auto action = utility::createQueueMatchAction(
        asic,
        utility::getCoppHighPriQueueId(asic),
        isSai,
        utility::getCpuActionType(asic));
    cfg::MatchToAction matchToAction;
    matchToAction.matcher() = *acl.name();
    matchToAction.action() = action;
    config.cpuTrafficPolicy()->trafficPolicy()->matchToAction()->push_back(
        matchToAction);
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_aifm_ctrl_handler = true;
  }

  void SetUp() override {
    AgentHwTest::SetUp();
  }

  void TearDown() override {
    teardownClient();
    AgentHwTest::TearDown();
  }

  void connectClient() {
    auto* handler = getSw()->getPacketStreamHandler();
    CHECK(handler);
    clientEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>(
        "packet_stream_test_client");
    client_ = std::make_unique<TestPacketStreamClient>(
        "packet_stream_test_client", clientEvbThread_->getEventBase());
    client_->connectToServer("::1", FLAGS_port);
    WITH_RETRIES_N_TIMED(
        30, 500ms, { ASSERT_EVENTUALLY_TRUE(client_->isConnectedToServer()); });
    WITH_RETRIES_N_TIMED(30, 500ms, {
      ASSERT_EVENTUALLY_TRUE(
          handler->isClientConnected("packet_stream_test_client"));
    });
    client_->createSink();
    WITH_RETRIES_N_TIMED(
        30, 500ms, { ASSERT_EVENTUALLY_TRUE(client_->isSinkReady()); });
  }

  void teardownClient() {
    client_.reset();
    clientEvbThread_.reset();
  }

  // Build a TxPacket with AIFM ctrl ethertype and send it out a port.
  // With loopback mode, it comes back and gets dispatched to
  // PacketStreamHandler::handlePacket() which delivers it via stream.
  void sendPacketOutPort(PortID port) {
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto iobuf = folly::IOBuf::create(64);
    iobuf->append(64);
    folly::io::RWPrivateCursor cursor(iobuf.get());
    auto dstMacBytes =
        std::array<uint8_t, 6>{0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    cursor.push(dstMacBytes.data(), folly::MacAddress::SIZE);
    cursor.push(srcMac.bytes(), folly::MacAddress::SIZE);
    cursor.writeBE<uint16_t>(PacketStreamHandler::ETHERTYPE_AIFM_CTRL);

    auto txPacket = getSw()->allocatePacket(iobuf->computeChainDataLength());
    folly::io::RWPrivateCursor txCursor(txPacket->buf());
    txCursor.push(iobuf->data(), iobuf->length());

    getSw()->sendNetworkControlPacketAsync(
        std::move(txPacket), PortDescriptor(port));
  }

  // Build a raw Ethernet frame with AIFM ctrl ethertype as a string,
  // suitable for sending via the packetSink sink.
  std::string buildAifmCtrlFrame() {
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto iobuf = folly::IOBuf::create(64);
    iobuf->append(64);
    folly::io::RWPrivateCursor cursor(iobuf.get());
    auto dstMacBytes =
        std::array<uint8_t, 6>{0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    cursor.push(dstMacBytes.data(), folly::MacAddress::SIZE);
    cursor.push(srcMac.bytes(), folly::MacAddress::SIZE);
    cursor.writeBE<uint16_t>(PacketStreamHandler::ETHERTYPE_AIFM_CTRL);
    return std::string(
        reinterpret_cast<const char*>(iobuf->data()), iobuf->length());
  }

  std::unique_ptr<folly::ScopedEventBaseThread> clientEvbThread_;
  std::unique_ptr<TestPacketStreamClient> client_;
};

/*
 * Combined sink + stream end-to-end loopback test:
 *
 * 1. Client sends packet via packetSink sink
 * 2. PacketStreamHandler.processReceivedPacket() sends it to HW
 * 3. Port is in loopback mode, so packet comes back to CPU
 * 4. SwSwitch dispatches by ethertype to PacketStreamHandler.handlePacket()
 * 5. handlePacket() delivers packet back to the stream client
 * 6. Verify both fromAifmCtrlAgent and toAifmCtrlAgent counters
 */
TEST_F(AgentPacketStreamHandlerTest, VerifySinkToStreamLoopback) {
  auto verify = [this]() {
    connectClient();
    auto port = masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0];
    auto portName = getSw()->getState()->getPorts()->getNode(port)->getName();

    // Capture all counters before
    auto rxReceivedBefore =
        fb303::fbData->getCounter("fromAifmCtrlAgent.packets_received.sum");
    auto rxSentToHwBefore =
        fb303::fbData->getCounter("fromAifmCtrlAgent.packets_sent_to_hw.sum");
    auto txSentBefore =
        fb303::fbData->getCounter("toAifmCtrlAgent.packets_sent.sum");
    auto clientCountBefore = client_->receivedCount();

    // Build a frame with AIFM ctrl ethertype so it loops back to us
    auto frame = buildAifmCtrlFrame();

    // Send packets via client_->send() -> sink -> processReceivedPacket -> HW
    constexpr int kPacketCount = 5;
    for (int i = 0; i < kPacketCount; i++) {
      TPacket pkt;
      pkt.l2Port() = portName;
      pkt.buf() = frame;
      client_->send(std::move(pkt));
    }

    // Verify Rx path counters (sink -> processReceivedPacket -> HW)
    WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(500), {
      auto rxReceivedAfter =
          fb303::fbData->getCounter("fromAifmCtrlAgent.packets_received.sum");
      ASSERT_EVENTUALLY_GE(rxReceivedAfter - rxReceivedBefore, kPacketCount);
    });
    auto rxSentToHwAfter =
        fb303::fbData->getCounter("fromAifmCtrlAgent.packets_sent_to_hw.sum");
    EXPECT_GE(rxSentToHwAfter - rxSentToHwBefore, kPacketCount);

    // Verify Tx path: HW loopback -> handlePacket -> stream -> client
    WITH_RETRIES_N_TIMED(20, std::chrono::milliseconds(500), {
      ASSERT_EVENTUALLY_GE(
          client_->receivedCount() - clientCountBefore, kPacketCount);
    });
    auto txSentAfter =
        fb303::fbData->getCounter("toAifmCtrlAgent.packets_sent.sum");
    EXPECT_GE(txSentAfter - txSentBefore, kPacketCount);
  };

  verifyAcrossWarmBoots([]() {}, verify);
}

} // namespace facebook::fboss

#endif // FOLLY_HAS_COROUTINES
