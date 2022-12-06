// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestMirrorUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestStatUtils.h"
#include "fboss/agent/hw/test/HwTestTrunkUtils.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/types.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>

namespace {
template <typename AddrT>
struct TestParams {
  AddrT senderIp;
  AddrT receiverIp;
  AddrT mirrorDestinationIp;

  TestParams(
      const AddrT& _senderIp,
      const AddrT& _receiverIp,
      const AddrT& _mirrorDestinationIp)
      : senderIp(_senderIp),
        receiverIp(_receiverIp),
        mirrorDestinationIp(_mirrorDestinationIp) {}
};

template <typename AddrT>
TestParams<AddrT> getTestParams();

template <>
TestParams<folly::IPAddressV4> getTestParams<folly::IPAddressV4>() {
  return TestParams<folly::IPAddressV4>(
      folly::IPAddressV4("101.0.0.10"), // sender
      folly::IPAddressV4("201.0.0.10"), // receiver
      folly::IPAddressV4("101.0.0.11")); // erspan destination
}

template <>
TestParams<folly::IPAddressV6> getTestParams<folly::IPAddressV6>() {
  return TestParams<folly::IPAddressV6>(
      folly::IPAddressV6("101::10"), // sender
      folly::IPAddressV6("201::10"), // receiver
      folly::IPAddressV6("101::11")); // erspan destination
}

using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;
const std::string kSpan = "span";
const std::string kErspan = "erspan";
} // namespace

