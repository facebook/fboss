// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

namespace facebook {
namespace fboss {
class MirrorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    config_ = testConfigA();
    platform_ = createMockPlatform();
    state_ = testState(config_);
  }
  void configureMirror(
      const std::string& name,
      folly::IPAddress ip,
      const std::string& portName) {
    cfg::MirrorDestination destination;
    destination.ip_ref().value_unchecked() = ip.str();
    destination.egressPort_ref().value_unchecked().set_name(portName);
    destination.__isset.ip = true;
    destination.__isset.egressPort = true;
    auto mirrorCount = config_.mirrors.size() + 1;
    config_.mirrors.resize(mirrorCount);
    config_.mirrors[mirrorCount-1].name = name;
    config_.mirrors[mirrorCount-1].destination = destination;
    config_.mirrors[mirrorCount-1].__isset.name = true;
    config_.mirrors[mirrorCount-1].__isset.destination = true;
  }

  void configureMirror(
      const std::string& name,
      folly::IPAddress ip,
      const PortID& portID) {
    cfg::MirrorDestination destination;
    destination.ip_ref().value_unchecked() = ip.str();
    destination.egressPort_ref().value_unchecked().set_logicalID(portID);
    destination.__isset.ip = true;
    destination.__isset.egressPort = true;
    auto mirrorCount = config_.mirrors.size() + 1;
    config_.mirrors.resize(mirrorCount);
    config_.mirrors[mirrorCount - 1].name = name;
    config_.mirrors[mirrorCount - 1].destination = destination;
    config_.mirrors[mirrorCount - 1].__isset.name = true;
    config_.mirrors[mirrorCount - 1].__isset.destination = true;
  }

  void configureMirror(const std::string& name, const std::string& portName) {
    cfg::MirrorDestination destination;
    destination.__isset.egressPort = true;
    destination.egressPort_ref().value_unchecked().set_name(portName);

    auto mirrorCount = config_.mirrors.size() + 1;
    config_.mirrors.resize(mirrorCount);
    config_.mirrors[mirrorCount-1].name = name;
    config_.mirrors[mirrorCount-1].destination = destination;
    config_.mirrors[mirrorCount-1].__isset.name = true;
    config_.mirrors[mirrorCount-1].__isset.destination = true;
  }

  void configureMirror(const std::string& name, const PortID& portID) {
    cfg::MirrorDestination destination;
    destination.__isset.egressPort = true;
    destination.egressPort_ref().value_unchecked().set_logicalID(portID);

    auto mirrorCount = config_.mirrors.size() + 1;
    config_.mirrors.resize(mirrorCount);
    config_.mirrors[mirrorCount - 1].name = name;
    config_.mirrors[mirrorCount - 1].destination = destination;
    config_.mirrors[mirrorCount - 1].__isset.name = true;
    config_.mirrors[mirrorCount - 1].__isset.destination = true;
  }

  void configureMirror(const std::string& name) {
    auto mirrorCount = config_.mirrors.size() + 1;
    config_.mirrors.resize(mirrorCount);
    config_.mirrors[mirrorCount-1].name = name;
    config_.mirrors[mirrorCount-1].__isset.name = true;
  }

  void configureMirror(const std::string& name, folly::IPAddress ip) {
    cfg::MirrorDestination destination;
    destination.ip_ref() = ip.str();

    auto mirrorCount = config_.mirrors.size() + 1;
    config_.mirrors.resize(mirrorCount);
    config_.mirrors[mirrorCount-1].name = name;
    config_.mirrors[mirrorCount-1].destination = destination;
    config_.mirrors[mirrorCount-1].__isset.name = true;
    config_.mirrors[mirrorCount-1].__isset.destination = true;
  }

  void configureAcl(const std::string& name, uint16_t dstL4Port=1234) {
    cfg::AclEntry aclEntry;
    auto aclCount = config_.acls.size() + 1;
    config_.acls.resize(aclCount);
    config_.acls[aclCount-1].name = name;
    config_.acls[aclCount-1].actionType = cfg::AclActionType::PERMIT;
    config_.acls[aclCount - 1].dstL4PortRange_ref().value_unchecked().min =
        dstL4Port;
    config_.acls[aclCount - 1].dstL4PortRange_ref().value_unchecked().max =
        dstL4Port;
    config_.acls[aclCount-1].__isset.dstL4PortRange = true;
  }

