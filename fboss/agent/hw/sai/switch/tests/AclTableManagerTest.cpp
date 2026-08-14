/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/MatchAction.h"
#include "fboss/agent/types.h"

#include <folly/ScopeGuard.h>

#include <string>

using namespace facebook::fboss;

namespace {
const std::string kAclTable2 = "AclTable2";
}

class AclTableManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
  }
  int kPriority() {
    return 1;
  }

  int kPriority2() {
    return 2;
  }

  uint8_t kDscp() {
    return 10;
  }

  uint8_t kDscp2() {
    return 20;
  }

  cfg::AclActionType kActionType() {
    return cfg::AclActionType::DENY;
  }
};

TEST_F(AclTableManagerTest, addAclTable) {
  auto aclTableId =
      saiManagerTable->aclTableManager()
          .getAclTableHandle(
              cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE())
          ->aclTable->adapterKey();
  // Acl table is added as part of sai switch init in test setup
  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_INGRESS);
}

TEST_F(AclTableManagerTest, addTwoAclTable) {
  // AclTable1 should already be added
  auto aclTableId =
      saiManagerTable->aclTableManager()
          .getAclTableHandle(
              cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE())
          ->aclTable->adapterKey();
  auto table2 = std::make_shared<AclTable>(0, kAclTable2);
  AclTableSaiId aclTableId2 = saiManagerTable->aclTableManager().addAclTable(
      table2, cfg::AclStage::INGRESS, nullptr /*state*/);

  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_INGRESS);

  auto stageGot2 = saiApiTable->aclApi().getAttribute(
      aclTableId2, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot2, SAI_ACL_STAGE_INGRESS);
}

TEST_F(AclTableManagerTest, addDupAclTable) {
  state::AclTableFields fields{};
  fields.priority() = 0;
  fields.id() = cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE();
  auto table1 = std::make_shared<AclTable>(std::move(fields));
  EXPECT_THROW(
      saiManagerTable->aclTableManager().addAclTable(
          table1, cfg::AclStage::INGRESS, nullptr /*state*/),
      FbossError);
}

TEST_F(AclTableManagerTest, getAclTable) {
  auto handle = saiManagerTable->aclTableManager().getAclTableHandle(
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());

  EXPECT_TRUE(handle);
  EXPECT_TRUE(handle->aclTable);
}

TEST_F(AclTableManagerTest, checkNonExistentAclTable) {
  auto handle =
      saiManagerTable->aclTableManager().getAclTableHandle(kAclTable2);

  EXPECT_FALSE(handle);
}

TEST_F(AclTableManagerTest, addAclEntry) {
  auto aclTableId =
      saiManagerTable->aclTableManager()
          .getAclTableHandle(
              cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE())
          ->aclTable->adapterKey();

  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  aclEntry->setDscp(kDscp());
  aclEntry->setActionType(kActionType());

  AclEntrySaiId aclEntryId = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);

  auto tableIdGot = saiApiTable->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot, aclTableId);
}

TEST_F(AclTableManagerTest, addAclEntryWithCounter) {
  auto aclTableId =
      saiManagerTable->aclTableManager()
          .getAclTableHandle(
              cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE())
          ->aclTable->adapterKey();

  auto counter = cfg::TrafficCounter();
  *counter.name() = "stat0.c";
  MatchAction action = MatchAction();
  action.setTrafficCounter(counter);

  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  aclEntry->setDscp(kDscp());
  aclEntry->setAclAction(action);

  AclEntrySaiId aclEntryId = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);

  auto tableIdGot = saiApiTable->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot, aclTableId);

  auto aclCounterIdGot =
      saiApiTable->aclApi()
          .getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::ActionCounter())
          .getData();

  auto tableIdGot2 = saiApiTable->aclApi().getAttribute(
      AclCounterSaiId(aclCounterIdGot),
      SaiAclCounterTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot2, aclTableId);
}

TEST_F(AclTableManagerTest, addTwoAclEntry) {
  auto aclTableId =
      saiManagerTable->aclTableManager()
          .getAclTableHandle(
              cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE())
          ->aclTable->adapterKey();

  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  aclEntry->setDscp(kDscp());
  aclEntry->setActionType(kActionType());

  AclEntrySaiId aclEntryId = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);

  auto tableIdGot = saiApiTable->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot, aclTableId);

  auto aclEntry2 =
      std::make_shared<AclEntry>(kPriority2(), std::string("AclEntry2"));
  aclEntry2->setDscp(kDscp2());
  aclEntry2->setActionType(kActionType());

  AclEntrySaiId aclEntryId2 = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry2,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);

  auto tableIdGot2 = saiApiTable->aclApi().getAttribute(
      aclEntryId2, SaiAclEntryTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot2, aclTableId);
}

