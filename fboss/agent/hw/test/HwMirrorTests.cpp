/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestMirrorUtils.h"

#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/test/TrunkUtils.h"

namespace facebook::fboss {

namespace {
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
constexpr auto kSpan = "local"; // SPAN mirror
constexpr auto kErspan = "mirror0"; // erspan mirror
constexpr auto kSflow = "mirror1"; // sflow mirror
constexpr auto kDscpDefault =
    cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_; // default dscp value
using TestTypes = ::testing::Types<IPAddressV4, folly::IPAddressV6>;

template <typename AddrT>
struct TestParams {
  std::array<AddrT, 4> ipAddrs;
  std::array<MacAddress, 4> macAddrs;
  TestParams();
};

template <>
TestParams<IPAddressV4>::TestParams()
    : ipAddrs{IPAddressV4("1.1.1.1"), IPAddressV4("1.1.1.10"), IPAddressV4("2.2.2.2"), IPAddressV4("2.2.2.10")},
      macAddrs{
          MacAddress("1:1:1:1:1:1"),
          MacAddress("1:1:1:1:1:10"),
          MacAddress("2:2:2:2:2:2"),
          MacAddress("2:2:2:2:2:10"),
      } {}

template <>
TestParams<IPAddressV6>::TestParams()
    : ipAddrs{IPAddressV6("1::1"), IPAddressV6("1:1::1:10"), IPAddressV6("2::1"), IPAddressV6("2:2::2:10")},
      macAddrs{
          MacAddress("1:1:1:1:1:1"),
          MacAddress("1:1:1:1:1:10"),
          MacAddress("2:2:2:2:2:2"),
          MacAddress("2:2:2:2:2:10"),
      } {}
} // namespace

template <typename AddrT>
class HwMirrorTest : public HwTest {
  using IPAddressV4 = folly::IPAddressV4;
  using IPAddressV6 = folly::IPAddressV6;

 public:
  void SetUp() override {
    HwTest::SetUp();
  }

 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
  }

  void updateMirror(const std::shared_ptr<Mirror>& mirror) {
    auto newState = this->getProgrammedState()->clone();
    auto mnpuMirrors =
        this->getProgrammedState()->getMirrors()->modify(&newState);
    mnpuMirrors->updateNode(mirror, scopeResolver().scope(mirror));
    this->applyNewState(newState);
  }
  void updateMirrors(const std::vector<std::shared_ptr<Mirror>>& mirrors) {
    for (const auto& mirror : mirrors) {
      updateMirror(mirror);
    }
  }

  cfg::Mirror getSpanMirror(
      const std::string& mirrorName = kSpan,
      const uint8_t dscp = kDscpDefault,
      const bool truncate = false) const {
    cfg::Mirror mirror;
    cfg::MirrorDestination destination;
    destination.egressPort() = cfg::MirrorEgressPort();
    destination.egressPort()->logicalID_ref() = masterLogicalPortIds()[0];

    cfg::Mirror mirrorConfig;
    mirrorConfig.name() = mirrorName;
    mirrorConfig.destination() = destination;
    mirrorConfig.dscp() = dscp;
    mirrorConfig.truncate() = truncate;
    return mirrorConfig;
  }

  cfg::Mirror getErspanMirror(
      const std::string& mirrorName = kErspan,
      const uint8_t dscp = kDscpDefault) const {
    cfg::Mirror mirror;
    auto params = testParams();
    auto destinationIp = params.ipAddrs[1];

    cfg::MirrorDestination destination;
    cfg::MirrorTunnel tunnel;
    cfg::GreTunnel greTunnel;
    greTunnel.ip() = destinationIp.str();
    tunnel.greTunnel() = greTunnel;
    destination.tunnel() = tunnel;

    cfg::Mirror mirrorConfig;
    mirrorConfig.name() = mirrorName;
    mirrorConfig.destination() = destination;
    mirrorConfig.dscp() = dscp;
    return mirrorConfig;
  }

