// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSrv6TunnelManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/Srv6Tunnel.h"

#include <string>

using namespace facebook::fboss;

std::shared_ptr<Srv6Tunnel> makeSrv6Tunnel(
    const std::string& tunnelId = "srv6tunnel0",
    uint32_t intfID = 0) {
  auto tunnel = std::make_shared<Srv6Tunnel>(tunnelId);
  tunnel->setType(TunnelType::SRV6_ENCAP);
  tunnel->setUnderlayIntfId(InterfaceID(intfID));
  tunnel->setSrcIP(folly::IPAddressV6("2001:db8::1"));
  tunnel->setTTLMode(cfg::TunnelMode::PIPE);
  tunnel->setDscpMode(cfg::TunnelMode::UNIFORM);
  tunnel->setEcnMode(cfg::TunnelMode::UNIFORM);
  return tunnel;
}

class Srv6TunnelManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
  }

  TestInterface intf0;
};

TEST_F(Srv6TunnelManagerTest, addSrv6Tunnel) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);
  auto handle =
      saiManagerTable->srv6TunnelManager().getSrv6TunnelHandle("srv6tunnel0");
  EXPECT_NE(handle, nullptr);
}

TEST_F(Srv6TunnelManagerTest, addTwoSrv6Tunnels) {
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(
      makeSrv6Tunnel("srv6tunnel0", intf0.id));
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(
      makeSrv6Tunnel("srv6tunnel1", intf0.id));
  EXPECT_NE(
      saiManagerTable->srv6TunnelManager().getSrv6TunnelHandle("srv6tunnel0"),
      nullptr);
  EXPECT_NE(
      saiManagerTable->srv6TunnelManager().getSrv6TunnelHandle("srv6tunnel1"),
      nullptr);
}

TEST_F(Srv6TunnelManagerTest, addDupSrv6Tunnel) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);
  EXPECT_THROW(
      saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel), FbossError);
}

TEST_F(Srv6TunnelManagerTest, changeSrv6Tunnel) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);
  auto swTunnel2 = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  swTunnel2->setSrcIP(
      folly::IPAddressV6("2001:db8:3333:4444:5555:6666:7777:8888"));
  saiManagerTable->srv6TunnelManager().changeSrv6Tunnel(swTunnel, swTunnel2);
  auto handle =
      saiManagerTable->srv6TunnelManager().getSrv6TunnelHandle("srv6tunnel0");
  EXPECT_NE(handle, nullptr);
}

TEST_F(Srv6TunnelManagerTest, getNonexistentSrv6Tunnel) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);
  auto handle =
      saiManagerTable->srv6TunnelManager().getSrv6TunnelHandle("srv6tunnel1");
  EXPECT_FALSE(handle);
}

TEST_F(Srv6TunnelManagerTest, removeSrv6Tunnel) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);
  saiManagerTable->srv6TunnelManager().removeSrv6Tunnel(swTunnel);
  auto handle =
      saiManagerTable->srv6TunnelManager().getSrv6TunnelHandle("srv6tunnel0");
  EXPECT_FALSE(handle);
}