TEST_F(AclTableManagerTest, addDupAclEntry) {
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  aclEntry->setDscp(kDscp());
  aclEntry->setActionType(kActionType());

  saiManagerTable->aclTableManager().addAclEntry(
      aclEntry,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);

  auto dupAclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  dupAclEntry->setDscp(kDscp());
  dupAclEntry->setActionType(cfg::AclActionType::DENY);

  EXPECT_THROW(
      saiManagerTable->aclTableManager().addAclEntry(
          dupAclEntry,
          cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
          nullptr /*state*/),
      FbossError);
}

TEST_F(AclTableManagerTest, getAclEntry) {
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  aclEntry->setDscp(kDscp());
  aclEntry->setActionType(kActionType());

  saiManagerTable->aclTableManager().addAclEntry(
      aclEntry,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);

  auto aclTableHandle = saiManagerTable->aclTableManager().getAclTableHandle(
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());

  EXPECT_TRUE(aclTableHandle);
  EXPECT_TRUE(aclTableHandle->aclTable);

  auto aclEntryHandle = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority());

  EXPECT_TRUE(aclEntryHandle);
  EXPECT_TRUE(aclEntryHandle->aclEntry);
}

TEST_F(AclTableManagerTest, checkNonExistentAclEntry) {
  auto aclTableHandle = saiManagerTable->aclTableManager().getAclTableHandle(
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());

  EXPECT_TRUE(aclTableHandle);
  EXPECT_TRUE(aclTableHandle->aclTable);

  auto aclEntryHandle = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority());
  EXPECT_FALSE(aclEntryHandle);
}

TEST_F(AclTableManagerTest, aclMirroring) {
  std::string mirrorId = "mirror1";
  auto mirror = std::make_shared<Mirror>(
      mirrorId,
      std::make_optional<PortDescriptor>(PortID(1)),
      std::optional<folly::IPAddress>());
  mirror->setEgressPortDesc(PortDescriptor(PortID(1)));
  saiManagerTable->mirrorManager().addNode(mirror);
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  aclEntry->setDscp(kDscp());
  aclEntry->setActionType(kActionType());
  MatchAction matchAction = MatchAction();
  matchAction.setIngressMirror(mirrorId);
  aclEntry->setAclAction(matchAction);
  AclEntrySaiId aclEntryId = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);
  SaiMirrorHandle* mirrorHandle =
      saiManagerTable->mirrorManager().getMirrorHandle(mirrorId);
  auto gotMirrorSaiIdList = saiApiTable->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::ActionMirrorIngress());
  EXPECT_EQ((gotMirrorSaiIdList.getData())[0], mirrorHandle->adapterKey());
}

TEST_F(AclTableManagerTest, addAclEntryWithL4DstPortRange) {
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  cfg::Range range;
  range.minimum() = 1000;
  range.maximum() = 2000;
  aclEntry->setL4DstPortRange(range);
  aclEntry->setActionType(kActionType());

  saiManagerTable->aclTableManager().addAclEntry(
      aclEntry,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);

  auto aclTableHandle = saiManagerTable->aclTableManager().getAclTableHandle(
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());
  auto aclEntryHandle = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority());
  ASSERT_TRUE(aclEntryHandle);
  ASSERT_TRUE(aclEntryHandle->dstPortRange);

  auto rangeType = saiApiTable->aclApi().getAttribute(
      aclEntryHandle->dstPortRange->adapterKey(),
      SaiAclRangeTraits::Attributes::Type());
  EXPECT_EQ(rangeType, SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE);

  auto rangeLimit = saiApiTable->aclApi().getAttribute(
      aclEntryHandle->dstPortRange->adapterKey(),
      SaiAclRangeTraits::Attributes::Limit());
  EXPECT_EQ(rangeLimit.min, 1000);
  EXPECT_EQ(rangeLimit.max, 2000);
}

TEST_F(AclTableManagerTest, removeAclEntryWithRange) {
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  cfg::Range range;
  range.minimum() = 1000;
  range.maximum() = 2000;
  aclEntry->setL4DstPortRange(range);
  aclEntry->setActionType(kActionType());

  saiManagerTable->aclTableManager().addAclEntry(
      aclEntry,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);

  auto& rangeStore = saiStore->get<SaiAclRangeTraits>();
  EXPECT_EQ(rangeStore.size(), 1);

  saiManagerTable->aclTableManager().removeAclEntry(
      aclEntry, cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());

  auto aclTableHandle = saiManagerTable->aclTableManager().getAclTableHandle(
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());
  auto aclEntryHandle = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority());
  EXPECT_FALSE(aclEntryHandle);
  EXPECT_EQ(rangeStore.size(), 0);
}

