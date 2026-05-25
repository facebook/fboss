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
static constexpr folly::StringPiece kEncapSrcIp = "2401:db00::1";
static constexpr folly::StringPiece srv6SrcIp = "2001:db8::1";

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
    return {
        type,
        underlay,
        overlay,
        ttlMode,
        dscpMode,
        ecnMode,
        std::nullopt,
        std::nullopt,
        std::nullopt};
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

  SaiIpInIpTunnelTraits::CreateAttributes createEncapTunnelAttrs() const {
    SaiIpInIpTunnelTraits::Attributes::Type type{SAI_TUNNEL_TYPE_IPINIP};
    SaiIpInIpTunnelTraits::Attributes::UnderlayInterface underlay{42};
    SaiIpInIpTunnelTraits::Attributes::OverlayInterface overlay{42};
    SaiIpInIpTunnelTraits::Attributes::EncapSrcIp srcIp{
        folly::IPAddress(kEncapSrcIp)};
    SaiIpInIpTunnelTraits::Attributes::EncapTtlMode ttlMode{
        SAI_TUNNEL_TTL_MODE_PIPE_MODEL};
    SaiIpInIpTunnelTraits::Attributes::EncapDscpMode dscpMode{
        SAI_TUNNEL_DSCP_MODE_PIPE_MODEL};
    return {
        type,
        underlay,
        overlay,
        SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL,
        SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL,
        SAI_TUNNEL_DECAP_ECN_MODE_STANDARD,
        srcIp,
        ttlMode,
        dscpMode};
  }
  TunnelSaiId createEncapTunnel() const {
    auto& tunnelApi = saiApiTable->tunnelApi();
    return tunnelApi.create<SaiIpInIpTunnelTraits>(createEncapTunnelAttrs(), 0);
  }

  SaiSrv6TunnelTraits::CreateAttributes createSrv6TunnelAttrs() const {
    SaiSrv6TunnelTraits::Attributes::EncapSrcIp encapSrcIp{
        folly::IPAddress(srv6SrcIp)};
    SaiSrv6TunnelTraits::Attributes::Type type{SAI_TUNNEL_TYPE_SRV6};
    SaiSrv6TunnelTraits::Attributes::UnderlayInterface underlay{42};
    SaiSrv6TunnelTraits::Attributes::EncapTtlMode encapTtlMode{
        SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL};
    SaiSrv6TunnelTraits::Attributes::EncapEcnMode encapEcnMode{
        SAI_TUNNEL_DECAP_ECN_MODE_STANDARD};
    SaiSrv6TunnelTraits::Attributes::EncapDscpMode encapDscpMode{
        SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL};
    return {
        encapSrcIp, type, underlay, encapTtlMode, encapEcnMode, encapDscpMode};
  }
  TunnelSaiId createSrv6Tunnel() const {
    auto& tunnelApi = saiApiTable->tunnelApi();
    return tunnelApi.create<SaiSrv6TunnelTraits>(createSrv6TunnelAttrs(), 0);
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
      SAI_TUNNEL_DECAP_ECN_MODE_STANDARD,
      folly::IPAddress("0.0.0.0"),
      0,
      0};
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
  SaiIpInIpTunnelTraits::CreateAttributes k{
      SAI_TUNNEL_TYPE_IPINIP,
      42,
      42,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt};
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

// IP-in-IP Encap Tunnel Store Tests

TEST_F(TunnelStoreTest, loadEncapTunnel) {
  auto tunnelId = createEncapTunnel();
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiIpInIpTunnelTraits>();
  auto k = createEncapTunnelAttrs();
  auto got = store.get(k);
  EXPECT_EQ(got->adapterKey(), tunnelId);
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, EncapSrcIp, got->attributes()),
      folly::IPAddress(kEncapSrcIp));
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, EncapTtlMode, got->attributes()),
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, EncapDscpMode, got->attributes()),
      SAI_TUNNEL_DSCP_MODE_PIPE_MODEL);
}

