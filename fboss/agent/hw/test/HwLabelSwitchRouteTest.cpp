// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestMplsUtils.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/mpls_types.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/AddressUtil.h"

using namespace ::testing;
namespace {
using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;
const facebook::fboss::Label kTopLabel{1101};
} // namespace

namespace facebook::fboss {

template <typename AddrT>
class HwLabelSwitchRouteTest : public HwLinkStateDependentTest {
 public:
  using EcmpSetupHelper = utility::MplsEcmpSetupTargetedPorts<AddrT>;
  using EcmpNextHop = utility::EcmpMplsNextHop<AddrT>;
  static auto constexpr kWidth = 4;

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports;
    for (auto i = 0; i < kWidth; i++) {
      ports.push_back(masterLogicalPortIds()[i]);
    }
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), ports, getAsic()->desiredLoopbackModes());
  }

  void configureStaticMplsRoute(
      cfg::SwitchConfig& config,
      LabelForwardingAction::LabelForwardingType labelAction) {
    config.staticMplsRoutesWithNhops()->resize(1);
    auto& route = config.staticMplsRoutesWithNhops()[0];
    route.ingressLabel() = kTopLabel.value();

    auto helper = setupECMPHelper(kTopLabel, labelAction);

    if (labelAction ==
        LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP) {
      NextHopThrift nexthop;
      nexthop.address() = network::toBinaryAddress(folly::IPAddress("::"));
      MplsAction action;
      action.action() = labelAction;
      nexthop.mplsAction() = action;
      route.nexthops()->push_back(nexthop);
      return;
    }

    for (auto i = 0; i < kWidth; i++) {
      NextHopThrift nexthop;
      auto ecmpHelperNhop = getNextHop(helper.get(), i);
      nexthop.address() = network::toBinaryAddress(ecmpHelperNhop.ip);
      nexthop.mplsAction() = ecmpHelperNhop.action.toThrift();
      nexthop.address()->ifName() =
          folly::to<std::string>("fboss", ecmpHelperNhop.intf);
      route.nexthops()->push_back(nexthop);
    }
  }

  void resolveNeighbors() {
    utility::EcmpSetupTargetedPorts<AddrT> helper(getProgrammedState());
    boost::container::flat_set<PortDescriptor> ports;
    for (auto i = 0; i < kWidth; i++) {
      ports.emplace(masterLogicalPortIds()[i]);
    }
    applyNewState(helper.resolveNextHops(getProgrammedState(), ports));
  }

  std::unique_ptr<EcmpSetupHelper> setupECMPHelper(
      Label topLabel,
      LabelForwardingAction::LabelForwardingType labelAction) {
    return std::make_unique<EcmpSetupHelper>(
        getProgrammedState(), topLabel, labelAction);
  }

  void setupECMPForwarding(EcmpSetupHelper* helper) {
    boost::container::flat_set<PortDescriptor> ports;
    for (auto i = 0; i < kWidth; i++) {
      ports.insert(PortDescriptor(masterLogicalPortIds()[i]));
    }
    helper->setupECMPForwarding(getProgrammedState(), std::move(ports));
  }

  EcmpNextHop getNextHop(EcmpSetupHelper* helper, int i) {
    CHECK_LT(i, kWidth);
    return helper->nhop(PortDescriptor(masterLogicalPortIds()[i]));
  }

  void setupLabelRibRoute(LabelNextHopSet nhops) {
    auto updater = getRouteUpdater();
    MplsRoute route;
    route.topLabel() = kTopLabel.value();
    route.nextHops() = util::fromRouteNextHopSet(nhops);
    updater->addRoute(ClientID(0), std::move(route));
    updater->program();
  }

  void setupLabelSwitchActionWithOneNextHop(
      LabelForwardingAction::LabelForwardingType action) {
    auto helper = setupECMPHelper(kTopLabel, action);
    LabelNextHopSet nhops;
    auto testNhop = getNextHop(helper.get(), 0);
    LabelNextHop nexthop{
        testNhop.ip,
        InterfaceID(utility::kBaseVlanId),
        ECMP_WEIGHT,
        testNhop.action};
    nhops.insert(nexthop);
    applyNewState(helper->resolveNextHop(getProgrammedState(), testNhop));

    setupLabelRibRoute(nhops);
  }

  void setupLabelSwitchActionWithMultiNextHop(
      LabelForwardingAction::LabelForwardingType action,
      int width = kWidth) {
    auto helper = setupECMPHelper(kTopLabel, action);
    LabelNextHopSet nhops;
    for (auto i = 0; i < width; i++) {
      auto testNhop = getNextHop(helper.get(), i);
      applyNewState(helper->resolveNextHop(getProgrammedState(), testNhop));
      nhops.insert(LabelNextHop{
          testNhop.ip,
          InterfaceID(utility::kBaseVlanId + i),
          NextHopWeight(1), // TODO - support ECMP_WEIGHT
          testNhop.action});
    }
    setupLabelRibRoute(nhops);
  }

  void verifyLabelSwitchAction(
      LabelForwardingAction::LabelForwardingType action) {
    auto helper = setupECMPHelper(kTopLabel, action);
    utility::verifyLabelSwitchAction(
        getHwSwitch(), kTopLabel, action, this->getNextHop(helper.get(), 0));
  }

  void verifyMultiPathLabelSwitchAction(
      LabelForwardingAction::LabelForwardingType action) {
    auto helper = setupECMPHelper(kTopLabel, action);
    std::vector<EcmpNextHop> nexthops;
    for (auto i = 0; i < kWidth; i++) {
      nexthops.push_back(this->getNextHop(helper.get(), i));
    }
    utility::verifyMultiPathLabelSwitchAction(
        getHwSwitch(), kTopLabel, action, nexthops);
  }
};

