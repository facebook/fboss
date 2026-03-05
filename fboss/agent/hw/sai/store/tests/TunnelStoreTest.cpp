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
  SaiIpInIpTunnelTraits::CreateAttributes createTunnelAttrs() const {
    SaiIpInIpTunnelTraits::Attributes::Type type{SAI_TUNNEL_TYPE_IPINIP};
    SaiIpInIpTunnelTraits::Attributes::UnderlayInterface underlay{42};
    SaiIpInIpTunnelTraits::Attributes::OverlayInterface overlay{42};
    SaiIpInIpTunnelTraits::Attributes::DecapTtlMode ttlMode{
        SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL};
    SaiIpInIpTunnelTraits::Attributes::DecapDscpMode dscpMode{
        SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL};
    SaiIpInIpTunnelTraits::Attributes::DecapEcnMode ecnMode{
        SAI_TUNNEL_DECAP_ECN_MODE_STANDARD};
    return {type, underlay, overlay, ttlMode, dscpMode, ecnMode};
  }
  TunnelSaiId createTunnel() const {
    auto& tunnelApi = saiApiTable->tunnelApi();
    return tunnelApi.create<SaiIpInIpTunnelTraits>(createTunnelAttrs(), 0);
  }

  SaiP2MPTunnelTermTraits::CreateAttributes createTunnelTermAttrs(
      TunnelSaiId _id) const {
    SaiP2MPTunnelTermTraits::Attributes::Type type{
        SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP};
    SaiP2MPTunnelTermTraits::Attributes::VrId vrId{43};
    SaiP2MPTunnelTermTraits::Attributes::DstIp dstIp{folly::IPAddress(dip)};
    SaiP2MPTunnelTermTraits::Attributes::TunnelType tunnelType{
        SAI_TUNNEL_TYPE_IPINIP};
    SaiP2MPTunnelTermTraits::Attributes::ActionTunnelId tunnelId{_id};
    return {type, vrId, dstIp, tunnelType, tunnelId};
  }
  TunnelTermSaiId createTunnelTerm(TunnelSaiId _id) {
    auto& tunnelApi = saiApiTable->tunnelApi();
    return tunnelApi.create<SaiP2MPTunnelTermTraits>(
        createTunnelTermAttrs(_id), 0);
  }
};

TEST_F(TunnelStoreTest, loadTunnel) {
  auto tunnelId = createTunnel();
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiIpInIpTunnelTraits>();
  SaiIpInIpTunnelTraits::AdapterHostKey k{
      SAI_TUNNEL_TYPE_IPINIP,
      42,
      42,
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL,
      SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL,
      SAI_TUNNEL_DECAP_ECN_MODE_STANDARD};
  auto got = store.get(k);
  EXPECT_EQ(got->adapterKey(), tunnelId);
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, DecapTtlMode, got->attributes()),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
}

TEST_F(TunnelStoreTest, loadTunnelTerm) {
  auto tunnelId = createTunnel();
  auto termId = createTunnelTerm(tunnelId);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiP2MPTunnelTermTraits>();
  auto got = store.get(createTunnelTermAttrs(tunnelId));
  EXPECT_EQ(got->adapterKey(), termId);
  EXPECT_EQ(
      GET_ATTR(P2MPTunnelTerm, TunnelType, got->attributes()),
      SAI_TUNNEL_TYPE_IPINIP);
  EXPECT_EQ(
      GET_ATTR(P2MPTunnelTerm, DstIp, got->attributes()),
      folly::IPAddress(dip));
}

TEST_F(TunnelStoreTest, tunnelLoadCtor) {
  auto tunnelId = createTunnel();
  SaiObject<SaiIpInIpTunnelTraits> obj =
      createObj<SaiIpInIpTunnelTraits>(tunnelId);
  EXPECT_EQ(obj.adapterKey(), tunnelId);
  EXPECT_EQ(GET_ATTR(IpInIpTunnel, OverlayInterface, obj.attributes()), 42);
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
}

TEST_F(TunnelStoreTest, tunnelTermLoadCtor) {
  auto tunnelId = createTunnel();
  auto termId = createTunnelTerm(tunnelId);
  SaiObject<SaiP2MPTunnelTermTraits> obj =
      createObj<SaiP2MPTunnelTermTraits>(termId);
  EXPECT_EQ(obj.adapterKey(), termId);
  EXPECT_EQ(GET_ATTR(P2MPTunnelTerm, VrId, obj.attributes()), 43);
}

