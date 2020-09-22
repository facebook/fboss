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
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

class AclApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    aclApi = std::make_unique<AclApi>();
  }

  sai_object_id_t kSwitchID() const {
    return 0;
  }

  sai_uint32_t kPriority() const {
    return 1;
  }

  sai_uint32_t kPriority2() const {
    return 2;
  }

  std::pair<folly::IPAddressV6, folly::IPAddressV6> kSrcIpV6() const {
    return std::make_pair(
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"));
  }

  std::pair<folly::IPAddressV6, folly::IPAddressV6> kSrcIpV6_2() const {
    return std::make_pair(
        folly::IPAddressV6("2620:0:1cfe:face:c00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:c00c::3"));
  }

  std::pair<folly::IPAddressV6, folly::IPAddressV6> kDstIpV6() const {
    return std::make_pair(
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"));
  }

  std::pair<folly::IPAddressV6, folly::IPAddressV6> kDstIpV6_2() const {
    return std::make_pair(
        folly::IPAddressV6("2620:0:1cfe:face:c00c::4"),
        folly::IPAddressV6("2620:0:1cfe:face:c00c::4"));
  }

  std::pair<folly::IPAddressV4, folly::IPAddressV4> kSrcIpV4() const {
    return std::make_pair(
        folly::IPAddressV4("10.0.0.1"), folly::IPAddressV4("255.255.255.0"));
  }

  std::pair<folly::IPAddressV4, folly::IPAddressV4> kSrcIpV4_2() const {
    return std::make_pair(
        folly::IPAddressV4("10.0.0.2"), folly::IPAddressV4("255.255.255.0"));
  }

  std::pair<folly::IPAddressV4, folly::IPAddressV4> kDstIpV4() const {
    return std::make_pair(
        folly::IPAddressV4("20.0.0.1"), folly::IPAddressV4("255.255.255.0"));
  }

  std::pair<folly::IPAddressV4, folly::IPAddressV4> kDstIpV4_2() const {
    return std::make_pair(
        folly::IPAddressV4("20.0.0.2"), folly::IPAddressV4("255.255.255.0"));
  }

  std::pair<sai_object_id_t, sai_uint32_t> kSrcPort() const {
    return std::make_pair(41, 0);
  }

  std::pair<sai_object_id_t, sai_uint32_t> kSrcPort2() const {
    return std::make_pair(410, 0);
  }

  std::pair<sai_object_id_t, sai_uint32_t> kOutPort() const {
    return std::make_pair(42, 0);
  }

  std::pair<sai_object_id_t, sai_uint32_t> kOutPort2() const {
    return std::make_pair(420, 0);
  }

  std::pair<sai_uint16_t, sai_uint16_t> kL4SrcPort() const {
    return std::make_pair(9001, 0xFFFF);
  }

  std::pair<sai_uint16_t, sai_uint16_t> kL4SrcPort2() const {
    return std::make_pair(10001, 0xFFFF);
  }

  std::pair<sai_uint16_t, sai_uint16_t> kL4DstPort() const {
    return std::make_pair(9002, 0xFFFF);
  }

  std::pair<sai_uint16_t, sai_uint16_t> kL4DstPort2() const {
    return std::make_pair(10002, 0xFFFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIpProtocol() const {
    return std::make_pair(6, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIpProtocol2() const {
    return std::make_pair(17, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kTcpFlags() const {
    return std::make_pair(1, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kTcpFlags2() const {
    return std::make_pair(2, 0xFF);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kIpFrag() const {
    return std::make_pair(
        SAI_ACL_IP_FRAG_ANY, 0 /* mask is N/A for field ip frag */);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kIpFrag2() const {
    return std::make_pair(
        SAI_ACL_IP_FRAG_NON_FRAG, 0 /* mask is N/A for field ip frag */);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV4Type() const {
    return std::make_pair(3 /* Destination unreachable */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV4Type2() const {
    return std::make_pair(5 /* Redirect unreachable */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV4Code() const {
    return std::make_pair(1 /* Host unreachable */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV4Code2() const {
    return std::make_pair(1 /* Redirect Datagram for the host*/, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV6Type() const {
    return std::make_pair(1 /* Destination unreachable */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV6Type2() const {
    return std::make_pair(3 /* Time exceeded */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV6Code() const {
    return std::make_pair(4 /* Port unreachable */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kIcmpV6Code2() const {
    return std::make_pair(1 /* Fragment reassembly time exceeded */, 0xFF);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kDscp() const {
    return std::make_pair(10, 0x3F);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kDscp2() const {
    return std::make_pair(20, 0x3F);
  }

  std::pair<folly::MacAddress, folly::MacAddress> kDstMac() const {
    return std::make_pair(
        folly::MacAddress{"00:11:22:33:44:55"},
        folly::MacAddress{"ff:ff:ff:ff:ff:ff"});
  }

  std::pair<folly::MacAddress, folly::MacAddress> kDstMac2() const {
    return std::make_pair(
        folly::MacAddress{"00:11:22:33:44:66"},
        folly::MacAddress{"ff:ff:ff:ff:ff:ff"});
  }

  std::pair<sai_uint32_t, sai_uint32_t> kIpType() const {
    return std::make_pair(SAI_ACL_IP_TYPE_IPV4ANY, 0);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kIpType2() const {
    return std::make_pair(SAI_ACL_IP_TYPE_IPV6ANY, 0);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kTtl() const {
    return std::make_pair(128, 128);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kTtl2() const {
    return std::make_pair(64, 64);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kFdbDstUserMeta() const {
    return std::make_pair(11, 0xFFFFFFFF);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kFdbDstUserMeta2() const {
    return std::make_pair(12, 0xFFFFFFFF);
  }
  std::pair<sai_uint32_t, sai_uint32_t> kRouteDstUserMeta() const {
    return std::make_pair(11, 0xFFFFFFFF);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kRouteDstUserMeta2() const {
    return std::make_pair(12, 0xFFFFFFFF);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kNeighborDstUserMeta() const {
    return std::make_pair(11, 0xFFFFFFFF);
  }
  std::pair<sai_uint32_t, sai_uint32_t> kNeighborDstUserMeta2() const {
    return std::make_pair(12, 0xFFFFFFFF);
  }

  sai_uint32_t kPacketAction() const {
    return SAI_PACKET_ACTION_DROP;
  }

  sai_uint32_t kPacketAction2() const {
    return SAI_PACKET_ACTION_TRAP;
  }

  sai_uint32_t kPacketAction3() const {
    return SAI_PACKET_ACTION_COPY;
  }

  sai_object_id_t kCounter() const {
    return 42;
  }

  sai_object_id_t kCounter2() const {
    return 43;
  }

  sai_uint8_t kSetTC() const {
    return 1;
  }

  sai_uint8_t kSetTC2() const {
    return 2;
  }

  sai_uint8_t kSetDSCP() const {
    return 10;
  }

  sai_uint8_t kSetDSCP2() const {
    return 20;
  }

  const std::vector<sai_object_id_t>& kMirrorIngress() const {
    static const std::vector<sai_object_id_t> mirrorIngress{10, 11};

    return mirrorIngress;
  }

  const std::vector<sai_object_id_t>& kMirrorIngress2() const {
    static const std::vector<sai_object_id_t> mirrorIngress{20, 21};

    return mirrorIngress;
  }

  const std::vector<sai_object_id_t>& kMirrorEgress() const {
    static const std::vector<sai_object_id_t> mirrorEgress{110, 111};

    return mirrorEgress;
  }

  const std::vector<sai_object_id_t>& kMirrorEgress2() const {
    static const std::vector<sai_object_id_t> mirrorEgress{220, 221};

    return mirrorEgress;
  }

  const std::vector<sai_int32_t>& kActionTypeList() const {
    static const std::vector<sai_int32_t> actionTypeList = {
        SAI_ACL_ACTION_TYPE_PACKET_ACTION,
        SAI_ACL_ACTION_TYPE_COUNTER,
        SAI_ACL_ACTION_TYPE_MIRROR_INGRESS,
        SAI_ACL_ACTION_TYPE_MIRROR_EGRESS,
        SAI_ACL_ACTION_TYPE_SET_TC,
        SAI_ACL_ACTION_TYPE_SET_DSCP};

    return actionTypeList;
  }

  AclTableSaiId createAclTable() const {
    SaiAclTableTraits::Attributes::Stage aclTableStageAttribute{
        SAI_ACL_STAGE_INGRESS};
    std::vector<sai_int32_t> aclTableBindPointTypeListAttribute{
        SAI_ACL_BIND_POINT_TYPE_PORT};

    return aclApi->create<SaiAclTableTraits>(
        {
            aclTableStageAttribute,
            aclTableBindPointTypeListAttribute,
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
            true, // icmpV4Type
            true, // icmpV4Code
            true, // icmpV6Type
            true, // icmpV6Code
            true, // dscp
            true, // dstMac
            true, // ipType
            true, // ttl
            true, // fdb meta
            true, // route meta
            true // neighbor meta
        },
        kSwitchID());
  }

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

  AclEntrySaiId createAclEntry(AclTableSaiId aclTableId) const {
    SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
    SaiAclEntryTraits::Attributes::Priority aclPriorityAttribute{kPriority()};
    SaiAclEntryTraits::Attributes::FieldSrcIpV6 aclFieldSrcIpV6{
        AclEntryFieldIpV6(kSrcIpV6())};
    SaiAclEntryTraits::Attributes::FieldDstIpV6 aclFieldDstIpV6{
        AclEntryFieldIpV6(kDstIpV6())};
    SaiAclEntryTraits::Attributes::FieldSrcIpV4 aclFieldSrcIpV4{
        AclEntryFieldIpV4(kSrcIpV4())};
    SaiAclEntryTraits::Attributes::FieldDstIpV4 aclFieldDstIpV4{
        AclEntryFieldIpV4(kDstIpV4())};
    SaiAclEntryTraits::Attributes::FieldSrcPort aclFieldSrcPort{
        AclEntryFieldSaiObjectIdT(kSrcPort())};
    SaiAclEntryTraits::Attributes::FieldOutPort aclFieldOutPort{
        AclEntryFieldSaiObjectIdT(kOutPort())};
    SaiAclEntryTraits::Attributes::FieldL4SrcPort aclFieldL4SrcPortAttribute{
        AclEntryFieldU16(kL4SrcPort())};
    SaiAclEntryTraits::Attributes::FieldL4DstPort aclFieldL4DstPortAttribute{
        AclEntryFieldU16(kL4DstPort())};
    SaiAclEntryTraits::Attributes::FieldIpProtocol aclFieldIpProtocolAttribute{
        AclEntryFieldU8(kIpProtocol())};
    SaiAclEntryTraits::Attributes::FieldTcpFlags aclFieldTcpFlagsAttribute{
        AclEntryFieldU8(kTcpFlags())};
    SaiAclEntryTraits::Attributes::FieldIpFrag aclFieldIpFragAttribute{
        AclEntryFieldU32(kIpFrag())};
    SaiAclEntryTraits::Attributes::FieldIcmpV4Type aclFieldIcmpV4TypeAttribute{
        AclEntryFieldU8(kIcmpV4Type())};
    SaiAclEntryTraits::Attributes::FieldIcmpV4Code aclFieldIcmpV4CodeAttribute{
        AclEntryFieldU8(kIcmpV4Code())};
    SaiAclEntryTraits::Attributes::FieldIcmpV6Type aclFieldIcmpV6TypeAttribute{
        AclEntryFieldU8(kIcmpV6Type())};
    SaiAclEntryTraits::Attributes::FieldIcmpV6Code aclFieldIcmpV6CodeAttribute{
        AclEntryFieldU8(kIcmpV6Code())};
    SaiAclEntryTraits::Attributes::FieldDscp aclFieldDscpAttribute{
        AclEntryFieldU8(kDscp())};
    SaiAclEntryTraits::Attributes::FieldDstMac aclFieldDstMacAttribute{
        AclEntryFieldMac(kDstMac())};
    SaiAclEntryTraits::Attributes::FieldIpType aclFieldIpTypeAttribute{
        AclEntryFieldU32(kIpType())};
    SaiAclEntryTraits::Attributes::FieldTtl aclFieldTtlAttribute{
        AclEntryFieldU8(kTtl())};
    SaiAclEntryTraits::Attributes::FieldFdbDstUserMeta
        aclFieldFdbDstUserMetaAttribute{AclEntryFieldU32(kFdbDstUserMeta())};
    SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta
        aclFieldRouteDstUserMetaAttribute{
            AclEntryFieldU32(kRouteDstUserMeta())};
    SaiAclEntryTraits::Attributes::FieldNeighborDstUserMeta
        aclFieldNeighborDstUserMetaAttribute{
            AclEntryFieldU32(kNeighborDstUserMeta())};

    SaiAclEntryTraits::Attributes::ActionPacketAction aclActionPacketAction{
        AclEntryActionU32(kPacketAction())};
    SaiAclEntryTraits::Attributes::ActionCounter aclActionCounter{
        AclEntryActionSaiObjectIdT(kCounter())};
    SaiAclEntryTraits::Attributes::ActionSetTC aclActionSetTC{
        AclEntryActionU8(kSetTC())};
    SaiAclEntryTraits::Attributes::ActionSetDSCP aclActionSetDSCP{
        AclEntryActionU8(kSetDSCP())};

    SaiAclEntryTraits::Attributes::ActionMirrorIngress aclActionMirrorIngress{
        AclEntryActionSaiObjectIdList(kMirrorIngress())};
    SaiAclEntryTraits::Attributes::ActionMirrorEgress aclActionMirrorEgress{
        AclEntryActionSaiObjectIdList(kMirrorEgress())};

    return aclApi->create<SaiAclEntryTraits>(
        {aclTableIdAttribute,
         aclPriorityAttribute,
         aclFieldSrcIpV6,
         aclFieldDstIpV6,
         aclFieldSrcIpV4,
         aclFieldDstIpV4,
         aclFieldSrcPort,
         aclFieldOutPort,
         aclFieldL4SrcPortAttribute,
         aclFieldL4DstPortAttribute,
         aclFieldIpProtocolAttribute,
         aclFieldTcpFlagsAttribute,
         aclFieldIpFragAttribute,
         aclFieldIcmpV4TypeAttribute,
         aclFieldIcmpV4CodeAttribute,
         aclFieldIcmpV6TypeAttribute,
         aclFieldIcmpV6CodeAttribute,
         aclFieldDscpAttribute,
         aclFieldDstMacAttribute,
         aclFieldIpTypeAttribute,
         aclFieldTtlAttribute,
         aclFieldFdbDstUserMetaAttribute,
         aclFieldRouteDstUserMetaAttribute,
         aclFieldNeighborDstUserMetaAttribute,
         aclActionPacketAction,
         aclActionCounter,
         aclActionSetTC,
         aclActionSetDSCP,
         aclActionMirrorIngress,
         aclActionMirrorEgress},
        kSwitchID());
  }

  AclCounterSaiId createAclCounter(AclTableSaiId aclTableId) const {
    SaiAclCounterTraits::Attributes::TableId aclTableIdAttribute{aclTableId};

    SaiAclCounterTraits::Attributes::EnablePacketCount
        enablePacketCountAttribute{kEnablePacketCount()};
    SaiAclCounterTraits::Attributes::EnableByteCount enableByteCountAttribute{
        kEnableByteCount()};

    SaiAclCounterTraits::Attributes::CounterPackets counterPacketsAttribute{
        kCounterPackets()};
    SaiAclCounterTraits::Attributes::CounterBytes counterBytesAttribute{
        kCounterBytes()};

    return aclApi->create<SaiAclCounterTraits>(
        {aclTableIdAttribute,
         enablePacketCountAttribute,
         enableByteCountAttribute,
         counterPacketsAttribute,
         counterBytesAttribute},
        kSwitchID());
  }

  void checkAclTable(AclTableSaiId aclTableId) const {
    EXPECT_EQ(aclTableId, fs->aclTableManager.get(aclTableId).id);
  }

  void checkAclEntry(AclTableSaiId aclTableId, AclEntrySaiId aclEntryId) const {
    EXPECT_EQ(aclEntryId, fs->aclEntryManager.get(aclEntryId).id);
    EXPECT_EQ(aclTableId, fs->aclEntryManager.get(aclEntryId).tableId);
  }

  void checkAclCounter(AclTableSaiId aclTableId, AclCounterSaiId aclCounterId) {
    EXPECT_EQ(aclCounterId, fs->aclCounterManager.get(aclCounterId).id);
    EXPECT_EQ(aclTableId, fs->aclCounterManager.get(aclCounterId).tableId);
  }

  AclTableGroupSaiId createAclTableGroup(
      const std::vector<sai_int32_t>& aclTableGroupBindPointTypeListAttribute =
          std::vector<sai_int32_t>{SAI_ACL_BIND_POINT_TYPE_PORT}) const {
    SaiAclTableGroupTraits::Attributes::Stage aclTableGroupStageAttribute{
        SAI_ACL_STAGE_INGRESS};
    SaiAclTableGroupTraits::Attributes::Type aclTableGroupTypeAttribute{
        SAI_ACL_TABLE_GROUP_TYPE_SEQUENTIAL};

    return aclApi->create<SaiAclTableGroupTraits>(
        {aclTableGroupStageAttribute,
         aclTableGroupBindPointTypeListAttribute,
         aclTableGroupTypeAttribute},
        kSwitchID());
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

    return aclApi->create<SaiAclTableGroupMemberTraits>(
        {aclTableGroupTableGroupIdAttribute,
         aclTableGroupTableIdAttribute,
         aclTableGroupPriorityAttribute},
        kSwitchID());
  }

  void checkAclTableGroup(AclTableGroupSaiId aclTableGroupId) const {
    EXPECT_EQ(
        aclTableGroupId, fs->aclTableGroupManager.get(aclTableGroupId).id);
  }

  void checkAclTableGroupMember(
      AclTableGroupSaiId aclTableGroupId,
      AclTableGroupMemberSaiId aclTableGroupMemberId) {
    EXPECT_EQ(
        aclTableGroupMemberId,
        fs->aclTableGroupManager.getMember(aclTableGroupMemberId).id);
    EXPECT_EQ(
        aclTableGroupId,
        fs->aclTableGroupManager.getMember(aclTableGroupMemberId).tableGroupId);
  }

  void getAndVerifyAclEntryAttribute(
      AclEntrySaiId aclEntryId,
      sai_uint32_t priority,
      const std::pair<folly::IPAddressV6, folly::IPAddressV6>& srcIpV6,
      const std::pair<folly::IPAddressV6, folly::IPAddressV6>& dstIpV6,
      const std::pair<folly::IPAddressV4, folly::IPAddressV4>& srcIpV4,
      const std::pair<folly::IPAddressV4, folly::IPAddressV4>& dstIpV4,
      const std::pair<sai_object_id_t, sai_uint32_t>& srcPort,
      const std::pair<sai_object_id_t, sai_uint32_t>& outPort,
      const std::pair<sai_uint16_t, sai_uint16_t>& l4SrcPort,
      const std::pair<sai_uint16_t, sai_uint16_t>& L4DstPort,
      const std::pair<sai_uint8_t, sai_uint8_t>& ipProtocol,
      const std::pair<sai_uint8_t, sai_uint8_t>& tcpFlags,
      const std::pair<sai_uint32_t, sai_uint32_t>& ipFrag,
      const std::pair<sai_uint8_t, sai_uint8_t>& icmpV4Type,
      const std::pair<sai_uint8_t, sai_uint8_t>& icmpV4Code,
      const std::pair<sai_uint8_t, sai_uint8_t>& icmpV6Type,
      const std::pair<sai_uint8_t, sai_uint8_t>& icmpV6Code,
      const std::pair<sai_uint8_t, sai_uint8_t>& dscp,
      const std::pair<folly::MacAddress, folly::MacAddress>& dstMac,
      const std::pair<sai_uint32_t, sai_uint32_t>& ipType,
      const std::pair<sai_uint8_t, sai_uint8_t>& ttl,
      const std::pair<sai_uint32_t, sai_uint32_t>& fdbDstUserMeta,
      const std::pair<sai_uint32_t, sai_uint32_t>& routeDstUserMeta,
      const std::pair<sai_uint32_t, sai_uint32_t>& neighborDstUserMeta,
      sai_uint32_t packetAction,
      sai_object_id_t counter,
      sai_uint8_t setTC,
      sai_uint8_t setDSCP,
      const std::vector<sai_object_id_t>& mirrorIngress,
      const std::vector<sai_object_id_t>& mirrorEgress) const {
    auto aclPriorityGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::Priority());

    auto aclFieldSrcIpV6Got = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldSrcIpV6());
    auto aclFieldDstIpV6Got = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDstIpV6());
    auto aclFieldSrcIpV4Got = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldSrcIpV4());
    auto aclFieldDstIpV4Got = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDstIpV4());
    auto aclFieldSrcPortGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldSrcPort());
    auto aclFieldOutPortGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldOutPort());
    auto aclFieldL4SrcPortGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldL4SrcPort());
    auto aclFieldL4DstPortGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldL4DstPort());
    auto aclFieldIpProtocolGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIpProtocol());
    auto aclFieldTcpFlagsGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldTcpFlags());
    auto aclFieldIpFragGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIpFrag());
    auto aclFieldIcmpV4TypeGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV4Type());
    auto aclFieldIcmpV4CodeGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV4Code());
    auto aclFieldIcmpV6TypeGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV6Type());
    auto aclFieldIcmpV6CodeGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV6Code());
    auto aclFieldDscpGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDscp());
    auto aclFieldDstMacGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDstMac());
    auto aclFieldIpTypeGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIpType());
    auto aclFieldTtlGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldTtl());
    auto aclFieldFdbDstUserMetaGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldFdbDstUserMeta());
    auto aclFieldRouteDstUserMetaGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta());
    auto aclFieldNeighborDstUserMetaGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldNeighborDstUserMeta());

    auto aclActionPacketActionGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::ActionPacketAction());
    auto aclActionCounterGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::ActionCounter());
    auto aclActionSetTCGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::ActionSetTC());
    auto aclActionSetDSCPGot = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::ActionSetDSCP());
    auto aclActionMirrorIngress = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::ActionMirrorIngress());
    auto aclActionMirrorEgress = aclApi->getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::ActionMirrorEgress());

    EXPECT_EQ(aclPriorityGot, priority);

    EXPECT_EQ(aclFieldSrcIpV6Got.getDataAndMask(), srcIpV6);
    EXPECT_EQ(aclFieldDstIpV6Got.getDataAndMask(), dstIpV6);
    EXPECT_EQ(aclFieldSrcIpV4Got.getDataAndMask(), srcIpV4);
    EXPECT_EQ(aclFieldDstIpV4Got.getDataAndMask(), dstIpV4);
    EXPECT_EQ(aclFieldSrcPortGot.getDataAndMask(), srcPort);
    EXPECT_EQ(aclFieldOutPortGot.getDataAndMask(), outPort);
    EXPECT_EQ(aclFieldL4SrcPortGot.getDataAndMask(), l4SrcPort);
    EXPECT_EQ(aclFieldL4DstPortGot.getDataAndMask(), L4DstPort);
    EXPECT_EQ(aclFieldIpProtocolGot.getDataAndMask(), ipProtocol);
    EXPECT_EQ(aclFieldTcpFlagsGot.getDataAndMask(), tcpFlags);
    EXPECT_EQ(aclFieldIpFragGot.getDataAndMask(), ipFrag);
    EXPECT_EQ(aclFieldIcmpV4TypeGot.getDataAndMask(), icmpV4Type);
    EXPECT_EQ(aclFieldIcmpV4CodeGot.getDataAndMask(), icmpV4Code);
    EXPECT_EQ(aclFieldIcmpV6TypeGot.getDataAndMask(), icmpV6Type);
    EXPECT_EQ(aclFieldIcmpV6CodeGot.getDataAndMask(), icmpV6Code);
    EXPECT_EQ(aclFieldDscpGot.getDataAndMask(), dscp);
    EXPECT_EQ(aclFieldDstMacGot.getDataAndMask(), dstMac);
    EXPECT_EQ(aclFieldIpTypeGot.getDataAndMask(), ipType);
    EXPECT_EQ(aclFieldTtlGot.getDataAndMask(), ttl);
    EXPECT_EQ(aclFieldFdbDstUserMetaGot.getDataAndMask(), fdbDstUserMeta);
    EXPECT_EQ(aclFieldRouteDstUserMetaGot.getDataAndMask(), routeDstUserMeta);
    EXPECT_EQ(
        aclFieldNeighborDstUserMetaGot.getDataAndMask(), neighborDstUserMeta);

    EXPECT_EQ(aclActionPacketActionGot.getData(), packetAction);
    EXPECT_EQ(aclActionCounterGot.getData(), counter);
    EXPECT_EQ(aclActionSetTCGot.getData(), setTC);
    EXPECT_EQ(aclActionSetDSCPGot.getData(), setDSCP);
    EXPECT_EQ(aclActionMirrorIngress.getData(), mirrorIngress);
    EXPECT_EQ(aclActionMirrorEgress.getData(), mirrorEgress);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<AclApi> aclApi;
};

TEST_F(AclApiTest, createAclTable) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);
}

TEST_F(AclApiTest, removeAclTable) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);
  aclApi->remove(aclTableId);
}

TEST_F(AclApiTest, createAclEntry) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);
}

TEST_F(AclApiTest, mulipleAclTables) {
  auto aclTableId1 = createAclTable();
  auto aclTableId2 = createAclTable();
  checkAclTable(aclTableId1);
  checkAclTable(aclTableId2);
  EXPECT_NE(aclTableId1, aclTableId2);

  auto aclEntryId1 = createAclEntry(aclTableId1);
  auto aclEntryId2 = createAclEntry(aclTableId2);
  checkAclEntry(aclTableId1, aclEntryId1);
  checkAclEntry(aclTableId2, aclEntryId2);
}

TEST_F(AclApiTest, removeAclEntry) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);

  aclApi->remove(aclEntryId);
}

