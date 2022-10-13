// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/TunnelApi.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>
#include <optional>

using namespace facebook::fboss;

static constexpr folly::StringPiece dip = "42.42.12.34";
static constexpr folly::StringPiece sip = "42.43.56.78";

class TunnelApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    tunnelApi = std::make_unique<TunnelApi>();
  }

  TunnelSaiId createTunnel(
      sai_tunnel_type_t _type,
      sai_object_id_t _underlay,
      sai_object_id_t _overlay) {
    SaiTunnelTraits::Attributes::Type type{_type};
    SaiTunnelTraits::Attributes::UnderlayInterface underlay{_underlay};
    SaiTunnelTraits::Attributes::OverlayInterface overlay{_overlay};
    SaiTunnelTraits::Attributes::DecapTtlMode ttlMode{
        SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL};
    SaiTunnelTraits::Attributes::DecapDscpMode dscpMode{
        SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL};
    SaiTunnelTraits::Attributes::DecapEcnMode ecnMode{
        SAI_TUNNEL_DECAP_ECN_MODE_STANDARD};

    SaiTunnelTraits::CreateAttributes a{
        type, underlay, overlay, ttlMode, dscpMode, ecnMode};
    return tunnelApi->create<SaiTunnelTraits>(a, 0);
  }

  void checkTunnel(TunnelSaiId id) const {
    SaiTunnelTraits::Attributes::Type type;
    auto gotType = tunnelApi->getAttribute(id, type);
    EXPECT_EQ(fs->tunnelManager.get(id).type, gotType);
    SaiTunnelTraits::Attributes::UnderlayInterface underlay;
    auto gotUnderlay = tunnelApi->getAttribute(id, underlay);
    EXPECT_EQ(fs->tunnelManager.get(id).underlay, gotUnderlay);
    SaiTunnelTraits::Attributes::OverlayInterface overlay;
    auto gotOverlay = tunnelApi->getAttribute(id, overlay);
    EXPECT_EQ(fs->tunnelManager.get(id).overlay, gotOverlay);
    SaiTunnelTraits::Attributes::DecapTtlMode ttl;
    auto gotTtl = tunnelApi->getAttribute(id, ttl);
    EXPECT_EQ(fs->tunnelManager.get(id).ttlMode, gotTtl);
    SaiTunnelTraits::Attributes::DecapDscpMode dscp;
    auto gotDscp = tunnelApi->getAttribute(id, dscp);
    EXPECT_EQ(fs->tunnelManager.get(id).dscpMode, gotDscp);
    SaiTunnelTraits::Attributes::DecapEcnMode ecn;
    auto gotEcn = tunnelApi->getAttribute(id, ecn);
    EXPECT_EQ(fs->tunnelManager.get(id).ecnMode, gotEcn);
  }

  TunnelTermSaiId createTunnelTerm(
      sai_tunnel_term_table_entry_type_t _type,
      sai_object_id_t _vrId,
      folly::IPAddress _dstIp,
      folly::IPAddress /*_srcIp*/,
      sai_tunnel_type_t _tunnelType,
      TunnelSaiId _tunnelId) {
    SaiP2MPTunnelTermTraits::Attributes::Type type{_type};
    SaiP2MPTunnelTermTraits::Attributes::VrId vrId{_vrId};
    SaiP2MPTunnelTermTraits::Attributes::DstIp dstIp{_dstIp};
    // src ip should be nullopt in P2MP, but Brm is only
    // supporting P2P, so P2MP will have issues in warmboot
    SaiP2MPTunnelTermTraits::Attributes::TunnelType tunnelType{_tunnelType};
    SaiP2MPTunnelTermTraits::Attributes::ActionTunnelId tunnelId{_tunnelId};

    SaiP2MPTunnelTermTraits::CreateAttributes a{
        type, vrId, dstIp, tunnelType, tunnelId};
    return tunnelApi->create<SaiP2MPTunnelTermTraits>(a, 0);
  }

  void checkTunnelTerm(TunnelTermSaiId id) const {
    SaiP2MPTunnelTermTraits::Attributes::Type type;
    auto gotType = tunnelApi->getAttribute(id, type);
    EXPECT_EQ(fs->tunnelTermManager.get(id).type, gotType);
    SaiP2MPTunnelTermTraits::Attributes::VrId vrId;
    auto gotVrId = tunnelApi->getAttribute(id, vrId);
    EXPECT_EQ(fs->tunnelTermManager.get(id).vrId, gotVrId);
    SaiP2MPTunnelTermTraits::Attributes::TunnelType tunnelType;
    auto gotTunnelType = tunnelApi->getAttribute(id, tunnelType);
    EXPECT_EQ(fs->tunnelTermManager.get(id).tunnelType, gotTunnelType);
    SaiP2MPTunnelTermTraits::Attributes::ActionTunnelId tunnelId;
    auto gotTunnelId = tunnelApi->getAttribute(id, tunnelId);
    EXPECT_EQ(fs->tunnelTermManager.get(id).tunnelId, gotTunnelId);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<TunnelApi> tunnelApi;
};

TEST_F(TunnelApiTest, createTunnel) {
  auto saiTunnelId = createTunnel(SAI_TUNNEL_TYPE_IPINIP, 42, 42);
  checkTunnel(saiTunnelId);
}

