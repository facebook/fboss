// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

class MySidRibUpdateTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    auto config = testConfigA();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
  }

  void TearDown() override {
    FLAGS_enable_nexthop_id_manager = false;
  }

  std::shared_ptr<MySid> getMySid(const std::string& key) {
    auto mySids = sw_->getState()->getMySids();
    for (const auto& [_, mySidMap] : std::as_const(*mySids)) {
      auto node = mySidMap->getNodeIf(key);
      if (node) {
        return node;
      }
    }
    return nullptr;
  }

  size_t getMySidCount() {
    size_t count = 0;
    auto mySids = sw_->getState()->getMySids();
    for (const auto& [_, mySidMap] : std::as_const(*mySids)) {
      count += mySidMap->size();
    }
    return count;
  }

  // Add a CONFIG-owned MySid via the new RIB overload
  void addConfigMySid(
      const std::string& addr,
      uint8_t len,
      MySidType type = MySidType::DECAPSULATE_AND_LOOKUP,
      RouteNextHopSet nhops = RouteNextHopSet{}) {
    state::MySidFields fields;
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() =
        facebook::network::toBinaryAddress(folly::IPAddressV6(addr));
    prefix.prefixLength() = len;
    fields.mySid() = prefix;
    fields.type() = type;
    fields.clientId() = ClientID::STATIC_ROUTE;
    auto mySid = std::make_shared<MySid>(fields);
    mySid->setUnresolveNextHopsId(std::nullopt);
    mySid->setResolvedNextHopsId(std::nullopt);

    auto rib = sw_->getRib();
    auto ribMySidFunc = createRibMySidToSwitchStateFunction(std::nullopt);
    rib->update(
        sw_->getScopeResolver(),
        std::vector<MySidWithNextHops>{{mySid, std::move(nhops), std::nullopt}},
        std::vector<MySidNeighborRemoved>{} /* toUnresolveIfMatch */,
        std::vector<IpPrefix>{} /* toDelete */,
        "addConfigMySid",
        ribMySidFunc,
        sw_);
  }

  // Add a THRIFT_RPC-owned MySid via ThriftHandler
  void addRpcMySid(const std::string& addr, uint8_t len) {
    ThriftHandler handler(sw_);
    auto entries = std::make_unique<std::vector<MySidEntry>>();
    MySidEntry entry;
    entry.type() = MySidType::BINDING_MICRO_SID;
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() =
        facebook::network::toBinaryAddress(folly::IPAddressV6(addr));
    prefix.prefixLength() = len;
    entry.mySid() = prefix;
    NextHopThrift nhop;
    nhop.address() =
        facebook::network::toBinaryAddress(folly::IPAddressV6("2001:db8::ff"));
    nhop.srv6SegmentList() = {
        facebook::network::toBinaryAddress(folly::IPAddressV6("2001:db8::10"))};
    nhop.tunnelType() = TunnelType::SRV6_ENCAP;
    nhop.tunnelId() = "tunnel1";
    entry.nextHops() = {nhop};
    entries->push_back(entry);
    handler.addMySidEntries(std::move(entries));
  }

  // Delete a MySid via the new RIB overload (config path)
  void deleteConfigMySid(const std::string& addr, uint8_t len) {
    IpPrefix prefix;
    prefix.ip() = facebook::network::toBinaryAddress(folly::IPAddressV6(addr));
    prefix.prefixLength() = len;

    auto rib = sw_->getRib();
    auto ribMySidFunc = createRibMySidToSwitchStateFunction(std::nullopt);
    rib->update(
        sw_->getScopeResolver(),
        std::vector<MySidWithNextHops>{},
        std::vector<MySidNeighborRemoved>{} /* toUnresolveIfMatch */,
        std::vector<IpPrefix>{prefix},
        "deleteConfigMySid",
        ribMySidFunc,
        sw_);
  }

  void addBindingSidWithNextHop(
      const std::string& mySidAddr,
      uint8_t len,
      const std::string& nhopAddr,
      const std::string& sid) {
    MySidEntry entry;
    entry.type() = MySidType::BINDING_MICRO_SID;
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() =
        facebook::network::toBinaryAddress(folly::IPAddressV6(mySidAddr));
    prefix.prefixLength() = len;
    entry.mySid() = prefix;
    NextHopThrift nhop;
    nhop.address() =
        facebook::network::toBinaryAddress(folly::IPAddressV6(nhopAddr));
    nhop.srv6SegmentList() = {
        facebook::network::toBinaryAddress(folly::IPAddressV6(sid))};
    nhop.tunnelType() = TunnelType::SRV6_ENCAP;
    nhop.tunnelId() = "tunnel1";
    entry.nextHops() = {nhop};

    auto rib = sw_->getRib();
    auto ribMySidFunc = createRibMySidToSwitchStateFunction(std::nullopt);
    rib->update(
        sw_->getScopeResolver(),
        {entry},
        {} /* toDelete */,
        "addBindingSidWithNextHop",
        ribMySidFunc,
        sw_);
  }

  void addOpenrRoute(
      const std::string& prefix,
      uint8_t len,
      const std::string& nhopAddr) {
    auto routeUpdater = sw_->getRouteUpdater();
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6(prefix),
        len,
        ClientID::OPENR,
        RouteNextHopEntry(
            RouteNextHopSet{ResolvedNextHop(
                folly::IPAddress(folly::IPAddressV6(nhopAddr)),
                InterfaceID(1),
                ECMP_WEIGHT)},
            AdminDistance::DIRECTLY_CONNECTED));
    routeUpdater.program();
  }

  void removeOpenrRoute(const std::string& prefix, uint8_t len) {
    auto routeUpdater = sw_->getRouteUpdater();
    routeUpdater.delRoute(
        RouterID(0), folly::IPAddressV6(prefix), len, ClientID::OPENR);
    routeUpdater.program();
  }

  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(MySidRibUpdateTest, ConfigMySidHasConfigOwnership) {
  addConfigMySid("3001:db8:7fff::", 48);
  auto mySid = getMySid("3001:db8:7fff::/48");
  ASSERT_NE(mySid, nullptr);
  EXPECT_EQ(mySid->getType(), MySidType::DECAPSULATE_AND_LOOKUP);
  EXPECT_EQ(mySid->getClientId(), ClientID::STATIC_ROUTE);
}

