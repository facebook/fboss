// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiTunnelManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/types.h"
#include "folly/IPAddressV6.h"

#include <string>

using namespace facebook::fboss;

std::shared_ptr<IpTunnel> makeTunnel(
    std::string tunnelId = "tunnel0",
    int32_t mode = 1) {
  auto tunnel = std::make_shared<IpTunnel>(tunnelId);
  tunnel->setType(IPINIP);
  tunnel->setUnderlayIntfId(InterfaceID(42));
  tunnel->setTTLMode(mode);
  tunnel->setDscpMode(mode);
  tunnel->setEcnMode(mode);
  tunnel->setTunnelTermType(MP2MP);
  tunnel->setDstIP(folly::IPAddressV6("2401:db00:11c:8202:0:0:0:100"));
  tunnel->setSrcIP(folly::IPAddressV6("::"));
  tunnel->setDstIPMask(folly::IPAddressV6("::"));
  tunnel->setSrcIPMask(folly::IPAddressV6("::"));
  return tunnel;
}

class TunnelManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::BLANK;
    ManagerTestBase::SetUp();
  }

  void checkTunnel(
      TunnelSaiId saiId,
      std::string id,
      int32_t expType,
      int32_t expTermType,
      InterfaceID expUnderlay,
      InterfaceID expOverlay,
      int32_t expTtl,
      int32_t expDscp,
      int32_t expEcn,
      const folly::IPAddress& expDstIp,
      const folly::IPAddress& expSrcIp) {
    auto type = saiApiTable->tunnelApi().getAttribute(
        saiId, SaiTunnelTraits::Attributes::Type());
    EXPECT_EQ(type, expType);
    uint32_t underlay = saiApiTable->tunnelApi().getAttribute(
        saiId, SaiTunnelTraits::Attributes::UnderlayInterface());
    EXPECT_EQ(underlay, expUnderlay);
    uint32_t overlay = saiApiTable->tunnelApi().getAttribute(
        saiId, SaiTunnelTraits::Attributes::UnderlayInterface());
    EXPECT_EQ(overlay, expOverlay);
    auto ttl = saiApiTable->tunnelApi().getAttribute(
        saiId, SaiTunnelTraits::Attributes::DecapTtlMode());
    EXPECT_EQ(ttl, expTtl);
    auto dscp = saiApiTable->tunnelApi().getAttribute(
        saiId, SaiTunnelTraits::Attributes::DecapDscpMode());
    EXPECT_EQ(dscp, expDscp);
    auto ecn = saiApiTable->tunnelApi().getAttribute(
        saiId, SaiTunnelTraits::Attributes::DecapEcnMode());
    EXPECT_EQ(ecn, expEcn);
    auto handle = saiManagerTable->tunnelManager().getTunnelHandle(id);
    SaiVirtualRouterHandle* virtualRouterHandle =
        saiManagerTable->virtualRouterManager().getVirtualRouterHandle(
            RouterID(0));
    VirtualRouterSaiId saiVirtualRouterId{
        virtualRouterHandle->virtualRouter->adapterKey()};
    EXPECT_EQ(
        GET_ATTR(TunnelTerm, VrId, handle->tunnelTerm->attributes()),
        saiVirtualRouterId);
    EXPECT_EQ(
        GET_ATTR(TunnelTerm, Type, handle->tunnelTerm->attributes()),
        expTermType);
    EXPECT_EQ(
        GET_ATTR(TunnelTerm, TunnelType, handle->tunnelTerm->attributes()),
        expType);
    EXPECT_EQ(
        GET_ATTR(TunnelTerm, DstIp, handle->tunnelTerm->attributes()),
        expDstIp);
    EXPECT_EQ(
        GET_ATTR(TunnelTerm, SrcIp, handle->tunnelTerm->attributes()),
        expSrcIp);
    EXPECT_EQ(
        GET_OPT_ATTR(TunnelTerm, DstIpMask, handle->tunnelTerm->attributes()),
        folly::IPAddressV6("::"));
    EXPECT_EQ(
        GET_OPT_ATTR(TunnelTerm, SrcIpMask, handle->tunnelTerm->attributes()),
        folly::IPAddressV6("::"));
    EXPECT_EQ(
        GET_ATTR(TunnelTerm, ActionTunnelId, handle->tunnelTerm->attributes()),
        saiId);
  }
};

TEST_F(TunnelManagerTest, addTunnel) {
  std::shared_ptr<IpTunnel> swTunnel = makeTunnel("tunnel0");
  TunnelSaiId saiId = saiManagerTable->tunnelManager().addTunnel(swTunnel);
  int expMode = 1; // TTL, DSCP and ECN should be the same value
  checkTunnel(
      saiId,
      "tunnel0",
      SAI_TUNNEL_TYPE_IPINIP,
      SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_MP2MP,
      InterfaceID(42),
      InterfaceID(42),
      expMode,
      expMode,
      expMode,
      folly::IPAddressV6("2401:db00:11c:8202:0:0:0:100"),
      folly::IPAddressV6("::"));
}

