// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <boost/container/flat_set.hpp>
#include <folly/Conv.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/logging/xlog.h>

#include <array>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/agent_hw_tests/AgentMPLSDataplaneTest.h"
#include "fboss/agent/test/agent_hw_tests/AgentMPLSDataplaneTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

namespace {

namespace mpls_test = facebook::fboss::utility::mpls_dataplane_test;
using mpls_test::MplsTrapPacketMechanism;

const facebook::fboss::Label kTopLabel{1101};
const facebook::fboss::LabelForwardingAction::Label kSwapLabel{201};
constexpr uint32_t kSinglePushedLabelBase = 101;
constexpr uint32_t kMaxPushedLabelBase = 1001;

using MplsMidpointPortTypes =
    ::testing::Types<facebook::fboss::PortID, facebook::fboss::AggregatePortID>;

} // namespace

namespace facebook::fboss {

template <typename PortType>
class AgentMPLSMidpointTest : public AgentMPLSDataplaneTest<PortType> {
 protected:
  using BaseT = AgentMPLSDataplaneTest<PortType>;
  using EcmpSetupHelper =
      utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV6>;

  using BaseT::applyConfigAndEnableTrunks;
  using BaseT::egressPort;
  using BaseT::egressPortDescriptor;
  using BaseT::getAgentEnsemble;
  using BaseT::getLatestPortStats;
  using BaseT::getProgrammedState;
  using BaseT::getSw;
  using BaseT::getVlanIDForTx;
  using BaseT::ingressPort;
  using BaseT::initialConfig;
  using BaseT::maxPushedLabelStack;
  using BaseT::pushedLabelStack;
  using BaseT::pushedTopLabel;
  using BaseT::routerMac;
  using BaseT::secondPassEgressPort;
  using BaseT::switchIdForPort;

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (BaseT::kIsTrunk) {
      return {ProductionFeature::MPLS_MIDPOINT, ProductionFeature::LAG};
    }
    return {ProductionFeature::MPLS_MIDPOINT};
  }