TEST_F(AclApiTest, multipleAclEntries) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId1 = createAclEntry(aclTableId);
  auto aclEntryId2 = createAclEntry(aclTableId);

  checkAclEntry(aclTableId, aclEntryId1);
  checkAclEntry(aclTableId, aclEntryId2);
  EXPECT_NE(aclEntryId1, aclEntryId2);
}

TEST_F(AclApiTest, createAclCounter) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclCounterId = createAclCounter(aclTableId);
  checkAclCounter(aclTableId, aclCounterId);
}

TEST_F(AclApiTest, getAclTableAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);

  auto aclTableStageGot =
      aclApi->getAttribute(aclTableId, SaiAclTableTraits::Attributes::Stage());
  auto aclTableBindPointTypeListGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::BindPointTypeList());
  auto aclTableActionTypeListGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::ActionTypeList());
  auto aclTableEntryListGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::EntryList());

  auto aclTableFieldSrcIpV6Got = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldSrcIpV6());
  auto aclTableFieldDstIpV6Got = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldDstIpV6());
  auto aclTableFieldSrcIpV4Got = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldSrcIpV4());
  auto aclTableFieldDstIpV4Got = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldDstIpV4());
  auto aclTableFieldL4SrcPortGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldL4SrcPort());
  auto aclTableFieldL4DstPortGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldL4DstPort());
  auto aclTableFieldIpProtocolGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldIpProtocol());
  auto aclTableFieldTcpFlagsGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldTcpFlags());
  auto aclTableFieldSrcPortGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldSrcPort());
  auto aclTableFieldOutPortGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldOutPort());
  auto aclTableFieldIpFragGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldIpFrag());
  auto aclTableFieldIcmpV4TypeGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldIcmpV4Type());
  auto aclTableFieldIcmpV4CodeGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldIcmpV4Code());
  auto aclTableFieldIcmpV6TypeGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldIcmpV6Type());
  auto aclTableFieldIcmpV6CodeGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldIcmpV6Code());
  auto aclTableFieldDscpGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldDscp());
  auto aclTableFieldDstMacGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldDstMac());
  auto aclTableFieldIpTypeGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldIpType());
  auto aclTableFieldTtlGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldTtl());
  auto aclTableFieldFdbDstUserMetaGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldFdbDstUserMeta());
  auto aclTableFieldRouteDstUserMetaGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldRouteDstUserMeta());
  auto aclTableFieldNeighborDstUserMetaGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldNeighborDstUserMeta());

  EXPECT_EQ(aclTableStageGot, SAI_ACL_STAGE_INGRESS);
  EXPECT_EQ(aclTableBindPointTypeListGot.size(), 1);
  EXPECT_EQ(aclTableBindPointTypeListGot[0], SAI_ACL_BIND_POINT_TYPE_PORT);
  EXPECT_TRUE(aclTableActionTypeListGot == kActionTypeList());

  EXPECT_EQ(aclTableEntryListGot.size(), 1);
  EXPECT_EQ(aclTableEntryListGot[0], static_cast<uint32_t>(aclEntryId));

  EXPECT_EQ(aclTableFieldSrcIpV6Got, true);
  EXPECT_EQ(aclTableFieldDstIpV6Got, true);
  EXPECT_EQ(aclTableFieldSrcIpV4Got, true);
  EXPECT_EQ(aclTableFieldDstIpV4Got, true);
  EXPECT_EQ(aclTableFieldL4SrcPortGot, true);
  EXPECT_EQ(aclTableFieldL4DstPortGot, true);
  EXPECT_EQ(aclTableFieldIpProtocolGot, true);
  EXPECT_EQ(aclTableFieldTcpFlagsGot, true);
  EXPECT_EQ(aclTableFieldSrcPortGot, true);
  EXPECT_EQ(aclTableFieldOutPortGot, true);
  EXPECT_EQ(aclTableFieldIpFragGot, true);
  EXPECT_EQ(aclTableFieldIcmpV4TypeGot, true);
  EXPECT_EQ(aclTableFieldIcmpV4CodeGot, true);
  EXPECT_EQ(aclTableFieldIcmpV6TypeGot, true);
  EXPECT_EQ(aclTableFieldIcmpV6CodeGot, true);
  EXPECT_EQ(aclTableFieldDscpGot, true);
  EXPECT_EQ(aclTableFieldDstMacGot, true);
  EXPECT_EQ(aclTableFieldIpTypeGot, true);
  EXPECT_EQ(aclTableFieldTtlGot, true);
  EXPECT_EQ(aclTableFieldFdbDstUserMetaGot, true);
  EXPECT_EQ(aclTableFieldRouteDstUserMetaGot, true);
  EXPECT_EQ(aclTableFieldNeighborDstUserMetaGot, true);
}