  void configurePortMirror(const std::string& mirror, PortID port) {
    int portIndex = int(port) - 1;
    config_.ports[portIndex].__isset.ingressMirror = true;
    config_.ports[portIndex].ingressMirror_ref().value_unchecked() = mirror;
    config_.ports[portIndex].__isset.egressMirror = true;
    config_.ports[portIndex].egressMirror_ref().value_unchecked() = mirror;
  }

  void configureAclMirror(
      const std::string& name,
      const std::string& mirror) {
    cfg::MatchAction action;
    action.ingressMirror_ref() = mirror;
    action.egressMirror_ref() = mirror;

    cfg::MatchToAction mirrorAction;
    mirrorAction.matcher = name;
    mirrorAction.action = action;
    config_.__isset.dataPlaneTrafficPolicy = true;
    auto count = config_.dataPlaneTrafficPolicy_ref()
                     .value_unchecked()
                     .matchToAction.size() +
        1;
    config_.dataPlaneTrafficPolicy_ref().value_unchecked().matchToAction.resize(
        count);
    config_.dataPlaneTrafficPolicy_ref()
        .value_unchecked()
        .matchToAction[count - 1] = mirrorAction;
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
};

const folly::IPAddress MirrorTest::tunnelDestination =
    folly::IPAddress("10.0.0.1");
const char* MirrorTest::egressPortName = "port5";
const PortID MirrorTest::egressPort = PortID(5);

TEST_F(MirrorTest, MirrorWithPort) {
  configureMirror("mirror0", MirrorTest::egressPortName);
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.hasValue(), true);
  EXPECT_EQ(port.value(), egressPort);
}

TEST_F(MirrorTest, MirrorWithPortId) {
  configureMirror("mirror0", MirrorTest::egressPort);
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.hasValue(), true);
  EXPECT_EQ(port.value(), egressPort);
}

TEST_F(MirrorTest, MirrorWithIp) {
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), false);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.hasValue(), false);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.hasValue(), true);
  EXPECT_EQ(ip.value(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror->isResolved(), false);
}

TEST_F(MirrorTest, MirrorWithPortAndIp) {
  configureMirror(
      "mirror0",
      MirrorTest::tunnelDestination,
      MirrorTest::egressPortName);
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.hasValue(), true);
  EXPECT_EQ(port.value(), egressPort);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.hasValue(), true);
  EXPECT_EQ(ip.value(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror->isResolved(), false);
}

TEST_F(MirrorTest, MirrorWithPortIdAndIp) {
  configureMirror(
      "mirror0", MirrorTest::tunnelDestination, MirrorTest::egressPort);
  publishWithStateUpdate();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), true);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.hasValue(), true);
  EXPECT_EQ(port.value(), egressPort);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.hasValue(), true);
  EXPECT_EQ(ip.value(), MirrorTest::tunnelDestination);
  EXPECT_EQ(mirror->isResolved(), false);
}

TEST_F(MirrorTest, MirrorWithNoPortNoIp) {
  configureMirror("mirror0");
  publishWithFbossError();
  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_EQ(mirror, nullptr);
}

TEST_F(MirrorTest, AclMirror) {
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();
  configureAcl("acl0");
  configureAclMirror("acl0", "mirror0");
  publishWithStateUpdate();
  auto entry = state_->getAcls()->getEntryIf("acl0");
  EXPECT_NE(entry, nullptr);
  auto action = entry->getAclAction();
  ASSERT_EQ(action.hasValue(), true);
  auto inMirror = action.value().getIngressMirror();
  EXPECT_EQ(inMirror.hasValue(), true);
  EXPECT_EQ(inMirror.value(), "mirror0");
  auto egMirror = action.value().getEgressMirror();
  EXPECT_EQ(egMirror.hasValue(), true);
  EXPECT_EQ(egMirror.value(), "mirror0");
}