namespace facebook::fboss {

template <typename AddrT>
class HwDataPlaneMirrorTest : public HwLinkStateDependentTest {
 public:
  using EcmpSetupAnyNPortsT = typename utility::EcmpSetupAnyNPorts<AddrT>;
  using EcmpSetupTargetedPortsT =
      typename utility::EcmpSetupTargetedPorts<AddrT>;

  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ =
        std::make_unique<EcmpSetupAnyNPortsT>(getProgrammedState(), kRid);
    trafficPort_ = ecmpHelper_->nhop(0).portDesc.phyPortID();
    mirrorToPort_ = ecmpHelper_->nhop(1).portDesc.phyPortID();
  }

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode(),
        true);
  }

  std::shared_ptr<Mirror> getSpanMirror() {
    return std::make_shared<Mirror>(
        kSpan,
        std::make_optional<PortID>(mirrorToPort_),
        std::optional<folly::IPAddress>());
  }

  std::shared_ptr<Mirror> getErSpanMirror() {
    auto params = getTestParams<AddrT>();
    auto mirror = std::make_shared<Mirror>(
        kErspan,
        std::make_optional(mirrorToPort_),
        std::make_optional(folly::IPAddress(params.mirrorDestinationIp)));
    mirror->setMirrorTunnel(MirrorTunnel(
        params.senderIp,
        params.mirrorDestinationIp,
        getPlatform()->getLocalMac(),
        folly::MacAddress("1:1:1:1:1:2")));
    return mirror;
  }

  void sendPackets(int count, size_t payloadSize = 1) {
    auto params = getTestParams<AddrT>();
    auto vlanId = VlanID(utility::kBaseVlanId);
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    std::vector<uint8_t> payload(payloadSize, 0xff);
    auto oldOutPkts = getPortOutPkts(getLatestPortStats(trafficPort_));
    auto i = 0;
    while (i < count) {
      auto pkt = utility::makeUDPTxPacket(
          getHwSwitch(),
          vlanId,
          intfMac,
          intfMac,
          params.senderIp,
          params.receiverIp,
          srcL4Port_,
          dstL4Port_,
          0,
          255,
          payload);
      getHwSwitch()->sendPacketSwitchedAsync(std::move(pkt));
      i++;
    }

    // ensure as many packets as expected have been sent
    getHwSwitchEnsemble()->waitPortStatsCondition(
        [oldOutPkts, count, trafficPort = trafficPort_](
            const auto& newPortStats) {
          auto iter = newPortStats.find(trafficPort);
          if (iter == newPortStats.end()) {
            return false;
          }
          auto& portStats = iter->second;
          auto newOutPkt = *portStats.outUnicastPkts_() +
              *portStats.outMulticastPkts_() + *portStats.outBroadcastPkts_();
          return (newOutPkt > oldOutPkts) &&
              ((newOutPkt - oldOutPkts) >= count);
        });
  }

  void setupDataPlaneWithMirror(
      const std::string& mirrorName,
      bool truncate = false) {
    resolveNeigborAndProgramRoutes(*ecmpHelper_, 1);
    auto state = getProgrammedState()->clone();
    auto mirrors = state->getMirrors()->clone();
    auto mirror = mirrorName == kSpan ? getSpanMirror() : getErSpanMirror();
    mirror->setTruncate(truncate);
    mirrors->addMirror(mirror);
    state->resetMirrors(mirrors);

    applyNewState(state);
  }

  void mirrorPort(const std::string& mirrorName) {
    auto ports = this->getProgrammedState()->getPorts()->clone();
    auto port = ports->getPort(trafficPort_)->clone();
    port->setIngressMirror(mirrorName);
    if (getHwSwitch()->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::EGRESS_MIRRORING)) {
      port->setEgressMirror(mirrorName);
    }
    ports->updateNode(port);
    auto state = this->getProgrammedState()->clone();
    state->resetPorts(ports);
    this->applyNewState(state);
  }

  void mirrorAcl(const std::string& mirrorName) {
    auto acl = std::make_shared<AclEntry>(201, std::string("acl0"));
    acl->setL4SrcPort(srcL4Port_);
    acl->setL4DstPort(dstL4Port_);
    acl->setDstPort(trafficPort_);
    acl->setProto(17 /* udp */);
    /*
     * The number of packets mirrorred through ACL is different in Native BCM
     * and BCM-SAI (CS00012203195). The number of packets mirrorred in Native
     * BCM is 2 whereas number of packets mirrored in BCM-SAI is 4.
     *
     * 1) Send packet from CPU and perform hw lookup in the ASIC
     * 2) Packet is routed and forwarded to Port 0
     * 3) Packet is mirrored (ingress and egress) to port 1
     * 4) For native BCM, the packet routed out of port 0 is looped back
     *    and dropped.
     *    For BCM-SAI, the packet routed out of port 0 is looped back
     *    but L2 forwarded, ACL is looked up which mirrors again (2
     *    more packets) and dropped due to same port check.
     *
     *  The reason for the difference is the packet in native BCM is
     *  treated as unknown unicast and the dst port lookup is not performed
     *  in IFP for unknown unicast packets. Whereas in BCM-SAI, we
     *  install a FDB entry for every neighbor entry. The FDB entry is hit
     *  which makes the packet to be treated as known unicast and the IFP
     *  is hit and the packets are mirrored again.
     *
     * In order to avoid the ACL lookup, we can add additional qualifiers.
     * - SRC PORT: can be used once TAJO supports it.
     * - DMAC: DMAC can be used since the routed packet will have a different
     *   DMAC. Not all platforms support DMAC as a qualifier (For eg, Tajo)
     * - DROP Bit: WB + Fitting implications
     * - TTL: Use TTL 255 to ensure this entry is hit only for the first packet.
     *
     * For now, using TTL qualifier to ensure the routed packet is dropped. We
     * can update the test to include source port qualifier with CPU port
     * and enable it on TAJO platform.
     */
    acl->setTtl(AclTtl(255, 0xFF));
    acl->setActionType(cfg::AclActionType::PERMIT);
    MatchAction action;
    action.setIngressMirror(mirrorName);
    if (getHwSwitch()->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::EGRESS_MIRRORING)) {
      action.setEgressMirror(mirrorName);
    }
    acl->setAclAction(action);

    auto state = this->getProgrammedState()->clone();
    auto acls = state->getAcls()->modify(&state);
    acls->addNode(acl);
    this->applyNewState(state);
  }

  void verify(const std::string& mirrorName, int payloadSize = 500) {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(mirrorName);
    ASSERT_NE(mirror, nullptr);
    EXPECT_EQ(mirror->isResolved(), true);

    auto trafficPortPktsBefore =
        getPortOutPkts(getLatestPortStats(trafficPort_));
    auto mirroredPortPktsBefore =
        getPortOutPkts(getLatestPortStats(mirrorToPort_));

    this->sendPackets(1, payloadSize);

    auto trafficPortPktsAfter =
        getPortOutPkts(getLatestPortStats(trafficPort_));
    auto mirroredPortPktsAfter =
        getPortOutPkts(getLatestPortStats(mirrorToPort_));

    EXPECT_EQ(trafficPortPktsAfter - trafficPortPktsBefore, 1);
    /*
     * Port mirror :
     * 2 packets are mirrored one egressing  and other ingressing the port
     * because of loopback mode
     * Acl mirror:
     * 2 packets are mirrored one ingressing IFP and other egressing IFP
     */
    auto expectedMirrorPackets = 1;
    if (this->getHwSwitch()->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::EGRESS_MIRRORING)) {
      expectedMirrorPackets += 1;
    }
    EXPECT_EQ(
        mirroredPortPktsAfter - mirroredPortPktsBefore, expectedMirrorPackets);
  }

  void testPortMirror(const std::string& mirrorName) {
    auto setup = [=]() {
      this->setupDataPlaneWithMirror(mirrorName);
      this->mirrorPort(mirrorName);
    };
    auto verify = [=]() { this->verify(mirrorName); };
    this->verifyAcrossWarmBoots(setup, verify);
  }

  void testAclMirror(const std::string& mirrorName) {
    auto setup = [=]() {
      this->setupDataPlaneWithMirror(mirrorName);
      this->mirrorAcl(mirrorName);
    };
    auto verify = [=]() { this->verify(mirrorName); };
    this->verifyAcrossWarmBoots(setup, verify);
  }

  void testPortMirrorWithLargePacket(const std::string& mirrorName) {
    auto setup = [=]() {
      this->setupDataPlaneWithMirror(mirrorName, true /* truncate */);
      this->mirrorPort(mirrorName);
    };
    auto verify = [=]() {
      auto statsBefore = getLatestPortStats(mirrorToPort_);
      this->verify(mirrorName, 8000);
      auto statsAfter = getLatestPortStats(mirrorToPort_);

      auto outBytes = (*statsAfter.outBytes_() - *statsBefore.outBytes_());
      // mirror is on both ingress and egress, packet loops back and gets
      // mirrored twice
      if (getHwSwitch()->getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::EGRESS_MIRRORING)) {
        outBytes = outBytes / 2;
      }
      // TODO: on TH3 for v6 packets, 254 bytes are mirrored which is a single
      // MMU cell. but for v4 packets, 234 bytes are mirrored. need to
      // investigate this behavior.
      EXPECT_LE(
          outBytes, 1500 /* payload is truncated to ethernet MTU of 1500 */);
    };
    this->verifyAcrossWarmBoots(setup, verify);
  }

  template <typename T = AddrT>
  bool skipTest() const {
    return std::is_same<T, folly::IPAddressV6>::value &&
        !getPlatform()->getAsic()->isSupported(HwAsic::Feature::ERSPANv6);
  }

  const RouterID kRid{0};
  PortID trafficPort_;
  PortID mirrorToPort_;
  const uint16_t srcL4Port_{1234};
  const uint16_t dstL4Port_{4321};
  std::unique_ptr<EcmpSetupAnyNPortsT> ecmpHelper_;
};

