// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropStatelessTest.h"

#include <gtest/gtest.h>

#include <chrono>

#include <folly/ScopeGuard.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/single/MonolithicHwSwitchHandler.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropTajoImpl.h"
#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropXgsImpl.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

namespace {

// Pack an ingress-pipeline drop code (default-route, ACL) into the wire
// format's two-slot drop-reason struct.
MirrorOnDropDropReasonCodes ingressOnly(uint16_t code) {
  return {.ingressDropReason = code, .egressDropReason = 0};
}

} // namespace

using namespace ::testing;

// Factory: ONE switch — adding a new ASIC means adding one case here.
std::unique_ptr<MirrorOnDropImpl> createMirrorOnDropImpl(cfg::AsicType type) {
  // NOLINTNEXTLINE(clang-diagnostic-switch-enum): default handles all others.
  switch (type) {
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK6:
      return std::make_unique<XgsMirrorOnDropImpl>();
    case cfg::AsicType::ASIC_TYPE_YUBA: // gibraltar
    case cfg::AsicType::ASIC_TYPE_G202X: // graphene200
      return std::make_unique<TajoMirrorOnDropImpl>();
    default:
      throw FbossError(
          "createMirrorOnDropImpl: no MirrorOnDropImpl for AsicType ",
          static_cast<int>(type));
  }
}

MirrorOnDropImpl* AgentMirrorOnDropStatelessTest::impl() {
  if (!impl_) {
    auto* asic =
        checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());
    impl_ = createMirrorOnDropImpl(asic->getAsicType());
  }
  return impl_.get();
}

std::vector<ProductionFeature>
AgentMirrorOnDropStatelessTest::getProductionFeaturesVerified() const {
  // The umbrella gate for any ASIC with a stateless MirrorOnDrop impl. We do
  // NOT call impl()->getProductionFeature() here: gtest invokes this during
  // test discovery on every platform, and the factory throws FbossError on
  // unsupported AsicTypes — calling it here would crash the test runner on
  // every non-stateless ASIC.
  return {
      ProductionFeature::MIRROR_ON_DROP,
      ProductionFeature::MIRROR_ON_DROP_STATELESS};
}

cfg::MirrorOnDropReport AgentMirrorOnDropStatelessTest::makeMirrorOnDropReport(
    const std::string& name,
    std::optional<int32_t> samplingRate) {
  return impl()->makeReport(
      name,
      kCollectorIp_,
      kMirrorDstPort,
      kMirrorSrcPort,
      kSwitchIp_,
      samplingRate);
}

MirrorOnDropPacketFields
AgentMirrorOnDropStatelessTest::parseMirrorOnDropPacket(
    const folly::IOBuf* buf) {
  return impl()->parsePacket(buf);
}

void AgentMirrorOnDropStatelessTest::verifyAsicSpecificInvariants(
    const folly::IOBuf* buf) {
  impl()->verifyInvariants(buf);
}

MirrorOnDropDropReasonCodes
AgentMirrorOnDropStatelessTest::getDefaultRouteDropReasons() {
  return ingressOnly(impl()->getDefaultRouteDropReason());
}

MirrorOnDropDropReasonCodes
AgentMirrorOnDropStatelessTest::getAclDropReasons() {
  return ingressOnly(impl()->getAclDropReason());
}

MirrorOnDropDropReasonCodes
AgentMirrorOnDropStatelessTest::getMmuDropReasons() {
  return impl()->packMmuDropReason(impl()->getMmuDropReason());
}

MirrorOnDropDropReasonCodes
AgentMirrorOnDropStatelessTest::getSrv6MidpointNonLastSidDropReasons() {
  return ingressOnly(impl()->getSrv6MidpointNonLastSidDropReason());
}

MirrorOnDropDropReasonCodes
AgentMirrorOnDropStatelessTest::getSrv6DecapNonLastSegmentDropReasons() {
  return ingressOnly(impl()->getSrv6DecapNonLastSegmentDropReason());
}