TEST_F(MirrorTest, PortMirror) {
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();
  configurePortMirror("mirror0", PortID(3));
  publishWithStateUpdate();
  auto port = state_->getPorts()->getPortIf(PortID(3));
  EXPECT_NE(port, nullptr);
  auto inMirror = port->getIngressMirror();
  EXPECT_EQ(inMirror.hasValue(), true);
  EXPECT_EQ(inMirror.value(), "mirror0");
  auto egMirror = port->getEgressMirror();
  EXPECT_EQ(egMirror.hasValue(), true);
  EXPECT_EQ(egMirror.value(), "mirror0");
}

TEST_F(MirrorTest, AclWrongMirror) {
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();
  configureAcl("acl0");
  configureAclMirror("acl0", "mirror1");
  publishWithFbossError();
}

TEST_F(MirrorTest, PortWrongMirror) {
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();
  configurePortMirror("mirror1", PortID(3));
  publishWithFbossError();
}

TEST_F(MirrorTest, MirrorWrongPort) {
  configureMirror("mirror0", MirrorTest::tunnelDestination, "port129");
  publishWithFbossError();
}

TEST_F(MirrorTest, MirrorWrongPortId) {
  configureMirror("mirror0", MirrorTest::tunnelDestination, PortID(129));
  publishWithFbossError();
}

TEST_F(MirrorTest, NoStateChange) {
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();
  config_.mirrors[0].destination.ip_ref().value_unchecked() =
      MirrorTest::tunnelDestination.str();
  publishWithNoStateUpdate();
}

TEST_F(MirrorTest, WithStateChange) {
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();
  config_.mirrors[0].destination.ip_ref().value_unchecked() = "10.0.0.2";
  publishWithStateUpdate();

  auto mirror = state_->getMirrors()->getMirrorIf("mirror0");
  EXPECT_NE(mirror, nullptr);
  EXPECT_EQ(mirror->getID(), "mirror0");
  EXPECT_EQ(mirror->configHasEgressPort(), false);
  auto port = mirror->getEgressPort();
  EXPECT_EQ(port.hasValue(), false);
  auto ip = mirror->getDestinationIp();
  EXPECT_EQ(ip.hasValue(), true);
  EXPECT_EQ(ip.value(), folly::IPAddress("10.0.0.2"));
  EXPECT_EQ(mirror->isResolved(), false);
}

TEST_F(MirrorTest, AddAclAndPortToMirror) {
  std::array<std::string, 2> acls {"acl0", "acl1"};
  std::array<PortID, 2> ports {PortID(3), PortID(4)};
  uint16 l4port = 1234;
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();

  for (int i=0; i<2; i++) {
    configureAcl(acls[i], l4port+i);
    configureAclMirror(acls[i], "mirror0");
    configurePortMirror("mirror0", ports[i]);
    publishWithStateUpdate();
  }

  for (int i=0; i<2; i++) {
    auto entry = state_->getAcls()->getEntryIf(acls[i]);
    EXPECT_NE(entry, nullptr);
    auto action = entry->getAclAction();
    ASSERT_EQ(action.hasValue(), true);
    auto aclInMirror = action.value().getIngressMirror();
    EXPECT_EQ(aclInMirror.hasValue(), true);
    EXPECT_EQ(aclInMirror.value(), "mirror0");
    auto aclEgMirror = action.value().getEgressMirror();
    EXPECT_EQ(aclEgMirror.hasValue(), true);
    EXPECT_EQ(aclEgMirror.value(), "mirror0");

    auto port = state_->getPorts()->getPortIf(ports[i]);
    EXPECT_NE(port, nullptr);
    auto portInMirror = port->getIngressMirror();
    EXPECT_EQ(portInMirror.hasValue(), true);
    EXPECT_EQ(portInMirror.value(), "mirror0");
  }
}

