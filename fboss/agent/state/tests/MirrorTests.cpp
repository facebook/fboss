// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/MirrorConfigs.h"
#include "fboss/agent/test/TestUtils.h"
#include "folly/MacAddress.h"

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

class MirrorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    config_ = testConfigA();
    platform_ = createMockPlatform();
    state_ = testStateA();
  }

  void configureAcl(const std::string& name, uint16_t dstL4Port = 1234) {
    cfg::AclEntry aclEntry;
    auto aclCount = config_.acls()->size() + 1;
    config_.acls()->resize(aclCount);
    *config_.acls()[aclCount - 1].name() = name;
    *config_.acls()[aclCount - 1].actionType() = cfg::AclActionType::PERMIT;
    config_.acls()[aclCount - 1].l4DstPort() = dstL4Port;
  }

  void configurePortMirror(const std::string& mirror, PortID port) {
    int portIndex = int(port) - 1;
    config_.ports()[portIndex].ingressMirror() = mirror;
    config_.ports()[portIndex].egressMirror() = mirror;
  }

  void configureAclMirror(const std::string& name, const std::string& mirror) {
    cfg::MatchAction action;
    action.ingressMirror() = mirror;
    action.egressMirror() = mirror;

    cfg::MatchToAction mirrorAction;
    *mirrorAction.matcher() = name;
    *mirrorAction.action() = action;
    // Initialize data plane traffic policy only when uninitialized.
    if (!config_.dataPlaneTrafficPolicy()) {
      config_.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
    }
    config_.dataPlaneTrafficPolicy()->matchToAction()->push_back(mirrorAction);
  }

  void publishWithStateUpdate() {
    state_ = publishAndApplyConfig(state_, &config_, platform_.get());
    ASSERT_NE(state_, nullptr);
  }
  void publishWithFbossError() {
    EXPECT_THROW(
        publishAndApplyConfig(state_, &config_, platform_.get()), FbossError);
  }
  void publishWithNoStateUpdate() {
    auto newState = publishAndApplyConfig(state_, &config_, platform_.get());
    EXPECT_EQ(newState, nullptr);
  }
  std::shared_ptr<SwitchState> state_;
  std::shared_ptr<Platform> platform_;
  cfg::SwitchConfig config_;
  static const folly::IPAddress tunnelDestination;
  static const char* egressPortName;
  static const PortID egressPort;
  static const uint8_t dscp;
  static const TunnelUdpPorts udpPorts;
};

const folly::IPAddress MirrorTest::tunnelDestination =
    folly::IPAddress("10.0.0.1");
const char* MirrorTest::egressPortName = "port5";
const PortID MirrorTest::egressPort = PortID(5);
const uint8_t MirrorTest::dscp = 46;
const TunnelUdpPorts MirrorTest::udpPorts = {6545, 5343};

TEST_F(MirrorTest, MirrorWithPort) {
  config_.mirrors()->push_back(
      utility::getSPANMirror("mirror0", MirrorTest::egressPortName));
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.has_value(), true);
  EXPECT_EQ(port.value(), egressPort);
  auto dscp = mirror->getDscp();
  EXPECT_EQ(dscp, cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_);
  EXPECT_FALSE(mirror->getTunnelUdpPorts().has_value());
}

TEST_F(MirrorTest, MirrorWithPortId) {
  config_.mirrors()->push_back(
      utility::getSPANMirror("mirror0", MirrorTest::egressPort));
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.has_value(), true);
  EXPECT_EQ(port.value(), egressPort);
  auto dscp = mirror->getDscp();
  EXPECT_EQ(dscp, cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_);
}

TEST_F(MirrorTest, MirrorWithPortIdAndDscp) {
  config_.mirrors()->push_back(utility::getGREMirrorWithPort(
      "mirror0",
      MirrorTest::egressPort,
      folly::IPAddress("0.0.0.0"),
      std::nullopt /*src addr*/,
      MirrorTest::dscp));
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.has_value(), true);
  EXPECT_EQ(port.value(), egressPort);
  auto dscp = mirror->getDscp();
  EXPECT_EQ(dscp, MirrorTest::dscp);
  EXPECT_FALSE(mirror->getTunnelUdpPorts().has_value());
}

