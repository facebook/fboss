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

  std::pair<sai_uint8_t, sai_uint8_t> kDscp() const {
    // TOS is 8-bits: 6-bit DSCP followed by 2-bit ECN.
    // mask of 0xFC to match on 6-bit DSCP
    return std::make_pair(10, 0xFC);
  }

  AclTableSaiId createAclTable(sai_int32_t stage) const {
    return saiApiTable->aclApi().create<SaiAclTableTraits>(
        {
            stage,
            kBindPointTypeList(),
            kActionTypeList(),
            true, // srcIpv6
            true, // dstIpv6
            true, // l4SrcPort
            true, // l4DstPort
            true, // ipProtocol
            true, // tcpFlags
            true, // inPort
            true, // outPort
            true, // ipFrag
            true, // dscp
            true, // dstMac
            true, // ipType
            true // ttl
        },
        0);
  }

  AclEntrySaiId createAclEntry(AclTableSaiId aclTableId) const {
    SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
    return saiApiTable->aclApi().create<SaiAclEntryTraits>(
        {aclTableIdAttribute,
         this->kPriority(),
         AclEntryFieldU8(this->kDscp())},
        0);
  }
};

TEST_F(AclTableStoreTest, loadAclTables) {
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  auto aclTableId2 = createAclTable(SAI_ACL_STAGE_EGRESS);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiAclTableTraits>();

  SaiAclTableTraits::AdapterHostKey k{
      SAI_ACL_STAGE_INGRESS,
      this->kBindPointTypeList(),
      this->kActionTypeList(),
      true, // srcIpv6
      true, // dstIpv6
      true, // l4SrcPort
      true, // l4DstPort
      true, // ipProtocol
      true, // tcpFlags
      true, // inPort
      true, // outPort
      true, // ipFrag
      true, // dscp
      true, // dstMac
      true, // ipType
      true // ttl
  };

  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), aclTableId);

  SaiAclTableTraits::AdapterHostKey k2{
      SAI_ACL_STAGE_EGRESS,
      this->kBindPointTypeList(),
      this->kActionTypeList(),
      true, // srcIpv6
      true, // dstIpv6
      true, // l4SrcPort
      true, // l4DstPort
      true, // ipProtocol
      true, // tcpFlags
      true, // inPort
      true, // outPort
      true, // ipFrag
      true, // dscp
      true, // dstMac
      true, // ipType
      true // ttl
  };

  auto got2 = store.get(k2);
  EXPECT_NE(got2, nullptr);
  EXPECT_EQ(got2->adapterKey(), aclTableId2);
}

class AclTableStoreParamTest
    : public AclTableStoreTest,
      public testing::WithParamInterface<sai_acl_stage_t> {};

TEST_P(AclTableStoreParamTest, loadAclEntry) {
  auto aclTableId = createAclTable(GetParam());
  auto aclEntryId = createAclEntry(aclTableId);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiAclEntryTraits>();

  SaiAclEntryTraits::AdapterHostKey k{
      aclTableId, this->kPriority(), this->kDscp()};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), aclEntryId);
}

TEST_P(AclTableStoreParamTest, aclTableCtorLoad) {
  auto aclTableId = createAclTable(GetParam());
  SaiObject<SaiAclTableTraits> obj(aclTableId);
  EXPECT_EQ(obj.adapterKey(), aclTableId);
}

TEST_P(AclTableStoreParamTest, aclEntryLoadCtor) {
  auto aclTableId = createAclTable(GetParam());
  auto aclEntryId = createAclEntry(aclTableId);

  SaiObject<SaiAclEntryTraits> obj(aclEntryId);
  EXPECT_EQ(obj.adapterKey(), aclEntryId);
}

TEST_P(AclTableStoreParamTest, aclTableCtorCreate) {
  SaiAclTableTraits::CreateAttributes c{
      GetParam(),
      this->kBindPointTypeList(),
      this->kActionTypeList(),
      true, // srcIpv6
      true, // dstIpv6
      true, // l4SrcPort
      true, // l4DstPort
      true, // ipProtocol
      true, // tcpFlags
      true, // inPort
      true, // outPort
      true, // ipFrag
      true, // dscp
      true, // dstMac
      true, // ipType
      true // ttl
  };

  SaiAclTableTraits::AdapterHostKey k{
      GetParam(),
      this->kBindPointTypeList(),
      this->kActionTypeList(),
      true, // srcIpv6
      true, // dstIpv6
      true, // l4SrcPort
      true, // l4DstPort
      true, // ipProtocol
      true, // tcpFlags
      true, // inPort
      true, // outPort
      true, // ipFrag
      true, // dscp
      true, // dstMac
      true, // ipType
      true // ttl
  };

  SaiObject<SaiAclTableTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(AclTable, Stage, obj.attributes()), GetParam());
}

TEST_P(AclTableStoreParamTest, AclEntryCreateCtor) {
  auto aclTableId = createAclTable(GetParam());

  SaiAclEntryTraits::CreateAttributes c{
      aclTableId, this->kPriority(), this->kDscp()};
  SaiAclEntryTraits::AdapterHostKey k{
      aclTableId, this->kPriority(), this->kDscp()};
  SaiObject<SaiAclEntryTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(AclEntry, TableId, obj.attributes()), aclTableId);
}

TEST_P(AclTableStoreParamTest, serDeserAclTableStore) {
  auto aclTableId = createAclTable(GetParam());
  verifyAdapterKeySerDeser<SaiAclTableTraits>({aclTableId});
}

TEST_P(AclTableStoreParamTest, serDeserAclEntryStore) {
  auto aclTableId = createAclTable(GetParam());
  auto aclEntryId = createAclEntry(aclTableId);
  verifyAdapterKeySerDeser<SaiAclEntryTraits>({aclEntryId});
}

INSTANTIATE_TEST_CASE_P(
    AclTableStoreParamTestInstantiation,
    AclTableStoreParamTest,
    testing::Values(SAI_ACL_STAGE_INGRESS, SAI_ACL_STAGE_EGRESS));