TEST_F(AclApiTest, getAclEntryAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);

  getAndVerifyAclEntryAttribute(
      aclEntryId,
      kPriority(),
      kSrcIpV6(),
      kDstIpV6(),
      kSrcIpV4(),
      kDstIpV4(),
      kSrcPort(),
      kOutPort(),
      kL4SrcPort(),
      kL4DstPort(),
      kIpProtocol(),
      kTcpFlags(),
      kIpFrag(),
      kIcmpV4Type(),
      kIcmpV4Code(),
      kIcmpV6Type(),
      kIcmpV6Code(),
      kDscp(),
      kDstMac(),
      kIpType(),
      kTtl(),
      kFdbDstUserMeta(),
      kRouteDstUserMeta(),
      kNeighborDstUserMeta(),
      kPacketAction(),
      kCounter(),
      kSetTC(),
      kSetDSCP(),
      kMirrorIngress(),
      kMirrorEgress());
}

TEST_F(AclApiTest, getAclCounterAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclCounterId = createAclCounter(aclTableId);
  checkAclCounter(aclTableId, aclCounterId);

  auto aclEnablePacketCount = aclApi->getAttribute(
      aclCounterId, SaiAclCounterTraits::Attributes::EnablePacketCount());
  auto aclEnableByteCount = aclApi->getAttribute(
      aclCounterId, SaiAclCounterTraits::Attributes::EnableByteCount());

  auto aclCounterPackets = aclApi->getAttribute(
      aclCounterId, SaiAclCounterTraits::Attributes::CounterPackets());
  auto aclCounterBytes = aclApi->getAttribute(
      aclCounterId, SaiAclCounterTraits::Attributes::CounterBytes());

  EXPECT_EQ(aclEnablePacketCount, kEnablePacketCount());
  EXPECT_EQ(aclEnableByteCount, kEnableByteCount());
  EXPECT_EQ(aclCounterPackets, kCounterPackets());
  EXPECT_EQ(aclCounterBytes, kCounterBytes());
}