TEST_F(TunnelManagerTest, addTwoTunnels) {
  TunnelSaiId saiId0 =
      saiManagerTable->tunnelManager().addTunnel(makeTunnel("tunn0"));
  int expMode = 1;
  checkTunnel(
      saiId0,
      "tunn0",
      SAI_TUNNEL_TYPE_IPINIP,
      SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_MP2MP,
      InterfaceID(42),
      InterfaceID(42),
      expMode,
      expMode,
      expMode,
      folly::IPAddressV6("2401:db00:11c:8202:0:0:0:100"),
      folly::IPAddressV6("::"));
  TunnelSaiId saiId1 =
      saiManagerTable->tunnelManager().addTunnel(makeTunnel("tunn1"));
  checkTunnel(
      saiId1,
      "tunn1",
      SAI_TUNNEL_TYPE_IPINIP,
      SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_MP2MP,
      InterfaceID(42),
      InterfaceID(42),
      expMode,
      expMode,
      expMode,
      folly::IPAddressV6("2401:db00:11c:8202:0:0:0:100"),
      folly::IPAddressV6("::"));
}

TEST_F(TunnelManagerTest, addDupTunnel) {
  std::shared_ptr<IpTunnel> swTunnel = makeTunnel();
  saiManagerTable->tunnelManager().addTunnel(swTunnel);
  EXPECT_THROW(
      saiManagerTable->tunnelManager().addTunnel(swTunnel), FbossError);
}

TEST_F(TunnelManagerTest, changeTunnel) {
  std::shared_ptr<IpTunnel> swTunnel = makeTunnel("tunnel0");
  saiManagerTable->tunnelManager().addTunnel(swTunnel);
  auto swTunnel2 = makeTunnel("tunnel1", 0);
  swTunnel2->setDstIP(
      folly::IPAddressV6("2001:db8:3333:4444:5555:6666:7777:8888"));
  swTunnel2->setSrcIPMask(folly::IPAddressV6("2001:db8::"));
  saiManagerTable->tunnelManager().changeTunnel(swTunnel, swTunnel2);
  auto handle = saiManagerTable->tunnelManager().getTunnelHandle("tunnel1");
  EXPECT_NE(handle, nullptr);
  EXPECT_EQ(
      GET_OPT_ATTR(Tunnel, DecapTtlMode, handle->tunnel->attributes()), 0);
  EXPECT_EQ(
      GET_ATTR(TunnelTerm, DstIp, handle->tunnelTerm->attributes()),
      folly::IPAddressV6("2001:db8:3333:4444:5555:6666:7777:8888"));
  EXPECT_EQ(
      GET_OPT_ATTR(TunnelTerm, SrcIpMask, handle->tunnelTerm->attributes()),
      folly::IPAddressV6("2001:db8::"));
  SaiVirtualRouterHandle* virtualRouterHandle =
      saiManagerTable->virtualRouterManager().getVirtualRouterHandle(
          RouterID(0));
  VirtualRouterSaiId saiVirtualRouterId{
      virtualRouterHandle->virtualRouter->adapterKey()};
  EXPECT_EQ(
      GET_ATTR(TunnelTerm, VrId, handle->tunnelTerm->attributes()),
      saiVirtualRouterId);
}

TEST_F(TunnelManagerTest, getNonexistentTunnel) {
  auto swTunnel = makeTunnel("tunnel0");
  saiManagerTable->tunnelManager().addTunnel(swTunnel);
  auto handle = saiManagerTable->tunnelManager().getTunnelHandle("tunnel1");
  EXPECT_FALSE(handle);
}

TEST_F(TunnelManagerTest, removeTunnel) {
  auto swTunnel = makeTunnel("tunnel0");
  saiManagerTable->tunnelManager().addTunnel(swTunnel);
  auto& tunnelManager = saiManagerTable->tunnelManager();
  tunnelManager.removeTunnel(swTunnel);
  auto handle = saiManagerTable->tunnelManager().getTunnelHandle("tunnel0");
  EXPECT_FALSE(handle);
}

TEST_F(TunnelManagerTest, removeNonexistentTunnel) {
  auto swTunnel = makeTunnel("tunnel0");
  saiManagerTable->tunnelManager().addTunnel(swTunnel);
  auto& tunnelManager = saiManagerTable->tunnelManager();
  auto swTunnel2 = makeTunnel("tunnel1");
  EXPECT_THROW(tunnelManager.removeTunnel(swTunnel2), FbossError);
}
