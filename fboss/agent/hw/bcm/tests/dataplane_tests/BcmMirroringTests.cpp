// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestStatUtils.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

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
class BcmDataPlaneMirrorTest : public BcmLinkStateDependentTests {
 public:
  using EcmpSetupAnyNPortsT = typename utility::EcmpSetupAnyNPorts<AddrT>;
  void SetUp() override {
    BcmLinkStateDependentTests::SetUp();
    ecmpHelper_ =
        std::make_unique<EcmpSetupAnyNPortsT>(getProgrammedState(), kRid);
    trafficPort_ = ecmpHelper_->nhop(0).portDesc.phyPortID();
    mirrorToPort_ = ecmpHelper_->nhop(1).portDesc.phyPortID();
  }

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        cfg::PortLoopbackMode::MAC,
        true);
  }

  std::shared_ptr<Mirror> getSpanMirror() {
    return std::make_shared<Mirror>(
        kSpan, std::make_optional(mirrorToPort_), std::nullopt);
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
    while (count--) {
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
      getHwSwitch()->sendPacketSwitchedSync(std::move(pkt));
    }
  }

  void setupDataPlaneWithMirror(
      const std::string& mirrorName,
      bool truncate = false) {
    auto mirrors = getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrorName == kSpan ? getSpanMirror() : getErSpanMirror();
    mirror->setTruncate(truncate);
    mirrors->addMirror(mirror);

    auto state = ecmpHelper_->setupECMPForwarding(
        ecmpHelper_->resolveNextHops(getProgrammedState(), 1), 1);
    state->resetMirrors(mirrors);

    applyNewState(state);
  }

  void mirrorPort(const std::string& mirrorName) {
    auto ports = this->getProgrammedState()->getPorts()->clone();
    auto port = ports->getPort(trafficPort_)->clone();
    port->setIngressMirror(mirrorName);
    port->setEgressMirror(mirrorName);
    ports->updateNode(port);
    auto state = this->getProgrammedState()->clone();
    state->resetPorts(ports);
    this->applyNewState(state);
  }

  void mirrorAcl(const std::string& mirrorName) {
    auto acl = std::make_shared<AclEntry>(201, "acl0");
    acl->setL4SrcPort(srcL4Port_);
    acl->setL4DstPort(dstL4Port_);
    acl->setDstPort(trafficPort_);
    acl->setProto(17 /* udp */);
    acl->setActionType(cfg::AclActionType::PERMIT);
    MatchAction action;
    action.setIngressMirror(mirrorName);
    action.setEgressMirror(mirrorName);
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
    EXPECT_EQ(mirroredPortPktsAfter - mirroredPortPktsBefore, 2);
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
      // mirror is on both ingress and egress, packet loops back and gets
      // mirrored twice
      auto outBytes =
          (*statsAfter.outBytes__ref() - *statsBefore.outBytes__ref()) / 2;
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
        !getPlatform()->v6MirrorTunnelSupported();
  }

  const RouterID kRid{0};
  PortID trafficPort_;
  PortID mirrorToPort_;
  const uint16_t srcL4Port_{1234};
  const uint16_t dstL4Port_{4321};
  std::unique_ptr<EcmpSetupAnyNPortsT> ecmpHelper_;
};

TYPED_TEST_SUITE(BcmDataPlaneMirrorTest, TestTypes);

TYPED_TEST(BcmDataPlaneMirrorTest, SpanPortMirror) {
  this->testPortMirror(kSpan);
}

TYPED_TEST(BcmDataPlaneMirrorTest, ErspanPortMirror) {
  if (this->skipTest()) {
    return;
  }
  this->testPortMirror(kErspan);
}

TYPED_TEST(BcmDataPlaneMirrorTest, SpanAclMirror) {
  this->testAclMirror(kSpan);
}

TYPED_TEST(BcmDataPlaneMirrorTest, ErspanAclMirror) {
  if (this->skipTest()) {
    return;
  }
  this->testAclMirror(kErspan);
}

TYPED_TEST(BcmDataPlaneMirrorTest, TrucatePortErspanMirror) {
  if (this->skipTest() ||
      !this->getPlatform()->mirrorPktTruncationSupported()) {
    return;
  }
  this->testPortMirrorWithLargePacket(kErspan);
}

} // namespace facebook::fboss
