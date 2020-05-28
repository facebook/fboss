// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestMplsUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

using namespace ::testing;
namespace {
using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;
const facebook::fboss::LabelForwardingEntry::Label kTopLabel{1101};
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
    utility::verifyLabelSwitchAction(
        getHwSwitch(), kTopLabel, action, this->getNextHop(0));
  }

  void verifyMultiPathLabelSwitchAction(
      LabelForwardingAction::LabelForwardingType action) {
    setupECMPHelper(kTopLabel, action);
    std::vector<EcmpNextHop> nexthops;
    for (auto i = 0; i < kWidth; i++) {
      nexthops.push_back(this->getNextHop(i));
    }
    utility::verifyMultiPathLabelSwitchAction(
        getHwSwitch(), kTopLabel, action, nexthops);
  }

 private:
  std::unique_ptr<EcmpSetupHelper> helper_;
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

} // namespace facebook::fboss
