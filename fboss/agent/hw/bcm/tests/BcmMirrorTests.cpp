// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/Interface.h"

extern "C" {
#include <bcm/field.h>
#include <bcm/mirror.h>
}

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
constexpr auto kDscp = 46; // non-default dscp value
using TestTypes = ::testing::Types<IPAddressV4, folly::IPAddressV6>;

template <typename AddrT>
struct TestParams {
  std::array<AddrT, 4> ipAddrs;
  std::array<MacAddress, 4> macAddrs;
  TestParams();
};

template <>
TestParams<IPAddressV4>::TestParams()
    : ipAddrs{IPAddressV4("1.1.1.1"),
              IPAddressV4("1.1.1.10"),
              IPAddressV4("2.2.2.2"),
              IPAddressV4("2.2.2.10")},
      macAddrs{
          MacAddress("1:1:1:1:1:1"),
          MacAddress("1:1:1:1:1:10"),
          MacAddress("2:2:2:2:2:2"),
          MacAddress("2:2:2:2:2:10"),
      } {}

template <>
TestParams<IPAddressV6>::TestParams()
    : ipAddrs{IPAddressV6("1::1"),
              IPAddressV6("1:1::1:10"),
              IPAddressV6("2::1"),
              IPAddressV6("2:2::2:10")},
      macAddrs{
          MacAddress("1:1:1:1:1:1"),
          MacAddress("1:1:1:1:1:10"),
          MacAddress("2:2:2:2:2:2"),
          MacAddress("2:2:2:2:2:10"),
      } {}
} // namespace

template <typename AddrT>
class BcmMirrorTest : public BcmTest {
  using IPAddressV4 = folly::IPAddressV4;
  using IPAddressV6 = folly::IPAddressV6;
  using MirrorTraverseArgT = std::pair<bcm_mirror_destination_t*, int>;

