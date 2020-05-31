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

class AclTableStoreTest : public SaiStoreTest {
 public:
  const std::vector<sai_int32_t>& kBindPointTypeList() const {
    static const std::vector<sai_int32_t> bindPointTypeList{
        SAI_ACL_BIND_POINT_TYPE_PORT};

    return bindPointTypeList;
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

  AclEntrySaiId createAclEntry(AclTableSaiId aclTableId) const {
    SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
    return saiApiTable->aclApi().create<SaiAclEntryTraits>(
        {aclTableIdAttribute, this->kPriority(), true}, 0);
  }
};

TEST_F(AclTableStoreTest, loadAclTables) {
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  auto aclTableId2 = createAclTable(SAI_ACL_STAGE_EGRESS);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiAclTableTraits>();

  SaiAclTableTraits::AdapterHostKey k{SAI_ACL_STAGE_INGRESS,
                                      this->kBindPointTypeList(),
                                      this->kActionTypeList(),
                                      true};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), aclTableId);

  SaiAclTableTraits::AdapterHostKey k2{SAI_ACL_STAGE_EGRESS,
                                       this->kBindPointTypeList(),
                                       this->kActionTypeList(),
                                       true};
  auto got2 = store.get(k2);
  EXPECT_NE(got2, nullptr);
  EXPECT_EQ(got2->adapterKey(), aclTableId2);
}

TEST_F(AclTableStoreTest, loadAclEntry) {
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  auto aclEntryId = createAclEntry(aclTableId);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiAclEntryTraits>();

  SaiAclEntryTraits::AdapterHostKey k{aclTableId, this->kPriority(), true};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), aclEntryId);
}

TEST_F(AclTableStoreTest, aclTableCtorLoad) {
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  SaiObject<SaiAclTableTraits> obj(aclTableId);
  EXPECT_EQ(obj.adapterKey(), aclTableId);
}

TEST_F(AclTableStoreTest, aclEntryLoadCtor) {
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  auto aclEntryId = createAclEntry(aclTableId);

  SaiObject<SaiAclEntryTraits> obj(aclEntryId);
  EXPECT_EQ(obj.adapterKey(), aclEntryId);
}

TEST_F(AclTableStoreTest, aclTableCtorCreate) {
  SaiAclTableTraits::CreateAttributes c{SAI_ACL_STAGE_INGRESS,
                                        this->kBindPointTypeList(),
                                        this->kActionTypeList(),
                                        true};
  SaiAclTableTraits::AdapterHostKey k{SAI_ACL_STAGE_INGRESS,
                                      this->kBindPointTypeList(),
                                      this->kActionTypeList(),
                                      true};
  SaiObject<SaiAclTableTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(AclTable, Stage, obj.attributes()), SAI_ACL_STAGE_INGRESS);
}

TEST_F(AclTableStoreTest, AclEntryCreateCtor) {
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);

  SaiAclEntryTraits::CreateAttributes c{aclTableId, this->kPriority(), true};
  SaiAclEntryTraits::AdapterHostKey k{aclTableId, this->kPriority(), true};
  SaiObject<SaiAclEntryTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(AclEntry, TableId, obj.attributes()), aclTableId);
}

TEST_F(AclTableStoreTest, serDeserAclTableStore) {
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  verifyAdapterKeySerDeser<SaiAclTableTraits>({aclTableId});
}

TEST_F(AclTableStoreTest, serDeserAclEntryStore) {
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  auto aclEntryId = createAclEntry(aclTableId);
  verifyAdapterKeySerDeser<SaiAclEntryTraits>({aclEntryId});
}
