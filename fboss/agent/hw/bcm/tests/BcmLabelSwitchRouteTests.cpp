// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmLabelSwitchingUtils.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmMplsTestUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

using namespace ::testing;
namespace {
using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;
const facebook::fboss::LabelForwardingEntry::Label kTopLabel{1101};
} // namespace

namespace facebook::fboss {

template <typename AddrT>
class BcmLabelSwitchRouteTest : public BcmLinkStateDependentTests {
 public:
  using EcmpSetupHelper = utility::MplsEcmpSetupTargetedPorts<AddrT>;
  using EcmpNextHop = utility::EcmpMplsNextHop<AddrT>;
  static auto constexpr kWidth = 4;

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports;
    for (auto i = 0; i < kWidth; i++) {
      ports.push_back(masterLogicalPortIds()[i]);
    }
    return utility::onePortPerVlanConfig(
        getHwSwitch(), ports, cfg::PortLoopbackMode::MAC);
  }

  void setupECMPHelper(
      LabelForwardingEntry::Label topLabel,
      LabelForwardingAction::LabelForwardingType labelAction) {
    helper_ = std::make_unique<EcmpSetupHelper>(
        getProgrammedState(), topLabel, labelAction);
  }

  void setupECMPForwarding() {
    boost::container::flat_set<PortDescriptor> ports;
    for (auto i = 0; i < kWidth; i++) {
      ports.insert(PortDescriptor(masterLogicalPortIds()[i]));
    }
    helper_->setupECMPForwarding(getProgrammedState(), std::move(ports));
  }

  EcmpNextHop getNextHop(int i) {
    CHECK_LT(i, kWidth);
    return helper_->nhop(PortDescriptor(masterLogicalPortIds()[i]));
  }

  void setupLabelSwitchActionWithOneNextHop(
      LabelForwardingAction::LabelForwardingType action) {
    setupECMPHelper(kTopLabel, action);
    LabelNextHopSet nhops;
    auto testNhop = getNextHop(0);
    LabelNextHop nexthop{testNhop.ip,
                         InterfaceID(utility::kBaseVlanId),
                         ECMP_WEIGHT,
                         testNhop.action};
    nhops.insert(nexthop);
    applyNewState(helper_->resolveNextHop(getProgrammedState(), testNhop));
    auto newState = getProgrammedState()->clone();
    newState->getLabelForwardingInformationBase()->programLabel(
        &newState,
        kTopLabel,
        ClientID(0),
        AdminDistance::DIRECTLY_CONNECTED,
        std::move(nhops));
    applyNewState(newState);
  }

  void setupLabelSwitchActionWithMultiNextHop(
      LabelForwardingAction::LabelForwardingType action) {
    setupECMPHelper(kTopLabel, action);
    LabelNextHopSet nhops;
    for (auto i = 0; i < kWidth; i++) {
      auto testNhop = getNextHop(i);
      applyNewState(helper_->resolveNextHop(getProgrammedState(), testNhop));
      nhops.insert(LabelNextHop{testNhop.ip,
                                InterfaceID(utility::kBaseVlanId + i),
                                NextHopWeight(1), // TODO - support ECMP_WEIGHT
                                testNhop.action});
    }
    auto newState = getProgrammedState()->clone();
    newState->getLabelForwardingInformationBase()->programLabel(
        &newState,
        kTopLabel,
        ClientID(0),
        AdminDistance::DIRECTLY_CONNECTED,
        std::move(nhops));
    applyNewState(newState);
  }

