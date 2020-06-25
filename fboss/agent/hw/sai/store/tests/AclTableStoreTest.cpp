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

  std::pair<folly::IPAddressV6, folly::IPAddressV6> kSrcIpV6() const {
    return std::make_pair(
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"));
  }

  std::pair<folly::IPAddressV6, folly::IPAddressV6> kDstIpV6() const {
    return std::make_pair(
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"));
  }

  std::pair<sai_uint16_t, sai_uint16_t> kL4SrcPort() const {
    return std::make_pair(9001, 0xFFFF);
  }

  std::pair<sai_uint16_t, sai_uint16_t> kL4DstPort() const {
    return std::make_pair(9002, 0xFFFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIpProtocol() const {
    return std::make_pair(6, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kTcpFlags() const {
    return std::make_pair(1, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kDscp() const {
    // TOS is 8-bits: 6-bit DSCP followed by 2-bit ECN.
    // mask of 0xFC to match on 6-bit DSCP
    return std::make_pair(10, 0xFC);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kTtl() const {
    return std::make_pair(128, 128);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kFdbDstUserMeta() const {
    return std::make_pair(11, 0xFFFFFFFF);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kRouteDstUserMeta() const {
    return std::make_pair(11, 0xFFFFFFFF);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kNeighborDstUserMeta() const {
    return std::make_pair(11, 0xFFFFFFFF);
  }

  sai_uint8_t kSetTC() const {
    return 1;
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
            true, // ttl
            true, // fdb meta
            true, // route meta
            true // neighbor meta
        },
        0);
  }

  AclEntrySaiId createAclEntry(AclTableSaiId aclTableId) const {
    SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
    return saiApiTable->aclApi().create<SaiAclEntryTraits>(
        {
            aclTableIdAttribute,
            this->kPriority(),
            AclEntryFieldIpV6(this->kSrcIpV6()),
            AclEntryFieldIpV6(this->kDstIpV6()),
            AclEntryFieldU16(this->kL4SrcPort()),
            AclEntryFieldU16(this->kL4DstPort()),
            AclEntryFieldU8(this->kIpProtocol()),
            AclEntryFieldU8(this->kTcpFlags()),
            AclEntryFieldU8(this->kDscp()),
            AclEntryFieldU8(this->kTtl()),
            AclEntryFieldU32(this->kFdbDstUserMeta()),
            AclEntryFieldU32(this->kRouteDstUserMeta()),
            AclEntryFieldU32(this->kNeighborDstUserMeta()),
            AclEntryActionU8(this->kSetTC()),
        },
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
      true, // ttl
      true, // fdb meta
      true, // route meta
      true // neighbor meta
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
      true, // ttl
      true, // fdb meta
      true, // route meta
      true // neighbor meta
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

  SaiAclEntryTraits::AdapterHostKey k{aclTableId,
                                      this->kPriority(),
                                      this->kSrcIpV6(),
                                      this->kDstIpV6(),
                                      this->kL4SrcPort(),
                                      this->kL4DstPort(),
                                      this->kIpProtocol(),
                                      this->kTcpFlags(),
                                      this->kDscp(),
                                      this->kTtl(),
                                      this->kFdbDstUserMeta(),
                                      this->kRouteDstUserMeta(),
                                      this->kNeighborDstUserMeta(),
                                      this->kSetTC()};
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
      true, // ttl
      true, // fdb meta
      true, // route meta
      true // neighbor meta
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
      true, // ttl
      true, // fdb meta
      true, // route meta
      true // neighbor meta
  };

  SaiObject<SaiAclTableTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(AclTable, Stage, obj.attributes()), GetParam());
}

TEST_P(AclTableStoreParamTest, AclEntryCreateCtor) {
  auto aclTableId = createAclTable(GetParam());

  SaiAclEntryTraits::CreateAttributes c{aclTableId,
                                        this->kPriority(),
                                        this->kSrcIpV6(),
                                        this->kDstIpV6(),
                                        this->kL4SrcPort(),
                                        this->kL4DstPort(),
                                        this->kIpProtocol(),
                                        this->kTcpFlags(),
                                        this->kDscp(),
                                        this->kTtl(),
                                        this->kFdbDstUserMeta(),
                                        this->kRouteDstUserMeta(),
                                        this->kNeighborDstUserMeta(),
                                        this->kSetTC()};
  SaiAclEntryTraits::AdapterHostKey k{aclTableId,
                                      this->kPriority(),
                                      this->kSrcIpV6(),
                                      this->kDstIpV6(),
                                      this->kL4SrcPort(),
                                      this->kL4DstPort(),
                                      this->kIpProtocol(),
                                      this->kTcpFlags(),
                                      this->kDscp(),
                                      this->kTtl(),
                                      this->kFdbDstUserMeta(),
                                      this->kRouteDstUserMeta(),
                                      this->kNeighborDstUserMeta(),
                                      this->kSetTC()};
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
