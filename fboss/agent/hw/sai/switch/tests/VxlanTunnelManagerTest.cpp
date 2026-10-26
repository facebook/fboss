// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/agent/hw/sai/switch/SaiVxlanTunnelManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/VxlanTunnel.h"

#include <string>

using namespace facebook::fboss;

std::shared_ptr<VxlanTunnel> makeVxlanTunnel(
    const std::string& tunnelId = "vxlantunnel0",
    uint32_t intfID = 0) {
  auto tunnel = std::make_shared<VxlanTunnel>(tunnelId);
  tunnel->setType(TunnelType::VXLAN);
  tunnel->setUnderlayIntfId(InterfaceID(intfID));
  tunnel->setSrcIP(folly::IPAddress("1.1.1.1"));
  tunnel->setDstIP(folly::IPAddress("2.2.2.2"));
  tunnel->setTTLMode(cfg::TunnelMode::PIPE);
  tunnel->setDscpMode(cfg::TunnelMode::UNIFORM);
  tunnel->setEcnMode(cfg::TunnelMode::UNIFORM);
  return tunnel;
}

class VxlanTunnelManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
  }

  TestInterface intf0;
};

TEST_F(VxlanTunnelManagerTest, addVxlanTunnel) {
  auto swTunnel = makeVxlanTunnel("vxlantunnel0", intf0.id);
  saiManagerTable->vxlanTunnelManager().addVxlanTunnel(swTunnel);
  auto handle = saiManagerTable->vxlanTunnelManager().getVxlanTunnelHandle(
      "vxlantunnel0");
  EXPECT_NE(handle, nullptr);
}

TEST_F(VxlanTunnelManagerTest, addTwoVxlanTunnels) {
  saiManagerTable->vxlanTunnelManager().addVxlanTunnel(
      makeVxlanTunnel("vxlantunnel0", intf0.id));
  saiManagerTable->vxlanTunnelManager().addVxlanTunnel(
      makeVxlanTunnel("vxlantunnel1", intf0.id));
  EXPECT_NE(
      saiManagerTable->vxlanTunnelManager().getVxlanTunnelHandle(
          "vxlantunnel0"),
      nullptr);
  EXPECT_NE(
      saiManagerTable->vxlanTunnelManager().getVxlanTunnelHandle(
          "vxlantunnel1"),
      nullptr);
}

TEST_F(VxlanTunnelManagerTest, addDupVxlanTunnel) {
  auto swTunnel = makeVxlanTunnel("vxlantunnel0", intf0.id);
  saiManagerTable->vxlanTunnelManager().addVxlanTunnel(swTunnel);
  EXPECT_THROW(
      saiManagerTable->vxlanTunnelManager().addVxlanTunnel(swTunnel),
      FbossError);
}

TEST_F(VxlanTunnelManagerTest, changeVxlanTunnel) {
  auto swTunnel = makeVxlanTunnel("vxlantunnel0", intf0.id);
  saiManagerTable->vxlanTunnelManager().addVxlanTunnel(swTunnel);
  auto swTunnel2 = makeVxlanTunnel("vxlantunnel0", intf0.id);
  swTunnel2->setSrcIP(folly::IPAddress("3.3.3.3"));
  saiManagerTable->vxlanTunnelManager().changeVxlanTunnel(swTunnel, swTunnel2);
  auto handle = saiManagerTable->vxlanTunnelManager().getVxlanTunnelHandle(
      "vxlantunnel0");
  EXPECT_NE(handle, nullptr);
}

TEST_F(VxlanTunnelManagerTest, getNonexistentVxlanTunnel) {
  auto swTunnel = makeVxlanTunnel("vxlantunnel0", intf0.id);
  saiManagerTable->vxlanTunnelManager().addVxlanTunnel(swTunnel);
  auto handle = saiManagerTable->vxlanTunnelManager().getVxlanTunnelHandle(
      "vxlantunnel1");
  EXPECT_FALSE(handle);
}

TEST_F(VxlanTunnelManagerTest, removeVxlanTunnel) {
  auto swTunnel = makeVxlanTunnel("vxlantunnel0", intf0.id);
  saiManagerTable->vxlanTunnelManager().addVxlanTunnel(swTunnel);
  saiManagerTable->vxlanTunnelManager().removeVxlanTunnel(swTunnel);
  auto handle = saiManagerTable->vxlanTunnelManager().getVxlanTunnelHandle(
      "vxlantunnel0");
  EXPECT_FALSE(handle);
}

TEST_F(VxlanTunnelManagerTest, addNonVxlanTunnelFails) {
  auto swTunnel = makeVxlanTunnel("vxlantunnel0", intf0.id);
  swTunnel->setType(TunnelType::SRV6_ENCAP);
  EXPECT_THROW(
      saiManagerTable->vxlanTunnelManager().addVxlanTunnel(swTunnel),
      FbossError);
}

TEST_F(VxlanTunnelManagerTest, addVxlanTunnelWithoutSrcIpFails) {
  // Build a tunnel without a src IP.
  const std::string tunnelId("vxlantunnel0");
  auto noSrc = std::make_shared<VxlanTunnel>(tunnelId);
  noSrc->setType(TunnelType::VXLAN);
  noSrc->setUnderlayIntfId(InterfaceID(intf0.id));
  noSrc->setDstIP(folly::IPAddress("2.2.2.2"));
  EXPECT_THROW(
      saiManagerTable->vxlanTunnelManager().addVxlanTunnel(noSrc), FbossError);
}