TEST_F(MirrorTest, MirrorWithIp) {
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), false);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.has_value(), false);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.has_value(), true);
  EXPECT_EQ(ip.value(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror->isResolved(), false);
  auto dscp = mirror->getDscp();
  EXPECT_EQ(dscp, cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_);
}

TEST_F(MirrorTest, MirrorWithIpAndDscp) {
  config_.mirrors()->push_back(utility::getGREMirror(
      "mirror0",
      MirrorTest::tunnelDestination,
      std::nullopt /* src addr*/,
      MirrorTest::dscp));

  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), false);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.has_value(), false);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.has_value(), true);
  EXPECT_EQ(ip.value(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror->isResolved(), false);
  auto dscp = mirror->getDscp();
  EXPECT_EQ(dscp, MirrorTest::dscp);
  EXPECT_FALSE(mirror->getTunnelUdpPorts().has_value());
}

TEST_F(MirrorTest, MirrorWithPortAndIp) {
  config_.mirrors()->push_back(utility::getGREMirrorWithPort(
      "mirror0", MirrorTest::egressPortName, MirrorTest::tunnelDestination));

  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.has_value(), true);
  EXPECT_EQ(port.value(), egressPort);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.has_value(), true);
  EXPECT_EQ(ip.value(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror->isResolved(), false);
  auto dscp = mirror->getDscp();
  EXPECT_EQ(dscp, cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_);
  EXPECT_FALSE(mirror->getTunnelUdpPorts().has_value());
}

TEST_F(MirrorTest, MirrorWithPortIdAndIp) {
  config_.mirrors()->push_back(utility::getGREMirrorWithPort(
      "mirror0", MirrorTest::egressPort, MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.has_value(), true);
  EXPECT_EQ(port.value(), egressPort);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.has_value(), true);
  EXPECT_EQ(ip.value(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror->isResolved(), false);
  auto dscp = mirror->getDscp();
  EXPECT_EQ(dscp, cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_);
}

TEST_F(MirrorTest, MirrorWithPortIdAndIpAndDscp) {
  config_.mirrors()->push_back(utility::getGREMirrorWithPort(
      "mirror0",
      MirrorTest::egressPort,
      MirrorTest::tunnelDestination,
      std::nullopt /* src addr */,
      MirrorTest::dscp));
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.has_value(), true);
  EXPECT_EQ(port.value(), egressPort);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.has_value(), true);
  EXPECT_EQ(ip.value(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror->isResolved(), false);
  auto dscp = mirror->getDscp();
  EXPECT_EQ(dscp, MirrorTest::dscp);
  EXPECT_FALSE(mirror->getTunnelUdpPorts().has_value());
}

TEST_F(MirrorTest, MirrorWithPortIdAndIpAndSflowTunnel) {
  config_.mirrors()->push_back(utility::getSFlowMirrorWithPort(
      "mirror0",
      MirrorTest::egressPort,
      MirrorTest::udpPorts.udpSrcPort,
      MirrorTest::udpPorts.udpDstPort,
      MirrorTest::tunnelDestination,
      std::nullopt,
      MirrorTest::dscp));
  publishWithStateUpdate();

  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.has_value(), true);
  EXPECT_EQ(port.value(), egressPort);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.has_value(), true);
  EXPECT_EQ(ip.value(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror->isResolved(), false);
  auto udpPorts = mirror->getTunnelUdpPorts();
  EXPECT_TRUE(udpPorts.has_value());
  EXPECT_EQ(udpPorts.value().udpSrcPort, MirrorTest::udpPorts.udpSrcPort);
  EXPECT_EQ(udpPorts.value().udpDstPort, MirrorTest::udpPorts.udpDstPort);
}