TYPED_TEST_SUITE(HwLabelSwitchRouteTest, TestTypes);

TYPED_TEST(HwLabelSwitchRouteTest, Push) {
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

TYPED_TEST(HwLabelSwitchRouteTest, Swap) {
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

TYPED_TEST(HwLabelSwitchRouteTest, EcmpPush) {
  if (!this->isSupported(HwAsic::Feature::MPLS_ECMP)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
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

TYPED_TEST(HwLabelSwitchRouteTest, EcmpSwap) {
  if (!this->isSupported(HwAsic::Feature::MPLS_ECMP)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
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

TYPED_TEST(HwLabelSwitchRouteTest, EcmpModify) {
  if (!this->isSupported(HwAsic::Feature::MPLS_ECMP)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [=]() {
    // set up initial ecmp with width 2
    this->setupLabelSwitchActionWithMultiNextHop(
        LabelForwardingAction::LabelForwardingType::SWAP, 2);

    // change to default ecmp with width 4
    this->setupLabelSwitchActionWithMultiNextHop(
        LabelForwardingAction::LabelForwardingType::SWAP);
  };
  auto verify = [=]() {
    this->verifyMultiPathLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::SWAP);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelSwitchRouteTest, Php) {
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

TYPED_TEST(HwLabelSwitchRouteTest, EcmpPhp) {
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

TYPED_TEST(HwLabelSwitchRouteTest, Pop) {
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

TYPED_TEST(HwLabelSwitchRouteTest, ConfigPush) {
  if (!this->isSupported(HwAsic::Feature::MPLS_ECMP)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [=]() {
    auto config = this->initialConfig();
    this->configureStaticMplsRoute(
        config, LabelForwardingAction::LabelForwardingType::PUSH);
    this->applyNewConfig(config);
    this->resolveNeighbors();
  };
  auto verify = [=]() {
    this->verifyMultiPathLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::PUSH);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelSwitchRouteTest, ConfigSwap) {
  if (!this->isSupported(HwAsic::Feature::MPLS_ECMP)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [=]() {
    auto config = this->initialConfig();
    this->configureStaticMplsRoute(
        config, LabelForwardingAction::LabelForwardingType::SWAP);
    this->applyNewConfig(config);
    this->resolveNeighbors();
  };
  auto verify = [=]() {
    this->verifyMultiPathLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::SWAP);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelSwitchRouteTest, ConfigPhp) {
  auto setup = [=]() {
    auto config = this->initialConfig();
    this->configureStaticMplsRoute(
        config, LabelForwardingAction::LabelForwardingType::PHP);
    this->applyNewConfig(config);
    this->resolveNeighbors();
  };
  auto verify = [=]() {
    this->verifyMultiPathLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::PHP);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelSwitchRouteTest, ConfigPop) {
  auto setup = [=]() {
    auto config = this->initialConfig();
    this->configureStaticMplsRoute(
        config, LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
    this->applyNewConfig(config);
    this->resolveNeighbors();
  };
  auto verify = [=]() {
    this->verifyLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelSwitchRouteTest, Swap2Php) {
  if (!this->isSupported(HwAsic::Feature::MPLS_ECMP)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [=]() {
    // set up initial ecmp with width 2
    this->setupLabelSwitchActionWithMultiNextHop(
        LabelForwardingAction::LabelForwardingType::SWAP);

    // change to default ecmp with width 4
    this->setupLabelSwitchActionWithMultiNextHop(
        LabelForwardingAction::LabelForwardingType::PHP);
  };
  auto verify = [=]() {
    this->verifyMultiPathLabelSwitchAction(
        LabelForwardingAction::LabelForwardingType::PHP);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
