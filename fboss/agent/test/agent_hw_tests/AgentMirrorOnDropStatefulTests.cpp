// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <algorithm>
#include <chrono>
#include <string_view>
#include <thread>
#include <vector>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/chenab/ChenabModHeader.h"
#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropTestBase.h"
#include "fboss/agent/test/utils/PacketSnooper.h"

#include <folly/Conv.h>
#include <folly/String.h>

namespace facebook::fboss {

class AgentMirrorOnDropStatefulTest : public AgentMirrorOnDropTestBase {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::MIRROR_ON_DROP_STATEFUL};
  }

 protected:
  static constexpr auto kSnooperPacketType =
      utility::packetSnooperReceivePacketType::PACKET_TYPE_CHENAB_MOD;
  static constexpr size_t kTruncateSize = 128;

  cfg::MirrorOnDropReport makeReport(const std::string& name, int truncateSize)
      const {
    cfg::MirrorOnDropReport report;
    report.name() = name;
    report.localSrcPort() = kMirrorSrcPort;
    report.collectorIp() = kCollectorIp_.str();
    report.collectorPort() = kMirrorDstPort;
    report.mtu() = 1500;
    report.truncateSize() = truncateSize;
    report.dscp() = 0;
    report.mirrorPort().ensure().tunnel().ensure().srcIp() = kSwitchIp_.str();
    return report;
  }

  void setupMirrorOnDrop(const std::string& name, int truncateSize) {
    const auto injectionPort = masterLogicalInterfacePortIds()[0];
    const auto collectorPort = masterLogicalInterfacePortIds()[1];
    auto config = getAgentEnsemble()->getCurrentConfig();
    addDropPacketAcl(&config, injectionPort);
    applyMirrorOnDropConfig(
        config, makeReport(name, truncateSize), collectorPort);
  }

  std::optional<ChenabModPacket> waitForModPacket(
      utility::SwSwitchPacketSnooper& snooper,
      uint32_t timeoutSeconds = 1) {
    auto packet = snooper.waitForPacket(timeoutSeconds);
    if (!packet) {
      return std::nullopt;
    }
    // The snooper parsed this same frame before adding it to its queue.
    folly::io::Cursor cursor{packet->get()};
    return parseChenabModPacket(cursor);
  }

  std::optional<ChenabTacNewEntry> waitForNewEntry(
      utility::SwSwitchPacketSnooper& snooper,
      const std::vector<uint8_t>& expectedPacket) {
    std::optional<ChenabTacNewEntry> result;
    WITH_RETRIES_N_TIMED(10, std::chrono::seconds(1), {
      auto modPacket = waitForModPacket(snooper);
      if (modPacket.has_value()) {
        if (auto* entry = std::get_if<ChenabTacNewEntry>(&*modPacket);
            entry && matchesTruncatedPacket(*entry, expectedPacket)) {
          result = *entry;
        }
      }
      EXPECT_EVENTUALLY_TRUE(result.has_value());
    });
    return result;
  }

  std::vector<uint8_t> expectedTruncatedPacket(const TxPacket& packet) const {
    folly::io::Cursor cursor{packet.buf()};
    utility::EthFrame frame{cursor};
    frame.stripVlans();
    auto untagged = frame.toIOBuf();
    folly::io::Cursor untaggedCursor{untagged.get()};
    std::vector<uint8_t> expected(kTruncateSize);
    untaggedCursor.pull(expected.data(), expected.size());
    return expected;
  }

  bool matchesTruncatedPacket(
      const ChenabTacNewEntry& entry,
      const std::vector<uint8_t>& expected) const {
    return entry.truncatedPacket == expected;
  }

  uint16_t portLabel(PortID portId) const {
    const auto port = getProgrammedState()->getPorts()->getNode(portId);
    std::vector<std::string_view> nameParts;
    folly::split('/', port->getName(), nameParts);
    CHECK_EQ(nameParts.size(), 3);
    const auto module = folly::to<uint16_t>(nameParts[1]);
    const auto split = folly::to<uint16_t>(nameParts[2]);
    CHECK_GT(split, 0);
    return static_cast<uint16_t>((module << 4) | (split - 1));
  }
};

} // namespace facebook::fboss