TEST_F(TunnelStoreTest, encapTunnelCreateCtor) {
  auto k = createEncapTunnelAttrs();
  SaiObject<SaiIpInIpTunnelTraits> obj =
      createObj<SaiIpInIpTunnelTraits>(k, k, 0);
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, EncapSrcIp, obj.attributes()),
      folly::IPAddress(kEncapSrcIp));
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, EncapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
}

TEST_F(TunnelStoreTest, serDeserEncapTunnel) {
  auto tunnelId = createEncapTunnel();
  verifyAdapterKeySerDeser<SaiIpInIpTunnelTraits>({tunnelId});
}

TEST_F(TunnelStoreTest, toStrEncapTunnel) {
  std::ignore = createEncapTunnel();
  verifyToStr<SaiIpInIpTunnelTraits>();
}

// SRv6 Tunnel Store Tests

TEST_F(TunnelStoreTest, loadSrv6Tunnel) {
  auto tunnelId = createSrv6Tunnel();
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiSrv6TunnelTraits>();
  SaiSrv6TunnelTraits::AdapterHostKey k{
      folly::IPAddress(srv6SrcIp),
      SAI_TUNNEL_TYPE_SRV6,
      42,
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL,
      SAI_TUNNEL_DECAP_ECN_MODE_STANDARD,
      SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL};
  auto got = store.get(k);
  EXPECT_EQ(got->adapterKey(), tunnelId);
  EXPECT_EQ(
      GET_ATTR(Srv6Tunnel, EncapSrcIp, got->attributes()),
      folly::IPAddress(srv6SrcIp));
}

TEST_F(TunnelStoreTest, srv6TunnelLoadCtor) {
  auto tunnelId = createSrv6Tunnel();
  SaiObject<SaiSrv6TunnelTraits> obj = createObj<SaiSrv6TunnelTraits>(tunnelId);
  EXPECT_EQ(obj.adapterKey(), tunnelId);
  EXPECT_EQ(
      GET_ATTR(Srv6Tunnel, EncapSrcIp, obj.attributes()),
      folly::IPAddress(srv6SrcIp));
}

TEST_F(TunnelStoreTest, srv6TunnelCreateCtor) {
  SaiSrv6TunnelTraits::AdapterHostKey k{
      folly::IPAddress(srv6SrcIp),
      SAI_TUNNEL_TYPE_SRV6,
      42,
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL,
      SAI_TUNNEL_DECAP_ECN_MODE_STANDARD,
      SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL};
  SaiObject<SaiSrv6TunnelTraits> obj = createObj<SaiSrv6TunnelTraits>(k, k, 0);
  EXPECT_EQ(
      GET_ATTR(Srv6Tunnel, EncapSrcIp, obj.attributes()),
      folly::IPAddress(srv6SrcIp));
}

TEST_F(TunnelStoreTest, serDeserSrv6Tunnel) {
  auto tunnelId = createSrv6Tunnel();
  verifyAdapterKeySerDeser<SaiSrv6TunnelTraits>({tunnelId});
}

TEST_F(TunnelStoreTest, toStrSrv6Tunnel) {
  std::ignore = createSrv6Tunnel();
  verifyToStr<SaiSrv6TunnelTraits>();
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
      SAI_TUNNEL_DECAP_ECN_MODE_STANDARD,
      std::nullopt,
      std::nullopt,
      std::nullopt};
  obj.setAttributes(newAttrs);
  EXPECT_EQ(
      GET_OPT_ATTR(IpInIpTunnel, DecapTtlMode, obj.attributes()),
      SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
  auto apiSpeed = saiApiTable->tunnelApi().getAttribute(
      tunnelId, SaiIpInIpTunnelTraits::Attributes::DecapTtlMode{});
  EXPECT_EQ(apiSpeed, SAI_TUNNEL_TTL_MODE_PIPE_MODEL);
}
