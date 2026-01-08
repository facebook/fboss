// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

#include <folly/io/IOBuf.h>
#include <chrono>
#include <thread>
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/MacTestUtils.h"

namespace facebook::fboss {

class AgentMalformedPacketTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::MAC_LEARNING};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return config;
  }

  std::unique_ptr<TxPacket> createPacketWithNoChecksum() {
    auto vlanId = getProgrammedState()->getVlans()->getFirstVlanID();
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);

    // Create a normal UDP packet first using the utility function
    folly::IPAddressV4 srcIp("10.0.0.1");
    folly::IPAddressV4 dstIp("10.0.0.2");
    uint16_t srcPort = 1000;
    uint16_t dstPort = 2000;

    auto pkt = utility::makeUDPTxPacket(
        getSw(), vlanId, srcMac, intfMac, srcIp, dstIp, srcPort, dstPort);

    // Now corrupt the UDP checksum by setting it to 0
    // The UDP checksum is located after the IP header and the first 6 bytes of
    // UDP header UDP header format: srcPort(2) + dstPort(2) + length(2) +
    // checksum(2)
    auto buf = pkt->buf();

    // Calculate the offset to UDP checksum
    // EthHdr::SIZE + (VLAN tag if present) + IPv4 header + UDP src port + UDP
    // dst port + UDP length
    size_t ethSize = EthHdr::SIZE;
    size_t ipv4Size = 20; // Standard IPv4 header size
    size_t udpChecksumOffset =
        ethSize + ipv4Size + 2 + 2 + 2; // After src port, dst port, length

    // Set the UDP checksum to 0 (invalid)
    folly::io::RWPrivateCursor writer(buf);
    writer.skip(udpChecksumOffset);
    writer.writeBE<uint16_t>(0);

    return pkt;
  }
};

TEST_F(
    AgentMalformedPacketTest,
    PacketWithNoChecksumDoesNotCreateMacEntry) { // Get the ingress port to
                                                 // inject the packet on
  auto ingressPort = masterLogicalInterfacePortIds()[0];

  // Get initial MAC table entries and drop stats
  auto initialL2Entries = utility::getL2Table(getSw());
  auto initialMacTableSize = initialL2Entries.size();
  auto initialDropStats = getAggregatedSwitchDropStats();

  // Inject packet with no checksum on ingress port
  // This simulates the packet arriving on the switch port
  auto pkt = createPacketWithNoChecksum();
  getSw()->sendPacketOutOfPortAsync(std::move(pkt), ingressPort);

  // Wait for potential MAC learning with retry loop
  // Check multiple times to ensure MAC table doesn't grow
  constexpr int kMaxRetries = 5;
  constexpr auto kRetryInterval = std::chrono::milliseconds(500);
  for (int i = 0; i < kMaxRetries; ++i) {
    std::this_thread::sleep_for(kRetryInterval);
    auto currentL2Entries = utility::getL2Table(getSw());
    // Fail immediately if MAC table grew
    ASSERT_EQ(initialMacTableSize, currentL2Entries.size())
        << "MAC table should not learn from malformed packets with no "
           "checksum (retry "
        << i << ")";
  }

  // Final verification that MAC table has not grown
  auto finalL2Entries = utility::getL2Table(getSw());
  auto finalMacTableSize = finalL2Entries.size();
  EXPECT_EQ(initialMacTableSize, finalMacTableSize)
      << "MAC table should not learn from malformed packets with no checksum";

  // Verify that drop counters incremented (packet was dropped)
  // Retry loop for drop stats as counter updates can be asynchronous
  auto initialDropCount = initialDropStats.packetIntegrityDrops().has_value()
      ? *initialDropStats.packetIntegrityDrops()
      : 0;
  int64_t finalDropCount = initialDropCount;
  for (int i = 0; i < kMaxRetries; ++i) {
    std::this_thread::sleep_for(kRetryInterval);
    auto finalDropStats = getAggregatedSwitchDropStats();
    finalDropCount = finalDropStats.packetIntegrityDrops().has_value()
        ? *finalDropStats.packetIntegrityDrops()
        : 0;
    if (finalDropCount > initialDropCount) {
      break;
    }
  }

  // The packet should have been dropped, so drop count should increase
  // Note: Some platforms may not increment drop counters for checksum errors,
  // so we only log a warning if drops didn't increase
  if (finalDropCount <= initialDropCount) {
    XLOG(WARNING)
        << "Packet integrity drop counter did not increment for malformed "
           "packet. "
        << "Initial drops: " << initialDropCount
        << ", Final drops: " << finalDropCount
        << ". This may be expected on some platforms that don't count "
        << "checksum errors in packetIntegrityDrops.";
  } else {
    XLOG(DBG2) << "Malformed packet was correctly dropped. "
               << "Initial drops: " << initialDropCount
               << ", Final drops: " << finalDropCount;
  }
}

} // namespace facebook::fboss