TEST_F(AclApiTest, setAclTableAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);

  // SAI spec does not support setting any attribute for ACL table post
  // creation.
  SaiAclTableTraits::Attributes::Stage stage{SAI_ACL_STAGE_EGRESS};
  SaiAclTableTraits::Attributes::BindPointTypeList bindPointTypeList{};
  SaiAclTableTraits::Attributes::ActionTypeList actionTypeList{};
  SaiAclTableTraits::Attributes::EntryList entryList{};

  SaiAclTableTraits::Attributes::FieldSrcIpV6 fieldSrcIpv6{false};
  SaiAclTableTraits::Attributes::FieldDstIpV6 fieldDstIpV6{false};
  SaiAclTableTraits::Attributes::FieldSrcIpV4 fieldSrcIpv4{false};
  SaiAclTableTraits::Attributes::FieldDstIpV4 fieldDstIpV4{false};
  SaiAclTableTraits::Attributes::FieldL4SrcPort fieldL4SrcPort{false};
  SaiAclTableTraits::Attributes::FieldL4DstPort fieldL4DstPort{false};
  SaiAclTableTraits::Attributes::FieldIpProtocol fieldIpProtocol{false};
  SaiAclTableTraits::Attributes::FieldTcpFlags fieldTcpFlags{false};
  SaiAclTableTraits::Attributes::FieldSrcPort fieldSrcPort{false};
  SaiAclTableTraits::Attributes::FieldOutPort fieldOutPort{false};
  SaiAclTableTraits::Attributes::FieldIpFrag fieldIpFrag{false};
  SaiAclTableTraits::Attributes::FieldIcmpV4Type fieldIcmpV4Type{false};
  SaiAclTableTraits::Attributes::FieldIcmpV4Code fieldIcmpV4Code{false};
  SaiAclTableTraits::Attributes::FieldIcmpV6Type fieldIcmpV6Type{false};
  SaiAclTableTraits::Attributes::FieldIcmpV6Code fieldIcmpV6Code{false};
  SaiAclTableTraits::Attributes::FieldDscp fieldDscp{false};
  SaiAclTableTraits::Attributes::FieldDstMac fieldDstMac{false};
  SaiAclTableTraits::Attributes::FieldIpType fieldIpType{false};
  SaiAclTableTraits::Attributes::FieldTtl fieldTtl{false};
  SaiAclTableTraits::Attributes::FieldFdbDstUserMeta fieldFdbDstUserMeta{false};
  SaiAclTableTraits::Attributes::FieldRouteDstUserMeta fieldRouteDstUserMeta{
      false};
  SaiAclTableTraits::Attributes::FieldNeighborDstUserMeta
      fieldNeieghborDstUserMeta{false};

  EXPECT_THROW(aclApi->setAttribute(aclTableId, stage), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableId, bindPointTypeList), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, actionTypeList), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, entryList), SaiApiError);

  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldSrcIpv6), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldDstIpV6), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldSrcIpv4), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldDstIpV4), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldL4SrcPort), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldL4DstPort), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldIpProtocol), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldTcpFlags), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldSrcPort), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldOutPort), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldIpFrag), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldIcmpV4Type), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldIcmpV4Code), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldIcmpV6Type), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldIcmpV6Code), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldDscp), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldDstMac), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldIpType), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldTtl), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableId, fieldFdbDstUserMeta), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableId, fieldRouteDstUserMeta), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableId, fieldNeieghborDstUserMeta), SaiApiError);
}