  cfg::Mirror getSflowMirror(
      const std::string& mirrorName = kSflow,
      const uint8_t dscp = kDscpDefault) const {
    cfg::Mirror mirror;
    auto params = testParams();
    auto destinationIp = params.ipAddrs[1];

    cfg::MirrorDestination destination;
    cfg::MirrorTunnel tunnel;
    cfg::SflowTunnel sflowTunnel;
    sflowTunnel.ip() = destinationIp.str();
    sflowTunnel.udpSrcPort() = 6545;
    sflowTunnel.udpDstPort() = 6343;
    tunnel.sflowTunnel() = sflowTunnel;

    destination.tunnel() = tunnel;

    cfg::Mirror mirrorConfig;
    mirrorConfig.name() = mirrorName;
    mirrorConfig.destination() = destination;
    mirrorConfig.dscp() = dscp;
    return mirrorConfig;
  }

  TestParams<AddrT> testParams() const {
    return TestParams<AddrT>();
  }

  template <typename T = AddrT>
  bool skipMirrorTest() const {
    return std::is_same<T, folly::IPAddressV6>::value &&
        !getPlatform()->getAsic()->isSupported(HwAsic::Feature::ERSPANv6) &&
        !getPlatform()->getAsic()->isSupported(HwAsic::Feature::SFLOWv6);
  }

  template <typename T = AddrT>
  bool skipSflowTest() {
    return std::is_same<T, folly::IPAddressV6>::value
        ? !isSupported(HwAsic::Feature::SFLOWv6)
        : !isSupported(HwAsic::Feature::SFLOWv4);
  }

  void addAclMirror(
      const std::string& mirror,
      const cfg::AclEntry& acl,
      cfg::SwitchConfig& cfg) {
    cfg.acls()->push_back(acl);
    cfg::MatchAction mirrorAction;
    mirrorAction.ingressMirror() = mirror;
    mirrorAction.egressMirror() = mirror;

    cfg::MatchToAction match2Action;
    match2Action.matcher() = *acl.name();
    match2Action.action() = mirrorAction;

    if (!cfg.dataPlaneTrafficPolicy()) {
      cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
      dataPlaneTrafficPolicy.matchToAction()->push_back(match2Action);
      cfg.dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
    } else {
      cfg.dataPlaneTrafficPolicy()->matchToAction()->push_back(match2Action);
    }
  }
};

TYPED_TEST_SUITE(HwMirrorTest, TestTypes);