TEST_F(MirrorTest, DeleleteAclAndPortToMirror) {
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();

  std::array<std::string, 2> acls {"acl0", "acl1"};
  std::array<PortID, 2> ports {PortID(3), PortID(4)};
  uint16 l4port = 1234;

  for (int i=0; i<2; i++) {
    configureAcl(acls[i], l4port+i);
    configureAclMirror(acls[i], "mirror0");
    configurePortMirror("mirror0", ports[i]);
  }
  publishWithStateUpdate();

  auto portIndex = int(PortID(4)) - 1;
  config_.ports[portIndex].__isset.ingressMirror = false;
  config_.ports[portIndex].ingressMirror_ref().value_unchecked() = "";
  config_.ports[portIndex].__isset.egressMirror = false;
  config_.ports[portIndex].egressMirror_ref().value_unchecked() = "";

  config_.acls.pop_back();
  config_.dataPlaneTrafficPolicy_ref()
      .value_unchecked()
      .matchToAction.pop_back();

  publishWithStateUpdate();

  for (int i=0; i<2; i++) {
    auto entry = state_->getAcls()->getEntryIf(acls[i]);
    if (i) {
      EXPECT_EQ(entry, nullptr);
      auto port = state_->getPorts()->getPortIf(ports[i]);
      EXPECT_NE(port, nullptr);
      auto portInMirror = port->getIngressMirror();
      EXPECT_EQ(portInMirror.hasValue(), false);
      auto portEgMirror = port->getEgressMirror();
      EXPECT_EQ(portEgMirror.hasValue(), false);
    } else {
      EXPECT_NE(entry, nullptr);
      auto action = entry->getAclAction();
      ASSERT_EQ(action.hasValue(), true);
      auto aclInMirror = action.value().getIngressMirror();
      EXPECT_EQ(aclInMirror.hasValue(), true);
      EXPECT_EQ(aclInMirror.value(), "mirror0");
      auto aclEgMirror = action.value().getEgressMirror();
      EXPECT_EQ(aclEgMirror.hasValue(), true);
      EXPECT_EQ(aclEgMirror.value(), "mirror0");

      auto port = state_->getPorts()->getPortIf(ports[i]);
      EXPECT_NE(port, nullptr);
      auto portInMirror = port->getIngressMirror();
      EXPECT_EQ(portInMirror.hasValue(), true);
      EXPECT_EQ(portInMirror.value(), "mirror0");
      auto portEgMirror = port->getEgressMirror();
      EXPECT_EQ(portEgMirror.hasValue(), true);
      EXPECT_EQ(portEgMirror.value(), "mirror0");
    }
  }
}

TEST_F(MirrorTest, AclMirrorDelete) {
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();
  configureAcl("acl0");
  configureAclMirror("acl0", "mirror0");
  publishWithStateUpdate();

  config_.mirrors.pop_back();
  publishWithFbossError();
}

TEST_F(MirrorTest, PortMirrorDelete) {
  configureMirror("mirror0", MirrorTest::tunnelDestination);
  publishWithStateUpdate();
  configurePortMirror("mirror0", PortID(3));
  publishWithStateUpdate();

  config_.mirrors.pop_back();
  publishWithFbossError();
}

TEST_F(MirrorTest, MirrorMirrorEgressPort) {
  configureMirror(
      "mirror0", MirrorTest::tunnelDestination, MirrorTest::egressPort);
  publishWithStateUpdate();
  configurePortMirror("mirror0", MirrorTest::egressPort);
  publishWithFbossError();
}

TEST_F(MirrorTest, ToAndFromDynamic) {
  configureMirror("span", MirrorTest::egressPort);
  configureMirror(
      "unresolved", MirrorTest::tunnelDestination);
  configureMirror(
      "resolved", MirrorTest::tunnelDestination);
  publishWithStateUpdate();
  auto span = state_->getMirrors()->getMirrorIf("span");
  auto unresolved = state_->getMirrors()->getMirrorIf("unresolved");
  auto resolved = state_->getMirrors()->getMirrorIf("resolved");
  resolved->setEgressPort(MirrorTest::egressPort);
  resolved->setMirrorTunnel(MirrorTunnel(
      folly::IPAddress("1.1.1.1"),
      folly::IPAddress("2.2.2.2"),
      folly::MacAddress("1:1:1:1:1:1"),
      folly::MacAddress("2:2:2:2:2:2")));

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
}

} // namespace fboss
} // namespace facebook
