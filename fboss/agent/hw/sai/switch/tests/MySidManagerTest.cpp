// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

#include "fboss/agent/AgentFeatures.h"
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
  auto state = getProgrammedState();
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

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

TEST_F(MySidManagerTest, addDuplicateThrows) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  auto state = getProgrammedState();
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);
  EXPECT_THROW(
      saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state),
      FbossError);
}

TEST_F(MySidManagerTest, addUnresolvedSkips) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  auto state = getProgrammedState();
  // No resolvedNextHopsId set and type is not DECAPSULATE_AND_LOOKUP
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  EXPECT_EQ(saiEntry, nullptr);
}

TEST_F(MySidManagerTest, removeDecapsulateAndLookup) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  auto state = getProgrammedState();
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid, state);
  EXPECT_EQ(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);
}

TEST_F(MySidManagerTest, removeNonexistentThrows) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  auto state = getProgrammedState();
  EXPECT_THROW(
      saiManagerTable->srv6MySidManager().removeMySidEntry(mySid, state),
      FbossError);
}

TEST_F(MySidManagerTest, removeUnresolvedReturnsEarly) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  auto state = getProgrammedState();
  // Should not throw even though entry doesn't exist — returns early
  EXPECT_NO_THROW(
      saiManagerTable->srv6MySidManager().removeMySidEntry(mySid, state));
}

TEST_F(MySidManagerTest, changeType) {
  auto oldMySid =
      makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  auto state = getProgrammedState();
  saiManagerTable->srv6MySidManager().addMySidEntry(oldMySid, state);

  auto key = getMySidAdapterHostKey(*oldMySid, saiManagerTable);
  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);

  // Change to a different DECAPSULATE_AND_LOOKUP (re-add)
  auto newMySid =
      makeMySid("fc00:100::1", 48, MySidType::DECAPSULATE_AND_LOOKUP);
  saiManagerTable->srv6MySidManager().changeMySidEntry(
      oldMySid, newMySid, state);

  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);
}

class MySidManagerWithNextHopIdTest : public MySidManagerTest {
 public:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    MySidManagerTest::SetUp();
  }
  void TearDown() override {
    MySidManagerTest::TearDown();
    FLAGS_enable_nexthop_id_manager = false;
  }
};

TEST_F(MySidManagerWithNextHopIdTest, addWithSingleResolvedNextHop) {
  // Allocate next hop IDs for a single next hop
  RouteNextHopSet nhopSet;
  nhopSet.insert(makeNextHop(testInterfaces[0]));
  auto allocResult = nextHopIDManager_->getOrAllocRouteNextHopSetID(nhopSet);
  auto nextHopSetId = allocResult.nextHopIdSetIter->second.id;

  auto state = getProgrammedState();

  auto mySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  mySid->setResolvedNextHopsId(nextHopSetId);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  EXPECT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotBehavior = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::EndpointBehavior{});
  EXPECT_EQ(gotBehavior, SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UA);

  auto gotAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(gotAction, SAI_PACKET_ACTION_FORWARD);

  auto gotNextHopId =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_NE(gotNextHopId, SAI_NULL_OBJECT_ID);
}

TEST_F(MySidManagerWithNextHopIdTest, addWithMultipleResolvedNextHops) {
  // Allocate next hop IDs for multiple next hops
  RouteNextHopSet nhopSet;
  nhopSet.insert(makeNextHop(testInterfaces[0]));
  nhopSet.insert(makeNextHop(testInterfaces[1]));
  auto allocResult = nextHopIDManager_->getOrAllocRouteNextHopSetID(nhopSet);
  auto nextHopSetId = allocResult.nextHopIdSetIter->second.id;

  auto state = getProgrammedState();

  auto mySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  mySid->setResolvedNextHopsId(nextHopSetId);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  EXPECT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotBehavior = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::EndpointBehavior{});
  EXPECT_EQ(gotBehavior, SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UA);

  // In fake SAI, NHG members are managed/deferred so the NHG adapter key
  // is SAI_NULL_OBJECT_ID, which correctly triggers DROP packet action
  auto gotAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(gotAction, SAI_PACKET_ACTION_DROP);
}

TEST_F(MySidManagerWithNextHopIdTest, removeWithResolvedNextHop) {
  RouteNextHopSet nhopSet;
  nhopSet.insert(makeNextHop(testInterfaces[0]));
  auto allocResult = nextHopIDManager_->getOrAllocRouteNextHopSetID(nhopSet);
  auto nextHopSetId = allocResult.nextHopIdSetIter->second.id;

  auto state = getProgrammedState();

  auto mySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  mySid->setResolvedNextHopsId(nextHopSetId);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid, state);
  EXPECT_EQ(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);
}

TEST_F(MySidManagerWithNextHopIdTest, changeUnresolvedToResolved) {
  auto state = getProgrammedState();

  // Start with an unresolved MySid (no resolvedNextHopsId)
  auto oldMySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  saiManagerTable->srv6MySidManager().addMySidEntry(oldMySid, state);

  auto key = getMySidAdapterHostKey(*oldMySid, saiManagerTable);
  // Unresolved entry should not be programmed
  EXPECT_EQ(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);

  // Now resolve it with a next hop
  RouteNextHopSet nhopSet;
  nhopSet.insert(makeNextHop(testInterfaces[0]));
  auto allocResult = nextHopIDManager_->getOrAllocRouteNextHopSetID(nhopSet);
  auto nextHopSetId = allocResult.nextHopIdSetIter->second.id;

  // Re-fetch state with populated FibInfo ID maps
  state = getProgrammedState();

  auto newMySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  newMySid->setResolvedNextHopsId(nextHopSetId);
  saiManagerTable->srv6MySidManager().changeMySidEntry(
      oldMySid, newMySid, state);

  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotBehavior = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::EndpointBehavior{});
  EXPECT_EQ(gotBehavior, SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UA);

  auto gotAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(gotAction, SAI_PACKET_ACTION_FORWARD);
}

TEST_F(MySidManagerWithNextHopIdTest, addNodeMicroSidWithResolvedNextHop) {
  RouteNextHopSet nhopSet;
  nhopSet.insert(makeNextHop(testInterfaces[0]));
  auto allocResult = nextHopIDManager_->getOrAllocRouteNextHopSetID(nhopSet);
  auto nextHopSetId = allocResult.nextHopIdSetIter->second.id;

  auto state = getProgrammedState();

  auto mySid = makeMySid("fc00:100::1", 48, MySidType::NODE_MICRO_SID);
  mySid->setResolvedNextHopsId(nextHopSetId);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  EXPECT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotBehavior = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::EndpointBehavior{});
  EXPECT_EQ(gotBehavior, SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_UN);

  auto gotAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(gotAction, SAI_PACKET_ACTION_FORWARD);
}

#endif