TEST_F(TunnelStoreTest, tunnelCreateCtor) {
  SaiIpInIpTunnelTraits::AdapterHostKey k{
      SAI_TUNNEL_TYPE_IPINIP, 42, 42, std::nullopt, std::nullopt, std::nullopt};
  SaiObject<SaiIpInIpTunnelTraits> obj =
      createObj<SaiIpInIpTunnelTraits>(k, k, 0);
  EXPECT_EQ(GET_ATTR(IpInIpTunnel, OverlayInterface, obj.attributes()), 42);
  EXPECT_EQ(
      saiApiTable->tunnelApi().getAttribute(
          obj.adapterKey(), SaiIpInIpTunnelTraits::Attributes::DecapTtlMode{}),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
}

TEST_F(TunnelStoreTest, tunnelTermCreateCtor) {
  auto tunnelId = createTunnel();
  SaiP2MPTunnelTermTraits::AdapterHostKey k{
      SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP,
      43,
      folly::IPAddress(dip),
      SAI_TUNNEL_TYPE_IPINIP,
      tunnelId};
  SaiObject<SaiP2MPTunnelTermTraits> obj =
      createObj<SaiP2MPTunnelTermTraits>(k, k, 0);
  EXPECT_EQ(
      GET_ATTR(P2MPTunnelTerm, TunnelType, obj.attributes()),
      SAI_TUNNEL_TYPE_IPINIP);
}

TEST_F(TunnelStoreTest, serDeserTunnel) {
  auto tunnelId = createTunnel();
  verifyAdapterKeySerDeser<SaiIpInIpTunnelTraits>({tunnelId});
}

TEST_F(TunnelStoreTest, serDeserTunnelTerm) {
  auto tunnelId = createTunnel();
  auto termId = createTunnelTerm(tunnelId);
  verifyAdapterKeySerDeser<SaiP2MPTunnelTermTraits>({termId});
}

TEST_F(TunnelStoreTest, toStrTunnel) {
  std::ignore = createTunnel();
  verifyToStr<SaiIpInIpTunnelTraits>();
}

TEST_F(TunnelStoreTest, toStrTunnelTerm) {
  auto _id = createTunnel();
  std::ignore = createTunnelTerm(_id);
  verifyToStr<SaiP2MPTunnelTermTraits>();
}

TEST_F(TunnelStoreTest, tunnelSetOnlyTtl) {
  auto tunnelId = createTunnel();
  SaiObject<SaiIpInIpTunnelTraits> obj =
      createObj<SaiIpInIpTunnelTraits>(tunnelId);
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
  auto ttl = saiApiTable->tunnelApi().getAttribute(
      tunnelId, SaiIpInIpTunnelTraits::Attributes::DecapTtlMode{});
  EXPECT_EQ(ttl, SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
  obj.setOptionalAttribute(
      SaiIpInIpTunnelTraits::Attributes::DecapTtlMode{
          SAI_TUNNEL_TTL_MODE_PIPE_MODEL});
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
  ttl = saiApiTable->tunnelApi().getAttribute(
      tunnelId, SaiIpInIpTunnelTraits::Attributes::DecapTtlMode{});
  EXPECT_EQ(ttl, SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
}

TEST_F(TunnelStoreTest, tunnelSetTtl) {
  auto tunnelId = createTunnel();
  SaiObject<SaiIpInIpTunnelTraits> obj =
      createObj<SaiIpInIpTunnelTraits>(tunnelId);
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL);
  auto newAttrs = SaiIpInIpTunnelTraits::CreateAttributes{
      SAI_TUNNEL_TYPE_IPINIP,
      42,
      42,
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL,
      SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL,
      SAI_TUNNEL_DECAP_ECN_MODE_STANDARD};
  obj.setAttributes(newAttrs);
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
  auto apiSpeed = saiApiTable->tunnelApi().getAttribute(
      tunnelId, SaiIpInIpTunnelTraits::Attributes::DecapTtlMode{});
  EXPECT_EQ(apiSpeed, SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
}
