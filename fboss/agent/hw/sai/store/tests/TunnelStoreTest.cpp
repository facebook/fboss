// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/IPAddress.h>
#include "fboss/agent/hw/sai/api/TunnelApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <variant>

using namespace facebook::fboss;

static constexpr folly::StringPiece dip = "42.42.12.34";

class TunnelStoreTest : public SaiStoreTest {
 public:
  SaiTunnelTraits::CreateAttributes createTunnelAttrs() const {
    SaiTunnelTraits::Attributes::Type type{SAI_TUNNEL_TYPE_IPINIP};
    SaiTunnelTraits::Attributes::UnderlayInterface underlay{42};
    SaiTunnelTraits::Attributes::OverlayInterface overlay{42};
    SaiTunnelTraits::Attributes::DecapTtlMode ttlMode{
        SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL};
    SaiTunnelTraits::Attributes::DecapDscpMode dscpMode{
        SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL};
    SaiTunnelTraits::Attributes::DecapEcnMode ecnMode{
        SAI_TUNNEL_DECAP_ECN_MODE_STANDARD};
    return {type, underlay, overlay, ttlMode, dscpMode, ecnMode};
  }
  TunnelSaiId createTunnel() const {
    auto& tunnelApi = saiApiTable->tunnelApi();
    return tunnelApi.create<SaiTunnelTraits>(createTunnelAttrs(), 0);
  }

  SaiTunnelTermTraits::CreateAttributes createTunnelTermAttrs(
      TunnelSaiId _id) const {
    SaiTunnelTermTraits::Attributes::Type type{
        SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP};
    SaiTunnelTermTraits::Attributes::VrId vrId{43};
    SaiTunnelTermTraits::Attributes::DstIp dstIp{folly::IPAddress(dip)};
    SaiTunnelTermTraits::Attributes::TunnelType tunnelType{
        SAI_TUNNEL_TYPE_IPINIP};
    SaiTunnelTermTraits::Attributes::ActionTunnelId tunnelId{_id};
    return {type, vrId, dstIp, tunnelType, tunnelId};
  }
  TunnelTermSaiId createTunnelTerm(TunnelSaiId _id) {
    auto& tunnelApi = saiApiTable->tunnelApi();
    return tunnelApi.create<SaiTunnelTermTraits>(createTunnelTermAttrs(_id), 0);
  }
};