TEST_F(MirrorTest, MirrorWithNameNoPortNoIp) {
  cfg::Mirror mirror0;
  mirror0.name() = "mirror0";
  config_.mirrors()->push_back(mirror0);
  publishWithFbossError();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_EQ(mirror, nullptr);
}

TEST_F(MirrorTest, MirrorWithNameAndDscpNoPortNoIp) {
  cfg::Mirror mirror0;
  mirror0.name() = "mirror0";
  config_.mirrors()->push_back(mirror0);
  mirror0.dscp() = MirrorTest::dscp;
  publishWithFbossError();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_EQ(mirror, nullptr);
}

TEST_F(MirrorTest, MirrorWithTunnelNoPortNoIp) {
  cfg::Mirror mirror0;
  mirror0.name() = "mirror0";
  config_.mirrors()->push_back(mirror0);
  publishWithFbossError();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
}

TEST_F(MirrorTest, MirrorWithTruncation) {
  config_.mirrors()->push_back(utility::getGREMirrorWithPort(
      "mirror0",
      MirrorTest::egressPort,
      MirrorTest::tunnelDestination,
      std::nullopt,
      MirrorTest::dscp,
      true));
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getTruncate(), true);
}

TEST_F(MirrorTest, MirrorWithoutTruncation) {
  config_.mirrors()->push_back(utility::getGREMirrorWithPort(
      "mirror0",
      MirrorTest::egressPort,
      MirrorTest::tunnelDestination,
      std::nullopt,
      MirrorTest::dscp,
      false));
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getTruncate(), false);
}

TEST_F(MirrorTest, AclMirror) {
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  configureAcl("acl0");
  configureAclMirror("acl0", "mirror0");
  publishWithStateUpdate();
  auto entry = state_->getAcls()->getEntryIf("acl0");
  EXPECT_NE(entry, nullptr);
  auto action = entry->getAclAction();
  ASSERT_EQ(action.has_value(), true);
  auto inMirror = action.value().getIngressMirror();
  EXPECT_EQ(inMirror.has_value(), true);
  EXPECT_EQ(inMirror.value(), "mirror0");
  auto egMirror = action.value().getEgressMirror();
  EXPECT_EQ(egMirror.has_value(), true);
  EXPECT_EQ(egMirror.value(), "mirror0");
}

TEST_F(MirrorTest, PortMirror) {
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  configurePortMirror("mirror0", PortID(3));
  publishWithStateUpdate();
  auto port = state_->getPorts()->getPortIf(PortID(3));
  EXPECT_NE(port, nullptr);
  auto inMirror = port->getIngressMirror();
  EXPECT_EQ(inMirror.has_value(), true);
  EXPECT_EQ(inMirror.value(), "mirror0");
  auto egMirror = port->getEgressMirror();
  EXPECT_EQ(egMirror.has_value(), true);
  EXPECT_EQ(egMirror.value(), "mirror0");
}

TEST_F(MirrorTest, AclWrongMirror) {
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  configureAcl("acl0");
  configureAclMirror("acl0", "mirror1");
  publishWithFbossError();
}

TEST_F(MirrorTest, PortWrongMirror) {
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  configurePortMirror("mirror1", PortID(3));
  publishWithFbossError();
}

TEST_F(MirrorTest, MirrorWrongPort) {
  config_.mirrors()->push_back(utility::getGREMirrorWithPort(
      "mirror0", "port129", MirrorTest::tunnelDestination));
  publishWithFbossError();
}

TEST_F(MirrorTest, MirrorWrongPortId) {
  config_.mirrors()->push_back(utility::getGREMirrorWithPort(
      "mirror0", PortID(129), MirrorTest::tunnelDestination));
  publishWithFbossError();
}

TEST_F(MirrorTest, NoStateChange) {
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  cfg::MirrorTunnel tunnel;
  cfg::GreTunnel greTunnel;
  *greTunnel.ip() = MirrorTest::tunnelDestination.str();
  tunnel.greTunnel() = greTunnel;
  config_.mirrors()[0].destination()->tunnel() = tunnel;
  publishWithNoStateUpdate();
}

