// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <boost/container/flat_set.hpp>
#include <folly/Conv.h>
#include <folly/IPAddressV6.h>

#include <memory>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/PortStatsTestUtils.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

namespace {

const facebook::fboss::Label kTopLabel{1101};

} // namespace

namespace facebook::fboss {

class AgentMPLSMidpointTest : public AgentHwTest {
 protected:
  using EcmpSetupHelper =
      utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV6>;

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /* interfaceHasSubnet */);
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::MPLS_MIDPOINT};
  }

  PortID egressPort() const {
    return masterLogicalInterfacePortIds()[0];
  }

  PortID ingressPort() const {
    return masterLogicalInterfacePortIds()[1];
  }

  std::unique_ptr<EcmpSetupHelper> setupECMPHelper() const {
    return std::make_unique<EcmpSetupHelper>(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        kTopLabel,
        LabelForwardingAction::LabelForwardingType::PUSH);
  }

  void configureStaticMplsPushRoute(cfg::SwitchConfig& config) const {
    config.staticMplsRoutesWithNhops()->resize(1);
    auto& route = config.staticMplsRoutesWithNhops()[0];
    route.ingressLabel() = kTopLabel.value();

    auto helper = setupECMPHelper();
    auto nhop = helper->nhop(PortDescriptor(egressPort()));

    NextHopThrift nextHop;
    nextHop.address() = network::toBinaryAddress(nhop.ip);
    nextHop.address()->ifName() = folly::to<std::string>("fboss", nhop.intf);
    nextHop.mplsAction() = nhop.action.toThrift();
    route.nexthops()->push_back(nextHop);
  }

  void resolveNextHop() {
    applyNewState(
        [this](const std::shared_ptr<SwitchState>& state) {
          auto helper = EcmpSetupHelper(
              state,
              getSw()->needL2EntryForNeighbor(),
              kTopLabel,
              LabelForwardingAction::LabelForwardingType::PUSH);
          return helper.resolveNextHops(
              state,
              boost::container::flat_set<PortDescriptor>{
                  PortDescriptor(egressPort())});
        },
        "resolve midpoint MPLS nexthop");
  }

  std::unique_ptr<TxPacket> makeMplsIngressPacket() const {
    auto vlan = getVlanIDForTx();
    CHECK(vlan.has_value());

    auto frame = utility::getEthFrame(
        utility::kLocalCpuMac(),
        utility::kLocalCpuMac(),
        {MPLSHdr::Label{
            static_cast<uint32_t>(kTopLabel.value()), 0, true, 128}},
        folly::IPAddressV6{"1001::1"},
        folly::IPAddressV6{"2001::1"},
        10000,
        20000,
        *vlan);

    return frame.getTxPacket(
        [sw = getSw()](uint32_t size) { return sw->allocatePacket(size); });
  }

  void sendMplsIngressPacket() {
    getAgentEnsemble()->ensureSendPacketOutOfPort(
        makeMplsIngressPacket(), ingressPort());
  }

  void verifyMplsPushForwarding() {
    auto outPktsBefore =
        utility::getPortOutPkts(getLatestPortStats(egressPort()));

    sendMplsIngressPacket();

    WITH_RETRIES({
      auto outPktsAfter =
          utility::getPortOutPkts(getLatestPortStats(egressPort()));
      EXPECT_EVENTUALLY_EQ(1, outPktsAfter - outPktsBefore);
    });
  }
};

TEST_F(AgentMPLSMidpointTest, StaticMplsRoutePush) {
  auto setup = [this]() {
    auto config = initialConfig(*getAgentEnsemble());
    configureStaticMplsPushRoute(config);
    applyNewConfig(config);
    resolveNextHop();
  };

  auto verify = [this]() { verifyMplsPushForwarding(); };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
