// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/hw/test/HwTestMplsUtils.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace facebook::fboss {

namespace {
using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;
const auto kRouter0 = RouterID(0);
const LabelForwardingAction::LabelStack kStack0{
    101,
    102,
    103,
    104,
    105,
    106,
    107,
    108,
    109,
    110,
};
const LabelForwardingAction::LabelStack kStack1{
    201,
    202,
    203,
    204,
    205,
    206,
    207,
    208,
    209,
    210,
};
} // namespace

template <typename AddrT>
struct TestParameters {
  using PrefixT = typename Route<AddrT>::Prefix;
  PrefixT prefix; // prefix for route to remote dest
  AddrT nexthop; // next hop for that route
  const LabelForwardingAction::LabelStack* stack; // label stack to prefix
  Label label; // labels advertised by "LDP" peer (OpenR adjacency)
};

template <typename AddrT>
class HwLabelEdgeRouteTest : public HwLinkStateDependentTest {
 public:
  using EcmpSetupTargetedPorts = utility::EcmpSetupTargetedPorts<AddrT>;
  using EcmpNextHop = utility::EcmpNextHop<AddrT>;
  using PrefixT = typename Route<AddrT>::Prefix;
  const static PrefixT kRoutePrefix;
  const static AddrT kRouteNextHopAddress;
  static auto constexpr kWidth = 4;

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports;
    for (auto i = 0; i < kWidth; i++) {
      ports.push_back(masterLogicalPortIds()[i]);
    }
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), ports, getAsic()->desiredLoopbackModes());
  }

  TestParameters<AddrT> testParams(int i) {
    return kParams[i % 2];
  }

  void setupL3Route(
      ClientID client,
      AddrT network,
      uint8_t mask,
      AddrT nexthop,
      LabelForwardingAction::LabelStack stack = {}) {
    /* setup route to network/mask via nexthop,
      setup ip2mpls if stack is given, else setup ip route */
    std::optional<LabelForwardingAction> labelAction;
    if (!stack.empty()) {
      labelAction = LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH, std::move(stack));
    }
    auto updater = getHwSwitchEnsemble()->getRouteUpdater();
    updater.addRoute(
        kRouter0,
        network,
        mask,
        client,
        RouteNextHopEntry(
            static_cast<NextHop>(
                UnresolvedNextHop(nexthop, ECMP_WEIGHT, labelAction)),
            AdminDistance::MAX_ADMIN_DISTANCE));
    updater.program();
  }

  void resolveLabeledNextHops(AddrT network, uint8_t mask) {
    // resolve neighbors of labeled egresses
    PrefixT prefix{network, mask};
    auto* ecmpHelper = ecmpHelpers_[prefix].get();
    auto state = getProgrammedState()->clone();
    applyNewState(ecmpHelper->resolveNextHops(state, labeledPorts_));
  }

  void resolveUnLabeledNextHops(AddrT network, uint8_t mask) {
    // resolve neighbors of unlabeled egresses
    PrefixT prefix{network, mask};
    auto* ecmpHelper = ecmpHelpers_[prefix].get();
    applyNewState(
        ecmpHelper->resolveNextHops(getProgrammedState(), unLabeledPorts_));
  }

  void unresolveLabeledNextHops(AddrT network, uint8_t mask) {
    // unresolve neighbors of labeled egresses
    PrefixT prefix{network, mask};
    auto* ecmpHelper = ecmpHelpers_[prefix].get();
    auto state = getProgrammedState()->clone();
    applyNewState(ecmpHelper->unresolveNextHops(state, labeledPorts_));
  }

  void unresolveUnLabeledNextHops(AddrT network, uint8_t mask) {
    // unresolve neighbors of unlabeled egresses
    PrefixT prefix{network, mask};
    auto* ecmpHelper = ecmpHelpers_[prefix].get();
    applyNewState(
        ecmpHelper->unresolveNextHops(getProgrammedState(), unLabeledPorts_));
  }

  std::map<PortDescriptor, LabelForwardingAction::LabelStack> port2LabelStacks(
      LabelForwardingAction::Label label) {
    std::map<PortDescriptor, LabelForwardingAction::LabelStack> result;
    for (auto port : labeledPorts_) {
      auto itr = result.emplace(port, LabelForwardingAction::LabelStack{});
      itr.first->second.push_back(label++);
    }
    for (auto port : unLabeledPorts_) {
      result.emplace(port, LabelForwardingAction::LabelStack{});
    }
    return result;
  }

  flat_set<PortDescriptor> allPorts() {
    flat_set<PortDescriptor> result{unLabeledPorts_};
    result.merge(labeledPorts_);
    return result;
  }

  void programRoutes(
      AddrT network,
      uint8_t mask,
      LabelForwardingAction::Label tunnelLabel) {
    std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks =
        port2LabelStacks(tunnelLabel);
    typename Route<AddrT>::Prefix prefix{network, mask};
    auto* ecmpHelper = ecmpHelpers_[prefix].get();
    if (!stacks.empty()) {
      ecmpHelper->programIp2MplsRoutes(
          getRouteUpdater(), allPorts(), std::move(stacks), {prefix});
    } else {
      ecmpHelper->programRoutes(getRouteUpdater(), allPorts(), {prefix});
    }
  }

  void makeEcmpHelper(
      uint8_t unlabeledPaths,
      uint8_t labeledPaths,
      AddrT network,
      uint8_t mask) {
    // setup ecmp helper for network and mask, with labeled and unlabaled paths
    typename Route<AddrT>::Prefix prefix{network, mask};

    auto emplaced = ecmpHelpers_.emplace(std::make_pair(
        prefix,
        std::make_unique<EcmpSetupTargetedPorts>(
            getProgrammedState(), kRouter0)));

    EXPECT_TRUE(emplaced.second);
    const auto& ecmpHelper = emplaced.first->second;
    auto ports = ecmpHelper->ecmpPortDescs(kWidth);
    for (auto i = 0; i < unlabeledPaths; i++) {
      unLabeledPorts_.insert(ports[i]);
    }
    for (auto i = unlabeledPaths; i < unlabeledPaths + labeledPaths; i++) {
      labeledPorts_.insert(ports[i]);
    }
  }

  boost::container::flat_set<PortDescriptor> labeledEgressPorts() {
    return labeledPorts_;
  }

  boost::container::flat_set<PortDescriptor> unLabeledEgressPorts() {
    return unLabeledPorts_;
  }

  void verifyLabeledNextHop(PrefixT prefix, Label label) {
    utility::verifyLabeledNextHop<AddrT>(getHwSwitch(), prefix, label);
  }

  void verifyLabeledNextHopWithStack(
      PrefixT prefix,
      const LabelForwardingAction::LabelStack& stack) {
    utility::verifyLabeledNextHopWithStack<AddrT>(getHwSwitch(), prefix, stack);
  }

  void verifyMultiPathNextHop(
      PrefixT prefix,
      const std::map<PortDescriptor, LabelForwardingAction::LabelStack>&
          stacks) {
    utility::verifyMultiPathNextHop<AddrT>(
        getHwSwitch(),
        prefix,
        stacks,
        unLabeledPorts_.size(),
        labeledPorts_.size());
  }

  void verifyLabeledMultiPathNextHopMemberWithStack(
      PrefixT prefix,
      int memberIndex,
      const LabelForwardingAction::LabelStack& stack,
      bool resolved) {
    utility::verifyLabeledMultiPathNextHopMemberWithStack<AddrT>(
        getHwSwitch(), prefix, memberIndex, stack, resolved);
  }

  void verifyProgrammedStackOnPort(
      const AddrT& nexthopAddr,
      const AddrT& dstAddr,
      uint8_t mask,
      const PortDescriptor& port,
      const LabelForwardingAction::LabelStack& stack,
      long refCount) {
    PrefixT prefix{nexthopAddr, mask};
    auto* ecmpHelper = ecmpHelpers_[prefix].get();
    auto vlanID = ecmpHelper->getVlan(port, getProgrammedState());
    EXPECT_TRUE(vlanID.has_value());
    auto intfID = getProgrammedState()
                      ->getVlans()
                      ->getNode(vlanID.value())
                      ->getInterfaceID();
    PrefixT dstPrefx{dstAddr, mask};
    utility::verifyProgrammedStack<AddrT>(
        getHwSwitch(), dstPrefx, intfID, stack, refCount);
  }

 protected:
  std::map<PrefixT, std::unique_ptr<EcmpSetupTargetedPorts>> ecmpHelpers_;
  boost::container::flat_set<PortDescriptor> labeledPorts_;
  boost::container::flat_set<PortDescriptor> unLabeledPorts_;
  static const std::array<TestParameters<AddrT>, 2> kParams;
};