TEST_F(MirrorTest, WithStateChange) {
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  *config_.mirrors()[0]
       .destination()
       ->tunnel()
       .value()
       .greTunnel()
       .value()
       .ip() = "10.0.0.2";
  publishWithStateUpdate();

  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), false);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.has_value(), false);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.has_value(), true);
  EXPECT_EQ(ip.value(), folly::IPAddress("10.0.0.2"));
  EXPECT_EQ(mirror->isResolved(), false);
}

TEST_F(MirrorTest, AddAclAndPortToMirror) {
  std::array<std::string, 2> acls{"acl0", "acl1"};
  std::array<PortID, 2> ports{PortID(3), PortID(4)};
  uint16_t l4port = 1234;
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();

  for (int i = 0; i < 2; i++) {
    configureAcl(acls[i], l4port + i);
    configureAclMirror(acls[i], "mirror0");
    configurePortMirror("mirror0", ports[i]);
    publishWithStateUpdate();
  }

  for (int i = 0; i < 2; i++) {
    auto entry = state_->getAcls()->getEntryIf(acls[i]);
    EXPECT_NE(entry, nullptr);
    auto action = entry->getAclAction();
    ASSERT_EQ(action.has_value(), true);
    auto aclInMirror = action.value().getIngressMirror();
    EXPECT_EQ(aclInMirror.has_value(), true);
    EXPECT_EQ(aclInMirror.value(), "mirror0");
    auto aclEgMirror = action.value().getEgressMirror();
    EXPECT_EQ(aclEgMirror.has_value(), true);
    EXPECT_EQ(aclEgMirror.value(), "mirror0");

    auto port = state_->getPorts()->getPortIf(ports[i]);
    EXPECT_NE(port, nullptr);
    auto portInMirror = port->getIngressMirror();
    EXPECT_EQ(portInMirror.has_value(), true);
    EXPECT_EQ(portInMirror.value(), "mirror0");
  }
}

TEST_F(MirrorTest, DeleleteAclAndPortToMirror) {
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();

  std::array<std::string, 2> acls{"acl0", "acl1"};
  std::array<PortID, 2> ports{PortID(3), PortID(4)};
  uint16_t l4port = 1234;

  for (int i = 0; i < 2; i++) {
    configureAcl(acls[i], l4port + i);
    configureAclMirror(acls[i], "mirror0");
    configurePortMirror("mirror0", ports[i]);
  }
  publishWithStateUpdate();

  auto portIndex = int(PortID(4)) - 1;
  config_.ports()[portIndex].ingressMirror().reset();
  config_.ports()[portIndex].egressMirror().reset();

  config_.acls()->pop_back();
  config_.dataPlaneTrafficPolicy()->matchToAction()->pop_back();

  publishWithStateUpdate();

  for (int i = 0; i < 2; i++) {
    auto entry = state_->getAcls()->getEntryIf(acls[i]);
    if (i) {
      EXPECT_EQ(entry, nullptr);
      auto port = state_->getPorts()->getPortIf(ports[i]);
      EXPECT_NE(port, nullptr);
      auto portInMirror = port->getIngressMirror();
      EXPECT_EQ(portInMirror.has_value(), false);
      auto portEgMirror = port->getEgressMirror();
      EXPECT_EQ(portEgMirror.has_value(), false);
    } else {
      EXPECT_NE(entry, nullptr);
      auto action = entry->getAclAction();
      ASSERT_EQ(action.has_value(), true);
      auto aclInMirror = action.value().getIngressMirror();
      EXPECT_EQ(aclInMirror.has_value(), true);
      EXPECT_EQ(aclInMirror.value(), "mirror0");
      auto aclEgMirror = action.value().getEgressMirror();
      EXPECT_EQ(aclEgMirror.has_value(), true);
      EXPECT_EQ(aclEgMirror.value(), "mirror0");

      auto port = state_->getPorts()->getPortIf(ports[i]);
      EXPECT_NE(port, nullptr);
      auto portInMirror = port->getIngressMirror();
      EXPECT_EQ(portInMirror.has_value(), true);
      EXPECT_EQ(portInMirror.value(), "mirror0");
      auto portEgMirror = port->getEgressMirror();
      EXPECT_EQ(portEgMirror.has_value(), true);
      EXPECT_EQ(portEgMirror.value(), "mirror0");
    }
  }
}