TEST_F(AclApiTest, setAclEntryAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);

  // SAI spec does not support setting tableId for ACL entry post
  // creation.
  SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute2{2};
  EXPECT_THROW(
      aclApi->setAttribute(aclEntryId, aclTableIdAttribute2), SaiApiError);

  // SAI spec supports setting priority and fieldDscp for ACL entry post
  // creation.
  SaiAclEntryTraits::Attributes::Priority aclPriorityAttribute2{kPriority2()};

  SaiAclEntryTraits::Attributes::FieldSrcIpV6 aclFieldSrcIpV6Attribute2{
      AclEntryFieldIpV6(kSrcIpV6_2())};
  SaiAclEntryTraits::Attributes::FieldDstIpV6 aclFieldDstIpV6Attribute2{
      AclEntryFieldIpV6(kDstIpV6_2())};
  SaiAclEntryTraits::Attributes::FieldSrcIpV4 aclFieldSrcIpV4Attribute2{
      AclEntryFieldIpV4(kSrcIpV4_2())};
  SaiAclEntryTraits::Attributes::FieldDstIpV4 aclFieldDstIpV4Attribute2{
      AclEntryFieldIpV4(kDstIpV4_2())};
  SaiAclEntryTraits::Attributes::FieldSrcPort aclFieldSrcPortAttribute2{
      AclEntryFieldSaiObjectIdT(kSrcPort2())};
  SaiAclEntryTraits::Attributes::FieldOutPort aclFieldOutPortAttribute2{
      AclEntryFieldSaiObjectIdT(kOutPort2())};
  SaiAclEntryTraits::Attributes::FieldL4SrcPort aclFieldL4SrcPortAttribute2{
      AclEntryFieldU16(kL4SrcPort2())};
  SaiAclEntryTraits::Attributes::FieldL4DstPort aclFieldL4DstPortAttribute2{
      AclEntryFieldU16(kL4DstPort2())};
  SaiAclEntryTraits::Attributes::FieldIpProtocol aclFieldIpProtocolAttribute2{
      AclEntryFieldU8(kIpProtocol2())};
  SaiAclEntryTraits::Attributes::FieldTcpFlags aclFieldTcpFlagsAttribute2{
      AclEntryFieldU8(kTcpFlags2())};
  SaiAclEntryTraits::Attributes::FieldIpFrag aclFieldIpFragAttribute2{
      AclEntryFieldU32(kIpFrag2())};
  SaiAclEntryTraits::Attributes::FieldIcmpV4Type aclFieldIcmpV4TypeAttribute2{
      AclEntryFieldU8(kIcmpV4Type2())};
  SaiAclEntryTraits::Attributes::FieldIcmpV4Code aclFieldIcmpV4CodeAttribute2{
      AclEntryFieldU8(kIcmpV4Code2())};
  SaiAclEntryTraits::Attributes::FieldIcmpV6Type aclFieldIcmpV6TypeAttribute2{
      AclEntryFieldU8(kIcmpV6Type2())};
  SaiAclEntryTraits::Attributes::FieldIcmpV6Code aclFieldIcmpV6CodeAttribute2{
      AclEntryFieldU8(kIcmpV6Code2())};
  SaiAclEntryTraits::Attributes::FieldDscp aclFieldDscpAttribute2{
      AclEntryFieldU8(kDscp2())};
  SaiAclEntryTraits::Attributes::FieldDstMac aclFieldDstMacAttribute2{
      AclEntryFieldMac(kDstMac2())};
  SaiAclEntryTraits::Attributes::FieldIpType aclFieldIpTypeAttribute2{
      AclEntryFieldU32(kIpType2())};
  SaiAclEntryTraits::Attributes::FieldTtl aclFieldTtlAttribute2{
      AclEntryFieldU8(kTtl2())};
  SaiAclEntryTraits::Attributes::FieldFdbDstUserMeta
      aclFieldFdbDstUserMetaAttribute2{AclEntryFieldU32(kFdbDstUserMeta2())};
  SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta
      aclFieldRouteDstUserMetaAttribute2{
          AclEntryFieldU32(kRouteDstUserMeta2())};
  SaiAclEntryTraits::Attributes::FieldNeighborDstUserMeta
      aclFieldNeighborDstUserMetaAttribute2{
          AclEntryFieldU32(kNeighborDstUserMeta2())};

  SaiAclEntryTraits::Attributes::ActionPacketAction aclActionPacketAction2{
      AclEntryActionU32(kPacketAction2())};
  SaiAclEntryTraits::Attributes::ActionCounter aclActionCounter2{
      AclEntryActionSaiObjectIdT(kCounter2())};
  SaiAclEntryTraits::Attributes::ActionSetTC aclActionSetTC2{
      AclEntryActionU8(kSetTC2())};
  SaiAclEntryTraits::Attributes::ActionSetDSCP aclActionSetDSCP2{
      AclEntryActionU8(kSetDSCP2())};
  SaiAclEntryTraits::Attributes::ActionMirrorIngress aclActionMirrorIngress2{
      AclEntryActionSaiObjectIdList(kMirrorIngress2())};
  SaiAclEntryTraits::Attributes::ActionMirrorEgress aclActionMirrorEgress2{
      AclEntryActionSaiObjectIdList(kMirrorEgress2())};

  aclApi->setAttribute(aclEntryId, aclPriorityAttribute2);

  aclApi->setAttribute(aclEntryId, aclFieldSrcIpV6Attribute2);
  aclApi->setAttribute(aclEntryId, aclFieldDstIpV6Attribute2);
  aclApi->setAttribute(aclEntryId, aclFieldSrcIpV4Attribute2);
  aclApi->setAttribute(aclEntryId, aclFieldDstIpV4Attribute2);
  aclApi->setAttribute(aclEntryId, aclFieldSrcPortAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldOutPortAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldL4SrcPortAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldL4DstPortAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldIpProtocolAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldTcpFlagsAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldIpFragAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldIcmpV4TypeAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldIcmpV4CodeAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldIcmpV6TypeAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldIcmpV6CodeAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldDscpAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldDstMacAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldIpTypeAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldTtlAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldFdbDstUserMetaAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldRouteDstUserMetaAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldNeighborDstUserMetaAttribute2);
  aclApi->setAttribute(aclEntryId, aclActionPacketAction2);
  aclApi->setAttribute(aclEntryId, aclActionCounter2);
  aclApi->setAttribute(aclEntryId, aclActionSetTC2);
  aclApi->setAttribute(aclEntryId, aclActionSetDSCP2);
  aclApi->setAttribute(aclEntryId, aclActionMirrorIngress2);
  aclApi->setAttribute(aclEntryId, aclActionMirrorEgress2);

  getAndVerifyAclEntryAttribute(
      aclEntryId,
      kPriority2(),
      kSrcIpV6_2(),
      kDstIpV6_2(),
      kSrcIpV4_2(),
      kDstIpV4_2(),
      kSrcPort2(),
      kOutPort2(),
      kL4SrcPort2(),
      kL4DstPort2(),
      kIpProtocol2(),
      kTcpFlags2(),
      kIpFrag2(),
      kIcmpV4Type2(),
      kIcmpV4Code2(),
      kIcmpV6Type2(),
      kIcmpV6Code2(),
      kDscp2(),
      kDstMac2(),
      kIpType2(),
      kTtl2(),
      kFdbDstUserMeta2(),
      kRouteDstUserMeta2(),
      kNeighborDstUserMeta2(),
      kPacketAction2(),
      kCounter2(),
      kSetTC2(),
      kSetDSCP2(),
      kMirrorIngress2(),
      kMirrorEgress2());

  SaiAclEntryTraits::Attributes::ActionPacketAction aclActionPacketAction3{
      AclEntryActionU32(kPacketAction3())};
  aclApi->setAttribute(aclEntryId, aclActionPacketAction3);
  auto aclActionPacketActionGot = aclApi->getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::ActionPacketAction());
  EXPECT_EQ(aclActionPacketActionGot.getData(), kPacketAction3());
}

