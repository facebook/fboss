// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

#include "fboss/agent/hw/sai/api/Srv6Api.h"
#include "fboss/agent/hw/sai/switch/SaiSrv6MySidManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/MySid.h"

using namespace facebook::fboss;

namespace {
std::shared_ptr<MySid> makeMySid(
    const std::string& addr = "fc00:100::1",
    uint8_t len = 48,
    MySidType type = MySidType::DECAPSULATE_AND_LOOKUP) {
  state::MySidFields fields;
  fields.type() = type;
  facebook::network::thrift::IPPrefix thriftPrefix;
  thriftPrefix.prefixAddress() =
      facebook::network::toBinaryAddress(folly::IPAddress(addr));
  thriftPrefix.prefixLength() = len;
  fields.mySid() = thriftPrefix;
  return std::make_shared<MySid>(fields);
}

std::shared_ptr<MySid> makeMySidWithNextHop(
    const folly::IPAddress& nhopIp,
    InterfaceID intfId,
    const std::string& addr = "fc00:100::1",
    uint8_t len = 48,
    MySidType type = MySidType::ADJACENCY_MICRO_SID) {
  auto mySid = makeMySid(addr, len, type);
  RouteNextHopEntry::NextHopSet nhops;
  nhops.emplace(ResolvedNextHop(nhopIp, intfId, ECMP_WEIGHT));
  RouteNextHopEntry nhopEntry(nhops, AdminDistance::STATIC_ROUTE);
  mySid->setResolvedNextHop(
      std::optional<RouteNextHopEntry>(nhopEntry.toThrift()));
  return mySid;
}

std::shared_ptr<MySid> makeMySidWithEcmpNextHops(
    const std::vector<std::pair<folly::IPAddress, InterfaceID>>& nextHops,
    const std::string& addr = "fc00:100::1",
    uint8_t len = 48,
    MySidType type = MySidType::ADJACENCY_MICRO_SID) {
  auto mySid = makeMySid(addr, len, type);
  RouteNextHopEntry::NextHopSet nhops;
  for (const auto& [ip, intfId] : nextHops) {
    nhops.emplace(ResolvedNextHop(ip, intfId, ECMP_WEIGHT));
  }
  RouteNextHopEntry nhopEntry(nhops, AdminDistance::STATIC_ROUTE);
  mySid->setResolvedNextHop(
      std::optional<RouteNextHopEntry>(nhopEntry.toThrift()));
  return mySid;
}
} // namespace

class MySidManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE |
        SetupStage::NEIGHBOR;
    ManagerTestBase::SetUp();
  }
};

TEST_F(MySidManagerTest, addDecapsulateAndLookup) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  EXPECT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotBehavior = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::EndpointBehavior{});
  EXPECT_EQ(gotBehavior, SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UDT46);

  auto gotAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(gotAction, SAI_PACKET_ACTION_FORWARD);
}

TEST_F(MySidManagerTest, addWithSingleNextHop) {
  auto mySid = makeMySidWithNextHop(
      testInterfaces.at(0).remoteHosts.at(0).ip,
      InterfaceID(testInterfaces.at(0).id),
      "fc00:200::1",
      48,
      MySidType::ADJACENCY_MICRO_SID);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  EXPECT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotBehavior = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::EndpointBehavior{});
  EXPECT_EQ(gotBehavior, SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UA);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid);
}

TEST_F(MySidManagerTest, addWithEcmpNextHops) {
  std::vector<std::pair<folly::IPAddress, InterfaceID>> nextHops;
  for (const auto& intf : testInterfaces) {
    nextHops.emplace_back(intf.remoteHosts.at(0).ip, InterfaceID(intf.id));
  }
  auto mySid = makeMySidWithEcmpNextHops(
      nextHops, "fc00:300::1", 48, MySidType::NODE_MICRO_SID);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  EXPECT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotBehavior = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::EndpointBehavior{});
  EXPECT_EQ(gotBehavior, SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UN);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid);
}

TEST_F(MySidManagerTest, addDuplicateThrows) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);
  EXPECT_THROW(
      saiManagerTable->srv6MySidManager().addMySidEntry(mySid), FbossError);
}

TEST_F(MySidManagerTest, addUnresolvedSkips) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  // No resolvedNextHop set and type is not DECAPSULATE_AND_LOOKUP
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  EXPECT_EQ(saiEntry, nullptr);
}

TEST_F(MySidManagerTest, removeDecapsulateAndLookup) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid);
  EXPECT_EQ(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);
}

TEST_F(MySidManagerTest, removeWithSingleNextHop) {
  auto mySid = makeMySidWithNextHop(
      testInterfaces.at(0).remoteHosts.at(0).ip,
      InterfaceID(testInterfaces.at(0).id),
      "fc00:200::1",
      48,
      MySidType::ADJACENCY_MICRO_SID);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid);
  EXPECT_EQ(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);
}

TEST_F(MySidManagerTest, removeNonexistentThrows) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  EXPECT_THROW(
      saiManagerTable->srv6MySidManager().removeMySidEntry(mySid), FbossError);
}

TEST_F(MySidManagerTest, removeUnresolvedReturnsEarly) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  // Should not throw even though entry doesn't exist — returns early
  EXPECT_NO_THROW(saiManagerTable->srv6MySidManager().removeMySidEntry(mySid));
}

TEST_F(MySidManagerTest, changeType) {
  auto oldMySid =
      makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  saiManagerTable->srv6MySidManager().addMySidEntry(oldMySid);

  auto key = getMySidAdapterHostKey(*oldMySid, saiManagerTable);
  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);

  // Change to a different DECAPSULATE_AND_LOOKUP (re-add)
  auto newMySid =
      makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  saiManagerTable->srv6MySidManager().changeMySidEntry(oldMySid, newMySid);

  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);
}

TEST_F(MySidManagerTest, changeFromDecapToNextHop) {
  auto oldMySid =
      makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  saiManagerTable->srv6MySidManager().addMySidEntry(oldMySid);

  auto newMySid = makeMySidWithNextHop(
      testInterfaces.at(0).remoteHosts.at(0).ip,
      InterfaceID(testInterfaces.at(0).id),
      "fc00:100::1",
      48,
      MySidType::ADJACENCY_MICRO_SID);
  saiManagerTable->srv6MySidManager().changeMySidEntry(oldMySid, newMySid);

  auto key = getMySidAdapterHostKey(*newMySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  EXPECT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotBehavior = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::EndpointBehavior{});
  EXPECT_EQ(gotBehavior, SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UA);

  saiManagerTable->srv6MySidManager().removeMySidEntry(newMySid);
}

#endif
