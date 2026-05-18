// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/api/Srv6Api.h"
#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSrv6MySidManager.h"
#include "fboss/agent/hw/sai/switch/SaiSrv6SidListManager.h"
#include "fboss/agent/hw/sai/switch/SaiSrv6TunnelManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/Srv6Tunnel.h"
#include "fboss/agent/test/utils/NextHopIdTestUtils.h"

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

TEST_F(MySidManagerTest, addUnresolvedUASidProgramsWithDropAction) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  auto state = getProgrammedState();
  // uA / uN unresolved entries are programmed in SAI with PacketAction=DROP
  // so a midpoint packet hits the SRv6 lookup and increments the discard
  // counter rather than silently falling through to regular IP routing.
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  EXPECT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  EXPECT_EQ(gotAction, SAI_PACKET_ACTION_DROP);

  auto gotNextHopId =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_EQ(gotNextHopId, SAI_NULL_OBJECT_ID);
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

TEST_F(MySidManagerTest, removeUnresolvedUASidErasesHandle) {
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::ADJACENCY_MICRO_SID);
  auto state = getProgrammedState();
  // uA / uN unresolved entries ARE programmed (with PacketAction=DROP), so
  // removeMySidEntry must erase the SAI handle.
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);

  saiManagerTable->srv6MySidManager().removeMySidEntry(mySid, state);
  EXPECT_EQ(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);
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
  // Unresolved uA entry is programmed with PacketAction=DROP.
  EXPECT_NE(saiManagerTable->srv6MySidManager().getMySidObject(key), nullptr);
  {
    auto& srv6Api = saiApiTable->srv6Api();
    auto gotAction = srv6Api.getAttribute(
        key, SaiMySidEntryTraits::Attributes::PacketAction{});
    EXPECT_EQ(gotAction, SAI_PACKET_ACTION_DROP);
  }

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

class MySidBindingSidTest : public MySidManagerWithNextHopIdTest {
 public:
  static inline const std::string kSrv6TunnelId{"srv6Tunnel0"};

  void addSrv6Tunnel() const {
    auto tunnel = std::make_shared<Srv6Tunnel>(kSrv6TunnelId);
    tunnel->setType(TunnelType::SRV6_ENCAP);
    tunnel->setUnderlayIntfId(InterfaceID(testInterfaces[0].id));
    tunnel->setSrcIP(folly::IPAddressV6("2001:db8::1"));
    tunnel->setTTLMode(cfg::TunnelMode::PIPE);
    tunnel->setDscpMode(cfg::TunnelMode::UNIFORM);
    tunnel->setEcnMode(cfg::TunnelMode::UNIFORM);
    saiManagerTable->srv6TunnelManager().addSrv6Tunnel(tunnel);
  }

  ResolvedNextHop makeSrv6NextHop(
      const TestInterface& testInterface,
      const folly::IPAddressV6& nextHopIp,
      const std::vector<folly::IPAddressV6>& sidList) const {
    return ResolvedNextHop{
        nextHopIp,
        InterfaceID(testInterface.id),
        ECMP_WEIGHT,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        sidList,
        TunnelType::SRV6_ENCAP,
        kSrv6TunnelId};
  }

  void resolveNdp(
      const TestInterface& testInterface,
      const folly::IPAddressV6& ip,
      const folly::MacAddress& mac) const {
    const auto& remote = testInterface.remoteHosts.at(0);
    auto ndpEntry = std::make_shared<NdpEntry>(
        ip,
        mac,
        PortDescriptor(PortID(remote.port.id)),
        InterfaceID(testInterface.id),
        state::NeighborEntryType::DYNAMIC_ENTRY);
    saiManagerTable->neighborManager().addNeighbor(ndpEntry);
    saiManagerTable->fdbManager().addFdbEntry(
        SaiPortDescriptor(PortID(remote.port.id)),
        InterfaceID(testInterface.id),
        mac,
        SAI_FDB_ENTRY_TYPE_STATIC,
        std::nullopt);
  }

  std::shared_ptr<SwitchState> makeStateWithNextHopIdMaps() {
    auto state = getProgrammedState()->clone();
    populateFibInfoIdMaps(nextHopIDManager_.get(), state);
    state->publish();
    return state;
  }

  sai_object_id_t verifySrv6SidListAndNextHop(
      const ResolvedNextHop& swNextHop) const {
    auto* rifHandle =
        saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
            swNextHop.intfID().value());
    EXPECT_NE(rifHandle, nullptr);
    if (!rifHandle) {
      return SAI_NULL_OBJECT_ID;
    }

    const auto expectedSegments = toSaiIp6List(swNextHop.srv6SegmentList());
    SaiSrv6SidListTraits::AdapterHostKey sidListKey{
        SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED,
        expectedSegments,
        rifHandle->adapterKey(),
        swNextHop.addr()};
    auto* sidListHandle =
        saiManagerTable->srv6SidListManager().getSrv6SidListHandle(sidListKey);
    EXPECT_NE(sidListHandle, nullptr);
    if (!sidListHandle || !sidListHandle->managedSidList->getSidList()) {
      return SAI_NULL_OBJECT_ID;
    }

    const auto sidListId =
        sidListHandle->managedSidList->getSidList()->adapterKey();
    auto gotSegments = saiApiTable->srv6Api().getAttribute(
        sidListId, SaiSrv6SidListTraits::Attributes::SegmentList{});
    EXPECT_EQ(gotSegments, expectedSegments);

    auto adapterHostKey = saiManagerTable->nextHopManager().getAdapterHostKey(
        swNextHop, sidListId);
    auto* srv6Key = std::get_if<SaiSrv6SidlistNextHopTraits::AdapterHostKey>(
        &adapterHostKey);
    EXPECT_NE(srv6Key, nullptr);
    if (!srv6Key) {
      return SAI_NULL_OBJECT_ID;
    }