MirrorOnDropDropReasonCodes
AgentMirrorOnDropStatelessTest::getSrv6BindingSidNonLastSidDropReasons() {
  return ingressOnly(impl()->getSrv6BindingSidNonLastSidDropReason());
}

MirrorOnDropDropReasonCodes
AgentMirrorOnDropStatelessTest::getSrv6MidpointUnresolvedDropReasons() {
  return ingressOnly(impl()->getSrv6MidpointUnresolvedDropReason());
}

void AgentMirrorOnDropStatelessTest::configureMmuDropBuffers(
    cfg::SwitchConfig& config,
    const PortID& injectionPortId,
    int priority) {
  auto hwAsic = checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());
  utility::setupPfcBuffers(
      getAgentEnsemble(),
      config,
      {injectionPortId},
      {}, // losslessPgIds
      {priority}, // lossyPgIds
      utility::PfcBufferParams::getPfcBufferParams(
          hwAsic->getAsicType(), kDefaultGlobalSharedBytes));
}

utility::PacketFilterFn
AgentMirrorOnDropStatelessTest::mirrorOnDropRxPacketFilter() {
  return [](const folly::IOBuf* buf) {
    return looksLikeMirrorOnDropOuterPacket(buf);
  };
}

void AgentMirrorOnDropStatelessTest::configureErspanMirror(
    cfg::SwitchConfig& config,
    const std::string& mirrorName,
    const folly::IPAddressV6& tunnelDstIp,
    const folly::IPAddressV6& tunnelSrcIp,
    const PortID& srcPortId) {
  impl()->configureErspanMirror(
      config, mirrorName, tunnelDstIp, tunnelSrcIp, srcPortId);
}

std::optional<std::unique_ptr<folly::IOBuf>>
AgentMirrorOnDropStatelessTest::waitForMirrorOnDropPacket(
    utility::SwSwitchPacketSnooper& snooper,
    uint32_t timeout_s) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(timeout_s);
  while (true) {
    const auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
      return std::nullopt;
    }
    const auto remainingSec =
        std::chrono::duration_cast<std::chrono::seconds>(deadline - now)
            .count();
    const uint32_t waitSec =
        remainingSec > 0 ? static_cast<uint32_t>(remainingSec) : 1;
    auto frameRx = snooper.waitForPacket(waitSec);
    if (!frameRx) {
      return std::nullopt;
    }
    if (looksLikeMirrorOnDropOuterPacket(frameRx->get())) {
      return frameRx;
    }
    XLOG(DBG2) << "Ignoring non-MOD CPU packet while waiting for MOD export "
               << "(expected outer IPv6 dst " << kCollectorIp_.str()
               << ", UDP dport " << kMirrorDstPort << ")";
  }
}