TEST_F(TunnelStoreTest, loadTunnel) {
  auto tunnelId = createTunnel();
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiTunnelTraits>();
  SaiTunnelTraits::AdapterHostKey k{
      SAI_TUNNEL_TYPE_IPINIP,
      42,
      42,
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL,
      SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL,
      SAI_TUNNEL_DECAP_ECN_MODE_STANDARD};
  auto got = store.get(k);
  EXPECT_EQ(got->adapterKey(), tunnelId);
  EXPECT_EQ(
      GET_OPT_ATTR(Tunnel, DecapTtlMode, got->attributes()),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
}

TEST_F(TunnelStoreTest, loadTunnelTerm) {
  auto tunnelId = createTunnel();
  auto termId = createTunnelTerm(tunnelId);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiTunnelTermTraits>();
  auto got = store.get(createTunnelTermAttrs(tunnelId));
  EXPECT_EQ(got->adapterKey(), termId);
  EXPECT_EQ(
      GET_ATTR(TunnelTerm, TunnelType, got->attributes()),
      SAI_TUNNEL_TYPE_IPINIP);
  EXPECT_EQ(
      GET_ATTR(TunnelTerm, DstIp, got->attributes()), folly::IPAddress(dip));
}

TEST_F(TunnelStoreTest, tunnelLoadCtor) {
  auto tunnelId = createTunnel();
  SaiObject<SaiTunnelTraits> obj = createObj<SaiTunnelTraits>(tunnelId);
  EXPECT_EQ(obj.adapterKey(), tunnelId);
  EXPECT_EQ(GET_ATTR(Tunnel, OverlayInterface, obj.attributes()), 42);
  EXPECT_EQ(
      GET_OPT_ATTR(Tunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
}

TEST_F(TunnelStoreTest, tunnelTermLoadCtor) {
  auto tunnelId = createTunnel();
  auto termId = createTunnelTerm(tunnelId);
  SaiObject<SaiTunnelTermTraits> obj = createObj<SaiTunnelTermTraits>(termId);
  EXPECT_EQ(obj.adapterKey(), termId);
  EXPECT_EQ(GET_ATTR(TunnelTerm, VrId, obj.attributes()), 43);
}

TEST_F(TunnelStoreTest, tunnelCreateCtor) {
  SaiTunnelTraits::AdapterHostKey k{
      SAI_TUNNEL_TYPE_IPINIP, 42, 42, std::nullopt, std::nullopt, std::nullopt};
  SaiObject<SaiTunnelTraits> obj = createObj<SaiTunnelTraits>(k, k, 0);
  EXPECT_EQ(GET_ATTR(Tunnel, OverlayInterface, obj.attributes()), 42);
  EXPECT_EQ(
      saiApiTable->tunnelApi().getAttribute(
          obj.adapterKey(), SaiTunnelTraits::Attributes::DecapTtlMode{}),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
}

TEST_F(TunnelStoreTest, tunnelTermCreateCtor) {
  auto tunnelId = createTunnel();
  SaiTunnelTermTraits::AdapterHostKey k{
      SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP,
      43,
      folly::IPAddress(dip),
      SAI_TUNNEL_TYPE_IPINIP,
      tunnelId};
  SaiObject<SaiTunnelTermTraits> obj = createObj<SaiTunnelTermTraits>(k, k, 0);
  EXPECT_EQ(
      GET_ATTR(TunnelTerm, TunnelType, obj.attributes()),
      SAI_TUNNEL_TYPE_IPINIP);
}

TEST_F(TunnelStoreTest, serDeserTunnel) {
  auto tunnelId = createTunnel();
  verifyAdapterKeySerDeser<SaiTunnelTraits>({tunnelId});
}

TEST_F(TunnelStoreTest, serDeserTunnelTerm) {
  auto tunnelId = createTunnel();
  auto termId = createTunnelTerm(tunnelId);
  verifyAdapterKeySerDeser<SaiTunnelTermTraits>({termId});
}

TEST_F(TunnelStoreTest, toStrTunnel) {
  std::ignore = createTunnel();
  verifyToStr<SaiTunnelTraits>();
}

TEST_F(TunnelStoreTest, toStrTunnelTerm) {
  auto _id = createTunnel();
  std::ignore = createTunnelTerm(_id);
  verifyToStr<SaiTunnelTermTraits>();
}

TEST_F(TunnelStoreTest, tunnelSetOnlyTtl) {
  auto tunnelId = createTunnel();
  SaiObject<SaiTunnelTraits> obj = createObj<SaiTunnelTraits>(tunnelId);
  EXPECT_EQ(
      GET_OPT_ATTR(Tunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
  auto ttl = saiApiTable->tunnelApi().getAttribute(
      tunnelId, SaiTunnelTraits::Attributes::DecapTtlMode{});
  EXPECT_EQ(ttl, SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
  obj.setOptionalAttribute(SaiTunnelTraits::Attributes::DecapTtlMode{
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL});
  EXPECT_EQ(
      GET_OPT_ATTR(Tunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
  ttl = saiApiTable->tunnelApi().getAttribute(
      tunnelId, SaiTunnelTraits::Attributes::DecapTtlMode{});
  EXPECT_EQ(ttl, SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
}

TEST_F(TunnelStoreTest, tunnelSetTtl) {
  auto tunnelId = createTunnel();
  SaiObject<SaiTunnelTraits> obj = createObj<SaiTunnelTraits>(tunnelId);
  EXPECT_EQ(
      GET_OPT_ATTR(Tunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
  auto newAttrs = SaiTunnelTraits::CreateAttributes{
      SAI_TUNNEL_TYPE_IPINIP,
      42,
      42,
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL,
      SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL,
      SAI_TUNNEL_DECAP_ECN_MODE_STANDARD};
  obj.setAttributes(newAttrs);
  EXPECT_EQ(
      GET_OPT_ATTR(Tunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
  auto apiSpeed = saiApiTable->tunnelApi().getAttribute(
      tunnelId, SaiTunnelTraits::Attributes::DecapTtlMode{});
  EXPECT_EQ(apiSpeed, SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
}