  void verifyLabelSwitchAction(
      LabelForwardingAction::LabelForwardingType action) {
    setupECMPHelper(kTopLabel, action);
    bcm_mpls_tunnel_switch_t info;
    bcm_mpls_tunnel_switch_t_init(&info);
    info.label = kTopLabel;
    info.port = BCM_GPORT_INVALID;
    auto rv = bcm_mpls_tunnel_switch_get(getHwSwitch()->getUnit(), &info);
    ASSERT_EQ(rv, 0);
    EXPECT_EQ(
        info.action,
        utility::getLabelSwitchAction(
            LabelNextHopEntry::Action::NEXTHOPS, action));
    auto nexthop = this->getNextHop(0);
    bcm_mac_t mac;
    macToBcm(nexthop.mac, &mac);
    bcm_if_t labelActionIntf;
    switch (action) {
      case LabelForwardingAction::LabelForwardingType::SWAP:
        utility::verifyLabeledEgress(
            info.egress_if, nexthop.action.swapWith().value());
        labelActionIntf = getHwSwitch()
                              ->getIntfTable()
                              ->getBcmIntf(InterfaceID(utility::kBaseVlanId))
                              ->getBcmIfId();
        break;

      case LabelForwardingAction::LabelForwardingType::PUSH: {
        auto stack = nexthop.action.pushStack().value();
        auto tunnelStack =
            LabelForwardingAction::LabelStack{stack.begin() + 1, stack.end()};
        utility::verifyTunneledEgress(info.egress_if, stack[0], tunnelStack);
        auto tunnel = getHwSwitch()
                          ->getIntfTable()
                          ->getBcmIntf(InterfaceID(utility::kBaseVlanId))
                          ->getBcmLabeledTunnel(tunnelStack);
        labelActionIntf = tunnel->getTunnelInterface();

      } break;
      case LabelForwardingAction::LabelForwardingType::PHP: {
        labelActionIntf = getHwSwitch()
                              ->getIntfTable()
                              ->getBcmIntf(InterfaceID(utility::kBaseVlanId))
                              ->getBcmIfId();
      } break;
      case LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP:
      case LabelForwardingAction::LabelForwardingType::NOOP:
        break;
    }

    if (action != LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP &&
        action != LabelForwardingAction::LabelForwardingType::NOOP) {
      utility::verifyEgress(
          info.egress_if,
          getHwSwitch()->getPortTable()->getBcmPortId(
              nexthop.portDesc.phyPortID()),
          mac,
          labelActionIntf);
    }
  }

  void verifyLabeledMultiPath(
      bcm_if_t egress_if,
      LabelForwardingAction::LabelForwardingType action) {
    setupECMPHelper(kTopLabel, action);
    std::map<
        bcm_port_t,
        std::pair<bcm_mpls_label_t, LabelForwardingAction::LabelStack>>
        stacks;
    for (auto i = 0; i < kWidth; i++) {
      auto port = getHwSwitch()->getPortTable()->getBcmPortId(
          masterLogicalPortIds()[i]);
      auto nexthop = getNextHop(i);
      if (!nexthop.action.pushStack()) {
        stacks.emplace(
            port,
            std::make_pair(
                nexthop.action.swapWith().value(),
                LabelForwardingAction::LabelStack()));
      } else {
        stacks.emplace(
            port,
            utility::getEgressLabelAndTunnelStackFromPushStack(
                nexthop.action.pushStack().value()));
      }
    }
    utility::verifyLabeledMultiPathEgress(0, kWidth, egress_if, stacks);
  }

  void verifyUnlabeledMultiPath(
      bcm_if_t egress_if,
      LabelForwardingAction::LabelForwardingType action) {
    setupECMPHelper(kTopLabel, action);

    std::vector<bcm_port_t> ports{kWidth};
    std::vector<bcm_mac_t> macs{kWidth};
    std::vector<bcm_if_t> intfs{kWidth};

    for (auto i = 0; i < kWidth; i++) {
      auto nexthop = getNextHop(i);
      ports[i] = getHwSwitch()->getPortTable()->getBcmPortId(
          nexthop.portDesc.phyPortID());
      macToBcm(nexthop.mac, &macs[i]);
      if (nexthop.action.type() ==
          LabelForwardingAction::LabelForwardingType::PUSH) {
        auto stack = nexthop.action.pushStack().value();

        auto tunnelStack =
            LabelForwardingAction::LabelStack{stack.begin() + 1, stack.end()};
        intfs[i] = getHwSwitch()
                       ->getIntfTable()
                       ->getBcmIntf(InterfaceID(utility::kBaseVlanId))
                       ->getBcmLabeledTunnel(tunnelStack)
                       ->getTunnelInterface();
      } else {
        intfs[i] = getHwSwitch()
                       ->getIntfTable()
                       ->getBcmIntf(InterfaceID(utility::kBaseVlanId))
                       ->getBcmIfId();
      }
    }
    utility::verifyMultiPathEgress(egress_if, ports, macs, intfs);
  }