 public:
  void SetUp() override {
    BcmTest::SetUp();
  }

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  }

  cfg::Mirror getSpanMirror(
      const std::string& mirrorName = kSpan,
      const uint8_t dscp = kDscpDefault,
      const bool truncate = false) const {
    cfg::Mirror mirror;
    cfg::MirrorDestination destination;
    destination.egressPort_ref() = cfg::MirrorEgressPort();
    destination.egressPort_ref()->set_logicalID(masterLogicalPortIds()[0]);

    cfg::Mirror mirrorConfig;
    *mirrorConfig.name_ref() = mirrorName;
    *mirrorConfig.destination_ref() = destination;
    *mirrorConfig.dscp_ref() = dscp;
    *mirrorConfig.truncate_ref() = truncate;
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
    *greTunnel.ip_ref() = destinationIp.str();
    tunnel.greTunnel_ref() = greTunnel;
    destination.tunnel_ref() = tunnel;

    cfg::Mirror mirrorConfig;
    *mirrorConfig.name_ref() = mirrorName;
    *mirrorConfig.destination_ref() = destination;
    *mirrorConfig.dscp_ref() = dscp;
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
    *sflowTunnel.ip_ref() = destinationIp.str();
    sflowTunnel.udpSrcPort_ref() = 6545;
    sflowTunnel.udpDstPort_ref() = 5343;
    tunnel.sflowTunnel_ref() = sflowTunnel;

    destination.tunnel_ref() = tunnel;

    cfg::Mirror mirrorConfig;
    *mirrorConfig.name_ref() = mirrorName;
    *mirrorConfig.destination_ref() = destination;
    *mirrorConfig.dscp_ref() = dscp;
    return mirrorConfig;
  }

  template <typename T = AddrT>
  bool areTunnelIpAddrsCorrect(
      bcm_mirror_destination_t* dest,
      const std::enable_if_t<
          std::is_same<T, folly::IPAddressV6>::value,
          MirrorTunnel>& tunnel) const {
    return dest->version == 6 && isIpAddrSame(dest->src6_addr, tunnel.srcIp) &&
        isIpAddrSame(dest->dst6_addr, tunnel.dstIp);
  }

  template <typename T = AddrT>
  bool areTunnelIpAddrsCorrect(
      bcm_mirror_destination_t* dest,
      const std::enable_if_t<
          std::is_same<T, folly::IPAddressV4>::value,
          MirrorTunnel>& tunnel) const {
    return dest->version == 4 && isIpAddrSame(dest->src_addr, tunnel.srcIp) &&
        isIpAddrSame(dest->dst_addr, tunnel.dstIp);
  }

  template <typename T = AddrT>
  bool isIpAddrSame(
      std::enable_if_t<std::is_same<T, folly::IPAddressV4>::value, bcm_ip_t>
          bcmIp,
      const folly::IPAddress& ipAddr) const {
    return ipAddr.asV4().toLongHBO() == bcmIp;
  }

  template <typename T = AddrT>
  bool isIpAddrSame(
      std::enable_if_t<std::is_same<T, folly::IPAddressV6>::value, bcm_ip6_t>&
          bcmIp6,
      const folly::IPAddress& ipAddr) const {
    return ipFromBcm(bcmIp6) == ipAddr;
  }

  bool isMacAddrSame(
      const bcm_mac_t& bcmMac,
      const folly::MacAddress& macAddres) {
    for (auto i = 0; i < 6; i++) {
      if (bcmMac[i] != *(macAddres.bytes() + i)) {
        return false;
      }
    }
    return true;
  }

  template <typename T = AddrT>
  static int
  traverse(int unit, bcm_mirror_destination_t* mirror_dest, void* user_data) {
    MirrorTraverseArgT* args = static_cast<MirrorTraverseArgT*>(user_data);
    bcm_mirror_destination_t* buffer = args->first;
    int handle = args->second;
    if (mirror_dest->mirror_dest_id == handle) {
      std::memcpy(buffer, mirror_dest, sizeof(bcm_mirror_destination_t));
    }
    return 0;
  }

  void verifyResolvedBcmMirror(const std::shared_ptr<Mirror>& mirror) {
    ASSERT_NE(mirror, nullptr);
    ASSERT_EQ(mirror->isResolved(), true);
    const auto* bcmMirrorTable = this->getHwSwitch()->getBcmMirrorTable();
    auto* bcmMirror = bcmMirrorTable->getMirrorIf(mirror->getID());
    ASSERT_NE(bcmMirror, nullptr);
    ASSERT_TRUE(bcmMirror->isProgrammed());

    auto handle = bcmMirror->getHandle().value();
    bcm_mirror_destination_t mirror_dest;
    bcm_mirror_destination_t_init(&mirror_dest);
    MirrorTraverseArgT args = std::make_pair(&mirror_dest, handle);

    bcm_mirror_destination_traverse(0, &BcmMirrorTest::traverse, &args);

    EXPECT_EQ(mirror_dest.mirror_dest_id, handle);
    bcm_gport_t gport;
    BCM_GPORT_MODPORT_SET(
        gport, this->getHwSwitch()->getUnit(), mirror->getEgressPort().value());
    EXPECT_EQ(mirror_dest.gport, gport);
    EXPECT_EQ(mirror_dest.tos, mirror->getDscp());
    EXPECT_EQ(
        bool(mirror_dest.truncate & BCM_MIRROR_PAYLOAD_TRUNCATE),
        mirror->getTruncate());
    if (!mirror->getMirrorTunnel().has_value()) {
      return;
    }
    const auto& tunnel = mirror->getMirrorTunnel();
    std::optional<TunnelUdpPorts> udpPorts = mirror->getTunnelUdpPorts();
    EXPECT_EQ(
        udpPorts.has_value(),
        bool(mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW));
    EXPECT_EQ(
        !udpPorts.has_value(),
        bool(mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE));

    if (udpPorts.has_value()) {
      EXPECT_EQ(udpPorts.value().udpSrcPort, mirror_dest.udp_src_port);
      EXPECT_EQ(udpPorts.value().udpDstPort, mirror_dest.udp_dst_port);
      EXPECT_NE(0, mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW);
    } else {
      EXPECT_EQ(tunnel->greProtocol, mirror_dest.gre_protocol);
      EXPECT_NE(0, mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE);
    }
    if (mirror->getDestinationIp()->isV4()) {
      EXPECT_EQ(mirror_dest.version, 4);
      EXPECT_EQ(
          tunnel->srcIp, folly::IPAddress::fromLongHBO(mirror_dest.src_addr));
      EXPECT_EQ(
          tunnel->dstIp, folly::IPAddress::fromLongHBO(mirror_dest.dst_addr));
    } else {
      EXPECT_EQ(mirror_dest.version, 6);
      EXPECT_EQ(
          tunnel->srcIp,
          folly::IPAddress::fromBinary(
              folly::ByteRange(mirror_dest.src6_addr, 16)));
      EXPECT_EQ(
          tunnel->dstIp,
          folly::IPAddress::fromBinary(
              folly::ByteRange(mirror_dest.dst6_addr, 16)));
    }
    EXPECT_EQ(tunnel->srcMac, macFromBcm(mirror_dest.src_mac));
    EXPECT_EQ(tunnel->dstMac, macFromBcm(mirror_dest.dst_mac));
    EXPECT_EQ(tunnel->ttl, mirror_dest.ttl);
  }

  void getAllMirrorDestinations(std::vector<bcm_gport_t>& destinations) {
    auto traverse = [&destinations](bcm_mirror_destination_t* destination) {
      destinations.push_back(destination->mirror_dest_id);
    };

    auto callback = [](int /*unit*/,
                       bcm_mirror_destination_t* destination,
                       void* closure) -> int {
      (*static_cast<decltype(traverse)*>(closure))(destination);
      return 0;
    };

    bcm_mirror_destination_traverse(
        getHwSwitch()->getUnit(), callback, &traverse);
  }

  void verifyUnResolvedBcmMirror(const std::shared_ptr<Mirror>& mirror) {
    ASSERT_NE(mirror, nullptr);
    ASSERT_EQ(mirror->isResolved(), false);
    const auto* bcmMirrorTable = this->getHwSwitch()->getBcmMirrorTable();
    auto* bcmMirror = bcmMirrorTable->getMirrorIf(mirror->getID());
    ASSERT_NE(bcmMirror, nullptr);
    ASSERT_FALSE(bcmMirror->isProgrammed());
  }

  void verifyPortMirrorDestination(
      PortID port,
      uint32 flags,
      bcm_gport_t mirror_dest_id) {
    bcm_gport_t port_mirror_dest_id = 0;
    int count = 0;
    bcm_mirror_port_dest_get(
        this->getHwSwitch()->getUnit(),
        port,
        flags,
        1,
        &port_mirror_dest_id,
        &count);

    EXPECT_NE(count, 0);
    EXPECT_EQ(port_mirror_dest_id, mirror_dest_id);
  }

  void verifyAclMirrorDestination(
      BcmAclEntryHandle bcmAclHandle,
      bcm_field_action_t flags,
      bcm_gport_t mirror_dest_id) {
    uint32_t param0 = 0;
    uint32_t param1 = 0;
    bcm_field_action_get(0, bcmAclHandle, flags, &param0, &param1);
    EXPECT_EQ(param1, mirror_dest_id);
  }

  void verifyNoAclMirrorDestination(
      BcmAclEntryHandle bcmAclHandle,
      bcm_field_action_t flags,
      bcm_gport_t mirror_dest_id) {
    uint32_t param0 = 0;
    uint32_t param1 = 0;
    bcm_field_action_get(0, bcmAclHandle, flags, &param0, &param1);
    EXPECT_NE(param1, mirror_dest_id);
  }

  void verifyPortNoMirrorDestination(PortID port, uint32 flags) {
    bcm_gport_t port_mirror_dest_id = 0;
    int count = 0;
    bcm_mirror_port_dest_get(
        this->getHwSwitch()->getUnit(),
        port,
        flags,
        1,
        &port_mirror_dest_id,
        &count);

    EXPECT_EQ(count, 0);
    EXPECT_EQ(port_mirror_dest_id, 0);
  }

  TestParams<AddrT> testParams() const {
    return TestParams<AddrT>();
  }

  template <typename T = AddrT>
  bool skipMirrorTest() const {
    return std::is_same<T, folly::IPAddressV6>::value &&
        !getPlatform()->v6MirrorTunnelSupported();
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
    cfg.acls_ref()->push_back(acl);
    cfg::MatchAction mirrorAction;
    mirrorAction.ingressMirror_ref() = mirror;
    mirrorAction.egressMirror_ref() = mirror;

    cfg::MatchToAction match2Action;
    *match2Action.matcher_ref() = *acl.name_ref();
    *match2Action.action_ref() = mirrorAction;

    if (!cfg.dataPlaneTrafficPolicy_ref()) {
      cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
      dataPlaneTrafficPolicy.matchToAction_ref()->push_back(match2Action);
      cfg.dataPlaneTrafficPolicy_ref() = dataPlaneTrafficPolicy;
    } else {
      cfg.dataPlaneTrafficPolicy_ref()->matchToAction_ref()->push_back(
          match2Action);
    }
  }
};