TEST_F(AclTableManagerTest, twoEntriesSameRange) {
  cfg::Range range;
  range.minimum() = 1000;
  range.maximum() = 2000;

  auto aclEntry1 =
      std::make_shared<AclEntry>(kPriority(), std::string("AclEntry1"));
  aclEntry1->setL4DstPortRange(range);
  aclEntry1->setActionType(kActionType());
  saiManagerTable->aclTableManager().addAclEntry(
      aclEntry1,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);

  auto aclEntry2 =
      std::make_shared<AclEntry>(kPriority2(), std::string("AclEntry2"));
  aclEntry2->setL4DstPortRange(range);
  aclEntry2->setActionType(kActionType());
  saiManagerTable->aclTableManager().addAclEntry(
      aclEntry2,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      nullptr /*state*/);

  auto& rangeStore = saiStore->get<SaiAclRangeTraits>();
  EXPECT_EQ(rangeStore.size(), 1);

  auto aclTableHandle = saiManagerTable->aclTableManager().getAclTableHandle(
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());
  auto handle1 = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority());
  auto handle2 = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority2());
  ASSERT_TRUE(handle1->dstPortRange);
  ASSERT_TRUE(handle2->dstPortRange);
  EXPECT_EQ(
      handle1->dstPortRange->adapterKey(), handle2->dstPortRange->adapterKey());

  saiManagerTable->aclTableManager().removeAclEntry(
      aclEntry1, cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());
  EXPECT_EQ(rangeStore.size(), 1);

  saiManagerTable->aclTableManager().removeAclEntry(
      aclEntry2, cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());
  EXPECT_EQ(rangeStore.size(), 0);
}

class AclTableManagerPbrTest : public AclTableManagerTest {
 public:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    AclTableManagerTest::SetUp();
  }
  void TearDown() override {
    AclTableManagerTest::TearDown();
    FLAGS_enable_nexthop_id_manager = false;
  }
  // A NextHopSetID that is never allocated, for negative-resolution tests.
  static constexpr int64_t kUnallocatedNhgId = 987654321;
};

TEST_F(AclTableManagerPbrTest, addPbrAclEntryWithMatchAndRedirectNhg) {
  // 1. Allocate two named NHGs: one the entry matches on, one it redirects to.
  // RIB would do this; here we drive it directly so the name->id mapping lands
  // in FibInfo, which is how HW resolves the ids to nexthops.
  RouteNextHopSet matchNhops;
  matchNhops.insert(makeNextHop(testInterfaces[0]));
  nextHopIDManager_->allocateNamedNextHopGroup("matchNhg", matchNhops);
  RouteNextHopSet redirectNhops;
  redirectNhops.insert(makeNextHop(testInterfaces[1]));
  nextHopIDManager_->allocateNamedNextHopGroup("redirectNhg", redirectNhops);
  auto matchNhgIdOpt = nextHopIDManager_->getNextHopSetIDForName("matchNhg");
  ASSERT_TRUE(matchNhgIdOpt.has_value());
  auto matchNhgId = static_cast<int64_t>(*matchNhgIdOpt);
  auto redirectNhgIdOpt =
      nextHopIDManager_->getNextHopSetIDForName("redirectNhg");
  ASSERT_TRUE(redirectNhgIdOpt.has_value());
  auto redirectNhgId = static_cast<int64_t>(*redirectNhgIdOpt);
  auto state = getProgrammedState();

  // 2. Build a PBR AclEntry: match on an NHG -> redirect to another NHG. The
  // RIB resolves the config redirect NHG name to a NextHopSetID; switch state
  // (and HW) carry the id. (The TC field is exercised at the SAI API level;
  // matching on it here would require a table that declares the TC qualifier.)
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("PbrEntry"));
  aclEntry->setNextHopGroupId(matchNhgId);
  MatchAction action;
  action.setRedirectNextHopGroupId(redirectNhgId);
  aclEntry->setAclAction(action);

  // 3. Program it.
  AclEntrySaiId aclEntryId = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      state);

  // 4. The entry handle holds refs on both NHGs, so the SAI NHG objects
  // survive even though no route references them.
  auto* aclTableHandle = saiManagerTable->aclTableManager().getAclTableHandle(
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());
  auto* entryHandle = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority());
  ASSERT_NE(entryHandle, nullptr);
  ASSERT_NE(entryHandle->matchNhgHandle, nullptr);
  ASSERT_NE(entryHandle->redirectNhgHandle, nullptr);

  // 5. The SAI entry's match-NHG field and redirect action point at exactly
  // those held NHG OIDs.
  auto matchNhgGot = saiApiTable->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::FieldRouteDestination());
  EXPECT_EQ(
      matchNhgGot.getDataAndMask().first,
      entryHandle->matchNhgHandle->nextHopGroup->adapterKey());
  auto redirectGot = saiApiTable->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::ActionRedirect());
  EXPECT_EQ(
      redirectGot.getData(),
      entryHandle->redirectNhgHandle->nextHopGroup->adapterKey());

  // 6. Removing the entry drops its NHG refs (the SAI ACL entry destructs
  // before the NHG handles it held).
  saiManagerTable->aclTableManager().removeAclEntry(
      aclEntry,
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
      state);
  EXPECT_EQ(
      saiManagerTable->aclTableManager().getAclEntryHandle(
          aclTableHandle, kPriority()),
      nullptr);
}

