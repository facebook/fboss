/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/if/gen-cpp2/AgentHwTestCtrl.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

class AgentHwCbfcTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::CREDIT_BASED_FLOW_CONTROL};
  }

 protected:
  static constexpr auto kCbfcConfigName = "cbfc_default";
  // The two lossless classes SUSWs run today: pg2 rdma, pg6 monitoring.
  static constexpr int16_t kRdmaVc = 2;
  static constexpr int16_t kMonitoringVc = 6;
  static constexpr int64_t kRdmaReservedCredits = 36;
  static constexpr int64_t kMonitoringReservedCredits = 12;
  static constexpr int64_t kSenderCreditLimit = 8192;

  utility::PortVcInfo getPortVcInfo(PortID portId) {
    auto switchId =
        getAgentEnsemble()->scopeResolver().scope(portId).switchId();
    auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);
    utility::PortVcInfo vcInfo;
    client->sync_getPortVcInfo(vcInfo, portId);
    return vcInfo;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentHwTest::initialConfig(ensemble);
    if (ensemble.getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::CBFC)) {
      addCbfcConfig(cfg, ensemble);
    }
    return cfg;
  }

  void addCbfcConfig(cfg::SwitchConfig& cfg, const AgentEnsemble& ensemble)
      const {
    cfg::PortVcConfig rdma;
    rdma.id() = kRdmaVc;
    rdma.name() = "rdma";
    rdma.senderEnable() = true;
    rdma.receiverEnable() = true;
    rdma.reservedCreditSize() = kRdmaReservedCredits;

    cfg::PortVcConfig monitoring;
    monitoring.id() = kMonitoringVc;
    monitoring.name() = "monitoring";
    monitoring.senderEnable() = true;
    monitoring.receiverEnable() = true;
    monitoring.reservedCreditSize() = kMonitoringReservedCredits;

    cfg::CbfcConfig cbfc;
    cbfc.virtualChannels() = {rdma, monitoring};
    cbfc.senderCreditLimit() = kSenderCreditLimit;
    cfg.cbfcConfigs() = {{kCbfcConfigName, cbfc}};

    for (const auto& portId : ensemble.masterLogicalInterfacePortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->cbfcConfigName() = kCbfcConfigName;
    }
  }

  void verifyCbfcProgrammed();
};

void AgentHwCbfcTest::verifyCbfcProgrammed() {
  auto setup = []() {};
  auto verify = [&]() {
    auto state = getProgrammedState();
    auto portStats = getLatestPortStats(masterLogicalInterfacePortIds());
    for (const auto& portId : masterLogicalInterfacePortIds()) {
      auto port = state->getPorts()->getNodeIf(portId);
      ASSERT_NE(port, nullptr);
      ASSERT_TRUE(port->getCbfcConfigName().has_value());
      EXPECT_EQ(*port->getCbfcConfigName(), kCbfcConfigName);
      EXPECT_EQ(port->getCbfcSenderCreditLimit(), kSenderCreditLimit);

      // Read the virtual channels back from hardware. This confirms the SAI
      // objects still exist, are bound to the port, and carry the last-applied
      // values after the warm boot -- not just that the SwitchState intent
      // survived.
      auto vcInfo = getPortVcInfo(portId);

      // Logged first, before any assertion that could abort the loop. These
      // are read-only and derived by hardware from the MMU carving; capturing
      // them is the main value of a run. Unset means the SDK refused the read,
      // which is a result rather than a failure.
      auto show = [](const auto& opt) {
        return opt.has_value() ? std::to_string(*opt)
                               : std::string("<not readable>");
      };
      XLOG(DBG2) << "Port " << portId << " CBFC native: creditSize="
                 << show(vcInfo.receiverNativeCreditSize()) << " pktOverhead="
                 << show(vcInfo.receiverNativePacketOverhead())
                 << " totalCredits="
                 << show(vcInfo.receiverNativeTotalCredits());
      for (const auto& vc : *vcInfo.virtualChannels()) {
        XLOG(DBG2) << "Port " << portId << " VC "
                   << static_cast<int>(*vc.index()) << " nativeCreditLimit="
                   << show(vc.receiverNativeCreditLimit());
      }

      ASSERT_EQ(vcInfo.virtualChannels()->size(), 2);

      std::map<int, utility::VcInfo> byIndex;
      for (const auto& vc : *vcInfo.virtualChannels()) {
        byIndex[*vc.index()] = vc;
      }
      ASSERT_TRUE(byIndex.contains(kRdmaVc));
      ASSERT_TRUE(byIndex.contains(kMonitoringVc));

      for (const auto& [index, vc] : byIndex) {
        EXPECT_NE(*vc.vcId(), 0);
        EXPECT_TRUE(*vc.senderEnable());
        EXPECT_TRUE(*vc.receiverEnable());
        EXPECT_TRUE(vc.reservedCreditSize().has_value());
        EXPECT_NE(*vc.creditProfileId(), 0);
      }
      EXPECT_EQ(*byIndex[kRdmaVc].reservedCreditSize(), kRdmaReservedCredits);
      EXPECT_EQ(
          *byIndex[kMonitoringVc].reservedCreditSize(),
          kMonitoringReservedCredits);

      // Distinct reservations must not collapse onto one profile object.
      EXPECT_NE(
          *byIndex[kRdmaVc].creditProfileId(),
          *byIndex[kMonitoringVc].creditProfileId());

      // Counters read 0 without induced traffic, so presence is the assertion.
      // Everything is logged before the first EXPECT so a run still yields
      // readings even when one fails.
      const auto& stats = portStats.at(portId);
      XLOG(DBG2) << "Port " << portId << " CBFC port counters:"
                 << " ccUpdateTx=" << show(stats.cbfcCcUpdateTx_())
                 << " cfUpdateTx=" << show(stats.cbfcCfUpdateTx_())
                 << " cfUpdateRx=" << show(stats.cbfcCfUpdateRx_());
      auto showVcMap = [&](folly::StringPiece name, const auto& vcMap) {
        for (const auto& [vcIndex, value] : vcMap) {
          XLOG(DBG2) << "Port " << portId << " VC " << vcIndex << " " << name
                     << "=" << value;
        }
        if (vcMap.empty()) {
          XLOG(DBG2) << "Port " << portId << " " << name << " <empty>";
        }
      };
      showVcMap("senderCreditsConsumed", *stats.cbfcVcSenderCreditsConsumed_());
      showVcMap("senderCreditsFreed", *stats.cbfcVcSenderCreditsFreed_());
      showVcMap(
          "receiverCreditsConsumed", *stats.cbfcVcReceiverCreditsConsumed_());
      showVcMap("receiverCreditsFreed", *stats.cbfcVcReceiverCreditsFreed_());

      EXPECT_TRUE(stats.cbfcCcUpdateTx_().has_value());
      EXPECT_TRUE(stats.cbfcCfUpdateTx_().has_value());
      EXPECT_TRUE(stats.cbfcCfUpdateRx_().has_value());
      // Only the receiver-consumed map is asserted. The other three virtual
      // channel counters are Tomahawk Ultra 1 B0 stepping only and are not
      // requested (see SaiVirtualChannelTraits::cbfcVcStats), so their maps
      // stay empty on A0 -- the logging above is how we find out if that
      // changes.
      EXPECT_TRUE(stats.cbfcVcReceiverCreditsConsumed_()->contains(kRdmaVc));
      EXPECT_TRUE(
          stats.cbfcVcReceiverCreditsConsumed_()->contains(kMonitoringVc));
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwCbfcTest, verifyCbfcConfig) {
  verifyCbfcProgrammed();
}

} // namespace facebook::fboss