template <>
const std::array<TestParameters<folly::IPAddressV4>, 2>
    HwLabelEdgeRouteTest<folly::IPAddressV4>::kParams{
        TestParameters<folly::IPAddressV4>{
            Route<folly::IPAddressV4>::Prefix{
                folly::IPAddressV4("101.102.103.0"),
                24},
            folly::IPAddressV4{"11.12.13.1"},
            &kStack0,
            1001},
        TestParameters<folly::IPAddressV4>{
            Route<folly::IPAddressV4>::Prefix{
                folly::IPAddressV4("201.202.203.0"),
                24},
            folly::IPAddressV4{"21.22.23.1"},
            &kStack1,
            2001}};

template <>
const std::array<TestParameters<folly::IPAddressV6>, 2>
    HwLabelEdgeRouteTest<folly::IPAddressV6>::kParams{
        TestParameters<folly::IPAddressV6>{
            Route<folly::IPAddressV6>::Prefix{
                folly::IPAddressV6("101:102::103:0:0"),
                96},
            folly::IPAddressV6{"101:102::103:0:0:0:1"},
            &kStack0,
            1001},
        TestParameters<folly::IPAddressV6>{
            Route<folly::IPAddressV6>::Prefix{
                folly::IPAddressV6("201:202::203:0:0"),
                96},
            folly::IPAddressV6{"201:202::203:0:0:0:1"},
            &kStack1,
            2001}};