TEST_F(AclApiTest, getAvailableEntries) {
  auto aclTableId = createAclTable();
  EXPECT_GT(
      aclApi->getAttribute(
          aclTableId, SaiAclTableTraits::Attributes::AvailableEntry{}),
      0);
}

TEST_F(AclApiTest, getAvailableCounters) {
  auto aclTableId = createAclTable();
  EXPECT_NE(
      aclApi->getAttribute(
          aclTableId, SaiAclTableTraits::Attributes::AvailableCounter{}),
      0);
}

TEST_F(AclApiTest, setAclCounterAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclCounterId = createAclCounter(aclTableId);
  checkAclCounter(aclTableId, aclCounterId);

  SaiAclCounterTraits::Attributes::TableId aclTableIdAttribute{2};
  EXPECT_THROW(
      aclApi->setAttribute(aclCounterId, aclTableIdAttribute), SaiApiError);

  SaiAclCounterTraits::Attributes::EnablePacketCount aclEnablePacketCount{
      false};
  EXPECT_THROW(
      aclApi->setAttribute(aclCounterId, aclEnablePacketCount), SaiApiError);
  SaiAclCounterTraits::Attributes::EnableByteCount aclEnableByteCount{false};
  EXPECT_THROW(
      aclApi->setAttribute(aclCounterId, aclEnableByteCount), SaiApiError);

  SaiAclCounterTraits::Attributes::CounterPackets aclCounterPackets{10};
  SaiAclCounterTraits::Attributes::CounterBytes aclCounterBytes{20};

  aclApi->setAttribute(aclCounterId, aclCounterPackets);
  aclApi->setAttribute(aclCounterId, aclCounterBytes);

  auto aclCounterPacketsGot = aclApi->getAttribute(
      aclCounterId, SaiAclCounterTraits::Attributes::CounterPackets());
  auto aclCounterBytesGot = aclApi->getAttribute(
      aclCounterId, SaiAclCounterTraits::Attributes::CounterBytes());

  EXPECT_EQ(aclCounterPacketsGot, 10);
  EXPECT_EQ(aclCounterBytesGot, 20);
}