TYPED_TEST_SUITE(BcmMirrorTest, TestTypes);

TYPED_TEST(BcmMirrorTest, ResolvedSpanMirror) {
  auto setup = [=]() {
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSpanMirror());
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSpan);
    this->verifyResolvedBcmMirror(mirror);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, DscpHasDefault) {
  auto setup = [=]() {
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSpanMirror(kSpan, kDscpDefault));
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSpan);
    EXPECT_EQ(mirror->getDscp(), kDscpDefault);
    this->verifyResolvedBcmMirror(mirror);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, DscpHasSetValue) {
  auto setup = [=]() {
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSpanMirror(kSpan, kDscp));
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSpan);
    EXPECT_EQ(mirror->getDscp(), kDscp);
    this->verifyResolvedBcmMirror(mirror);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, MirrorWithTruncation) {
  if (!this->getPlatform()->mirrorPktTruncationSupported()) {
    return;
  }
  auto setup = [=]() {
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSpanMirror(kSpan, kDscp, true));
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSpan);
    EXPECT_EQ(mirror->getDscp(), kDscp);
    this->verifyResolvedBcmMirror(mirror);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, ResolvedErspanMirror) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1]));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyResolvedBcmMirror(mirror);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, ResolvedSflowMirror) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    this->applyNewConfig(cfg);
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    this->verifyResolvedBcmMirror(mirror);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, UnresolvedErspanMirror) {
  auto setup = [=]() {
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyUnResolvedBcmMirror(mirror);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, MirrorRemoved) {
  auto setup = [=]() {
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSpanMirror());
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    mirrors->removeNode(kSpan);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto local = this->getProgrammedState()->getMirrors()->getMirrorIf(kSpan);
    EXPECT_EQ(local, nullptr);
    auto erspan =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyUnResolvedBcmMirror(erspan);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, UnresolvedToUnresolvedUpdate) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSpanMirror());
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        std::optional<folly::IPAddress>(folly::IPAddress(params.ipAddrs[3])));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto local = this->getProgrammedState()->getMirrors()->getMirrorIf(kSpan);
    this->verifyResolvedBcmMirror(local);
    auto erspan =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyUnResolvedBcmMirror(erspan);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, ResolvedToResolvedUpdate) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto resolvedMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    resolvedMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    resolvedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1]));
    mirrors->updateNode(resolvedMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    mirrors = this->getProgrammedState()->getMirrors()->clone();
    mirror = mirrors->getMirrorIf(kErspan);
    auto updatedMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    updatedMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    updatedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    mirrors->updateNode(updatedMirror);
    auto updatedState = this->getProgrammedState()->clone();
    updatedState->resetMirrors(mirrors);
    this->applyNewState(updatedState);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyResolvedBcmMirror(mirror);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, ResolvedToUnresolvedUpdate) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto resolvedMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    resolvedMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    resolvedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1]));
    mirrors->updateNode(resolvedMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    mirrors = this->getProgrammedState()->getMirrors()->clone();
    mirror = mirrors->getMirrorIf(kErspan);
    auto updatedMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    mirrors->updateNode(updatedMirror);
    auto updatedState = this->getProgrammedState()->clone();
    updatedState->resetMirrors(mirrors);
    this->applyNewState(updatedState);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyUnResolvedBcmMirror(mirror);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, NoPortMirroringIfUnResolved) {
  auto setup = [=]() {
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[0]);
    portCfg->ingressMirror_ref() = kErspan;
    portCfg->egressMirror_ref() = kErspan;
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyUnResolvedBcmMirror(mirror);
    this->verifyPortNoMirrorDestination(
        PortID(this->masterLogicalPortIds()[0]), BCM_MIRROR_PORT_INGRESS);
    this->verifyPortNoMirrorDestination(
        PortID(this->masterLogicalPortIds()[0]), BCM_MIRROR_PORT_EGRESS);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, PortMirroringIfResolved) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[0]);
    portCfg->ingressMirror_ref() = kErspan;
    portCfg->egressMirror_ref() = kErspan;
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto updatedMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    updatedMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    updatedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    mirrors->updateNode(updatedMirror);
    auto updatedState = this->getProgrammedState()->clone();
    updatedState->resetMirrors(mirrors);
    this->applyNewState(updatedState);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);

    ASSERT_EQ(destinations.size(), 1);
    this->verifyPortMirrorDestination(
        PortID(this->masterLogicalPortIds()[0]),
        BCM_MIRROR_PORT_INGRESS,
        destinations[0]);
    this->verifyPortMirrorDestination(
        PortID(this->masterLogicalPortIds()[0]),
        BCM_MIRROR_PORT_EGRESS,
        destinations[0]);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, PortMirrorUpdateIfMirrorUpdate) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[0]);
    portCfg->ingressMirror_ref() = kErspan;
    portCfg->egressMirror_ref() = kErspan;
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto updatedMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    updatedMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    updatedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    mirrors->updateNode(updatedMirror);
    auto updatedState = this->getProgrammedState()->clone();
    updatedState->resetMirrors(mirrors);
    this->applyNewState(updatedState);

    mirrors = this->getProgrammedState()->getMirrors()->clone();
    mirror = mirrors->getMirrorIf(kErspan);
    updatedMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    updatedMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    updatedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[3],
        params.ipAddrs[2],
        params.macAddrs[3],
        params.macAddrs[2]));

    mirrors->updateNode(updatedMirror);
    updatedState = this->getProgrammedState()->clone();
    updatedState->resetMirrors(mirrors);
    this->applyNewState(updatedState);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);

    ASSERT_EQ(destinations.size(), 1);
    this->verifyPortMirrorDestination(
        PortID(this->masterLogicalPortIds()[0]),
        BCM_MIRROR_PORT_INGRESS,
        destinations[0]);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, PortMirror) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[0]);
    portCfg->ingressMirror_ref() = kErspan;
    portCfg->egressMirror_ref() = kErspan;
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);

    ASSERT_EQ(destinations.size(), 1);
    this->verifyPortMirrorDestination(
        PortID(this->masterLogicalPortIds()[0]),
        BCM_MIRROR_PORT_INGRESS,
        destinations[0]);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, UpdatePortMirror) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[0]);
    portCfg->ingressMirror_ref() = kErspan;
    portCfg->egressMirror_ref() = kErspan;
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    cfg.mirrors_ref()[0] = this->getSpanMirror();
    portCfg->ingressMirror_ref().reset();
    portCfg->egressMirror_ref().reset();
    portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[1]);
    portCfg->ingressMirror_ref() = kSpan;
    portCfg->egressMirror_ref() = kSpan;
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSpan);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);

    ASSERT_EQ(destinations.size(), 1);
    this->verifyPortNoMirrorDestination(
        PortID(this->masterLogicalPortIds()[0]), BCM_MIRROR_PORT_INGRESS);
    this->verifyPortNoMirrorDestination(
        PortID(this->masterLogicalPortIds()[0]), BCM_MIRROR_PORT_EGRESS);

    this->verifyPortMirrorDestination(
        PortID(this->masterLogicalPortIds()[1]),
        BCM_MIRROR_PORT_INGRESS,
        destinations[0]);
    this->verifyPortMirrorDestination(
        PortID(this->masterLogicalPortIds()[1]),
        BCM_MIRROR_PORT_EGRESS,
        destinations[0]);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, RemovePortMirror) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[0]);
    portCfg->ingressMirror_ref() = kErspan;
    portCfg->egressMirror_ref() = kErspan;
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    portCfg->ingressMirror_ref().reset();
    portCfg->egressMirror_ref().reset();
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 1);

    this->verifyPortNoMirrorDestination(
        PortID(this->masterLogicalPortIds()[0]), BCM_MIRROR_PORT_INGRESS);
    this->verifyPortNoMirrorDestination(
        PortID(this->masterLogicalPortIds()[0]), BCM_MIRROR_PORT_EGRESS);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, BcmMirrorStat) {
  auto setup = [=]() {
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSpanMirror());
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto stats = this->getHwSwitch()->getStatUpdater()->getHwTableStats();
    EXPECT_EQ(*stats.mirrors_used_ref(), 1);
    EXPECT_EQ(*stats.mirrors_span_ref(), 1);
    EXPECT_EQ(*stats.mirrors_erspan_ref(), 0);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, BcmResolvedMirrorStat) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->resize(3);
    cfg.mirrors_ref()[0] = this->getSpanMirror();
    cfg.mirrors_ref()[1] = this->getErspanMirror();
    cfg.mirrors_ref()[2] = this->getSflowMirror();
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1]));
    mirrors->updateNode(newMirror);

    auto sflowMirror = mirrors->getMirrorIf(kSflow);
    auto newSflowMirror = std::make_shared<Mirror>(
        sflowMirror->getID(),
        sflowMirror->getEgressPort(),
        sflowMirror->getDestinationIp(),
        sflowMirror->getSrcIp(),
        sflowMirror->getTunnelUdpPorts());
    newSflowMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newSflowMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        sflowMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newSflowMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto stats = this->getHwSwitch()->getStatUpdater()->getHwTableStats();
    auto span = this->getProgrammedState()->getMirrors()->getMirrorIf(kSpan);
    auto erspan =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);

    auto sflowMirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    EXPECT_TRUE(span->isResolved());
    EXPECT_TRUE(erspan->isResolved());
    EXPECT_TRUE(sflowMirror->isResolved());

    EXPECT_EQ(*stats.mirrors_used_ref(), 3);
    EXPECT_EQ(*stats.mirrors_span_ref(), 1);
    EXPECT_EQ(*stats.mirrors_erspan_ref(), 1);
    EXPECT_EQ(*stats.mirrors_sflow_ref(), 1);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, BcmUnresolvedMirrorStat) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->resize(3);
    cfg.mirrors_ref()[0] = this->getSpanMirror();
    cfg.mirrors_ref()[1] = this->getErspanMirror();
    cfg.mirrors_ref()[2] = this->getSflowMirror();
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto resolvedMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    resolvedMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    resolvedMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1]));
    mirrors->updateNode(resolvedMirror);

    auto sflowMirror = mirrors->getMirrorIf(kSflow);
    auto newSflowMirror = std::make_shared<Mirror>(
        sflowMirror->getID(),
        sflowMirror->getEgressPort(),
        sflowMirror->getDestinationIp(),
        sflowMirror->getSrcIp(),
        sflowMirror->getTunnelUdpPorts());
    newSflowMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newSflowMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newSflowMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(sflowMirror);

    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto unresolvedMirror = std::make_shared<Mirror>(
        resolvedMirror->getID(),
        resolvedMirror->getEgressPort(),
        resolvedMirror->getDestinationIp());
    EXPECT_TRUE(!unresolvedMirror->isResolved());
    mirrors->updateNode(unresolvedMirror);
    auto unresolvedSflowMirror = std::make_shared<Mirror>(
        newSflowMirror->getID(),
        newSflowMirror->getEgressPort(),
        newSflowMirror->getDestinationIp());
    EXPECT_TRUE(!unresolvedSflowMirror->isResolved());
    mirrors->updateNode(unresolvedSflowMirror);
    newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto stats = this->getHwSwitch()->getStatUpdater()->getHwTableStats();
    auto span = this->getProgrammedState()->getMirrors()->getMirrorIf(kSpan);
    auto erspan =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    auto sflowMirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    EXPECT_TRUE(span->isResolved());
    EXPECT_TRUE(!erspan->isResolved());
    EXPECT_EQ(*stats.mirrors_used_ref(), 1);
    EXPECT_EQ(*stats.mirrors_span_ref(), 1);
    EXPECT_EQ(*stats.mirrors_erspan_ref(), 0);
    EXPECT_EQ(*stats.mirrors_sflow_ref(), 0);
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, AclMirror) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    cfg::AclEntry acl;
    *acl.name_ref() = "acl0";
    acl.dstIp_ref() = "192.168.0.0/16";
    this->addAclMirror(kErspan, acl, cfg);
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);

    ASSERT_EQ(destinations.size(), 1);
    auto* bcmSwitch = this->getHwSwitch();
    bcmSwitch->getAclTable()->forFilteredEach(
        [=](const auto& pair) {
          const auto& bcmAclEntry = pair.second;
          return (bcmAclEntry->getIngressAclMirror() == kErspan) ||
              (bcmAclEntry->getEgressAclMirror() == kErspan);
        },
        [&](const auto& pair) {
          const auto& bcmAclEntry = pair.second;
          auto bcmAclHandle = bcmAclEntry->getHandle();
          this->verifyAclMirrorDestination(
              bcmAclHandle, bcmFieldActionMirrorIngress, destinations[0]);
          this->verifyAclMirrorDestination(
              bcmAclHandle, bcmFieldActionMirrorEgress, destinations[0]);
        });
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, UpdateAclMirror) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSpanMirror());
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    cfg::AclEntry acl;
    *acl.name_ref() = "acl0";
    acl.dstIp_ref() = "192.168.0.0/16";
    this->addAclMirror(kErspan, acl, cfg);
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    cfg.mirrors_ref()->clear();
    cfg.mirrors_ref()->push_back(this->getSpanMirror());
    for (auto& match2Action :
         *cfg.dataPlaneTrafficPolicy_ref()->matchToAction_ref()) {
      if (*match2Action.matcher_ref() == "acl0") {
        match2Action.action_ref()->ingressMirror_ref() = kSpan;
        match2Action.action_ref()->egressMirror_ref() = kSpan;
      }
    }
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto erspan =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    EXPECT_EQ(erspan, nullptr);

    auto span = this->getProgrammedState()->getMirrors()->getMirrorIf(kSpan);
    this->verifyResolvedBcmMirror(span);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 1);
    auto* bcmSwitch = this->getHwSwitch();
    bcmSwitch->getAclTable()->forFilteredEach(
        [=](const auto& pair) {
          const auto& bcmAclEntry = pair.second;
          return (bcmAclEntry->getIngressAclMirror() == kSpan) ||
              (bcmAclEntry->getEgressAclMirror() == kSpan);
        },
        [&](const auto& pair) {
          const auto& bcmAclEntry = pair.second;
          auto bcmAclHandle = bcmAclEntry->getHandle();
          this->verifyAclMirrorDestination(
              bcmAclHandle, bcmFieldActionMirrorIngress, destinations[0]);
          this->verifyAclMirrorDestination(
              bcmAclHandle, bcmFieldActionMirrorEgress, destinations[0]);
        });
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, RemoveAclMirror) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getErspanMirror());
    cfg::AclEntry acl;
    *acl.name_ref() = "acl0";
    acl.dstIp_ref() = "192.168.0.0/16";
    this->addAclMirror(kErspan, acl, cfg);
    this->applyNewConfig(cfg);

    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kErspan);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(), mirror->getEgressPort(), mirror->getDestinationIp());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[1]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[2],
        params.ipAddrs[3],
        params.macAddrs[2],
        params.macAddrs[3]));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    for (auto& match2Action :
         *cfg.dataPlaneTrafficPolicy_ref()->matchToAction_ref()) {
      if (*match2Action.matcher_ref() == "acl0") {
        match2Action.action_ref()->ingressMirror_ref().reset();
        match2Action.action_ref()->egressMirror_ref().reset();
      }
    }
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror =
        this->getProgrammedState()->getMirrors()->getMirrorIf(kErspan);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 1);
    auto* bcmSwitch = this->getHwSwitch();
    bcmSwitch->getAclTable()->forFilteredEach(
        [=](const auto& /*pair*/) {
          return true; /* check for all acls */
        },
        [&](const auto& pair) {
          const auto& bcmAclEntry = pair.second;
          auto bcmAclHandle = bcmAclEntry->getHandle();
          this->verifyNoAclMirrorDestination(
              bcmAclHandle, bcmFieldActionMirrorIngress, destinations[0]);
          this->verifyNoAclMirrorDestination(
              bcmAclHandle, bcmFieldActionMirrorEgress, destinations[0]);
        });
  };
  if (this->skipMirrorTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, BcmMirrorLimitExceeded) {
  if (this->skipMirrorTest()) {
    return;
  }
  auto cfg = this->initialConfig();
  auto oldState = this->applyNewConfig(cfg);

  cfg.mirrors_ref()->resize(5);
  cfg.mirrors_ref()[0] = this->getErspanMirror("mirror0");
  cfg.mirrors_ref()[1] = this->getErspanMirror("mirror1");
  cfg.mirrors_ref()[2] = this->getErspanMirror("mirror2");
  cfg.mirrors_ref()[3] = this->getErspanMirror("mirror3");
  cfg.mirrors_ref()[4] = this->getErspanMirror("mirror4");

  auto newState = this->applyNewConfig(cfg);

  EXPECT_FALSE(
      this->getHwSwitch()->isValidStateUpdate(StateDelta(oldState, newState)));
}