TEST_F(MirrorTest, AclMirrorDelete) {
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  configureAcl("acl0");
  configureAclMirror("acl0", "mirror0");
  publishWithStateUpdate();

  config_.mirrors()->pop_back();
  publishWithFbossError();
}

TEST_F(MirrorTest, PortMirrorDelete) {
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror0", MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  configurePortMirror("mirror0", PortID(3));
  publishWithStateUpdate();

  config_.mirrors()->pop_back();
  publishWithFbossError();
}

TEST_F(MirrorTest, MirrorMirrorEgressPort) {
  config_.mirrors()->push_back(utility::getGREMirrorWithPort(
      "mirror0", MirrorTest::egressPort, MirrorTest::tunnelDestination));
  publishWithStateUpdate();
  configurePortMirror("mirror0", MirrorTest::egressPort);
  publishWithFbossError();
}

TEST_F(MirrorTest, ToAndFromDynamic) {
  config_.mirrors()->push_back(
      utility::getSPANMirror("span", MirrorTest::egressPort));

  config_.mirrors()->push_back(utility::getGREMirror(
      "unresolved",
      MirrorTest::tunnelDestination,
      folly::IPAddress("10.0.1.10")));
  config_.mirrors()->push_back(utility::getGREMirror(
      "resolved",
      MirrorTest::tunnelDestination,
      folly::IPAddress("10.0.1.10")));
  config_.mirrors()->push_back(utility::getGREMirror(
      "with_dscp", MirrorTest::tunnelDestination, std::nullopt, 3));
  config_.mirrors()->push_back(utility::getSFlowMirror(
      "with_tunnel_type",
      udpPorts.udpSrcPort,
      udpPorts.udpDstPort,
      MirrorTest::tunnelDestination,
      folly::IPAddress("10.0.1.10"),
      MirrorTest::dscp));
  publishWithStateUpdate();
  auto span = state_->getMirrors()->getMirrorIf("span");
  auto unresolved = state_->getMirrors()->getMirrorIf("unresolved");
  auto with_dscp = state_->getMirrors()->getMirrorIf("with_dscp");
  auto resolved = state_->getMirrors()->getMirrorIf("resolved");
  resolved->setEgressPort(MirrorTest::egressPort);
  resolved->setMirrorTunnel(MirrorTunnel(
      folly::IPAddress("1.1.1.1"),
      folly::IPAddress("2.2.2.2"),
      folly::MacAddress("1:1:1:1:1:1"),
      folly::MacAddress("2:2:2:2:2:2")));
  auto withTunnelType = state_->getMirrors()->getMirrorIf("with_tunnel_type");
  auto reconstructedState =
      SwitchState::fromFollyDynamic(state_->toFollyDynamic());
  EXPECT_EQ(
      *(reconstructedState->getMirrors()->getMirrorIf("span")),
      *(state_->getMirrors()->getMirrorIf("span")));
  EXPECT_EQ(
      *(reconstructedState->getMirrors()->getMirrorIf("unresolved")),
      *(state_->getMirrors()->getMirrorIf("unresolved")));
  EXPECT_EQ(
      *(reconstructedState->getMirrors()->getMirrorIf("resolved")),
      *(state_->getMirrors()->getMirrorIf("resolved")));
  EXPECT_EQ(
      *(reconstructedState->getMirrors()->getMirrorIf("with_dscp")),
      *(state_->getMirrors()->getMirrorIf("with_dscp")));
  EXPECT_EQ(
      *(reconstructedState->getMirrors()->getMirrorIf("with_tunnel_type")),
      *(state_->getMirrors()->getMirrorIf("with_tunnel_type")));
}