TEST_F(AclApiTest, formatAclTableStageAttribute) {
  SaiAclTableTraits::Attributes::Stage stage{0};
  std::string expected("Stage: 0");
  EXPECT_EQ(expected, fmt::format("{}", stage));
}

TEST_F(AclApiTest, formatAclEntryAttribute) {
  SaiAclEntryTraits::Attributes::TableId tableId{0};
  std::string expected("TableId: 0");
  EXPECT_EQ(expected, fmt::format("{}", tableId));
}

TEST_F(AclApiTest, formatAclCounterAttribute) {
  SaiAclCounterTraits::Attributes::TableId tableId{0};
  std::string expected("TableId: 0");
  EXPECT_EQ(expected, fmt::format("{}", tableId));
}

TEST_F(AclApiTest, createAclTableGroup) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
}

TEST_F(AclApiTest, removeAclTableGroup) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  aclApi->remove(aclTableGroupId);
}

TEST_F(AclApiTest, createAclTableGroupMember) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId);
}

TEST_F(AclApiTest, removeAclTableGroupMember) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId);

  aclApi->remove(aclTableGroupMemberId);
}

TEST_F(AclApiTest, multipleAclTableGroupMember) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);

  auto aclTableId1 = createAclTable();
  checkAclTable(aclTableId1);
  auto aclTableId2 = createAclTable();
  checkAclTable(aclTableId2);

  auto aclTableGroupMemberId1 =
      createAclTableGroupMember(aclTableGroupId, aclTableId1);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId1);
  auto aclTableGroupMemberId2 =
      createAclTableGroupMember(aclTableGroupId, aclTableId1);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId2);
}