void AgentMirrorOnDropStatelessTest::validateMirrorOnDropPacket(
    const folly::IOBuf* captured,
    const PortID& injectionPortId,
    const MirrorOnDropDropReasonCodes& expectedReasons,
    std::optional<folly::IPAddressV6> expectedInnerDstIp,
    std::optional<folly::IPAddressV6> expectedInnerSrcIp,
    std::optional<PortID> expectedIngressPortId) {
  EXPECT_TRUE(looksLikeMirrorOnDropOuterPacket(captured))
      << "Captured CPU packet is not a MOD export (expected outer IPv6 dst "
      << kCollectorIp_.str() << ", UDP dport " << kMirrorDstPort << ")";
  verifyAsicSpecificInvariants(captured);
  auto fields = parseMirrorOnDropPacket(captured);

  folly::MacAddress expectedOuterSrcMac = getLocalMacAddress();
  const bool isTajoImpl =
      dynamic_cast<TajoMirrorOnDropImpl*>(impl()) != nullptr;
  if (isTajoImpl) {
    expectedOuterSrcMac =
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    const auto& cfg = getAgentEnsemble()->getCurrentConfig();
    if (cfg.switchSettings().has_value()) {
      const auto& infoRef = cfg.switchSettings().value().switchIdToSwitchInfo();
      if (infoRef.has_value() && !infoRef.value().empty()) {
        const auto& switchInfo = infoRef.value().begin()->second;
        if (switchInfo.switchMac().has_value()) {
          expectedOuterSrcMac =
              folly::MacAddress(switchInfo.switchMac().value());
        }
      }
    }
  }
  EXPECT_EQ(fields.outerSrcMac, expectedOuterSrcMac);
  EXPECT_EQ(fields.outerDstMac, kCollectorNextHopMac_);
  EXPECT_EQ(fields.outerSrcIp, kSwitchIp_);
  EXPECT_EQ(fields.outerDstIp, kCollectorIp_);
  EXPECT_EQ(fields.outerDstPort, static_cast<uint16_t>(kMirrorDstPort));
  const HwSwitch* hwSwitch = nullptr;
  if (auto* monoHandler = getSw()->getMonolithicHwSwitchHandler()) {
    hwSwitch = monoHandler->getHwSwitch();
  }
  const PortID& ingressPortForCheck =
      expectedIngressPortId.value_or(injectionPortId);
  const uint16_t expectedIngress = isTajoImpl
      ? expectedTajoModIngressPort(ingressPortForCheck, hwSwitch)
      : impl()->expectedIngressPort(hwSwitch, ingressPortForCheck);
  EXPECT_EQ(fields.ingressPort, expectedIngress);
  EXPECT_EQ(fields.dropReasonIngress, expectedReasons.ingressDropReason);
  EXPECT_EQ(fields.dropReasonEgress, expectedReasons.egressDropReason);
  EXPECT_EQ(fields.innerSrcIp, expectedInnerSrcIp.value_or(kPacketSrcIp_));

  if (expectedInnerDstIp.has_value()) {
    EXPECT_EQ(fields.innerDstIp, expectedInnerDstIp.value());
  }
}

void AgentMirrorOnDropStatelessTest::waitForStatsToStabilize(
    const std::vector<PortID>& ports) {
  constexpr int kStableIterations = 3;
  int stableCount = 0;
  // Compare against stats from the previous WITH_RETRIES iteration, not the
  // moment before WITH_RETRIES started — back-to-back samples with no time
  // gap can spuriously appear stable even under active traffic.
  std::optional<std::map<PortID, HwPortStats>> prevStats;
  WITH_RETRIES({
    auto curStats = getLatestPortStats(ports);
    if (prevStats.has_value()) {
      const bool unchanged =
          std::all_of(ports.begin(), ports.end(), [&](const auto& portId) {
            // .at() throws on missing port — operator[] would silently
            // default-construct HwPortStats and crash on the dereference of
            // an empty optional outUnicastPkts_(), masking the real cause.
            return *curStats.at(portId).outUnicastPkts_() ==
                *prevStats.value().at(portId).outUnicastPkts_();
          });
      stableCount = unchanged ? stableCount + 1 : 0;
    }
    prevStats = std::move(curStats);
    EXPECT_EVENTUALLY_GE(stableCount, kStableIterations);
  });
}