TEST_F(MirrorTest, GreMirrorWithSrcIP) {
  config_.mirrors()->push_back(utility::getGREMirror(
      "mirror0",
      MirrorTest::tunnelDestination,
      folly::IPAddress("10.0.0.1"),
      MirrorTest::dscp,
      true));
  publishWithStateUpdate();
  auto mirror0 = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_EQ(mirror0->getID(), "mirror0");
  EXPECT_EQ(mirror0->getDestinationIp(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror0->getSrcIp(), folly::IPAddress("10.0.0.1"));
  EXPECT_EQ(mirror0->getDscp(), MirrorTest::dscp);
  EXPECT_EQ(mirror0->getTruncate(), true);
  EXPECT_EQ(mirror0->getTunnelUdpPorts(), std::nullopt);
}

TEST_F(MirrorTest, SflowMirrorWithSrcIP) {
  config_.mirrors()->push_back(utility::getSFlowMirror(
      "mirror0",
      8998,
      9889,
      MirrorTest::tunnelDestination,
      folly::IPAddress("10.0.0.1"),
      MirrorTest::dscp,
      true));
  publishWithStateUpdate();
  auto mirror0 = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_EQ(mirror0->getID(), "mirror0");
  EXPECT_EQ(mirror0->getDestinationIp(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror0->getSrcIp(), folly::IPAddress("10.0.0.1"));
  EXPECT_EQ(mirror0->getDscp(), MirrorTest::dscp);
  EXPECT_EQ(mirror0->getTruncate(), true);
  EXPECT_TRUE(mirror0->getTunnelUdpPorts().has_value());
  EXPECT_EQ(mirror0->getTunnelUdpPorts().value().udpSrcPort, 8998);
  EXPECT_EQ(mirror0->getTunnelUdpPorts().value().udpDstPort, 9889);
}

TEST_F(MirrorTest, MirrorThrifty) {
  config_.mirrors()->push_back(
      utility::getSPANMirror("mirror0", MirrorTest::egressPort));
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror1", MirrorTest::tunnelDestination));
  config_.mirrors()->push_back(utility::getSFlowMirror(
      "mirror2",
      8998,
      9889,
      MirrorTest::tunnelDestination,
      folly::IPAddress("10.0.0.1"),
      MirrorTest::dscp,
      true));
  publishWithStateUpdate();
  auto& mirrors = state_->getMirrors();
  auto mirrorsThrift = mirrors->toThrift();
  auto newMirrors = MirrorMap::fromThrift(mirrorsThrift);
  EXPECT_EQ(*mirrors, *newMirrors);
  EXPECT_EQ(mirrorsThrift, newMirrors->toThrift());
}

TEST_F(MirrorTest, MirrorMapThrifty) {
  config_.mirrors()->push_back(
      utility::getSPANMirror("mirror0", MirrorTest::egressPort));
  config_.mirrors()->push_back(
      utility::getGREMirror("mirror1", MirrorTest::tunnelDestination));
  config_.mirrors()->push_back(utility::getSFlowMirror(
      "mirror2",
      8998,
      9889,
      MirrorTest::tunnelDestination,
      folly::IPAddress("10.0.0.1"),
      MirrorTest::dscp,
      true));
  publishWithStateUpdate();
  auto mirrors = state_->getMirrors()->modify(&state_);
  auto mirror2 = mirrors->getNode("mirror2")->clone();

  MirrorTunnel tunnel(
      mirror2->getSrcIp().value(),
      mirror2->getDestinationIp().value(),
      folly::MacAddress("1:2:3:4:5:6"),
      folly::MacAddress("6:5:4:3:2:1"),
      mirror2->getTunnelUdpPorts().value());

  mirror2->setMirrorTunnel(tunnel);
  mirror2->setEgressPort(PortID(1));
  mirrors->updateNode(mirror2);

  validateThriftyMigration(*mirrors);
  for (auto name : {"mirror0", "mirror1", "mirror2"}) {
    auto mirror = mirrors->getNode(name);
    validateThriftyMigration(*mirror);
    auto dyn = mirror->toFollyDynamicLegacy();
  }
}

} // namespace facebook::fboss