TEST_F(MySidRibUpdateTest, RpcMySidHasRpcOwnership) {
  addRpcMySid("2001:db8::1", 64);
  auto mySid = getMySid("2001:db8::1/64");
  ASSERT_NE(mySid, nullptr);
  EXPECT_EQ(mySid->getType(), MySidType::BINDING_MICRO_SID);
  EXPECT_EQ(mySid->getClientId(), ClientID::TE_AGENT);
}

TEST_F(MySidRibUpdateTest, ConfigAndRpcMySidsCoexist) {
  addConfigMySid("3001:db8:7fff::", 48);
  addRpcMySid("2001:db8::1", 64);

  EXPECT_EQ(getMySidCount(), 2);
  EXPECT_NE(getMySid("3001:db8:7fff::/48"), nullptr);
  EXPECT_NE(getMySid("2001:db8::1/64"), nullptr);
  EXPECT_EQ(
      getMySid("3001:db8:7fff::/48")->getClientId(), ClientID::STATIC_ROUTE);
  EXPECT_EQ(getMySid("2001:db8::1/64")->getClientId(), ClientID::TE_AGENT);
}

TEST_F(MySidRibUpdateTest, DeleteConfigMySidDoesNotAffectRpcMySid) {
  addConfigMySid("3001:db8:7fff::", 48);
  addRpcMySid("2001:db8::1", 64);
  EXPECT_EQ(getMySidCount(), 2);

  // Delete only the config entry
  deleteConfigMySid("3001:db8:7fff::", 48);

  EXPECT_EQ(getMySidCount(), 1);
  EXPECT_EQ(getMySid("3001:db8:7fff::/48"), nullptr);
  // RPC entry still present
  EXPECT_NE(getMySid("2001:db8::1/64"), nullptr);
  EXPECT_EQ(getMySid("2001:db8::1/64")->getClientId(), ClientID::TE_AGENT);
}