TYPED_TEST(HwMirrorTest, UnresolvedToUnresolvedUpdate) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors()->push_back(this->getSpanMirror());
    cfg.mirrors()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        std::optional<folly::IPAddress>(folly::IPAddress(params.ipAddrs[3])));
    this->updateMirror(newMirror);
  };
  auto verify = [=, this]() {
    auto local = this->getProgrammedState()->getMirrors()->getNodeIf(kSpan);
    utility::verifyResolvedMirror(this->getHwSwitch(), local);
    auto erspan = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    utility::verifyUnResolvedMirror(this->getHwSwitch(), erspan);
  };
  if (this->skipMirrorTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, ResolvedToResolvedUpdate) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);

    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    auto resolvedMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp());
    resolvedMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    resolvedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1]));
    this->updateMirror(resolvedMirror);

    mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    auto updatedMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp());
    updatedMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[1]));
    updatedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    this->updateMirror(updatedMirror);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
  };
  if (this->skipMirrorTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, ResolvedToUnresolvedUpdate) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);

    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    auto resolvedMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp());
    resolvedMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    resolvedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1]));
    this->updateMirror(resolvedMirror);

    mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    auto updatedMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp());
    this->updateMirror(updatedMirror);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    utility::verifyUnResolvedMirror(this->getHwSwitch(), mirror);
  };
  if (this->skipMirrorTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, NoPortMirroringIfUnResolved) {
  auto setup = [=, this]() {
    auto cfg = this->initialConfig();
    cfg.mirrors()->push_back(this->getErspanMirror());
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[0]);
    portCfg->ingressMirror() = kErspan;
    portCfg->egressMirror() = kErspan;
    this->applyNewConfig(cfg);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    utility::verifyUnResolvedMirror(this->getHwSwitch(), mirror);
    utility::verifyPortNoMirrorDestination(
        this->getHwSwitch(),
        PortID(this->masterLogicalPortIds()[0]),
        utility::getMirrorPortIngressFlags());
    utility::verifyPortNoMirrorDestination(
        this->getHwSwitch(),
        PortID(this->masterLogicalPortIds()[0]),
        utility::getMirrorPortEgressFlags());
  };
  if (this->skipMirrorTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, PortMirrorUpdateIfMirrorUpdate) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors()->push_back(this->getErspanMirror());
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[0]);
    portCfg->ingressMirror() = kErspan;
    portCfg->egressMirror() = kErspan;
    this->applyNewConfig(cfg);

    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    auto updatedMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp());
    updatedMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[1]));
    updatedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    this->updateMirror(updatedMirror);

    mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    updatedMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp());
    updatedMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[1]));
    updatedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[3],
        params.ipAddrs[2],
        params.macAddrs[3],
        params.macAddrs[2]));
    this->updateMirror(updatedMirror);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);

    ASSERT_EQ(destinations.size(), 1);
    utility::verifyPortMirrorDestination(
        this->getHwSwitch(),
        PortID(this->masterLogicalPortIds()[0]),
        utility::getMirrorPortIngressFlags(),
        destinations[0]);
  };
  if (this->skipMirrorTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, HwMirrorStat) {
  auto setup = [=, this]() {
    auto cfg = this->initialConfig();
    cfg.mirrors()->push_back(this->getSpanMirror());
    cfg.mirrors()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);
  };
  auto verify = [=, this]() {
    auto stats = utility::getHwTableStats(this->getHwSwitch());
    EXPECT_EQ(*stats.mirrors_used(), 1);
    EXPECT_EQ(*stats.mirrors_span(), 1);
    EXPECT_EQ(*stats.mirrors_erspan(), 0);
  };
  if (this->skipMirrorTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, HwResolvedMirrorStat) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors()->resize(3);
    cfg.mirrors()[0] = this->getSpanMirror();
    cfg.mirrors()[1] = this->getErspanMirror();
    cfg.mirrors()[2] = this->getSflowMirror();
    this->applyNewConfig(cfg);

    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[1]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1]));
    this->updateMirror(newMirror);

    auto sflowMirror =
        this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    auto newSflowMirror = std::make_shared<Mirror>(
        sflowMirror->getID(),
        sflowMirror->getEgressPortDesc(),
        sflowMirror->getDestinationIp(),
        sflowMirror->getSrcIp(),
        sflowMirror->getTunnelUdpPorts());
    newSflowMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[1]));
    newSflowMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        sflowMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newSflowMirror);
  };
  auto verify = [=, this]() {
    auto stats = utility::getHwTableStats(this->getHwSwitch());
    auto span = this->getProgrammedState()->getMirrors()->getNodeIf(kSpan);
    auto erspan = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);

    auto sflowMirror =
        this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    EXPECT_TRUE(span->isResolved());
    EXPECT_TRUE(erspan->isResolved());
    EXPECT_TRUE(sflowMirror->isResolved());

    EXPECT_EQ(*stats.mirrors_used(), 3);
    EXPECT_EQ(*stats.mirrors_span(), 1);
    EXPECT_EQ(*stats.mirrors_erspan(), 1);
    EXPECT_EQ(*stats.mirrors_sflow(), 1);
  };
  if (this->skipMirrorTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, HwUnresolvedMirrorStat) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors()->resize(3);
    cfg.mirrors()[0] = this->getSpanMirror();
    cfg.mirrors()[1] = this->getErspanMirror();
    cfg.mirrors()[2] = this->getSflowMirror();
    this->applyNewConfig(cfg);

    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    auto resolvedMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp());
    resolvedMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[1]));
    resolvedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1]));
    this->updateMirror(resolvedMirror);

    auto sflowMirror =
        this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    auto newSflowMirror = std::make_shared<Mirror>(
        sflowMirror->getID(),
        sflowMirror->getEgressPortDesc(),
        sflowMirror->getDestinationIp(),
        sflowMirror->getSrcIp(),
        sflowMirror->getTunnelUdpPorts());
    newSflowMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[1]));
    newSflowMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newSflowMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newSflowMirror);

    auto unresolvedMirror = std::make_shared<Mirror>(
        resolvedMirror->getID(),
        resolvedMirror->getEgressPortDesc(),
        resolvedMirror->getDestinationIp());
    EXPECT_TRUE(!unresolvedMirror->isResolved());
    this->updateMirror(unresolvedMirror);
    auto unresolvedSflowMirror = std::make_shared<Mirror>(
        newSflowMirror->getID(),
        newSflowMirror->getEgressPortDesc(),
        newSflowMirror->getDestinationIp());
    EXPECT_TRUE(!unresolvedSflowMirror->isResolved());
    this->updateMirror(unresolvedSflowMirror);
  };
  auto verify = [=, this]() {
    auto stats = utility::getHwTableStats(this->getHwSwitch());
    auto span = this->getProgrammedState()->getMirrors()->getNodeIf(kSpan);
    auto erspan = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    auto sflowMirror =
        this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    EXPECT_TRUE(span->isResolved());
    EXPECT_TRUE(!erspan->isResolved());
    EXPECT_EQ(*stats.mirrors_used(), 1);
    EXPECT_EQ(*stats.mirrors_span(), 1);
    EXPECT_EQ(*stats.mirrors_erspan(), 0);
    EXPECT_EQ(*stats.mirrors_sflow(), 0);
  };
  if (this->skipMirrorTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, HwMirrorLimitExceeded) {
  if (this->skipMirrorTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto cfg = this->initialConfig();
  auto oldState = this->applyNewConfig(cfg);

  cfg.mirrors()->resize(5);
  cfg.mirrors()[0] = this->getErspanMirror("mirror0");
  cfg.mirrors()[1] = this->getErspanMirror("mirror1");
  cfg.mirrors()[2] = this->getErspanMirror("mirror2");
  cfg.mirrors()[3] = this->getErspanMirror("mirror3");
  cfg.mirrors()[4] = this->getErspanMirror("mirror4");

  auto newState = this->applyNewConfig(cfg);

  EXPECT_FALSE(
      this->getHwSwitch()->isValidStateUpdate(StateDelta(oldState, newState)));
}

TYPED_TEST(HwMirrorTest, SampleOnePort) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling one port and send traffic to sflow mirror */
    cfg.mirrors()->push_back(this->getSflowMirror());
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[1]);
    portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
    *portCfg->sFlowIngressRate() = 90000;
    this->applyNewConfig(cfg);

    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    auto mirror = mirrors->getNodeIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newMirror);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 1);
    utility::verifyPortMirrorDestination(
        this->getHwSwitch(),
        PortID(this->masterLogicalPortIds()[1]),
        utility::getMirrorPortIngressAndSflowFlags(),
        destinations[0]);
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, SampleAllPorts) {
  /* Setup sample destination to mirror for all ports, and mirror only one port
  this will ensure all port traffic is sampled and sent to that mirror */
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    auto mirror = mirrors->getNodeIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newMirror);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 1);
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags(),
          destinations[0]);
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, SflowMirrorWithErspanMirror) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors()->push_back(this->getSflowMirror());
    cfg.mirrors()->push_back(this->getErspanMirror());

    for (auto i = 0; i < 2; i++) {
      auto portId = this->masterLogicalPortIds()[i];
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[2]);
    portCfg->ingressMirror() = *cfg.mirrors()[1].name();
    portCfg->egressMirror() = *cfg.mirrors()[1].name();
    this->applyNewConfig(cfg);
    // resolve both mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    std::vector<std::shared_ptr<Mirror>> updatedMirrors;
    for (auto mniter = mirrors->cbegin(); mniter != mirrors->cend(); ++mniter) {
      for (auto iter : std::as_const(*mniter->second)) {
        auto mirror = iter.second;
        auto newMirror = mirror->getTunnelUdpPorts()
            ? std::make_shared<Mirror>(
                  mirror->getID(),
                  mirror->getEgressPortDesc(),
                  mirror->getDestinationIp(),
                  mirror->getSrcIp(),
                  mirror->getTunnelUdpPorts())
            : std::make_shared<Mirror>(
                  mirror->getID(),
                  mirror->getEgressPortDesc(),
                  mirror->getDestinationIp(),
                  mirror->getSrcIp());
        newMirror->setEgressPortDesc(
            PortDescriptor(this->masterLogicalPortIds()[0]));
        newMirror->setMirrorTunnel(
            newMirror->getTunnelUdpPorts()
                ? MirrorTunnel(
                      params.ipAddrs[0],
                      params.ipAddrs[1],
                      params.macAddrs[0],
                      params.macAddrs[1],
                      newMirror->getTunnelUdpPorts().value())
                : MirrorTunnel(
                      params.ipAddrs[0],
                      params.ipAddrs[1],
                      params.macAddrs[0],
                      params.macAddrs[1]));
        updatedMirrors.push_back(newMirror);
      }
    }
    this->updateMirrors(updatedMirrors);
  };
  auto verify = [=, this]() {
    for (auto mniter :
         std::as_const(*this->getProgrammedState()->getMirrors())) {
      for (auto iter : std::as_const(*mniter.second)) {
        auto mirror = iter.second;
        utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
      }
    }
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 2);
    uint64_t sflow = 0;
    uint64_t erspan = 0;
    for (auto destination : destinations) {
      if (utility::isMirrorSflowTunnelEnabled(
              this->getHwSwitch(), destination)) {
        sflow = destination;
      } else {
        erspan = destination;
      }
    }
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags(),
          sflow);
    }
    utility::verifyPortMirrorDestination(
        this->getHwSwitch(),
        this->masterLogicalPortIds()[2],
        utility::getMirrorPortIngressFlags(),
        erspan);
    utility::verifyPortMirrorDestination(
        this->getHwSwitch(),
        this->masterLogicalPortIds()[2],
        utility::getMirrorPortEgressFlags(),
        erspan);
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, SflowMirrorWithErspanMirrorOnePortSflow) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors()->push_back(this->getSflowMirror());
    cfg.mirrors()->push_back(this->getErspanMirror());

    for (auto i = 0; i < 2; i++) {
      auto portId = this->masterLogicalPortIds()[i];
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[2]);
    portCfg->ingressMirror() = *cfg.mirrors()[1].name();
    portCfg->egressMirror() = *cfg.mirrors()[1].name();
    this->applyNewConfig(cfg);
    // resolve both mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    std::vector<std::shared_ptr<Mirror>> updatedMirrors;
    for (auto mniter = mirrors->cbegin(); mniter != mirrors->cend(); ++mniter) {
      for (auto iter : std::as_const(*mniter->second)) {
        auto mirror = iter.second;
        auto newMirror = mirror->getTunnelUdpPorts()
            ? std::make_shared<Mirror>(
                  mirror->getID(),
                  mirror->getEgressPortDesc(),
                  mirror->getDestinationIp(),
                  mirror->getSrcIp(),
                  mirror->getTunnelUdpPorts())
            : std::make_shared<Mirror>(
                  mirror->getID(),
                  mirror->getEgressPortDesc(),
                  mirror->getDestinationIp(),
                  mirror->getSrcIp());
        newMirror->setEgressPortDesc(
            PortDescriptor(this->masterLogicalPortIds()[0]));
        newMirror->setMirrorTunnel(
            newMirror->getTunnelUdpPorts()
                ? MirrorTunnel(
                      params.ipAddrs[0],
                      params.ipAddrs[1],
                      params.macAddrs[0],
                      params.macAddrs[1],
                      newMirror->getTunnelUdpPorts().value())
                : MirrorTunnel(
                      params.ipAddrs[0],
                      params.ipAddrs[1],
                      params.macAddrs[0],
                      params.macAddrs[1]));
        updatedMirrors.push_back(std::move(newMirror));
      }
    }
    this->updateMirrors(updatedMirrors);

    portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[1]);
    portCfg->sampleDest() = cfg::SampleDestination::CPU;
    *portCfg->sFlowIngressRate() = 0;
    portCfg->ingressMirror().reset();
    this->applyNewConfig(cfg);
  };
  auto verify = [=, this]() {
    for (auto mniter :
         std::as_const(*this->getProgrammedState()->getMirrors())) {
      for (auto iter : std::as_const(*mniter.second)) {
        auto mirror = iter.second;
        utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
      }
    }
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 2);
    uint64_t sflow = 0;
    uint64_t erspan = 0;
    for (auto destination : destinations) {
      if (utility::isMirrorSflowTunnelEnabled(
              this->getHwSwitch(), destination)) {
        sflow = destination;
      } else {
        erspan = destination;
      }
    }
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags(),
          sflow);
    }
    utility::verifyPortMirrorDestination(
        this->getHwSwitch(),
        this->masterLogicalPortIds()[2],
        utility::getMirrorPortIngressFlags(),
        erspan);
    utility::verifyPortMirrorDestination(
        this->getHwSwitch(),
        this->masterLogicalPortIds()[2],
        utility::getMirrorPortEgressFlags(),
        erspan);
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, SflowMirrorWithErspanMirrorNoPortSflow) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors()->push_back(this->getSflowMirror());
    cfg.mirrors()->push_back(this->getErspanMirror());

    for (auto i = 0; i < 2; i++) {
      auto portId = this->masterLogicalPortIds()[i];
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[2]);
    portCfg->ingressMirror() = *cfg.mirrors()[1].name();
    portCfg->egressMirror() = *cfg.mirrors()[1].name();
    this->applyNewConfig(cfg);
    // resolve both mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    std::vector<std::shared_ptr<Mirror>> updatedMirrors;
    for (auto mniter = mirrors->cbegin(); mniter != mirrors->cend(); ++mniter) {
      for (auto iter : std::as_const(*mniter->second)) {
        auto mirror = iter.second;
        auto newMirror = mirror->getTunnelUdpPorts()
            ? std::make_shared<Mirror>(
                  mirror->getID(),
                  mirror->getEgressPortDesc(),
                  mirror->getDestinationIp(),
                  mirror->getSrcIp(),
                  mirror->getTunnelUdpPorts())
            : std::make_shared<Mirror>(
                  mirror->getID(),
                  mirror->getEgressPortDesc(),
                  mirror->getDestinationIp(),
                  mirror->getSrcIp());
        newMirror->setEgressPortDesc(
            PortDescriptor(this->masterLogicalPortIds()[0]));

        newMirror->setMirrorTunnel(
            newMirror->getTunnelUdpPorts()
                ? MirrorTunnel(
                      params.ipAddrs[0],
                      params.ipAddrs[1],
                      params.macAddrs[0],
                      params.macAddrs[1],
                      newMirror->getTunnelUdpPorts().value())
                : MirrorTunnel(
                      params.ipAddrs[0],
                      params.ipAddrs[1],
                      params.macAddrs[0],
                      params.macAddrs[1]));
        updatedMirrors.push_back(std::move(newMirror));
      }
    }
    XLOG(INFO) << " UPDATING MIRRORS";
    this->updateMirrors(updatedMirrors);
    for (auto i = 0; i < 2; i++) {
      auto portId = this->masterLogicalPortIds()[i];
      auto portConfig = utility::findCfgPort(cfg, portId);
      portConfig->sampleDest() = cfg::SampleDestination::CPU;
      *portConfig->sFlowIngressRate() = 0;
      portConfig->ingressMirror().reset();
    }
    this->applyNewConfig(cfg);
  };
  auto verify = [=, this]() {
    for (auto mniter :
         std::as_const(*this->getProgrammedState()->getMirrors())) {
      for (auto iter : std::as_const(*mniter.second)) {
        auto mirror = iter.second;
        utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
      }
    }
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 2);
    uint64_t erspan = 0;
    for (auto destination : destinations) {
      if (utility::isMirrorSflowTunnelEnabled(
              this->getHwSwitch(), destination)) {
        std::ignore = destination;
      } else {
        erspan = destination;
      }
    }
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortNoMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags());
    }
    utility::verifyPortMirrorDestination(
        this->getHwSwitch(),
        this->masterLogicalPortIds()[2],
        utility::getMirrorPortIngressFlags(),
        erspan);
    utility::verifyPortMirrorDestination(
        this->getHwSwitch(),
        this->masterLogicalPortIds()[2],
        utility::getMirrorPortEgressFlags(),
        erspan);
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, SampleAllPortsMirrorUnresolved) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    auto mirror = mirrors->getNodeIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newMirror);

    /* unresolve */
    mirrors = this->getProgrammedState()->getMirrors();
    mirror = mirrors->getNodeIf(kSflow);
    newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());

    this->updateMirror(newMirror);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    utility::verifyUnResolvedMirror(this->getHwSwitch(), mirror);
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 0); // no mirror found
    EXPECT_TRUE(mirror->getTunnelUdpPorts().has_value());
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortNoMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags()); // noo mirroring
                                                         // to port
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, SampleAllPortsMirrorUnresolvedResolved) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    auto mirror = mirrors->getNodeIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newMirror);

    /* unresolve */
    mirrors = this->getProgrammedState()->getMirrors();
    mirror = mirrors->getNodeIf(kSflow);
    newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());

    this->updateMirror(newMirror);

    /* reresolve */
    mirrors = this->getProgrammedState()->getMirrors();
    mirror = mirrors->getNodeIf(kSflow);
    newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newMirror);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 1);
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags(),
          destinations[0]);
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, SampleAllPortsMirrorUpdate) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    auto mirror = mirrors->getNodeIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newMirror);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 1);
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags(),
          destinations[0]);
    }
  };
  auto setupPostWb = [=, this] {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors()->push_back(this->getSflowMirror());
    // update to truncate if supported
    *cfg.mirrors()[0].truncate() = this->getPlatform()->getAsic()->isSupported(
        HwAsic::Feature::MIRROR_PACKET_TRUNCATION);
    // update destination port now
    cfg.mirrors()[0].destination()->tunnel()->sflowTunnel()->udpDstPort() =
        9898;
    /* sampling all ports and send traffic to sflow mirror */
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    auto mirror = mirrors->getNodeIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newMirror);
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify, setupPostWb, verify);
}

