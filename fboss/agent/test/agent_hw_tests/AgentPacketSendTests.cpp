// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace facebook::fboss {

class AgentPacketSendTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::CPU_RX_TX};
  }
};

TEST_F(AgentPacketSendTest, LldpToFrontPanelOutOfPort) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto portStatsBefore =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto payLoadSize = 256;
    auto txPacket = utility::makeEthTxPacket(
        getSw(),
        vlanId,
        srcMac,
        folly::MacAddress("01:80:c2:00:00:0e"),
        facebook::fboss::ETHERTYPE::ETHERTYPE_LLDP,
        std::vector<uint8_t>(payLoadSize, 0xff));
    // vlan tag should be removed
    auto pktLengthSent = EthHdr::SIZE + payLoadSize;
    getSw()->sendPacketOutOfPortAsync(
        std::move(txPacket), masterLogicalInterfacePortIds()[0], std::nullopt);
    WITH_RETRIES({
      auto portStatsAfter =
          getLatestPortStats(masterLogicalInterfacePortIds()[0]);
      XLOG(DBG2) << "Lldp Packet:"
                 << " before pkts:" << *portStatsBefore.outMulticastPkts_()
                 << ", after pkts:" << *portStatsAfter.outMulticastPkts_()
                 << ", before bytes:" << *portStatsBefore.outBytes_()
                 << ", after bytes:" << *portStatsAfter.outBytes_();
      // GE as some platforms include FCS in outBytes count
      EXPECT_EVENTUALLY_GE(
          *portStatsAfter.outBytes_() - *portStatsBefore.outBytes_(),
          pktLengthSent);
      auto portSwitchId =
          scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
      auto asicType = getAsic(portSwitchId).getAsicType();
      if (asicType != cfg::AsicType::ASIC_TYPE_EBRO &&
          asicType != cfg::AsicType::ASIC_TYPE_YUBA) {
        EXPECT_EVENTUALLY_EQ(
            1,
            *portStatsAfter.outMulticastPkts_() -
                *portStatsBefore.outMulticastPkts_());
      }
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