TYPED_TEST_SUITE(HwLabelEdgeRouteTest, TestTypes);

TYPED_TEST(HwLabelEdgeRouteTest, OneLabel) {
  // setup nexthop with only one label
  // test that labeled egress is used
  // test that tunnel initiator is setup
  // test that route is setup to labeled egress
  auto params = this->testParams(0);
  this->makeEcmpHelper(0, 1, params.nexthop, params.prefix.mask());
  auto setup = [=]() {
    this->setupL3Route(
        ClientID::BGPD,
        params.prefix.network(),
        params.prefix.mask(),
        params.nexthop); // unlabaled route from client
    this->resolveLabeledNextHops(params.nexthop, params.prefix.mask());
    this->programRoutes(
        params.nexthop, params.prefix.mask(), params.label.value());
    this->getProgrammedState();
  };
  auto verify = [=]() {
    this->verifyLabeledNextHop(params.prefix, params.label);
    for (const auto& port : this->labeledEgressPorts()) {
      this->verifyProgrammedStackOnPort(
          params.nexthop,
          params.prefix.network(),
          params.prefix.mask(),
          port,
          LabelForwardingAction::LabelStack{params.label.value()},
          1);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, MaxLabels) {
  // setup nexthop with max labels
  // test that labeled egress is used
  // test that tunnel initiator is setup
  // test that route is setup to labeled egress
  // test that labeled egress is associated with tunnel
  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto params = this->testParams(0);
  this->makeEcmpHelper(0, 1, params.nexthop, params.prefix.mask());
  auto setup = [=]() {
    // program l3 route with stack of size one less than maximum supported
    // additional label is adjacency label, which completes stack depth
    this->setupL3Route(
        ClientID::BGPD,
        params.prefix.network(),
        params.prefix.mask(),
        params.nexthop,
        LabelForwardingAction::LabelStack(
            params.stack->begin(), params.stack->begin() + maxSize - 1));
    this->resolveLabeledNextHops(params.nexthop, params.prefix.mask());
    this->programRoutes(
        params.nexthop,
        params.prefix.mask(),
        params.label.value()); // apply adjacency label
    this->getProgrammedState();
  };
  auto verify = [=]() {
    // prepare expected stack
    // adjacency/tunnel label will be on top,
    // other labels will at the bottom of stack
    // the bottom most label in route's label stack will be attached to egress
    LabelForwardingAction::LabelStack stack{
        params.stack->begin(), params.stack->begin() + maxSize - 1};
    stack.push_back(params.label.value());
    this->verifyLabeledNextHopWithStack(params.prefix, stack);

    for (const auto& port : this->labeledEgressPorts()) {
      this->verifyProgrammedStackOnPort(
          params.nexthop,
          params.prefix.network(),
          params.prefix.mask(),
          port,
          stack,
          1);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, ExceedMaxLabels) {
  // setup nexthop with stack exceeding labels
  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto params = this->testParams(0);
  this->makeEcmpHelper(0, 1, params.nexthop, params.prefix.mask());
  auto setup = [=]() {
    this->setupL3Route(
        ClientID::BGPD,
        params.prefix.network(),
        params.prefix.mask(),
        params.nexthop,
        LabelForwardingAction::LabelStack(
            params.stack->begin(), params.stack->begin() + maxSize));
    this->resolveLabeledNextHops(params.nexthop, params.prefix.mask());
    this->programRoutes(
        params.nexthop, params.prefix.mask(), params.label.value());
  };
  EXPECT_THROW(setup(), FbossError);
}

TYPED_TEST(HwLabelEdgeRouteTest, HalfPathsWithLabels) {
  // setup half next hops with labels and half without
  // test that labeled egress is used for labeled nexthops
  // test that unlabeled egress is used for unlabeled nexthops
  // test that tunnel initiator is setup correctly
  // test that labeled egress is associated with tunnel
  auto params = this->testParams(0);
  this->makeEcmpHelper(1, 1, params.nexthop, params.prefix.mask());
  auto setup = [=]() {
    // program l3 route with stack of size one less than maximum supported
    // additional label is adjacency label, which completes stack depth
    this->setupL3Route(
        ClientID::BGPD,
        params.prefix.network(),
        params.prefix.mask(),
        params.nexthop);
    this->resolveUnLabeledNextHops(params.nexthop, params.prefix.mask());
    this->resolveLabeledNextHops(params.nexthop, params.prefix.mask());
    this->programRoutes(
        params.nexthop, params.prefix.mask(), params.label.value());
    this->getProgrammedState();
  };
  auto verify = [=]() {
    std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks;

    for (const auto unLabeledPort : this->unLabeledEgressPorts()) {
      stacks.emplace(unLabeledPort, LabelForwardingAction::LabelStack{});
    }

    for (const auto labeledPort : this->labeledEgressPorts()) {
      auto itr =
          stacks.emplace(labeledPort, LabelForwardingAction::LabelStack{});
      itr.first->second.push_back(params.label.value());
      this->verifyProgrammedStackOnPort(
          params.nexthop,
          params.prefix.network(),
          params.prefix.mask(),
          labeledPort,
          itr.first->second,
          1);
    }
    this->verifyMultiPathNextHop(params.prefix, stacks);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, PathWithDifferentTunnelLabels) {
  // setup nexthops with common tunnel stack but different egress labels
  // test that labeled egress is used for labeled nexthops
  // test that only required tunnel initiators are set up
  // test that labeled egresses are associated with tunnel
  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto params = this->testParams(0);
  this->makeEcmpHelper(0, 2, params.nexthop, params.prefix.mask());
  auto setup = [=]() {
    this->setupL3Route(
        ClientID::BGPD,
        params.prefix.network(),
        params.prefix.mask(),
        params.nexthop,
        LabelForwardingAction::LabelStack(
            params.stack->begin(), params.stack->begin() + maxSize - 1));
    this->resolveLabeledNextHops(params.nexthop, params.prefix.mask());
    this->programRoutes(
        params.nexthop, params.prefix.mask(), params.label.value());
    this->getProgrammedState();
  };
  auto verify = [=]() {
    std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks;
    auto i = 0;
    for (auto labeledPort : this->labeledEgressPorts()) {
      LabelForwardingAction::LabelStack stack{
          params.stack->begin(), params.stack->begin() + maxSize - 1};
      stack.push_back(params.label.value() + i++);
      stacks.emplace(labeledPort, stack);

      this->verifyProgrammedStackOnPort(
          params.nexthop,
          params.prefix.network(),
          params.prefix.mask(),
          labeledPort,
          stack,
          1);
    }
    this->verifyMultiPathNextHop(params.prefix, stacks);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, PathsWithDifferentLabelStackSameTunnelLabel) {
  // setup nexthops with different tunnel stack but with same tunnel labels
  // test that labeled egress is used for labeled nexthops
  // test that only required tunnel initiators are set up
  // test that labeled egresses are associated with tunnel
  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();

  using ParamsT = TestParameters<TypeParam>;
  std::vector<ParamsT> params{
      this->testParams(0),
      this->testParams(1),
  };
  this->makeEcmpHelper(0, 2, params[0].nexthop, params[0].prefix.mask());
  this->makeEcmpHelper(0, 2, params[1].nexthop, params[1].prefix.mask());

  Label tunnelLabel{511};
  auto setup = [=]() {
    for (auto i = 0; i < params.size(); i++) {
      this->setupL3Route(
          ClientID::BGPD,
          params[i].prefix.network(),
          params[i].prefix.mask(),
          params[i].nexthop,
          LabelForwardingAction::LabelStack(
              params[i].stack->begin(),
              params[i].stack->begin() + maxSize - 1));
      this->resolveLabeledNextHops(params[i].nexthop, params[i].prefix.mask());

      this->programRoutes(
          params[i].nexthop, params[i].prefix.mask(), tunnelLabel.value());
    }
    this->getProgrammedState();
  };
  auto verify = [=]() {
    for (auto i = 0; i < params.size(); i++) {
      std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks;
      auto localTunnelLabel = tunnelLabel;

      for (auto labeledPort : this->labeledEgressPorts()) {
        LabelForwardingAction::LabelStack stack;
        auto pushStack = LabelForwardingAction::LabelStack(
            params[i].stack->begin(), params[i].stack->begin() + maxSize - 1);
        pushStack.push_back(localTunnelLabel.value());
        stacks.emplace(labeledPort, pushStack);
        localTunnelLabel = Label(localTunnelLabel.label() + 1);
      }
      this->verifyMultiPathNextHop(params[i].prefix, stacks);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, PathsWithSameLabelStackDifferentTunnelLabel) {
  // setup nexthops with common tunnel stack but different egress labels
  // test that labeled egress is used for labeled nexthops
  // test that only required tunnel initiators are set up
  // test that labeled egresses are associated with tunnel
  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();

  using ParamsT = TestParameters<TypeParam>;
  std::vector<ParamsT> params{
      this->testParams(0),
      this->testParams(1),
  };
  this->makeEcmpHelper(0, 2, params[0].nexthop, params[0].prefix.mask());
  this->makeEcmpHelper(0, 2, params[1].nexthop, params[1].prefix.mask());

  auto setup = [=]() {
    for (auto i = 0; i < params.size(); i++) {
      this->setupL3Route(
          ClientID::BGPD,
          params[i].prefix.network(),
          params[i].prefix.mask(),
          params[i].nexthop,
          LabelForwardingAction::LabelStack(
              params[0].stack->begin(),
              params[0].stack->begin() + maxSize - 1));
      this->resolveLabeledNextHops(params[i].nexthop, params[i].prefix.mask());

      this->programRoutes(
          params[i].nexthop, params[i].prefix.mask(), params[i].label.value());
    }
    this->getProgrammedState();
  };
  auto verify = [=]() {
    for (auto i = 0; i < params.size(); i++) {
      std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks;
      auto j = 0;
      for (auto labeledPort : this->labeledEgressPorts()) {
        auto pushStack = LabelForwardingAction::LabelStack(
            params[0].stack->begin(), params[0].stack->begin() + maxSize - 1);
        pushStack.push_back(params[i].label.value() + j);
        this->verifyProgrammedStackOnPort(
            params[i].nexthop,
            params[i].prefix.network(),
            params[i].prefix.mask(),
            labeledPort,
            pushStack,
            1);
        stacks.emplace(labeledPort, pushStack);
        j += 1;
      }
      this->verifyMultiPathNextHop(params[i].prefix, stacks);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, RoutesToSameNextHopWithDifferentStack) {
  // setup nexthops with common tunnel stack but different egress labels
  // test that labeled egress is used for labeled nexthops
  // test that only required tunnel initiators are set up
  // test that labeled egresses are associated with tunnel
  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();

  using ParamsT = TestParameters<TypeParam>;
  std::vector<ParamsT> params{
      this->testParams(0),
      this->testParams(1),
  };
  this->makeEcmpHelper(0, 2, params[0].nexthop, params[0].prefix.mask());

  auto setup = [=]() {
    for (auto i = 0; i < params.size(); i++) {
      this->setupL3Route(
          ClientID::BGPD,
          params[i].prefix.network(),
          params[i].prefix.mask(),
          params[0].nexthop,
          LabelForwardingAction::LabelStack(
              params[i].stack->begin(),
              params[i].stack->begin() + maxSize - 1));
    }
    /* same next hop to 2 prefixes with different stacks */
    this->resolveLabeledNextHops(params[0].nexthop, params[0].prefix.mask());
    this->programRoutes(
        params[0].nexthop, params[0].prefix.mask(), params[0].label.value());
    this->getProgrammedState();
  };
  auto verify = [=]() {
    for (auto i = 0; i < params.size(); i++) {
      std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks;

      auto j = 0;
      for (auto labeledPort : this->labeledEgressPorts()) {
        LabelForwardingAction::LabelStack pushStack{
            params[i].stack->begin(), params[i].stack->begin() + maxSize - 1};
        pushStack.push_back(params[0].label.value() + j);

        stacks.emplace(labeledPort, pushStack);
        j += 1;
      }
      this->verifyMultiPathNextHop(params[i].prefix, stacks);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, UnresolvedNextHops) {
  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto params = this->testParams(0);
  this->makeEcmpHelper(
      0, 2, params.nexthop, params.prefix.mask()); // two labeled ports

  auto setup = [=]() {
    this->setupL3Route(
        ClientID::BGPD,
        params.prefix.network(),
        params.prefix.mask(),
        params.nexthop,
        LabelForwardingAction::LabelStack(
            params.stack->begin(), params.stack->begin() + maxSize - 1));
    this->programRoutes(
        params.nexthop, params.prefix.mask(), params.label.value());
    this->getProgrammedState();
  };
  auto verify = [=]() {
    for (auto i = 0; i < 2; i++) {
      LabelForwardingAction::LabelStack stack{
          params.stack->begin(), params.stack->begin() + maxSize - 1};
      stack.push_back(params.label.value() + i);
      this->verifyLabeledMultiPathNextHopMemberWithStack(
          params.prefix, i, stack, false);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, UnresolveResolvedNextHops) {
  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto params = this->testParams(0);
  this->makeEcmpHelper(
      0, 2, params.nexthop, params.prefix.mask()); // two labeled ports

  auto setup = [=]() {
    this->setupL3Route(
        ClientID::BGPD,
        params.prefix.network(),
        params.prefix.mask(),
        params.nexthop,
        LabelForwardingAction::LabelStack(
            params.stack->begin(), params.stack->begin() + maxSize - 1));
    this->programRoutes(
        params.nexthop, params.prefix.mask(), params.label.value());
    this->resolveLabeledNextHops(params.nexthop, params.prefix.mask());
    this->unresolveLabeledNextHops(params.nexthop, params.prefix.mask());
    this->getProgrammedState();
  };
  auto verify = [=]() {
    for (auto i = 0; i < 2; i++) {
      LabelForwardingAction::LabelStack stack{
          params.stack->begin(), params.stack->begin() + maxSize - 1};
      stack.push_back(params.label.value() + i);
      this->verifyLabeledMultiPathNextHopMemberWithStack(
          params.prefix, i, stack, false);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, UnresolvedHybridNextHops) {
  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto params = this->testParams(0);
  this->makeEcmpHelper(1, 1, params.nexthop, params.prefix.mask());

  auto setup = [=]() {
    this->setupL3Route(
        ClientID::BGPD,
        params.prefix.network(),
        params.prefix.mask(),
        params.nexthop,
        LabelForwardingAction::LabelStack(
            params.stack->begin(), params.stack->begin() + maxSize - 1));
    this->programRoutes(
        params.nexthop, params.prefix.mask(), params.label.value());
    this->resolveLabeledNextHops(params.nexthop, params.prefix.mask());
    this->resolveUnLabeledNextHops(params.nexthop, params.prefix.mask());
    this->unresolveLabeledNextHops(params.nexthop, params.prefix.mask());
    this->unresolveUnLabeledNextHops(params.nexthop, params.prefix.mask());
    this->getProgrammedState();
  };
  auto verify = [=]() {
    for (auto i = 0; i < 2; i++) {
      if (!i) {
        LabelForwardingAction::LabelStack stack{
            params.stack->begin(), params.stack->begin() + maxSize - 1};
        this->verifyLabeledMultiPathNextHopMemberWithStack(
            params.prefix, i, stack, false);
      } else {
        LabelForwardingAction::LabelStack stack{
            params.stack->begin(), params.stack->begin() + maxSize - 1};
        stack.push_back(params.label.value());
        this->verifyLabeledMultiPathNextHopMemberWithStack(
            params.prefix, i, stack, false);
      }
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, UnresolvedAndResolvedNextHopMultiPathGroup) {
  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto params = this->testParams(0);
  this->makeEcmpHelper(1, 1, params.nexthop, params.prefix.mask());

  auto setup = [=]() {
    this->setupL3Route(
        ClientID::BGPD,
        params.prefix.network(),
        params.prefix.mask(),
        params.nexthop,
        LabelForwardingAction::LabelStack(
            params.stack->begin(), params.stack->begin() + maxSize - 1));
    this->programRoutes(
        params.nexthop, params.prefix.mask(), params.label.value());
    this->resolveUnLabeledNextHops(params.nexthop, params.prefix.mask());
    this->getProgrammedState();
  };
  auto verify = [=]() {
    for (auto i = 0; i < 2; i++) {
      if (!i) {
        LabelForwardingAction::LabelStack stack{
            params.stack->begin(), params.stack->begin() + maxSize - 1};
        // resolved
        this->verifyLabeledMultiPathNextHopMemberWithStack(
            params.prefix, i, stack, true);
      } else {
        LabelForwardingAction::LabelStack stack{
            params.stack->begin(), params.stack->begin() + maxSize - 1};
        stack.push_back(params.label.value());
        // unresolved
        this->verifyLabeledMultiPathNextHopMemberWithStack(
            params.prefix, i, stack, false);
      }
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, UpdateRouteLabels) {
  using ParamsT = TestParameters<TypeParam>;
  std::vector<ParamsT> params{
      this->testParams(0),
      this->testParams(1),
  };
  this->makeEcmpHelper(0, 2, params[0].nexthop, params[0].prefix.mask());
  this->makeEcmpHelper(0, 2, params[1].nexthop, params[1].prefix.mask());

  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto setup = [=]() {
    for (auto i = 0; i < 2; i++) {
      this->setupL3Route(
          ClientID::BGPD,
          params[i].prefix.network(),
          params[i].prefix.mask(),
          params[i].nexthop,
          LabelForwardingAction::LabelStack(
              params[i].stack->begin(),
              params[i].stack->begin() + maxSize - 1));
      this->resolveLabeledNextHops(params[i].nexthop, params[i].prefix.mask());
      this->programRoutes(
          params[i].nexthop, params[i].prefix.mask(), params[i].label.value());
    }
    // update label stack for prefix of param 1
    this->setupL3Route(
        ClientID::BGPD,
        params[1].prefix.network(),
        params[1].prefix.mask(),
        params[1].nexthop,
        LabelForwardingAction::LabelStack(
            params[0].stack->begin(), params[0].stack->begin() + maxSize - 1));
    this->getProgrammedState();
  };
  auto verify = [=]() {
    for (auto i = 0; i < 2; i++) {
      std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks;
      auto j = 0;
      for (auto labeledPort : this->labeledEgressPorts()) {
        LabelForwardingAction::LabelStack stack{
            params[0].stack->begin(), params[0].stack->begin() + maxSize - 1};
        stack.push_back(params[i].label.value() + j);
        this->verifyProgrammedStackOnPort(
            params[i].nexthop,
            params[i].prefix.network(),
            params[i].prefix.mask(),
            labeledPort,
            stack,
            1);
        stacks.emplace(labeledPort, stack);
        j++;
      }
      this->verifyMultiPathNextHop(params[i].prefix, stacks);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, UpdatePortLabel) {
  using ParamsT = TestParameters<TypeParam>;
  std::vector<ParamsT> params{
      this->testParams(0),
      this->testParams(1),
  };
  this->makeEcmpHelper(0, 2, params[0].nexthop, params[0].prefix.mask());
  this->makeEcmpHelper(0, 2, params[1].nexthop, params[1].prefix.mask());

  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto setup = [=]() {
    for (auto i = 0; i < 2; i++) {
      this->setupL3Route(
          ClientID::BGPD,
          params[i].prefix.network(),
          params[i].prefix.mask(),
          params[i].nexthop,
          LabelForwardingAction::LabelStack(
              params[i].stack->begin(),
              params[i].stack->begin() + maxSize - 1));
      this->resolveLabeledNextHops(params[i].nexthop, params[i].prefix.mask());
      this->programRoutes(
          params[i].nexthop, params[i].prefix.mask(), params[i].label.value());
    }

    //  update tunnel label for prefix 1
    this->setupL3Route(
        ClientID::BGPD,
        params[1].prefix.network(),
        params[1].prefix.mask(),
        params[0].nexthop,
        LabelForwardingAction::LabelStack(
            params[1].stack->begin(), params[1].stack->begin() + maxSize - 1));
    this->getProgrammedState();
  };
  auto verify = [=]() {
    for (auto i = 0; i < 2; i++) {
      std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks;
      auto j = 0;
      for (auto labeledPort : this->labeledEgressPorts()) {
        LabelForwardingAction::LabelStack stack{
            params[i].stack->begin(), params[i].stack->begin() + maxSize - 1};
        stack.push_back(params[0].label.value() + j);
        stacks.emplace(labeledPort, stack);
        j++;
      }
      this->verifyMultiPathNextHop(params[i].prefix, stacks);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwLabelEdgeRouteTest, RecursiveStackResolution) {
  using ParamsT = TestParameters<TypeParam>;

  std::vector<ParamsT> params{
      this->testParams(0),
      this->testParams(1),
  };

  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto halfSize = (maxSize >>= 1); // half label stack
  this->makeEcmpHelper(0, 2, params[1].nexthop, params[1].nexthop.bitCount());

  auto setup = [=]() {
    this->setupL3Route(
        ClientID::BGPD,
        params[0].prefix.network(),
        params[0].prefix.mask(),
        params[0].nexthop,
        LabelForwardingAction::LabelStack(
            params[0].stack->begin(), params[0].stack->begin() + halfSize - 1));
    this->setupL3Route(
        ClientID::BGPD,
        params[0].nexthop,
        params[0].nexthop.bitCount(),
        params[1].nexthop,
        LabelForwardingAction::LabelStack(
            params[0].stack->begin() + halfSize,
            params[0].stack->begin() + maxSize - 1));
    this->programRoutes(
        params[1].nexthop,
        params[1].nexthop.bitCount(),
        params[1].label.value());
    this->getProgrammedState();
  };
  auto verify = [=]() {
    std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks;
    auto j = 0;
    for (auto labeledPort : this->labeledEgressPorts()) {
      LabelForwardingAction::LabelStack stack{
          params[0].stack->begin(), params[0].stack->begin() + maxSize - 1};
      stack.push_back(params[1].label.value() + j);
      this->verifyProgrammedStackOnPort(
          params[1].nexthop,
          params[1].prefix.network(),
          params[1].nexthop.bitCount(),
          labeledPort,
          stack,
          1);
      stacks.emplace(labeledPort, stack);
      j++;
    }
    this->verifyMultiPathNextHop(params[0].prefix, stacks);
  };
}

// BCM Specific Test
TYPED_TEST(HwLabelEdgeRouteTest, TunnelRefTest) {
  using ParamsT = TestParameters<TypeParam>;

  std::vector<ParamsT> params{
      this->testParams(0),
      this->testParams(1),
  };

  auto maxSize = this->getPlatform()->getAsic()->getMaxLabelStackDepth();
  this->makeEcmpHelper(0, 2, params[0].nexthop, params[0].nexthop.bitCount());
  this->makeEcmpHelper(0, 2, params[1].nexthop, params[1].nexthop.bitCount());

  auto setup = [=]() {
    for (auto i = 0; i < 2; i++) {
      LabelForwardingAction::LabelStack stack{
          params[i].stack->begin(), params[i].stack->begin() + 1};
      std::copy(
          params[0].stack->begin() + 1,
          params[0].stack->begin() + maxSize - 1,
          std::back_inserter(stack));

      this->setupL3Route(
          ClientID::BGPD,
          params[i].prefix.network(),
          params[i].prefix.mask(),
          params[i].nexthop,
          stack);

      this->resolveLabeledNextHops(
          params[i].nexthop, params[i].nexthop.bitCount());
      this->programRoutes(
          params[i].nexthop,
          params[i].nexthop.bitCount(),
          params[0].label.value());
    }
    this->getProgrammedState();
  };
  auto verify = [=]() {
    for (auto i = 0; i < 1; i++) {
      std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks;
      auto j = 0;
      for (auto labeledPort : this->labeledEgressPorts()) {
        LabelForwardingAction::LabelStack stack{
            params[0].stack->begin(), params[0].stack->begin() + maxSize - 1};
        stack.push_back(params[0].label.value() + j);

        this->verifyProgrammedStackOnPort(
            params[i].nexthop,
            params[i].prefix.network(),
            params[i].nexthop.bitCount(),
            labeledPort,
            stack,
            2);
        stacks.emplace(labeledPort, stack);
        j++;
      }
      this->verifyMultiPathNextHop(params[i].prefix, stacks);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
