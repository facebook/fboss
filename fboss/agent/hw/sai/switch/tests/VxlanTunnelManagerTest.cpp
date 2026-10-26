// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/agent/hw/sai/api/TunnelMapApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/sai/switch/SaiVxlanTunnelManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/VxlanTunnel.h"

#include <fmt/format.h>
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

TEST_F(VxlanTunnelManagerTest, listVxlanObjects) {
  auto swTunnel = makeVxlanTunnel("vxlantunnel0", intf0.id);
  saiManagerTable->vxlanTunnelManager().addVxlanTunnel(swTunnel);
  auto tunnelSaiId = saiManagerTable->vxlanTunnelManager()
                         .getVxlanTunnelHandle("vxlantunnel0")
                         ->tunnel->adapterKey();

  auto* vrHandle =
      saiManagerTable->virtualRouterManager().getVirtualRouterHandle(
          RouterID(0));
  ASSERT_NE(vrHandle, nullptr);
  auto vrSaiId = vrHandle->virtualRouter->adapterKey();

  // P2MP tunnel term matching the tunnel's dst IP.
  SaiP2MPTunnelTermTraits::CreateAttributes termAttrs{
      SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP,
      vrSaiId,
      folly::IPAddress("2.2.2.2"),
      SAI_TUNNEL_TYPE_VXLAN,
      tunnelSaiId};
  auto termObj =
      saiStore->get<SaiP2MPTunnelTermTraits>().setObject(termAttrs, termAttrs);

  // One map per direction plus one entry in each.
  SaiTunnelMapTraits::CreateAttributes vniToVrMapAttrs{
      SAI_TUNNEL_MAP_TYPE_VNI_TO_VIRTUAL_ROUTER_ID};
  auto vniToVrMap = saiStore->get<SaiTunnelMapTraits>().setObject(
      vniToVrMapAttrs, vniToVrMapAttrs);
  SaiTunnelMapTraits::CreateAttributes vrToVniMapAttrs{
      SAI_TUNNEL_MAP_TYPE_VIRTUAL_ROUTER_ID_TO_VNI};
  auto vrToVniMap = saiStore->get<SaiTunnelMapTraits>().setObject(
      vrToVniMapAttrs, vrToVniMapAttrs);
  SaiVniToVrTunnelMapEntryTraits::CreateAttributes vniToVrEntryAttrs{
      SAI_TUNNEL_MAP_TYPE_VNI_TO_VIRTUAL_ROUTER_ID,
      vniToVrMap->adapterKey(),
      100,
      vrSaiId};
  auto vniToVrEntry = saiStore->get<SaiVniToVrTunnelMapEntryTraits>().setObject(
      vniToVrEntryAttrs, vniToVrEntryAttrs);
  SaiVrToVniTunnelMapEntryTraits::CreateAttributes vrToVniEntryAttrs{
      SAI_TUNNEL_MAP_TYPE_VIRTUAL_ROUTER_ID_TO_VNI,
      vrToVniMap->adapterKey(),
      vrSaiId,
      100};
  auto vrToVniEntry = saiStore->get<SaiVrToVniTunnelMapEntryTraits>().setObject(
      vrToVniEntryAttrs, vrToVniEntryAttrs);

  auto* saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  std::string out = saiSwitch->listObjects({HwObjectType::VXLAN}, true);

  // All four VXLAN object types are reported with their objects.
  for (const auto& header :
       {"Object type: tunnel\n",
        "Object type: tunnel-term\n",
        "Object type: tunnel-map\n",
        "Object type: tunnel-map-entry\n"}) {
    EXPECT_NE(out.find(header), std::string::npos) << header;
  }
  for (const auto& key : {
           fmt::format("{}", tunnelSaiId),
           fmt::format("{}", termObj->adapterKey()),
           fmt::format("{}", vniToVrMap->adapterKey()),
           fmt::format("{}", vrToVniMap->adapterKey()),
           fmt::format("{}", vniToVrEntry->adapterKey()),
           fmt::format("{}", vrToVniEntry->adapterKey()),
       }) {
    EXPECT_NE(out.find(key), std::string::npos) << key;
  }
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