TEST_F(TunnelApiTest, removeTunnel) {
  auto saiTunnelId = createTunnel(SAI_TUNNEL_TYPE_IPINIP, 42, 42);
  tunnelApi->remove(saiTunnelId);
  EXPECT_THROW(
      tunnelApi->getAttribute(saiTunnelId, SaiTunnelTraits::CreateAttributes{}),
      std::exception);
}

TEST_F(TunnelApiTest, getTunnelAttributes) {
  auto id = createTunnel(SAI_TUNNEL_TYPE_IPINIP, 42, 42);
  EXPECT_EQ(
      tunnelApi->getAttribute(id, SaiTunnelTraits::Attributes::Type{}),
      SAI_TUNNEL_TYPE_IPINIP);
  EXPECT_EQ(
      tunnelApi->getAttribute(
          id, SaiTunnelTraits::Attributes::UnderlayInterface{}),
      42);
  EXPECT_EQ(
      tunnelApi->getAttribute(
          id, SaiTunnelTraits::Attributes::OverlayInterface{}),
      42);
  EXPECT_EQ(
      tunnelApi->getAttribute(id, SaiTunnelTraits::Attributes::DecapTtlMode{}),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
  EXPECT_EQ(
      tunnelApi->getAttribute(id, SaiTunnelTraits::Attributes::DecapDscpMode{}),
      SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL);
  EXPECT_EQ(
      tunnelApi->getAttribute(id, SaiTunnelTraits::Attributes::DecapEcnMode{}),
      SAI_TUNNEL_DECAP_ECN_MODE_STANDARD);
}

TEST_F(TunnelApiTest, createTunnelTerm) {
  auto saiTunnelId = createTunnel(SAI_TUNNEL_TYPE_IPINIP, 42, 42);
  auto termId = createTunnelTerm(
      SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP,
      43,
      folly::IPAddress(dip),
      folly::IPAddress(sip),
      SAI_TUNNEL_TYPE_IPINIP,
      saiTunnelId);
  checkTunnelTerm(termId);
}

TEST_F(TunnelApiTest, removeTunnelTerm) {
  auto saiTunnelId = createTunnel(SAI_TUNNEL_TYPE_IPINIP, 42, 42);
  auto termId = createTunnelTerm(
      SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP,
      43,
      folly::IPAddress(dip),
      folly::IPAddress(sip),
      SAI_TUNNEL_TYPE_IPINIP,
      saiTunnelId);
  tunnelApi->remove(termId);
  EXPECT_THROW(
      tunnelApi->getAttribute(
          termId, SaiP2MPTunnelTermTraits::CreateAttributes{}),
      std::exception);
}

TEST_F(TunnelApiTest, getTunnelTermAttributes) {
  auto saiTunnelId = createTunnel(SAI_TUNNEL_TYPE_IPINIP, 42, 42);
  auto termId = createTunnelTerm(
      SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP,
      43,
      folly::IPAddress(dip),
      folly::IPAddress(sip),
      SAI_TUNNEL_TYPE_IPINIP,
      saiTunnelId);
  EXPECT_EQ(
      tunnelApi->getAttribute(
          termId, SaiP2MPTunnelTermTraits::Attributes::Type{}),
      SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP);
  EXPECT_EQ(
      tunnelApi->getAttribute(
          termId, SaiP2MPTunnelTermTraits::Attributes::VrId{}),
      43);
  EXPECT_EQ(
      tunnelApi->getAttribute(
          termId, SaiP2MPTunnelTermTraits::Attributes::DstIp{}),
      folly::IPAddress(dip));
  EXPECT_EQ(
      tunnelApi->getAttribute(
          termId, SaiP2MPTunnelTermTraits::Attributes::TunnelType{}),
      SAI_TUNNEL_TYPE_IPINIP);
  EXPECT_EQ(
      tunnelApi->getAttribute(
          termId, SaiP2MPTunnelTermTraits::Attributes::ActionTunnelId{}),
      saiTunnelId);
}

TEST_F(TunnelApiTest, setTunnelAttributes) {
  auto saiTunnelId = createTunnel(SAI_TUNNEL_TYPE_IPINIP, 42, 42);
  tunnelApi->setAttribute(
      saiTunnelId,
      SaiTunnelTraits::Attributes::DecapTtlMode{
          SAI_TUNNEL_TTL_MODE_PIPE_MODEL});
  EXPECT_EQ(
      tunnelApi->getAttribute(
          saiTunnelId, SaiTunnelTraits::Attributes::DecapTtlMode{}),
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL);

  tunnelApi->setAttribute(
      saiTunnelId,
      SaiTunnelTraits::Attributes::DecapDscpMode{
          SAI_TUNNEL_DSCP_MODE_PIPE_MODEL});
  EXPECT_EQ(
      tunnelApi->getAttribute(
          saiTunnelId, SaiTunnelTraits::Attributes::DecapDscpMode{}),
      SAI_TUNNEL_DSCP_MODE_PIPE_MODEL);

  tunnelApi->setAttribute(
      saiTunnelId,
      SaiTunnelTraits::Attributes::DecapEcnMode{
          SAI_TUNNEL_DECAP_ECN_MODE_COPY_FROM_OUTER});
  EXPECT_EQ(
      tunnelApi->getAttribute(
          saiTunnelId, SaiTunnelTraits::Attributes::DecapEcnMode{}),
      SAI_TUNNEL_DECAP_ECN_MODE_COPY_FROM_OUTER);
}