TYPED_TEST(BcmMirrorTest, SampleOnePort) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling one port and send traffic to sflow mirror */
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[1]);
    portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
    *portCfg->sFlowIngressRate_ref() = 90000;
    this->applyNewConfig(cfg);

    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 1);
    this->verifyPortMirrorDestination(
        PortID(this->masterLogicalPortIds()[1]),
        BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW,
        destinations[0]);
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, SampleAllPorts) {
  /* Setup sample destination to mirror for all ports, and mirror only one port
  this will ensure all port traffic is sampled and sent to that mirror */
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 1);
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortMirrorDestination(
          port,
          BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW,
          destinations[0]);
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, SflowMirrorWithErspanMirror) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    cfg.mirrors_ref()->push_back(this->getErspanMirror());

    for (auto i = 0; i < 2; i++) {
      auto portId = this->masterLogicalPortIds()[i];
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[2]);
    portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[1].name_ref();
    portCfg->egressMirror_ref() = *cfg.mirrors_ref()[1].name_ref();
    this->applyNewConfig(cfg);
    // resolve both mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    for (auto mirror : *mirrors) {
      auto newMirror = mirror->getTunnelUdpPorts()
          ? std::make_shared<Mirror>(
                mirror->getID(),
                mirror->getEgressPort(),
                mirror->getDestinationIp(),
                mirror->getSrcIp(),
                mirror->getTunnelUdpPorts().value())
          : std::make_shared<Mirror>(
                mirror->getID(),
                mirror->getEgressPort(),
                mirror->getDestinationIp(),
                mirror->getSrcIp());
      newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
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
      mirrors->updateNode(newMirror);
    }
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirrors = this->getProgrammedState()->getMirrors();
    for (auto mirror : *mirrors) {
      this->verifyResolvedBcmMirror(mirror);
    }
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 2);
    bcm_gport_t sflow = 0;
    bcm_gport_t erspan = 0;
    for (auto destination : destinations) {
      bcm_mirror_destination_t mirror_dest;
      bcm_mirror_destination_t_init(&mirror_dest);
      bcm_mirror_destination_get(
          this->getHwSwitch()->getUnit(), destination, &mirror_dest);
      if (mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW) {
        sflow = destination;
      } else {
        erspan = destination;
      }
    }
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortMirrorDestination(
          port, BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW, sflow);
    }
    this->verifyPortMirrorDestination(
        this->masterLogicalPortIds()[2], BCM_MIRROR_PORT_INGRESS, erspan);
    this->verifyPortMirrorDestination(
        this->masterLogicalPortIds()[2], BCM_MIRROR_PORT_EGRESS, erspan);
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, SflowMirrorWithErspanMirrorOnePortSflow) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    cfg.mirrors_ref()->push_back(this->getErspanMirror());

    for (auto i = 0; i < 2; i++) {
      auto portId = this->masterLogicalPortIds()[i];
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[2]);
    portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[1].name_ref();
    portCfg->egressMirror_ref() = *cfg.mirrors_ref()[1].name_ref();
    this->applyNewConfig(cfg);
    // resolve both mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    for (auto mirror : *mirrors) {
      auto newMirror = mirror->getTunnelUdpPorts()
          ? std::make_shared<Mirror>(
                mirror->getID(),
                mirror->getEgressPort(),
                mirror->getDestinationIp(),
                mirror->getSrcIp(),
                mirror->getTunnelUdpPorts().value())
          : std::make_shared<Mirror>(
                mirror->getID(),
                mirror->getEgressPort(),
                mirror->getDestinationIp(),
                mirror->getSrcIp());
      newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
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
      mirrors->updateNode(newMirror);
    }
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[1]);
    portCfg->sampleDest_ref() = cfg::SampleDestination::CPU;
    *portCfg->sFlowIngressRate_ref() = 0;
    portCfg->ingressMirror_ref().reset();
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirrors = this->getProgrammedState()->getMirrors();
    for (auto mirror : *mirrors) {
      this->verifyResolvedBcmMirror(mirror);
    }
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 2);
    bcm_gport_t sflow = 0;
    bcm_gport_t erspan = 0;
    for (auto destination : destinations) {
      bcm_mirror_destination_t mirror_dest;
      bcm_mirror_destination_t_init(&mirror_dest);
      bcm_mirror_destination_get(
          this->getHwSwitch()->getUnit(), destination, &mirror_dest);
      if (mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW) {
        sflow = destination;
      } else {
        erspan = destination;
      }
    }
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortMirrorDestination(
          port, BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW, sflow);
    }
    this->verifyPortMirrorDestination(
        this->masterLogicalPortIds()[2], BCM_MIRROR_PORT_INGRESS, erspan);
    this->verifyPortMirrorDestination(
        this->masterLogicalPortIds()[2], BCM_MIRROR_PORT_EGRESS, erspan);
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, SflowMirrorWithErspanMirrorNoPortSflow) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    cfg.mirrors_ref()->push_back(this->getErspanMirror());

    for (auto i = 0; i < 2; i++) {
      auto portId = this->masterLogicalPortIds()[i];
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    auto portCfg = utility::findCfgPort(cfg, this->masterLogicalPortIds()[2]);
    portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[1].name_ref();
    portCfg->egressMirror_ref() = *cfg.mirrors_ref()[1].name_ref();
    this->applyNewConfig(cfg);
    // resolve both mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    for (auto mirror : *mirrors) {
      auto newMirror = mirror->getTunnelUdpPorts()
          ? std::make_shared<Mirror>(
                mirror->getID(),
                mirror->getEgressPort(),
                mirror->getDestinationIp(),
                mirror->getSrcIp(),
                mirror->getTunnelUdpPorts().value())
          : std::make_shared<Mirror>(
                mirror->getID(),
                mirror->getEgressPort(),
                mirror->getDestinationIp(),
                mirror->getSrcIp());
      newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
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
      mirrors->updateNode(newMirror);
    }
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    for (auto i = 0; i < 2; i++) {
      auto portId = this->masterLogicalPortIds()[i];
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::CPU;
      *portCfg->sFlowIngressRate_ref() = 0;
      portCfg->ingressMirror_ref().reset();
    }
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirrors = this->getProgrammedState()->getMirrors();
    for (auto mirror : *mirrors) {
      this->verifyResolvedBcmMirror(mirror);
    }
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 2);
    bcm_gport_t erspan = 0;
    for (auto destination : destinations) {
      bcm_mirror_destination_t mirror_dest;
      bcm_mirror_destination_t_init(&mirror_dest);
      bcm_mirror_destination_get(
          this->getHwSwitch()->getUnit(), destination, &mirror_dest);
      if (mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW) {
        std::ignore = destination;
      } else {
        erspan = destination;
      }
    }
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortNoMirrorDestination(
          port, BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW);
    }
    this->verifyPortMirrorDestination(
        this->masterLogicalPortIds()[2], BCM_MIRROR_PORT_INGRESS, erspan);
    this->verifyPortMirrorDestination(
        this->masterLogicalPortIds()[2], BCM_MIRROR_PORT_EGRESS, erspan);
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, SampleAllPortsMirrorUnresolved) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    /* unresolve */
    mirrors = this->getProgrammedState()->getMirrors()->clone();
    mirror = mirrors->getMirrorIf(kSflow);
    newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());

    mirrors->updateNode(newMirror);
    newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    this->verifyUnResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 0); // no mirror found
    EXPECT_TRUE(mirror->getTunnelUdpPorts().has_value());
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortNoMirrorDestination(
          port,
          BCM_MIRROR_PORT_INGRESS |
              BCM_MIRROR_PORT_SFLOW); // noo mirroring to port
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, SampleAllPortsMirrorUnresolvedResolved) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    /* unresolve */
    mirrors = this->getProgrammedState()->getMirrors()->clone();
    mirror = mirrors->getMirrorIf(kSflow);
    newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());

    mirrors->updateNode(newMirror);
    newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    /* reresolve */
    mirrors = this->getProgrammedState()->getMirrors()->clone();
    mirror = mirrors->getMirrorIf(kSflow);
    newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 1);
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortMirrorDestination(
          port,
          BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW,
          destinations[0]);
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, SampleAllPortsMirrorUpdate) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 1);
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortMirrorDestination(
          port,
          BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW,
          destinations[0]);
    }
  };
  auto setupPostWb = [=] {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    // update to truncate if supported
    *cfg.mirrors_ref()[0].truncate_ref() =
        this->getPlatform()->mirrorPktTruncationSupported();
    // update destination port now
    cfg.mirrors_ref()[0]
        .destination_ref()
        ->tunnel_ref()
        ->sflowTunnel_ref()
        ->udpDstPort_ref() = 9898;
    /* sampling all ports and send traffic to sflow mirror */
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify, setupPostWb, verify);
}

