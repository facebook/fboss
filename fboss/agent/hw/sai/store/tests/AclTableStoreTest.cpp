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

  std::pair<folly::IPAddressV4, folly::IPAddressV4> kSrcIpV4() const {
    return std::make_pair(
        folly::IPAddressV4("10.0.0.1"), folly::IPAddressV4("255.255.255.0"));
  }

  std::pair<folly::IPAddressV4, folly::IPAddressV4> kDstIpV4() const {
    return std::make_pair(
        folly::IPAddressV4("20.0.0.1"), folly::IPAddressV4("255.255.255.0"));
  }

  std::pair<sai_object_id_t, sai_uint32_t> kSrcPort() const {
    return std::make_pair(41, 0);
  }

  std::pair<sai_object_id_t, sai_uint32_t> kOutPort() const {
    return std::make_pair(42, 0);
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

  std::pair<sai_uint32_t, sai_uint32_t> kIpFrag() const {
    return std::make_pair(
        SAI_ACL_IP_FRAG_ANY, 0 /*mask is N/A for field ip frag */);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV4Type() const {
    return std::make_pair(3 /* Destination unreachable */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV4Code() const {
    return std::make_pair(1 /* Host unreachable */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV6Type() const {
    return std::make_pair(1 /* Destination unreachable */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV6Code() const {
    return std::make_pair(4 /* Port unreachable */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kDscp() const {
    return std::make_pair(10, 0x3F);
  }

  std::pair<folly::MacAddress, folly::MacAddress> kDstMac() const {
    return std::make_pair(
        folly::MacAddress{"00:11:22:33:44:55"},
        folly::MacAddress{"ff:ff:ff:ff:ff:ff"});
  }

  std::pair<sai_uint32_t, sai_uint32_t> kIpType() const {
    return std::make_pair(SAI_ACL_IP_TYPE_IPV4ANY, 0);
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

  std::pair<sai_uint16_t, sai_uint16_t> kEtherType() const {
    return std::make_pair(0x0800, 0xFFFF);
  }

  std::pair<sai_uint16_t, sai_uint16_t> kOuterVlanId() const {
    return std::make_pair(2000, 0xFFFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kBthOpcode() const {
    return std::make_pair(17, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIpv6NextHeader() const {
    return std::make_pair(17, 0xFF);
  }

  sai_uint32_t kPacketAction() const {
    return SAI_PACKET_ACTION_DROP;
  }

  sai_object_id_t kCounter() const {
    return 42;
  }

  sai_object_id_t kMacsecFlow() const {
    return 42;
  }

  sai_object_id_t kSetUserTrap() const {
    return 50;
  }

  bool kDisableArsForwarding() const {
    return false;
  }

  sai_uint8_t kSetTC() const {
    return 1;
  }

  sai_uint8_t kSetDSCP() const {
    return 10;
  }

  std::vector<sai_object_id_t> kMirrorIngress() const {
    return {10, 11};
  }

  std::vector<sai_object_id_t> kMirrorEgress() const {
    return {110, 111};
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
  std::array<char, 32> kCounterLabel() const {
    std::array<char, 32> retVal = {"aclCounter1"};
    return retVal;
  }
#endif

  bool kEnablePacketCount() const {
    return true;
  }

  bool kEnableByteCount() const {
    return true;
  }

  sai_uint64_t kCounterPackets() const {
    return 0;
  }

  sai_uint64_t kCounterBytes() const {
    return 0;
  }

  sai_object_id_t kUdfGroupId() const {
    return 1;
  }

  std::pair<std::vector<sai_uint8_t>, std::vector<sai_uint8_t>> kUdfGroupData()
      const {
    std::vector<sai_uint8_t> data = {0x11, 0x22};
    std::vector<sai_uint8_t> mask = {0xFF, 0xFF};
    return std::make_pair(std::move(data), std::move(mask));
  }

  AclTableSaiId createAclTable(sai_int32_t stage) const {
    return saiApiTable->aclApi().create<SaiAclTableTraits>(
        {
            stage,
            kBindPointTypeList(),
            kActionTypeList(),
            true, // srcIpv6
            true, // dstIpv6
            true, // srcIpv4
            true, // dstIpv4
            true, // l4SrcPort
            true, // l4DstPort
            true, // ipProtocol
            true, // tcpFlags
            true, // srcPort
            true, // outPort
            true, // ipFrag
            true, // icmpv4Type
            true, // icmpv4Code
            true, // icmpv6Type
            true, // icmpv6Code
            true, // dscp
            true, // dstMac
            true, // ipType
            true, // ttl
            true, // fdb meta
            true, // route meta
            true, // neighbor meta
            true, // ethertype
            true, // outer vlan id
            true, // bth opcode
            true, // ipv6 next header
            kUdfGroupId(), // udf group 0
            kUdfGroupId() + 1, // udf group 1
            kUdfGroupId() + 2, // udf group 2
            kUdfGroupId() + 3, // udf group 3
            kUdfGroupId() + 4, // udf group 4
        },
        0);
  }

  AclEntrySaiId createAclEntry(AclTableSaiId aclTableId) const {
    SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
    return saiApiTable->aclApi().create<SaiAclEntryTraits>(
        {
            aclTableIdAttribute,
            this->kPriority(),
            true, // enabled
            AclEntryFieldIpV6(this->kSrcIpV6()),
            AclEntryFieldIpV6(this->kDstIpV6()),
            AclEntryFieldIpV4(this->kSrcIpV4()),
            AclEntryFieldIpV4(this->kDstIpV4()),
            AclEntryFieldSaiObjectIdT(this->kSrcPort()),
            AclEntryFieldSaiObjectIdT(this->kOutPort()),
            AclEntryFieldU16(this->kL4SrcPort()),
            AclEntryFieldU16(this->kL4DstPort()),
            AclEntryFieldU8(this->kIpProtocol()),
            AclEntryFieldU8(this->kTcpFlags()),
            AclEntryFieldU32(this->kIpFrag()),
            AclEntryFieldU8(this->kIcmpV4Type()),
            AclEntryFieldU8(this->kIcmpV4Code()),
            AclEntryFieldU8(this->kIcmpV6Type()),
            AclEntryFieldU8(this->kIcmpV6Code()),
            AclEntryFieldU8(this->kDscp()),
            AclEntryFieldMac(this->kDstMac()),
            AclEntryFieldU32(this->kIpType()),
            AclEntryFieldU8(this->kTtl()),
            AclEntryFieldU32(this->kFdbDstUserMeta()),
            AclEntryFieldU32(this->kRouteDstUserMeta()),
            AclEntryFieldU32(this->kNeighborDstUserMeta()),
            AclEntryFieldU16(this->kEtherType()),
            AclEntryFieldU16(this->kOuterVlanId()),
            AclEntryFieldU8(this->kBthOpcode()),
            AclEntryFieldU8(this->kIpv6NextHeader()),
            AclEntryFieldU8List(this->kUdfGroupData()),
            AclEntryFieldU8List(this->kUdfGroupData()),
            AclEntryFieldU8List(this->kUdfGroupData()),
            AclEntryFieldU8List(this->kUdfGroupData()),
            AclEntryFieldU8List(this->kUdfGroupData()),
            AclEntryActionU32(this->kPacketAction()),
            AclEntryActionSaiObjectIdT(this->kCounter()),
            AclEntryActionU8(this->kSetTC()),
            AclEntryActionU8(this->kSetDSCP()),
            AclEntryActionSaiObjectIdList(this->kMirrorIngress()),
            AclEntryActionSaiObjectIdList(this->kMirrorEgress()),
            AclEntryActionSaiObjectIdT(this->kMacsecFlow()),
            AclEntryActionSaiObjectIdT(this->kSetUserTrap()),
            AclEntryActionBool(this->kDisableArsForwarding()),
        },
        0);
  }

  AclCounterSaiId createAclCounter(AclTableSaiId aclTableId) const {
    SaiAclCounterTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
    return saiApiTable->aclApi().create<SaiAclCounterTraits>(
        {
            aclTableIdAttribute,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
            this->kCounterLabel(),
#endif
            this->kEnablePacketCount(),
            this->kEnableByteCount(),
            this->kCounterPackets(),
            this->kCounterBytes(),
        },
        0);
  }
};

TEST_F(AclTableStoreTest, loadAclTables) {
  auto aclTableId = createAclTable(SAI_ACL_STAGE_INGRESS);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiAclTableTraits>();

  SaiAclTableTraits::AdapterHostKey k{
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE()};

  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), aclTableId);
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

  SaiAclEntryTraits::AdapterHostKey k{aclTableId, this->kPriority()};
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), aclEntryId);
}
TEST_P(AclTableStoreParamTest, loadAclCounter) {
  auto aclTableId = createAclTable(GetParam());
  auto aclCounterId = createAclCounter(aclTableId);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiAclCounterTraits>();

  SaiAclCounterTraits::AdapterHostKey k{
      aclTableId,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      this->kCounterLabel(),
#endif
      this->kEnablePacketCount(),
      this->kEnableByteCount()};

  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), aclCounterId);
}

TEST_P(AclTableStoreParamTest, aclTableCtorLoad) {
  auto aclTableId = createAclTable(GetParam());
  auto obj = createObj<SaiAclTableTraits>(
      aclTableId, cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());
  EXPECT_EQ(obj.adapterKey(), aclTableId);
}

TEST_P(AclTableStoreParamTest, aclEntryLoadCtor) {
  auto aclTableId = createAclTable(GetParam());
  auto aclEntryId = createAclEntry(aclTableId);

  SaiObject<SaiAclEntryTraits> obj = createObj<SaiAclEntryTraits>(aclEntryId);
  EXPECT_EQ(obj.adapterKey(), aclEntryId);
}

TEST_P(AclTableStoreParamTest, aclCounterLoadCtor) {
  auto aclTableId = createAclTable(GetParam());
  auto aclCounterId = createAclCounter(aclTableId);

  SaiObject<SaiAclCounterTraits> obj =
      createObj<SaiAclCounterTraits>(aclCounterId);
  EXPECT_EQ(obj.adapterKey(), aclCounterId);
}

TEST_P(AclTableStoreParamTest, aclTableCtorCreate) {
  SaiAclTableTraits::CreateAttributes c{
      GetParam(),
      this->kBindPointTypeList(),
      this->kActionTypeList(),
      true, // srcIpv6
      true, // dstIpv6
      true, // srcIpv4
      true, // dstIpv4
      true, // l4SrcPort
      true, // l4DstPort
      true, // ipProtocol
      true, // tcpFlags
      true, // srcPort
      true, // outPort
      true, // ipFrag
      true, // icmpv4Type
      true, // icmpv4Code
      true, // icmpv6Type
      true, // icmpv6Code
      true, // dscp
      true, // dstMac
      true, // ipType
      true, // ttl
      true, // fdb meta
      true, // route meta
      true, // neighbor meta
      true, // ethertype
      true, // outer vlan id
      true, // bth opcode
      true, // ipv6 next header
      kUdfGroupId(), // udf group 0
      kUdfGroupId() + 1, // udf group 1
      kUdfGroupId() + 2, // udf group 2
      kUdfGroupId() + 3, // udf group 3
      kUdfGroupId() + 4, // udf group 4
  };

  SaiAclTableTraits::AdapterHostKey k{
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE()};

  SaiObject<SaiAclTableTraits> obj = createObj<SaiAclTableTraits>(k, c, 0);
  EXPECT_EQ(GET_ATTR(AclTable, Stage, obj.attributes()), GetParam());
}

TEST_P(AclTableStoreParamTest, AclEntryCreateCtor) {
  auto aclTableId = createAclTable(GetParam());

  SaiAclEntryTraits::AdapterHostKey k{aclTableId, this->kPriority()};

  SaiAclEntryTraits::CreateAttributes c{
      aclTableId,
      this->kPriority(),
      true, // enabled
      this->kSrcIpV6(),
      this->kDstIpV6(),
      this->kSrcIpV4(),
      this->kDstIpV4(),
      this->kSrcPort(),
      this->kOutPort(),
      this->kL4SrcPort(),
      this->kL4DstPort(),
      this->kIpProtocol(),
      this->kTcpFlags(),
      this->kIpFrag(),
      this->kIcmpV4Type(),
      this->kIcmpV4Code(),
      this->kIcmpV6Type(),
      this->kIcmpV6Code(),
      this->kDscp(),
      this->kDstMac(),
      this->kIpType(),
      this->kTtl(),
      this->kFdbDstUserMeta(),
      this->kRouteDstUserMeta(),
      this->kNeighborDstUserMeta(),
      this->kEtherType(),
      this->kOuterVlanId(),
      this->kBthOpcode(),
      this->kIpv6NextHeader(),
      this->kUdfGroupData(),
      this->kUdfGroupData(),
      this->kUdfGroupData(),
      this->kUdfGroupData(),
      this->kUdfGroupData(),
      this->kPacketAction(),
      this->kCounter(),
      this->kSetTC(),
      this->kSetDSCP(),
      this->kMirrorIngress(),
      this->kMirrorEgress(),
      this->kMacsecFlow(),
      this->kSetUserTrap(),
      this->kDisableArsForwarding()};

  SaiObject<SaiAclEntryTraits> obj = createObj<SaiAclEntryTraits>(k, c, 0);
  EXPECT_EQ(GET_ATTR(AclEntry, TableId, obj.attributes()), aclTableId);
}

TEST_P(AclTableStoreParamTest, AclCounterCreateCtor) {
  auto aclTableId = createAclTable(GetParam());

  SaiAclCounterTraits::CreateAttributes c{
      aclTableId,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      this->kCounterLabel(),
#endif
      this->kEnablePacketCount(),
      this->kEnableByteCount(),
      this->kCounterPackets(),
      this->kCounterBytes()};

  SaiAclCounterTraits::AdapterHostKey k{
      aclTableId,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      this->kCounterLabel(),
#endif
      this->kEnablePacketCount(),
      this->kEnableByteCount()};

  SaiObject<SaiAclCounterTraits> obj = createObj<SaiAclCounterTraits>(k, c, 0);
  EXPECT_EQ(GET_ATTR(AclCounter, TableId, obj.attributes()), aclTableId);
}

TEST_P(AclTableStoreParamTest, serDeserAclTableStore) {
  auto aclTableId = createAclTable(GetParam());
  verifyAdapterKeySerDeser<SaiAclTableTraits>({aclTableId});
}

TEST_P(AclTableStoreParamTest, toStrAclTableStore) {
  std::ignore = createAclTable(GetParam());
  verifyToStr<SaiAclTableTraits>();
}

TEST_P(AclTableStoreParamTest, serDeserAclEntryStore) {
  auto aclTableId = createAclTable(GetParam());
  auto aclEntryId = createAclEntry(aclTableId);
  verifyAdapterKeySerDeser<SaiAclEntryTraits>({aclEntryId});
}

TEST_P(AclTableStoreParamTest, toStrAclEntryStore) {
  auto aclTableId = createAclTable(GetParam());
  std::ignore = createAclEntry(aclTableId);
  verifyToStr<SaiAclEntryTraits>();
}

TEST_P(AclTableStoreParamTest, serDeserAclCounterStore) {
  auto aclTableId = createAclTable(GetParam());
  auto aclCounterId = createAclCounter(aclTableId);
  verifyAdapterKeySerDeser<SaiAclCounterTraits>({aclCounterId});
}

TEST_P(AclTableStoreParamTest, toStrAclCounterStore) {
  auto aclTableId = createAclTable(GetParam());
  std::ignore = createAclCounter(aclTableId);
  verifyToStr<SaiAclCounterTraits>();
}

INSTANTIATE_TEST_CASE_P(
    AclTableStoreParamTestInstantiation,
    AclTableStoreParamTest,
    testing::Values(SAI_ACL_STAGE_INGRESS, SAI_ACL_STAGE_EGRESS));
