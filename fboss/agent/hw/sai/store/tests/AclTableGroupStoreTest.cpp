/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/AclApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class AclTableGroupStoreTest : public SaiStoreTest {
 public:
  const std::vector<sai_int32_t>& kBindPointTypeList() const {
    static const std::vector<sai_int32_t> bindPointTypeList{
        SAI_ACL_BIND_POINT_TYPE_SWITCH};

    return bindPointTypeList;
  }

  sai_acl_table_group_type_t kTableGroupType() const {
    return SAI_ACL_TABLE_GROUP_TYPE_PARALLEL;
  }

  const std::vector<sai_int32_t>& kActionTypeList() const {
    static const std::vector<sai_int32_t> actionTypeList{
        SAI_ACL_ACTION_TYPE_SET_DSCP};

    return actionTypeList;
  }

  sai_uint32_t kPriority() const {
    return SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY;
  }

  AclTableSaiId createAclTable(sai_int32_t stage) const {
    return saiApiTable->aclApi().create<SaiAclTableTraits>(
        {stage, kBindPointTypeList(), kActionTypeList(), true}, 0);
  }

  AclTableGroupSaiId createAclTableGroup(sai_int32_t stage) const {
    return saiApiTable->aclApi().create<SaiAclTableGroupTraits>(
        {stage, kBindPointTypeList(), kTableGroupType()}, 0);
  }

  AclTableGroupMemberSaiId createAclTableGroupMember(
      AclTableGroupSaiId aclTableGroupId,
      AclTableSaiId aclTableId) const {
    SaiAclTableGroupMemberTraits::Attributes::TableGroupId
        aclTableGroupTableGroupIdAttribute{aclTableGroupId};
    SaiAclTableGroupMemberTraits::Attributes::TableId
        aclTableGroupTableIdAttribute{aclTableId};
    SaiAclTableGroupMemberTraits::Attributes::Priority
        aclTableGroupPriorityAttribute{kPriority()};

    return saiApiTable->aclApi().create<SaiAclTableGroupMemberTraits>(
        {aclTableGroupTableGroupIdAttribute,
         aclTableGroupTableIdAttribute,
         aclTableGroupPriorityAttribute},
        0);
  }
};

TEST_F(AclTableGroupStoreTest, loadAclTableGroups) {
  auto aclTableGroupId = createAclTableGroup(SAI_ACL_STAGE_INGRESS);
  auto aclTableGroupId2 = createAclTableGroup(SAI_ACL_STAGE_EGRESS);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiAclTableGroupTraits>();

  SaiAclTableGroupTraits::AdapterHostKey k{SAI_ACL_STAGE_INGRESS,
                                           this->kBindPointTypeList(),
                                           this->kTableGroupType()};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), aclTableGroupId);

  SaiAclTableGroupTraits::AdapterHostKey k2{SAI_ACL_STAGE_EGRESS,
                                            this->kBindPointTypeList(),
                                            this->kTableGroupType()};
  auto got2 = store.get(k2);
  EXPECT_NE(got2, nullptr);
  EXPECT_EQ(got2->adapterKey(), aclTableGroupId2);
}

TEST_F(AclTableGroupStoreTest, loadAclTableGroupMember) {
  auto aclTableGroupId = createAclTableGroup(SAI_ACL_STAGE_INGRESS);
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiAclTableGroupMemberTraits>();

  SaiAclTableGroupMemberTraits::AdapterHostKey k{
      aclTableGroupId, aclTableId, kPriority()};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), aclTableGroupMemberId);
  EXPECT_EQ(got->adapterKey(), aclTableId);
}

TEST_F(AclTableGroupStoreTest, aclTableGroupLoadCtor) {
  auto aclTableGroupId = createAclTableGroup(SAI_ACL_STAGE_INGRESS);
  SaiObject<SaiAclTableGroupTraits> obj(aclTableGroupId);
  EXPECT_EQ(obj.adapterKey(), aclTableGroupId);
}

TEST_F(AclTableGroupStoreTest, aclTableGroupMemberLoadCtor) {
  auto aclTableGroupId = createAclTableGroup(SAI_ACL_STAGE_INGRESS);
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);

  SaiObject<SaiAclTableGroupMemberTraits> obj(aclTableGroupMemberId);
  EXPECT_EQ(obj.adapterKey(), aclTableGroupMemberId);
}

TEST_F(AclTableGroupStoreTest, aclTableGroupCreateCtor) {
  SaiAclTableGroupTraits::CreateAttributes c{SAI_ACL_STAGE_INGRESS,
                                             this->kBindPointTypeList(),
                                             this->kTableGroupType()};
  SaiAclTableGroupTraits::AdapterHostKey k{SAI_ACL_STAGE_INGRESS,
                                           this->kBindPointTypeList(),
                                           this->kTableGroupType()};
  SaiObject<SaiAclTableGroupTraits> obj(k, c, 0);
  EXPECT_EQ(
      GET_ATTR(AclTableGroup, Stage, obj.attributes()), SAI_ACL_STAGE_INGRESS);
}

TEST_F(AclTableGroupStoreTest, aclTableGroupMemberCreateCtor) {
  auto aclTableGroupId = createAclTableGroup(SAI_ACL_STAGE_INGRESS);
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);

  SaiAclTableGroupMemberTraits::CreateAttributes c{
      aclTableGroupId, aclTableId, this->kPriority()};

  SaiAclTableGroupMemberTraits::AdapterHostKey k{
      aclTableGroupId, aclTableId, this->kPriority()};
  SaiObject<SaiAclTableGroupMemberTraits> obj(k, c, 0);
  EXPECT_EQ(
      GET_ATTR(AclTableGroupMember, TableGroupId, obj.attributes()),
      aclTableGroupId);
}

TEST_F(AclTableGroupStoreTest, serDeserAclTableGroupStore) {
  auto aclTableGroupId = createAclTableGroup(SAI_ACL_STAGE_INGRESS);
  verifyAdapterKeySerDeser<SaiAclTableGroupTraits>({aclTableGroupId});
}

TEST_F(AclTableGroupStoreTest, serDeserAclTableGroupMemberStore) {
  auto aclTableGroupId = createAclTableGroup(SAI_ACL_STAGE_INGRESS);
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  verifyAdapterKeySerDeser<SaiAclTableGroupMemberTraits>(
      {aclTableGroupMemberId});
}