TEST_F(MySidRibUpdateTest, DeleteRpcMySidDoesNotAffectConfigMySid) {
  addConfigMySid("3001:db8:7fff::", 48);
  addRpcMySid("2001:db8::1", 64);
  EXPECT_EQ(getMySidCount(), 2);

  // Delete the RPC entry via ThriftHandler
  ThriftHandler handler(sw_);
  auto prefixes = std::make_unique<std::vector<IpPrefix>>();
  IpPrefix prefix;
  prefix.ip() =
      facebook::network::toBinaryAddress(folly::IPAddressV6("2001:db8::1"));
  prefix.prefixLength() = 64;
  prefixes->push_back(prefix);
  handler.deleteMySidEntries(std::move(prefixes));

  EXPECT_EQ(getMySidCount(), 1);
  EXPECT_EQ(getMySid("2001:db8::1/64"), nullptr);
  // Config entry still present
  EXPECT_NE(getMySid("3001:db8:7fff::/48"), nullptr);
  EXPECT_EQ(
      getMySid("3001:db8:7fff::/48")->getClientId(), ClientID::STATIC_ROUTE);
}

TEST_F(MySidRibUpdateTest, ReplaceConfigMySidPreservesOwnership) {
  addConfigMySid("3001:db8:7fff::", 48);
  EXPECT_EQ(
      getMySid("3001:db8:7fff::/48")->getClientId(), ClientID::STATIC_ROUTE);

  // Re-add the same entry via config path — should still be CONFIG
  addConfigMySid("3001:db8:7fff::", 48);
  EXPECT_EQ(getMySidCount(), 1);
  EXPECT_EQ(
      getMySid("3001:db8:7fff::/48")->getClientId(), ClientID::STATIC_ROUTE);
}

TEST_F(MySidRibUpdateTest, NodeMySidWithUnresolvedNextHops) {
  // NODE_MICRO_SID via config path with actual unresolved next hops
  RouteNextHopSet nhops;
  nhops.insert(UnresolvedNextHop(folly::IPAddress("fc00:100::1"), ECMP_WEIGHT));
  addConfigMySid(
      "3001:db8:3::", 48, MySidType::NODE_MICRO_SID, std::move(nhops));

  auto mySid = getMySid("3001:db8:3::/48");
  ASSERT_NE(mySid, nullptr);
  EXPECT_EQ(mySid->getType(), MySidType::NODE_MICRO_SID);
  EXPECT_EQ(mySid->getClientId(), ClientID::STATIC_ROUTE);
  // Should have unresolvedNextHopsId allocated by NextHopIDManager
  EXPECT_TRUE(mySid->getUnresolveNextHopsId().has_value());
}

TEST_F(MySidRibUpdateTest, BindingSidResolvesWithOpenrRoute) {
  // BGP route: 2001:db8::ff/128 -> fdad::1:1 (unresolved, no OpenR route yet)
  auto routeUpdater = sw_->getRouteUpdater();
  routeUpdater.addRoute(
      RouterID(0),
      folly::IPAddressV6("2001:db8::ff"),
      128,
      ClientID::BGPD,
      RouteNextHopEntry(
          RouteNextHopSet{UnresolvedNextHop(
              folly::IPAddress(folly::IPAddressV6("fdad::1:1")), ECMP_WEIGHT)},
          AdminDistance::EBGP));
  routeUpdater.program();

  // Add binding SID pointing to the BGP route
  addBindingSidWithNextHop(
      "fc00:100:1::", 48, "2001:db8::ff", "3001:db8:1:2:3:4:5:6");

  // Without OpenR route, binding SID should be unresolved
  auto mySid = getMySid("fc00:100:1::/48");
  ASSERT_NE(mySid, nullptr);
  EXPECT_FALSE(mySid->getResolvedNextHopsId().has_value());

  // Add OpenR route that resolves the BGP next hop
  addOpenrRoute("fdad::1:0", 112, "fe80::1");

  // Binding SID should now be resolved
  mySid = getMySid("fc00:100:1::/48");
  ASSERT_NE(mySid, nullptr);
  EXPECT_TRUE(mySid->getResolvedNextHopsId().has_value());

  // Remove OpenR route
  removeOpenrRoute("fdad::1:0", 112);

  // Binding SID should go back to unresolved
  mySid = getMySid("fc00:100:1::/48");
  ASSERT_NE(mySid, nullptr);
  EXPECT_FALSE(mySid->getResolvedNextHopsId().has_value());

  // Re-add OpenR route — should resolve again
  addOpenrRoute("fdad::1:0", 112, "fe80::1");

  mySid = getMySid("fc00:100:1::/48");
  ASSERT_NE(mySid, nullptr);
  EXPECT_TRUE(mySid->getResolvedNextHopsId().has_value());
}