TYPED_TEST(HwMirrorTest, RemoveSampleAllPorts) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    auto mirror = mirrors->getNodeIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newMirror);

    /* reset all config */
    cfg = this->initialConfig();
    this->applyNewConfig(cfg);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    EXPECT_EQ(mirror, nullptr);
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 0);
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortNoMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags());
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, RemoveSampleAllPortsAfterWarmBoot) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    auto mirror = mirrors->getNodeIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newMirror);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 1);
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags(),
          destinations[0]);
    }
  };
  auto setupPostWb = [=, this]() {
    /* reset all config */
    this->applyNewConfig(this->initialConfig());
  };
  auto verifyPostWb = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    EXPECT_EQ(mirror, nullptr);
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 0);
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortNoMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags());
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify, setupPostWb, verifyPostWb);
}

TYPED_TEST(HwMirrorTest, SampleAllPortsReloadConfig) {
  /* Setup sample destination to mirror for all ports, and mirror only one port
  this will ensure all port traffic is sampled and sent to that mirror */
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate() = 90000;
      portCfg->ingressMirror() = *cfg.mirrors()[0].name();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors();
    auto mirror = mirrors->getNodeIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    this->updateMirror(newMirror);

    // reload config and verify mirror stays resolved
    this->applyNewConfig(cfg);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kSflow);
    utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
    std::vector<uint64_t> destinations;
    utility::getAllMirrorDestinations(this->getHwSwitch(), destinations);
    ASSERT_EQ(destinations.size(), 1);
    for (auto port : this->masterLogicalPortIds()) {
      utility::verifyPortMirrorDestination(
          this->getHwSwitch(),
          port,
          utility::getMirrorPortIngressAndSflowFlags(),
          destinations[0]);
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMirrorTest, ResolvedErspanMirrorOnTrunk) {
  auto setup = [=, this]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();

    utility::addAggPort(1, {this->masterLogicalPortIds()[0]}, &cfg);
    cfg.mirrors()->push_back(this->getErspanMirror());
    auto state = this->applyNewConfig(cfg);
    this->applyNewState(utility::enableTrunkPorts(state));

    auto mirrors = this->getProgrammedState()->getMirrors();
    auto mirror = mirrors->getNodeIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPortDesc(),
        mirror->getDestinationIp(),
        mirror->getSrcIp());
    newMirror->setEgressPortDesc(
        PortDescriptor(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1]));
    this->updateMirror(newMirror);
  };
  auto verify = [=, this]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getNodeIf(kErspan);
    utility::verifyResolvedMirror(this->getHwSwitch(), mirror);
  };
  if (this->skipMirrorTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