  MplsTrapPacketMechanism trapPacketMechanism() const {
    auto asic = checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());
    return asic->isSupported(HwAsic::Feature::SAI_ACL_ENTRY_SRC_PORT_QUALIFIER)
        ? MplsTrapPacketMechanism::SrcPortAcl
        : MplsTrapPacketMechanism::TtlExpiry;
  }

  std::unique_ptr<EcmpSetupHelper> setupECMPHelper(
      Label topLabel,
      LabelForwardingAction::LabelForwardingType actionType) const {
    return std::make_unique<EcmpSetupHelper>(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        topLabel,
        actionType);
  }

  LabelForwardingAction::LabelStack singlePushedLabelStack() const {
    return pushedLabelStack(kSinglePushedLabelBase, 1);
  }

  LabelForwardingAction::LabelStack maxPushedLabelStack() const {
    return maxPushedLabelStack(kMaxPushedLabelBase);
  }

  void configureStaticMplsRoute(
      cfg::SwitchConfig& config,
      Label ingressLabel,
      const LabelForwardingAction& action,
      PortDescriptor nextHop) const {
    config.staticMplsRoutesWithNhops()->emplace_back();
    auto& route = config.staticMplsRoutesWithNhops()->back();
    route.ingressLabel() = ingressLabel.value();

    auto helper = setupECMPHelper(ingressLabel, action.type());
    auto nhop = helper->nhop(std::move(nextHop));

    NextHopThrift nextHopThrift;
    CHECK(nhop.linkLocalNhopIp.has_value());
    nextHopThrift.address() =
        network::toBinaryAddress(folly::IPAddress(*nhop.linkLocalNhopIp));
    nextHopThrift.address()->ifName() =
        folly::to<std::string>("fboss", nhop.intf);
    nextHopThrift.mplsAction() = action.toThrift();
    XLOG(INFO) << "MPLS midpoint route ingress label " << ingressLabel.value()
               << " uses link-local nexthop " << *nhop.linkLocalNhopIp
               << " on interface " << nhop.intf;
    route.nexthops()->push_back(nextHopThrift);
  }

  void configureStaticMplsPushRoute(
      cfg::SwitchConfig& config,
      const LabelForwardingAction::LabelStack& pushStack) const {
    configureStaticMplsRoute(
        config,
        kTopLabel,
        LabelForwardingAction(
            LabelForwardingAction::LabelForwardingType::PUSH, pushStack),
        egressPortDescriptor());
  }

  void configureStaticMplsSwapRoute(
      cfg::SwitchConfig& config,
      Label ingressLabel,
      LabelForwardingAction::Label swapLabel,
      PortDescriptor nextHop) const {
    configureStaticMplsRoute(
        config,
        ingressLabel,
        LabelForwardingAction(
            LabelForwardingAction::LabelForwardingType::SWAP, swapLabel),
        std::move(nextHop));
  }

  void configureTrapPacketMechanism(
      cfg::SwitchConfig& config,
      MplsTrapPacketMechanism mechanism,
      const LabelForwardingAction::LabelStack& pushStack) const {
    switch (mechanism) {
      case MplsTrapPacketMechanism::SrcPortAcl: {
        auto asic =
            checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());
        utility::addTrapPacketAcl(asic, &config, egressPort());
        break;
      }
      case MplsTrapPacketMechanism::TtlExpiry:
        configureStaticMplsSwapRoute(
            config,
            pushedTopLabel(pushStack),
            kSwapLabel,
            PortDescriptor(secondPassEgressPort()));
        break;
    }
  }

  void resolveNextHopForPort(
      const PortDescriptor& nextHop,
      Label topLabel,
      LabelForwardingAction::LabelForwardingType actionType) {
    this->applyNewState(
        [this, nextHop, topLabel, actionType](
            const std::shared_ptr<SwitchState>& state) {
          auto helper = EcmpSetupHelper(
              state, getSw()->needL2EntryForNeighbor(), topLabel, actionType);
          return helper.resolveNextHops(
              state,
              boost::container::flat_set<PortDescriptor>{nextHop},
              true /* useLinkLocal */);
        },
        "resolve midpoint MPLS nexthop");
  }

  void resolveNextHop() {
    resolveNextHopForPort(
        egressPortDescriptor(),
        kTopLabel,
        LabelForwardingAction::LabelForwardingType::PUSH);
  }

  void resolveNextHopForPortWithMac(
      const PortDescriptor& nextHop,
      folly::MacAddress nextHopMac) {
    this->applyNewState(
        [this, nextHop, nextHopMac](const std::shared_ptr<SwitchState>& state) {
          utility::EcmpSetupTargetedPorts6 helper(
              state, getSw()->needL2EntryForNeighbor(), nextHopMac);
          return helper.resolveNextHops(
              state,
              boost::container::flat_set<PortDescriptor>{nextHop},
              true /* useLinkLocal */);
        },
        "resolve midpoint MPLS nexthop with explicit MAC");
  }

  void unresolveNextHopForPortWithMac(
      const PortDescriptor& nextHop,
      folly::MacAddress nextHopMac) {
    this->applyNewState(
        [this, nextHop, nextHopMac](const std::shared_ptr<SwitchState>& state) {
          utility::EcmpSetupTargetedPorts6 helper(
              state, getSw()->needL2EntryForNeighbor(), nextHopMac);
          return helper.unresolveNextHops(
              state,
              boost::container::flat_set<PortDescriptor>{nextHop},
              true /* useLinkLocal */);
        },
        "unresolve midpoint MPLS nexthop with explicit MAC");
  }

  void flapEgressPortAndReresolveNextHop() {
    this->bringDownPort(egressPort());
    unresolveNextHopForPortWithMac(egressPortDescriptor(), routerMac());
    this->bringUpPort(egressPort());

    // After the SAI fast-path link-down handler disables the single LAG member,
    // tests need a switch state delta to re-enable the member because LACP is
    // not running.
    if constexpr (BaseT::kIsTrunk) {
      this->applyNewState(
          [](const std::shared_ptr<SwitchState>& state) {
            return utility::disableTrunkPorts(state);
          },
          "disable trunk ports to sync with SAI state");
      this->applyNewState(
          [](const std::shared_ptr<SwitchState>& state) {
            return utility::enableTrunkPorts(state);
          },
          "re-enable trunk ports after link flap");
    }

    resolveNextHopForPortWithMac(egressPortDescriptor(), routerMac());
  }

  std::unique_ptr<TxPacket>
  makeMplsIngressPacket(Label label, uint8_t ttl, bool isV4) const {
    auto vlan = getVlanIDForTx();
    CHECK(vlan.has_value());

    MPLSHdr::Label mplsLabel{
        static_cast<uint32_t>(label.value()), 0, true, ttl};
    std::unique_ptr<TxPacket> pkt;
    if (isV4) {
      auto frame = utility::getEthFrame(
          utility::kLocalCpuMac(),
          utility::kLocalCpuMac(),
          {mplsLabel},
          folly::IPAddressV4{"100.1.1.1"},
          folly::IPAddressV4{"200.1.1.1"},
          10000,
          20000,
          *vlan);
      pkt = frame.getTxPacket(
          [sw = getSw()](uint32_t size) { return sw->allocatePacket(size); });
    } else {
      auto frame = utility::getEthFrame(
          utility::kLocalCpuMac(),
          utility::kLocalCpuMac(),
          {mplsLabel},
          folly::IPAddressV6{"1001::1"},
          folly::IPAddressV6{"2001::1"},
          10000,
          20000,
          *vlan);
      pkt = frame.getTxPacket(
          [sw = getSw()](uint32_t size) { return sw->allocatePacket(size); });
    }
    return pkt;
  }

  void sendMplsIngressPacket(
      Label label,
      uint8_t ttl,
      bool isV4,
      std::optional<PortID> injectPort) {
    auto pkt = makeMplsIngressPacket(label, ttl, isV4);
    if (injectPort.has_value()) {
      EXPECT_TRUE(
          getAgentEnsemble()->ensureSendPacketOutOfPort(
              std::move(pkt), *injectPort));
    } else {
      EXPECT_TRUE(getAgentEnsemble()->ensureSendPacketSwitched(std::move(pkt)));
    }
  }

  void setupStaticMplsRoutePush(
      const LabelForwardingAction::LabelStack& pushStack) {
    auto mechanism = trapPacketMechanism();
    auto config = initialConfig(*getAgentEnsemble());

    applyConfigAndEnableTrunks(config);
    // Resolve the PUSH nexthop before programming the MPLS route. TH6/BRCM-SAI
    // currently requires the MPLS nexthop object to exist when the InSeg entry
    // is created. A follow-up link-flap test covers nexthop unresolve and
    // re-resolve convergence after the route has been programmed.
    resolveNextHopForPortWithMac(egressPortDescriptor(), routerMac());

    // TTL-expiry fallback traps the post-PUSH packet on its second pass:
    // - The first pass imposes the pushed label and egresses to a loopback
    // port.
    // - The looped packet uses the router MAC so it gets routed again.
    // - The second pass matches the pushed-label SWAP route and expires TTL.
    // - The MPLS TTL trap sends the packet to CPU for packet snooper
    // inspection.
    if (mechanism == MplsTrapPacketMechanism::TtlExpiry) {
      resolveNextHopForPort(
          PortDescriptor(secondPassEgressPort()),
          pushedTopLabel(pushStack),
          LabelForwardingAction::LabelForwardingType::SWAP);
    }

    configureStaticMplsPushRoute(config, pushStack);
    configureTrapPacketMechanism(config, mechanism, pushStack);
    applyConfigAndEnableTrunks(config);
  }

  void verifyMplsPushAndTrapPacket(
      bool isV4,
      std::optional<PortID> injectPort,
      const LabelForwardingAction::LabelStack& expectedPushStack) {
    auto mechanism = trapPacketMechanism();
    BaseT::verifyMplsPushAndTrapPacket(
        "mpls-midpoint-push-verifier",
        isV4,
        injectPort,
        mechanism,
        expectedPushStack,
        [this, isV4, injectPort](uint8_t ttl) {
          sendMplsIngressPacket(kTopLabel, ttl, isV4, injectPort);
        });
  }
};