void AgentMirrorOnDropStatelessTest::testDefaultRouteDrop() {
  PortID injectionPortId = masterLogicalInterfacePortIds()[0];
  PortID collectorPortId = masterLogicalInterfacePortIds()[1];

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    config.mirrorOnDropReports()->push_back(
        makeMirrorOnDropReport("mod-default-route-drop"));
    auto* asic =
        checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &config,
        folly::CIDRNetwork(folly::IPAddress(kCollectorIp_), 128),
        cfg::ToCpuAction::TRAP);
    applyNewConfig(config);
    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
    waitForStateUpdates(getSw());
  };

  auto verify = [&]() {
    utility::SwSwitchPacketSnooper snooper(
        getSw(),
        "mod-snooper",
        std::nullopt,
        std::nullopt,
        std::nullopt,
        utility::packetSnooperReceivePacketType::PACKET_TYPE_ALL,
        mirrorOnDropRxPacketFilter());
    snooper.ignoreUnclaimedRxPkts();
    sendPackets(1, injectionPortId, kDropDestIp);

    WITH_RETRIES_N(5, {
      auto frameRx = waitForMirrorOnDropPacket(snooper, 1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
      if (frameRx.has_value()) {
        XLOG(INFO) << "Captured MirrorOnDrop packet for default-route drop:\n"
                   << PktUtil::hexDump(frameRx->get());
        validateMirrorOnDropPacket(
            frameRx->get(),
            injectionPortId,
            getDefaultRouteDropReasons(),
            kDropDestIp);
      }
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

void AgentMirrorOnDropStatelessTest::testAclDrop() {
  PortID injectionPortId = masterLogicalInterfacePortIds()[0];
  PortID collectorPortId = masterLogicalInterfacePortIds()[1];
  const folly::IPAddressV6 kAclDropDestIp{"2401:db00:e112:9100:1006::1"};

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    config.mirrorOnDropReports()->push_back(
        makeMirrorOnDropReport("mod-acl-drop"));

    cfg::AclEntry aclEntry;
    aclEntry.name() = "acl-drop-by-dstip";
    aclEntry.dstIp() = fmt::format("{}/128", kAclDropDestIp.str());
    aclEntry.actionType() = cfg::AclActionType::DENY;
    utility::addAclEntry(&config, aclEntry, utility::kDefaultAclTable());

    auto* asic =
        checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &config,
        folly::CIDRNetwork(folly::IPAddress(kCollectorIp_), 128),
        cfg::ToCpuAction::TRAP);
    applyNewConfig(config);
    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
    waitForStateUpdates(getSw());
  };

  auto verify = [&]() {
    utility::SwSwitchPacketSnooper snooper(
        getSw(),
        "mod-acl-snooper",
        std::nullopt,
        std::nullopt,
        std::nullopt,
        utility::packetSnooperReceivePacketType::PACKET_TYPE_ALL,
        mirrorOnDropRxPacketFilter());
    snooper.ignoreUnclaimedRxPkts();
    auto pkt = sendPackets(1, injectionPortId, kAclDropDestIp);
    XLOG(INFO) << "Sent packet to trigger ACL drop:\n"
               << PktUtil::hexDump(pkt->buf());

    WITH_RETRIES_N(5, {
      auto frameRx = waitForMirrorOnDropPacket(snooper, 1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
      if (frameRx.has_value()) {
        XLOG(INFO) << "Captured MirrorOnDrop packet for ACL drop:\n"
                   << PktUtil::hexDump(frameRx->get());
        validateMirrorOnDropPacket(
            frameRx->get(),
            injectionPortId,
            getAclDropReasons(),
            kAclDropDestIp);
      }
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

void AgentMirrorOnDropStatelessTest::testMmuDrop() {
  PortID injectionPortId = masterLogicalInterfacePortIds()[0];
  PortID collectorPortId = masterLogicalInterfacePortIds()[1];
  PortID txOffPortId = masterLogicalInterfacePortIds()[2];
  // constexpr (not just const) so Infer doesn't flag the lambda capture as a
  // dead store: the value is purely compile-time and needs no storage.
  constexpr int kPriority = 2;

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    config.mirrorOnDropReports()->push_back(
        makeMirrorOnDropReport("mod-mmu-drop"));
    configureMmuDropBuffers(config, injectionPortId, kPriority);
    auto* asic =
        checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &config,
        folly::CIDRNetwork(folly::IPAddress(kCollectorIp_), 128),
        cfg::ToCpuAction::TRAP);
    applyNewConfig(config);
    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
    setupEcmpTraffic(
        txOffPortId,
        kDropDestIp,
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState()));
    waitForStateUpdates(getSw());
  };

  auto verify = [&]() {
    utility::setCreditWatchdogAndPortTx(getAgentEnsemble(), txOffPortId, false);
    // Ensure TX is restored even if an assertion fails or an exception is
    // thrown below. Otherwise the port stays in TX-off across the warmboot
    // verify iteration and across subsequent tests.
    SCOPE_EXIT {
      utility::setCreditWatchdogAndPortTx(
          getAgentEnsemble(), txOffPortId, true);
    };

    utility::SwSwitchPacketSnooper snooper(
        getSw(),
        "mod-mmu-snooper",
        std::nullopt,
        std::nullopt,
        std::nullopt,
        utility::packetSnooperReceivePacketType::PACKET_TYPE_ALL,
        mirrorOnDropRxPacketFilter());
    snooper.ignoreUnclaimedRxPkts();

    // Snapshot inCongestionDiscards before sending. The counter is monotonic
    // across the warmboot iteration, so checking > 0 alone would pass on the
    // post-warmboot pass even if no new drops occurred.
    const int64_t discardsBefore =
        *getLatestPortStats(injectionPortId).inCongestionDiscards_();

    sendPackets(10000, injectionPortId, kDropDestIp, kPriority);

    WITH_RETRIES({
      auto portStats = getLatestPortStats(injectionPortId);
      EXPECT_EVENTUALLY_GT(*portStats.inCongestionDiscards_(), discardsBefore);
    });

    WITH_RETRIES_N(5, {
      auto frameRx = waitForMirrorOnDropPacket(snooper, 1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
      if (frameRx.has_value()) {
        XLOG(INFO) << "Captured MirrorOnDrop packet for MMU drop:\n"
                   << PktUtil::hexDump(frameRx->get());
        auto fields = parseMirrorOnDropPacket(frameRx->get());
        if (auto puntCodeIt = fields.asicMetadata.find("puntCode");
            puntCodeIt != fields.asicMetadata.end()) {
          XLOG(INFO) << fmt::format(
              "MMU punt code on wire: {} (0x{:x}), parsed ingressDrop={} "
              "egressDrop={}, expected ingress=0 egress=0x12",
              puntCodeIt->second,
              puntCodeIt->second,
              fields.dropReasonIngress,
              fields.dropReasonEgress);
        }
        validateMirrorOnDropPacket(
            frameRx->get(), injectionPortId, getMmuDropReasons());
      }
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

void AgentMirrorOnDropStatelessTest::testWarmbootToggleSampling(
    bool enablePostWarmboot,
    int samplingRate) {
  const std::string kReportName = "mod-wb-toggle-sampling";

  auto addMod = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    config.mirrorOnDropReports()->push_back(
        makeMirrorOnDropReport(kReportName, samplingRate));
    applyNewConfig(config);
    waitForStateUpdates(getSw());
  };

  auto removeMod = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    config.mirrorOnDropReports()->clear();
    applyNewConfig(config);
    waitForStateUpdates(getSw());
  };

  auto verifyPresent = [&]() {
    auto state = getProgrammedState();
    auto reports = state->getMirrorOnDropReports();
    ASSERT_NE(reports, nullptr);
    auto report = reports->getNodeIf(kReportName);
    ASSERT_NE(report, nullptr);
    EXPECT_EQ(report->getSamplingRate(), samplingRate);
  };

  auto verifyAbsent = [&]() {
    auto state = getProgrammedState();
    auto reports = state->getMirrorOnDropReports();
    EXPECT_TRUE(reports == nullptr || reports->numNodes() == 0);
  };

  if (enablePostWarmboot) {
    verifyAcrossWarmBoots([]() {}, verifyAbsent, addMod, verifyPresent);
  } else {
    verifyAcrossWarmBoots(addMod, verifyPresent, removeMod, verifyAbsent);
  }
}

void AgentMirrorOnDropStatelessTest::testWarmbootEnableSampling(
    int samplingRate) {
  testWarmbootToggleSampling(/*enablePostWarmboot=*/true, samplingRate);
}

void AgentMirrorOnDropStatelessTest::testWarmbootDisableSampling(
    int samplingRate) {
  testWarmbootToggleSampling(/*enablePostWarmboot=*/false, samplingRate);
}

} // namespace facebook::fboss