TEST_F(AclApiTest, getAclTableGroupAttribute) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId);

  auto aclTableGroupStageGot = aclApi->getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::Stage());
  auto aclTableGroupBindPointTypeListGot = aclApi->getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::BindPointTypeList());
  auto aclTableGroupTypeGot = aclApi->getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::Type());
  auto aclTableGroupMembersGot = aclApi->getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::MemberList());

  EXPECT_EQ(aclTableGroupStageGot, SAI_ACL_STAGE_INGRESS);
  EXPECT_EQ(aclTableGroupBindPointTypeListGot.size(), 1);
  EXPECT_EQ(aclTableGroupBindPointTypeListGot[0], SAI_ACL_BIND_POINT_TYPE_PORT);
  EXPECT_EQ(aclTableGroupTypeGot, SAI_ACL_TABLE_GROUP_TYPE_SEQUENTIAL);
  EXPECT_EQ(aclTableGroupMembersGot.size(), 1);
  EXPECT_EQ(aclTableGroupMembersGot[0], aclTableGroupMemberId);
}

TEST_F(AclApiTest, getAclTableGroupMemberAttribute) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId);

  auto aclTableGroupIdGot = aclApi->getAttribute(
      aclTableGroupMemberId,
      SaiAclTableGroupMemberTraits::Attributes::TableGroupId());
  auto aclTableIdGot = aclApi->getAttribute(
      aclTableGroupMemberId,
      SaiAclTableGroupMemberTraits::Attributes::TableId());
  auto aclPriorityGot = aclApi->getAttribute(
      aclTableGroupMemberId,
      SaiAclTableGroupMemberTraits::Attributes::Priority());

  EXPECT_EQ(aclTableGroupIdGot, aclTableGroupId);
  EXPECT_EQ(aclTableIdGot, aclTableId);
  EXPECT_EQ(aclPriorityGot, 1);
}

TEST_F(AclApiTest, setAclTableGroupAttribute) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);

  // SAI spec does not support setting any attribute for ACL table group post
  // creation.
  SaiAclTableGroupTraits::Attributes::Stage stage{SAI_ACL_STAGE_EGRESS};
  SaiAclTableGroupTraits::Attributes::BindPointTypeList bindPointTypeList{};
  SaiAclTableGroupTraits::Attributes::Type type{
      SAI_ACL_TABLE_GROUP_TYPE_PARALLEL};
  SaiAclTableGroupTraits::Attributes::MemberList memberList{};

  EXPECT_THROW(aclApi->setAttribute(aclTableGroupId, stage), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableGroupId, bindPointTypeList), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableGroupId, type), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableGroupId, memberList), SaiApiError);
}

TEST_F(AclApiTest, setAclTableGroupMemberAttribute) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);
  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId);

  // SAI spec does not support setting any attribute for ACL table group member
  // post creation.
  SaiAclTableGroupMemberTraits::Attributes::TableGroupId tableGroupId{1};
  SaiAclTableGroupMemberTraits::Attributes::TableId tableId{1};
  SaiAclTableGroupMemberTraits::Attributes::Priority priority{kPriority()};

  EXPECT_THROW(
      aclApi->setAttribute(aclTableGroupMemberId, tableGroupId), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableGroupMemberId, tableId), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableGroupMemberId, priority), SaiApiError);
}

TEST_F(AclApiTest, formatAclTableGroupStageAttribute) {
  SaiAclTableGroupTraits::Attributes::Stage stage{0};
  std::string expected("Stage: 0");
  EXPECT_EQ(expected, fmt::format("{}", stage));
}

TEST_F(AclApiTest, formatAclTableGroupMemberAttribute) {
  SaiAclTableGroupMemberTraits::Attributes::TableGroupId tableGroupId{0};
  std::string expected("TableGroupId: 0");
  EXPECT_EQ(expected, fmt::format("{}", tableGroupId));
}