TYPED_TEST_SUITE(AgentMPLSMidpointTest, MplsMidpointPortTypes);

// PushLabel verifies MPLS midpoint PUSH behavior across:
// - IPv4 and IPv6 payloads carried inside the injected MPLS packet.
// - Front-panel and CPU-switched injection paths.
// - Physical-port and single-port LAG nexthops.
// - First-pass egress forwarding to prove the route imposed labels.
// - Trapped packet label-stack inspection:
//   - If src-port ACL is supported, trap packets looped back from the egress
//     port because those packets have already completed the PUSH pass.
//   - Otherwise, use MPLS TTL expiry on the second pass: the looped packet
//     routes again with router MAC, matches the pushed label, expires TTL, and
//     reaches CPU through the MPLS TTL trap.
// Note: the TTL-expiry fallback is needed because programming a simple "trap
// MPLS packet to CPU" inSegEntry does not work and needs an SAI SDK fix.
TYPED_TEST(AgentMPLSMidpointTest, PushLabel) {
  auto setup = [this]() {
    this->setupStaticMplsRoutePush(this->singlePushedLabelStack());
  };

  auto verify = [this]() {
    auto pushStack = this->singlePushedLabelStack();
    const std::array<std::optional<PortID>, 2> injectPorts{
        std::nullopt,
        std::optional<PortID>{this->ingressPort()},
    };
    for (bool isV4 : {false, true}) {
      for (auto injectPort : injectPorts) {
        this->verifyMplsPushAndTrapPacket(isV4, injectPort, pushStack);
      }
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

// PushLabelAfterLinkFlap verifies that an already-programmed MPLS PUSH route
// converges after its egress nexthop link flaps:
// - Program and verify the normal MPLS midpoint PUSH route.
// - Bring the egress link down and unresolve the nexthop.
// - Bring the egress link up and re-resolve the nexthop.
// - Verify PUSH dataplane forwarding and trapped pushed-label inspection.
TYPED_TEST(AgentMPLSMidpointTest, PushLabelAfterLinkFlap) {
  auto setup = [this]() {
    this->setupStaticMplsRoutePush(this->singlePushedLabelStack());
    this->flapEgressPortAndReresolveNextHop();
  };

  auto verify = [this]() {
    auto pushStack = this->singlePushedLabelStack();
    const std::array<std::optional<PortID>, 2> injectPorts{
        std::nullopt,
        std::optional<PortID>{this->ingressPort()},
    };
    for (auto injectPort : injectPorts) {
      this->verifyMplsPushAndTrapPacket(
          false /* isV4 */, injectPort, pushStack);
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

// PushMaxLabelStack verifies that midpoint PUSH can impose the maximum label
// depth reported by the ASIC and that the captured packet carries the full
// stack in wire/top-first order.
TYPED_TEST(AgentMPLSMidpointTest, PushMaxLabelStack) {
  auto setup = [this]() {
    this->setupStaticMplsRoutePush(this->maxPushedLabelStack());
  };

  auto verify = [this]() {
    auto pushStack = this->maxPushedLabelStack();
    const std::array<std::optional<PortID>, 2> injectPorts{
        std::nullopt,
        std::optional<PortID>{this->ingressPort()},
    };
    for (bool isV4 : {false, true}) {
      for (auto injectPort : injectPorts) {
        this->verifyMplsPushAndTrapPacket(isV4, injectPort, pushStack);
      }
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
