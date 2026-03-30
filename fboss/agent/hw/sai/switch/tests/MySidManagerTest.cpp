// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/api/Srv6Api.h"
#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
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

TEST_F(MySidManagerTest, addWithUnresolvedNeighborThenResolve) {
  const auto& intf0 = testInterfaces.at(0);
  const auto& remoteHost = intf0.remoteHosts.at(0);

  // Remove the neighbor so the next hop is unresolved
  auto arpEntry = makeArpEntry(intf0.id, remoteHost);
  saiManagerTable->neighborManager().removeNeighbor(arpEntry);

  // Create MySid entry pointing to the unresolved next hop
  auto mySid = makeMySidWithNextHop(
      remoteHost.ip,
      InterfaceID(intf0.id),
      "fc00:200::1",
      48,
      MySidType::ADJACENCY_MICRO_SID);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  ASSERT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();

  // NextHopId should be null and PacketAction should be DROP
  auto initialNextHopId =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_EQ(initialNextHopId, SAI_NULL_OBJECT_ID);
  auto initialAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(initialAction, SAI_PACKET_ACTION_DROP);

  // Resolve the neighbor — triggers afterCreate on the managed next hop
  resolveArp(intf0.id, remoteHost);

  // NextHopId should now be set and PacketAction should be FORWARD
  auto restoredNextHopId =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_NE(restoredNextHopId, SAI_NULL_OBJECT_ID);
  auto forwardAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(forwardAction, SAI_PACKET_ACTION_FORWARD);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid);
}

TEST_F(MySidManagerTest, linkDownAndReResolveWithSingleNextHop) {
  const auto& intf0 = testInterfaces.at(0);
  const auto& remoteHost = intf0.remoteHosts.at(0);
  auto mySid = makeMySidWithNextHop(
      remoteHost.ip,
      InterfaceID(intf0.id),
      "fc00:200::1",
      48,
      MySidType::ADJACENCY_MICRO_SID);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  ASSERT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();

  // Verify initial state: NextHopId is set and PacketAction is FORWARD
  auto initialNextHopId =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_NE(initialNextHopId, SAI_NULL_OBJECT_ID);
  auto initialAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(initialAction, SAI_PACKET_ACTION_FORWARD);

  // Simulate link down — cascades through FDB → neighbor → managed next hop
  saiManagerTable->fdbManager().handleLinkDown(
      SaiPortDescriptor(PortID(remoteHost.port.id)));

  // NextHopId should be cleared and PacketAction should be DROP
  auto clearedNextHopId =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_EQ(clearedNextHopId, SAI_NULL_OBJECT_ID);
  auto dropAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(dropAction, SAI_PACKET_ACTION_DROP);

  // Remove the neighbor and FDB entry, then re-resolve
  auto arpEntry = makeArpEntry(intf0.id, remoteHost);
  saiManagerTable->neighborManager().removeNeighbor(arpEntry);
  resolveArp(intf0.id, remoteHost);

  // NextHopId should be restored and PacketAction should be FORWARD
  auto restoredNextHopId =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_NE(restoredNextHopId, SAI_NULL_OBJECT_ID);
  auto forwardAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(forwardAction, SAI_PACKET_ACTION_FORWARD);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid);
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

TEST_F(MySidManagerTest, ecmpNextHopGroupRemainsNonNullWhenMembersUnresolve) {
  const auto& intf0 = testInterfaces.at(0);
  const auto& intf1 = testInterfaces.at(1);
  const auto& host0 = intf0.remoteHosts.at(0);
  const auto& host1 = intf1.remoteHosts.at(0);

  std::vector<std::pair<folly::IPAddress, InterfaceID>> nextHops = {
      {host0.ip, InterfaceID(intf0.id)}, {host1.ip, InterfaceID(intf1.id)}};
  auto mySid = makeMySidWithEcmpNextHops(
      nextHops, "fc00:300::1", 48, MySidType::NODE_MICRO_SID);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  ASSERT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();

  // Verify initial state: NextHopId is set (points to next hop group)
  // Note: FakeSai NHG OIDs can be 0, which equals SAI_NULL_OBJECT_ID,
  // so we verify the NHG is valid by checking it has the expected members.
  auto initialNextHopId =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  auto nhgId = NextHopGroupSaiId(initialNextHopId);

  // Both members should be in the group — this proves NextHopId points to a
  // valid NHG
  SaiNextHopGroupTraits::Attributes::NextHopMemberList memberList{};
  auto members = nextHopGroupApi.getAttribute(nhgId, memberList);
  EXPECT_EQ(members.size(), 2);

  // Remove one neighbor — that member should be removed from the group
  auto arpEntry0 = makeArpEntry(intf0.id, host0);
  saiManagerTable->neighborManager().removeNeighbor(arpEntry0);

  members = nextHopGroupApi.getAttribute(nhgId, memberList);
  EXPECT_EQ(members.size(), 1);

  // NextHopId on MySid should still be the same (non-null) next hop group
  auto nhIdAfterOneDown =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_EQ(nhIdAfterOneDown, initialNextHopId);

  // Remove the second neighbor — group should have 0 members but still exist
  auto arpEntry1 = makeArpEntry(intf1.id, host1);
  saiManagerTable->neighborManager().removeNeighbor(arpEntry1);

  members = nextHopGroupApi.getAttribute(nhgId, memberList);
  EXPECT_EQ(members.size(), 0);

  // NextHopId on MySid should STILL be non-null (group object persists)
  auto nhIdAfterAllDown =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_EQ(nhIdAfterAllDown, initialNextHopId);

  // Re-resolve both neighbors — members should reappear
  resolveArp(intf0.id, host0);
  resolveArp(intf1.id, host1);

  members = nextHopGroupApi.getAttribute(nhgId, memberList);
  EXPECT_EQ(members.size(), 2);

  auto nhIdAfterReResolve =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_EQ(nhIdAfterReResolve, initialNextHopId);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid);
}

TEST_F(MySidManagerTest, ecmpPartialUnresolveThenReResolve) {
  const auto& intf0 = testInterfaces.at(0);
  const auto& intf1 = testInterfaces.at(1);
  const auto& host0 = intf0.remoteHosts.at(0);
  const auto& host1 = intf1.remoteHosts.at(0);

  std::vector<std::pair<folly::IPAddress, InterfaceID>> nextHops = {
      {host0.ip, InterfaceID(intf0.id)}, {host1.ip, InterfaceID(intf1.id)}};
  auto mySid = makeMySidWithEcmpNextHops(
      nextHops, "fc00:400::1", 48, MySidType::NODE_MICRO_SID);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto& srv6Api = saiApiTable->srv6Api();
  auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();

  auto nhgOid =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  auto nhgId = NextHopGroupSaiId(nhgOid);

  SaiNextHopGroupTraits::Attributes::NextHopMemberList memberList{};
  EXPECT_EQ(nextHopGroupApi.getAttribute(nhgId, memberList).size(), 2);

  // Remove one neighbor, re-resolve it, verify group is back to 2 members
  auto arpEntry0 = makeArpEntry(intf0.id, host0);
  saiManagerTable->neighborManager().removeNeighbor(arpEntry0);
  EXPECT_EQ(nextHopGroupApi.getAttribute(nhgId, memberList).size(), 1);

  resolveArp(intf0.id, host0);
  EXPECT_EQ(nextHopGroupApi.getAttribute(nhgId, memberList).size(), 2);

  // NextHopId on MySid unchanged throughout
  auto nhIdAfter =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_EQ(nhIdAfter, nhgOid);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid);
}

#endif