TYPED_TEST(BcmMirrorTest, RemoveSampleAllPorts) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    /* reset all config */
    cfg = this->initialConfig();
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    EXPECT_EQ(mirror, nullptr);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 0);
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortNoMirrorDestination(
          port, BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW);
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(BcmMirrorTest, RemoveSampleAllPortsAfterWarmBoot) {
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 1);
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortMirrorDestination(
          port,
          BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW,
          destinations[0]);
    }
  };
  auto setupPostWb = [=]() {
    /* reset all config */
    this->applyNewConfig(this->initialConfig());
  };
  auto verifyPostWb = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    EXPECT_EQ(mirror, nullptr);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 0);
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortNoMirrorDestination(
          port, BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW);
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify, setupPostWb, verifyPostWb);
}

TYPED_TEST(BcmMirrorTest, SampleAllPortsReloadConfig) {
  /* Setup sample destination to mirror for all ports, and mirror only one port
  this will ensure all port traffic is sampled and sent to that mirror */
  auto setup = [=]() {
    auto params = this->testParams();
    auto cfg = this->initialConfig();
    /* sampling all ports and send traffic to sflow mirror */
    cfg.mirrors_ref()->push_back(this->getSflowMirror());
    for (auto portId : this->masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      *portCfg->sFlowIngressRate_ref() = 90000;
      portCfg->ingressMirror_ref() = *cfg.mirrors_ref()[0].name_ref();
    }
    this->applyNewConfig(cfg);
    // resolve mirror
    auto mirrors = this->getProgrammedState()->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf(kSflow);
    auto newMirror = std::make_shared<Mirror>(
        mirror->getID(),
        mirror->getEgressPort(),
        mirror->getDestinationIp(),
        mirror->getSrcIp(),
        mirror->getTunnelUdpPorts().value());
    newMirror->setEgressPort(PortID(this->masterLogicalPortIds()[0]));
    newMirror->setMirrorTunnel(MirrorTunnel(
        params.ipAddrs[0],
        params.ipAddrs[1],
        params.macAddrs[0],
        params.macAddrs[1],
        newMirror->getTunnelUdpPorts().value()));
    mirrors->updateNode(newMirror);
    auto newState = this->getProgrammedState()->clone();
    newState->resetMirrors(mirrors);
    this->applyNewState(newState);

    // reload config and verify mirror stays resolved
    this->applyNewConfig(cfg);
  };
  auto verify = [=]() {
    auto mirror = this->getProgrammedState()->getMirrors()->getMirrorIf(kSflow);
    this->verifyResolvedBcmMirror(mirror);
    std::vector<bcm_gport_t> destinations;
    this->getAllMirrorDestinations(destinations);
    ASSERT_EQ(destinations.size(), 1);
    for (auto port : this->masterLogicalPortIds()) {
      this->verifyPortMirrorDestination(
          port,
          BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW,
          destinations[0]);
    }
  };
  if (this->skipMirrorTest() || this->skipSflowTest()) {
    return;
  }
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
