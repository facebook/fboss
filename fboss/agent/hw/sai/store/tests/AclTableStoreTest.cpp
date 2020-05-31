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

  AclTableSaiId createAclTable(sai_int32_t stage) const {
    return saiApiTable->aclApi().create<SaiAclTableTraits>(
        {stage, kBindPointTypeList(), kActionTypeList(), true}, 0);
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