TYPED_TEST_SUITE(HwDataPlaneMirrorTest, TestTypes);

TYPED_TEST(HwDataPlaneMirrorTest, SpanPortMirror) {
  this->testPortMirror(kSpan);
}

TYPED_TEST(HwDataPlaneMirrorTest, ErspanPortMirror) {
  if (this->skipTest()) {
    return;
  }
  this->testPortMirror(kErspan);
}

TYPED_TEST(HwDataPlaneMirrorTest, SpanAclMirror) {
  this->testAclMirror(kSpan);
}

TYPED_TEST(HwDataPlaneMirrorTest, ErspanAclMirror) {
  if (this->skipTest()) {
    return;
  }
  this->testAclMirror(kErspan);
}

TYPED_TEST(HwDataPlaneMirrorTest, TrucatePortErspanMirror) {
  if (this->skipTest() ||
      !this->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::MIRROR_PACKET_TRUNCATION)) {
    return;
  }
  this->testPortMirrorWithLargePacket(kErspan);
}

TYPED_TEST(HwDataPlaneMirrorTest, ErspanMirrorWithLagMember) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    auto config = this->initialConfig();

    utility::addAggPort(AggregatePortID(1), {this->mirrorToPort_}, &config);
    auto state0 = this->applyNewConfig(config);
    state0 = utility::setTrunkMinLinkCount(state0, 1);
    state0 = utility::enableTrunkPorts(state0);
    this->applyNewState(state0);

    this->setupDataPlaneWithMirror(kErspan);
    this->mirrorPort(kErspan);
  };
  auto verify = [=]() {
    utility::verifyAggregatePortCount(this->getHwSwitchEnsemble(), 1);
    utility::verifyAggregatePort(
        this->getHwSwitchEnsemble(), AggregatePortID(1));
    utility::verifyAggregatePortMemberCount(
        this->getHwSwitchEnsemble(), AggregatePortID(1), 1, 1);

    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 1);

    utility::verifyPortMirrorDestination(
        this->getHwSwitch(),
        this->trafficPort_,
        utility::getMirrorPortIngressFlags(),
        destinations[0]);

    if (this->getHwSwitch()->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::EGRESS_MIRRORING)) {
      utility::verifyPortMirrorDestination(
          this->getHwSwitch(),
          this->trafficPort_,
          utility::getMirrorPortEgressFlags(),
          destinations[0]);
    }
    this->verify(kErspan);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