TEST_F(AclTableManagerPbrTest, addPbrAclEntryMatchesOnTrafficClass) {
  // The fake rejects entry fields not declared as table qualifiers, so program
  // a table that declares TC. enable_acl_table_group lets the table's own
  // qualifier list take effect.
  FLAGS_enable_acl_table_group = true;
  SCOPE_EXIT {
    FLAGS_enable_acl_table_group = false;
  };
  const auto kTcTable = std::string("PbrTcTable");
  auto table = std::make_shared<AclTable>(0, kTcTable);
  table->setQualifiers({cfg::AclTableQualifier::TC});
  saiManagerTable->aclTableManager().addAclTable(
      table, cfg::AclStage::INGRESS, nullptr /*state*/);

  // A PBR entry matching on traffic class programs FieldTc with a full-value
  // (0xFF) mask.
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("PbrTcEntry"));
  aclEntry->setTrafficClass(3);
  aclEntry->setActionType(kActionType());

  AclEntrySaiId aclEntryId = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry, kTcTable, nullptr /*state*/);

  auto fieldTcGot = saiApiTable->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::FieldTc());
  EXPECT_EQ(fieldTcGot.getDataAndMask().first, 3);
  EXPECT_EQ(fieldTcGot.getDataAndMask().second, 0xFF);
}

TEST_F(AclTableManagerPbrTest, pbrMatchNhgUnknownIdThrows) {
  // A match-on-NHG id with no FibInfo mapping must fail loudly, not silently
  // drop the match.
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("PbrBadMatch"));
  aclEntry->setNextHopGroupId(kUnallocatedNhgId);
  aclEntry->setActionType(kActionType());
  EXPECT_THROW(
      saiManagerTable->aclTableManager().addAclEntry(
          aclEntry,
          cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
          getProgrammedState()),
      FbossError);
}

TEST_F(AclTableManagerPbrTest, pbrRedirectNhgUnknownIdThrows) {
  // A redirect to a NextHopSetID with no FibInfo mapping must fail loudly.
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("PbrBadRedirect"));
  MatchAction action;
  action.setRedirectNextHopGroupId(kUnallocatedNhgId);
  aclEntry->setAclAction(action);
  EXPECT_THROW(
      saiManagerTable->aclTableManager().addAclEntry(
          aclEntry,
          cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
          getProgrammedState()),
      FbossError);
}

TEST_F(AclTableManagerPbrTest, pbrMatchNhgNullStateThrows) {
  // Resolving a PBR NHG requires SwitchState; a null state must throw rather
  // than silently program an entry without its match NHG.
  RouteNextHopSet matchNhops;
  matchNhops.insert(makeNextHop(testInterfaces[0]));
  nextHopIDManager_->allocateNamedNextHopGroup("matchNhg", matchNhops);
  auto matchNhgIdOpt = nextHopIDManager_->getNextHopSetIDForName("matchNhg");
  ASSERT_TRUE(matchNhgIdOpt.has_value());
  auto matchNhgId = static_cast<int64_t>(*matchNhgIdOpt);
  auto aclEntry =
      std::make_shared<AclEntry>(kPriority(), std::string("PbrNullState"));
  aclEntry->setNextHopGroupId(matchNhgId);
  aclEntry->setActionType(kActionType());
  EXPECT_THROW(
      saiManagerTable->aclTableManager().addAclEntry(
          aclEntry,
          cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
          nullptr),
      FbossError);
}