    auto* managedNh =
        saiManagerTable->nextHopManager().getManagedNextHop(*srv6Key);
    EXPECT_NE(managedNh, nullptr);
    if (!managedNh || !managedNh->getSaiObject()) {
      return SAI_NULL_OBJECT_ID;
    }
    return managedNh->getSaiObject()->adapterKey();
  }
};

TEST_F(MySidBindingSidTest, addWithResolvedSrv6NextHopGroup) {
  addSrv6Tunnel();

  const std::vector<folly::IPAddressV6> sidList0{
      folly::IPAddressV6("2001:db8::10"), folly::IPAddressV6("2001:db8::20")};
  const std::vector<folly::IPAddressV6> sidList1{
      folly::IPAddressV6("2001:db8::30"), folly::IPAddressV6("2001:db8::40")};
  const auto srv6NextHop0 = makeSrv6NextHop(
      testInterfaces[0], folly::IPAddressV6("fe80::10"), sidList0);
  const auto srv6NextHop1 = makeSrv6NextHop(
      testInterfaces[1], folly::IPAddressV6("fe80::11"), sidList1);
  resolveNdp(
      testInterfaces[0],
      srv6NextHop0.addr().asV6(),
      folly::MacAddress{"10:10:10:10:10:a0"});
  resolveNdp(
      testInterfaces[1],
      srv6NextHop1.addr().asV6(),
      folly::MacAddress{"10:10:10:10:10:a1"});

  RouteNextHopSet nhopSet{srv6NextHop0, srv6NextHop1};
  auto allocResult = nextHopIDManager_->getOrAllocRouteNextHopSetID(nhopSet);
  auto nextHopSetId = allocResult.nextHopIdSetIter->second.id;

  auto state = makeStateWithNextHopIdMaps();
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::BINDING_MICRO_SID);
  mySid->setResolvedNextHopsId(nextHopSetId);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  ASSERT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotBehavior = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::EndpointBehavior{});
  EXPECT_EQ(gotBehavior, SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_B6_ENCAPS_RED);

  auto gotAction = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::PacketAction{});
  // In fake SAI, NHG members are managed/deferred so the NHG adapter key
  // is SAI_NULL_OBJECT_ID, which correctly triggers DROP packet action.
  EXPECT_EQ(gotAction, SAI_PACKET_ACTION_DROP);

  auto gotNextHopId =
      srv6Api.getAttribute(key, SaiMySidEntryTraits::Attributes::NextHopId{});
  EXPECT_EQ(gotNextHopId, SAI_NULL_OBJECT_ID);

  auto nhgHandle =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(nhopSet, std::nullopt));
  ASSERT_NE(nhgHandle, nullptr);
  EXPECT_EQ(nhgHandle->nextHopGroupSize(), 2);

  std::set<sai_object_id_t> nextHopIds;
  nextHopIds.insert(verifySrv6SidListAndNextHop(srv6NextHop0));
  nextHopIds.insert(verifySrv6SidListAndNextHop(srv6NextHop1));
  EXPECT_EQ(nextHopIds.size(), 2);
}

TEST_F(MySidBindingSidTest, addWithDistinctResolvedSrv6NextHopsAndSameSidList) {
  addSrv6Tunnel();

  const std::vector<folly::IPAddressV6> sidList{
      folly::IPAddressV6("2001:db8::10"), folly::IPAddressV6("2001:db8::20")};
  const auto srv6NextHop0 = makeSrv6NextHop(
      testInterfaces[0], folly::IPAddressV6("fe80::10"), sidList);
  const auto srv6NextHop1 = makeSrv6NextHop(
      testInterfaces[1], folly::IPAddressV6("fe80::11"), sidList);
  resolveNdp(
      testInterfaces[0],
      srv6NextHop0.addr().asV6(),
      folly::MacAddress{"10:10:10:10:10:a0"});
  resolveNdp(
      testInterfaces[1],
      srv6NextHop1.addr().asV6(),
      folly::MacAddress{"10:10:10:10:10:a1"});

  RouteNextHopSet nhopSet{srv6NextHop0, srv6NextHop1};
  auto allocResult = nextHopIDManager_->getOrAllocRouteNextHopSetID(nhopSet);
  auto nextHopSetId = allocResult.nextHopIdSetIter->second.id;

  auto state = makeStateWithNextHopIdMaps();
  auto mySid = makeMySid("fc00:100::1", 48, MySidType::BINDING_MICRO_SID);
  mySid->setResolvedNextHopsId(nextHopSetId);
  saiManagerTable->srv6MySidManager().addMySidEntry(mySid, state);

  auto key = getMySidAdapterHostKey(*mySid, saiManagerTable);
  auto saiEntry = saiManagerTable->srv6MySidManager().getMySidObject(key);
  ASSERT_NE(saiEntry, nullptr);

  auto& srv6Api = saiApiTable->srv6Api();
  auto gotBehavior = srv6Api.getAttribute(
      key, SaiMySidEntryTraits::Attributes::EndpointBehavior{});
  EXPECT_EQ(gotBehavior, SAI_MY_SID_ENTRY_ENDPOINT_BEHAVIOR_B6_ENCAPS_RED);

  auto nhgHandle =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(nhopSet, std::nullopt));
  ASSERT_NE(nhgHandle, nullptr);
  EXPECT_EQ(nhgHandle->nextHopGroupSize(), 2);

  std::set<sai_object_id_t> nextHopIds;
  nextHopIds.insert(verifySrv6SidListAndNextHop(srv6NextHop0));
  nextHopIds.insert(verifySrv6SidListAndNextHop(srv6NextHop1));
  EXPECT_EQ(nextHopIds.size(), 2);
}

#endif