  void verifyMultiPathLabelSwitchAction(
      LabelForwardingAction::LabelForwardingType action) {
    setupECMPHelper(kTopLabel, action);
    bcm_mpls_tunnel_switch_t info;
    bcm_mpls_tunnel_switch_t_init(&info);
    info.label = kTopLabel;
    info.port = BCM_GPORT_INVALID;
    auto rv = bcm_mpls_tunnel_switch_get(getHwSwitch()->getUnit(), &info);
    ASSERT_EQ(rv, 0);
    EXPECT_EQ(
        info.action,
        utility::getLabelSwitchAction(
            LabelNextHopEntry::Action::NEXTHOPS, action));
    auto nexthop = this->getNextHop(0);
    switch (action) {
      case LabelForwardingAction::LabelForwardingType::SWAP:
      case LabelForwardingAction::LabelForwardingType::PUSH: {
        verifyLabeledMultiPath(info.egress_if, action);
      } break;
      case LabelForwardingAction::LabelForwardingType::PHP:
      case LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP:
      case LabelForwardingAction::LabelForwardingType::NOOP:
        break;
    }
  }

 private:
  std::unique_ptr<EcmpSetupHelper> helper_;
};

TYPED_TEST_CASE(BcmLabelSwitchRouteTest, TestTypes);

TYPED_TEST(BcmLabelSwitchRouteTest, Push) {
  auto setup = [=]() {
    this->setupLabelSwitchActionWithOneNextHop(
        LabelForwardingAction::LabelForwardingType::PUSH);
  };
  auto verify = [=]() {
    this->verifyLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::PUSH);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmLabelSwitchRouteTest, Swap) {
  auto setup = [=]() {
    this->setupLabelSwitchActionWithOneNextHop(
        LabelForwardingAction::LabelForwardingType::SWAP);
  };
  auto verify = [=]() {
    this->verifyLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::SWAP);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmLabelSwitchRouteTest, EcmpPush) {
  if (!this->getPlatform()->isMultiPathLabelSwitchActionSupported()) {
    return;
  }
  auto setup = [=]() {
    this->setupLabelSwitchActionWithMultiNextHop(
        LabelForwardingAction::LabelForwardingType::PUSH);
  };
  auto verify = [=]() {
    this->verifyMultiPathLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::PUSH);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmLabelSwitchRouteTest, EcmpSwap) {
  if (!this->getPlatform()->isMultiPathLabelSwitchActionSupported()) {
    return;
  }
  auto setup = [=]() {
    this->setupLabelSwitchActionWithMultiNextHop(
        LabelForwardingAction::LabelForwardingType::SWAP);
  };
  auto verify = [=]() {
    this->verifyMultiPathLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::SWAP);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmLabelSwitchRouteTest, Php) {
  auto setup = [=]() {
    this->setupLabelSwitchActionWithOneNextHop(
        LabelForwardingAction::LabelForwardingType::PHP);
  };
  auto verify = [=]() {
    this->verifyLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::PHP);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmLabelSwitchRouteTest, EcmpPhp) {
  auto setup = [=]() {
    this->setupLabelSwitchActionWithMultiNextHop(
        LabelForwardingAction::LabelForwardingType::PHP);
  };
  auto verify = [=]() {
    this->verifyMultiPathLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::PHP);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmLabelSwitchRouteTest, Pop) {
  auto setup = [=]() {
    this->setupLabelSwitchActionWithOneNextHop(
        LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
  };
  auto verify = [=]() {
    this->verifyLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
