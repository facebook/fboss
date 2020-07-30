/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>

extern "C" {
#include <sai.h>
}
namespace facebook::fboss {

class FakeAclTableGroupMember {
 public:
  explicit FakeAclTableGroupMember(
      sai_object_id_t tableGroupId,
      sai_object_id_t tableId,
      sai_uint32_t priority)
      : tableGroupId(tableGroupId), tableId(tableId), priority(priority) {}

  sai_object_id_t tableGroupId;
  sai_object_id_t tableId;
  sai_uint32_t priority;

  sai_object_id_t id;
};

class FakeAclTableGroup {
 public:
  explicit FakeAclTableGroup(
      sai_int32_t stage,
      std::vector<sai_int32_t> bindPointTypeList,
      sai_int32_t type)
      : stage(stage), bindPointTypeList(bindPointTypeList), type(type) {}

  sai_object_id_t id;

  sai_int32_t stage;
  std::vector<sai_int32_t> bindPointTypeList;
  sai_int32_t type;

  FakeManager<sai_object_id_t, FakeAclTableGroupMember>& fm() {
    return fm_;
  }
  const FakeManager<sai_object_id_t, FakeAclTableGroupMember>& fm() const {
    return fm_;
  }

 private:
  FakeManager<sai_object_id_t, FakeAclTableGroupMember> fm_;
};

using FakeAclTableGroupManager =
    FakeManagerWithMembers<FakeAclTableGroup, FakeAclTableGroupMember>;

class FakeAclEntry {
 public:
  explicit FakeAclEntry(sai_object_id_t tableId) : tableId(tableId) {}

  sai_object_id_t tableId;
  sai_uint32_t priority;

  bool fieldSrcIpV6Enable{false};
  folly::IPAddressV6 fieldSrcIpV6Data;
  folly::IPAddressV6 fieldSrcIpV6Mask;

  bool fieldDstIpV6Enable{false};
  folly::IPAddressV6 fieldDstIpV6Data;
  folly::IPAddressV6 fieldDstIpV6Mask;

  bool fieldSrcIpV4Enable{false};
  folly::IPAddressV4 fieldSrcIpV4Data;
  folly::IPAddressV4 fieldSrcIpV4Mask;

  bool fieldDstIpV4Enable{false};
  folly::IPAddressV4 fieldDstIpV4Data;
  folly::IPAddressV4 fieldDstIpV4Mask;

  /*
   * Mask is not needed for sai_object_id_t Acl Entry field.
   * Thus, there is no oid field in sai_acl_field_data_mask_t.
   * oid is u64, but unfortunately, u64 was not added to
   * sai_acl_field_data_mask_t till SAI 1.6, so use u32.
   * This will be ignored by the implementation anyway.
   */

  bool fieldSrcPortEnable{false};
  sai_object_id_t fieldSrcPortData;
  sai_uint32_t fieldSrcPortMask;

  bool fieldOutPortEnable{false};
  sai_object_id_t fieldOutPortData;
  sai_uint32_t fieldOutPortMask;

  bool fieldL4SrcPortEnable{false};
  sai_uint16_t fieldL4SrcPortData;
  sai_uint16_t fieldL4SrcPortMask;

  bool fieldL4DstPortEnable{false};
  sai_uint16_t fieldL4DstPortData;
  sai_uint16_t fieldL4DstPortMask;

  bool fieldIpProtocolEnable{false};
  sai_uint8_t fieldIpProtocolData;
  sai_uint8_t fieldIpProtocolMask;

  bool fieldTcpFlagsEnable{false};
  sai_uint8_t fieldTcpFlagsData;
  sai_uint8_t fieldTcpFlagsMask;

  bool fieldIpFragEnable{false};
  sai_uint8_t fieldIpFragData;
  sai_uint8_t fieldIpFragMask;

  bool fieldIcmpV4TypeEnable{false};
  sai_uint8_t fieldIcmpV4TypeData;
  sai_uint8_t fieldIcmpV4TypeMask;

  bool fieldIcmpV4CodeEnable{false};
  sai_uint8_t fieldIcmpV4CodeData;
  sai_uint8_t fieldIcmpV4CodeMask;

  bool fieldIcmpV6TypeEnable{false};
  sai_uint8_t fieldIcmpV6TypeData;
  sai_uint8_t fieldIcmpV6TypeMask;

  bool fieldIcmpV6CodeEnable{false};
  sai_uint8_t fieldIcmpV6CodeData;
  sai_uint8_t fieldIcmpV6CodeMask;

  bool fieldDscpEnable{false};
  sai_uint8_t fieldDscpData;
  sai_uint8_t fieldDscpMask;

  bool fieldDstMacEnable{false};
  folly::MacAddress fieldDstMacData;
  folly::MacAddress fieldDstMacMask;

  bool fieldIpTypeEnable{false};
  sai_uint8_t fieldIpTypeData;
  sai_uint8_t fieldIpTypeMask;

  bool fieldTtlEnable{false};
  sai_uint8_t fieldTtlData;
  sai_uint8_t fieldTtlMask;

  bool fieldFdbDstUserMetaEnable{false};
  sai_uint32_t fieldFdbDstUserMetaData;
  sai_uint32_t fieldFdbDstUserMetaMask;

  bool fieldRouteDstUserMetaEnable{false};
  sai_uint32_t fieldRouteDstUserMetaData;
  sai_uint32_t fieldRouteDstUserMetaMask;

  bool fieldNeighborDstUserMetaEnable{false};
  sai_uint32_t fieldNeighborDstUserMetaData;
  sai_uint32_t fieldNeighborDstUserMetaMask;

  bool actionPacketActionEnable{false};
  sai_uint32_t actionPacketActionData;

  bool actionCounterEnable{false};
  sai_object_id_t actionCounterData;

  bool actionSetTCEnable{false};
  sai_uint8_t actionSetTCData;

  bool actionSetDSCPEnable{false};
  sai_uint8_t actionSetDSCPData;

  bool actionMirrorIngressEnable{false};
  std::vector<sai_object_id_t> actionMirrorIngressData;

  bool actionMirrorEgressEnable{false};
  std::vector<sai_object_id_t> actionMirrorEgressData;

  sai_object_id_t id;
};

class FakeAclTable {
 public:
  FakeAclTable(
      sai_int32_t stage,
      std::vector<sai_int32_t> bindPointTypeList,
      std::vector<sai_int32_t> actionTypeList,
      bool fieldSrcIpV6,
      bool fieldDstIpV6,
      bool fieldSrcIpV4,
      bool fieldDstIpV4,
      bool fieldL4SrcPort,
      bool fieldL4DstPort,
      bool fieldIpProtocol,
      bool fieldTcpFlags,
      bool fieldSrcPort,
      bool fieldOutPort,
      bool fieldIpFrag,
      bool fieldIcmpV4Type,
      bool fieldIcmpV4Code,
      bool fieldIcmpV6Type,
      bool fieldIcmpV6Code,
      bool fieldDscp,
      bool fieldDstMac,
      bool fieldIpType,
      bool fieldTtl,
      bool fieldFdbDstUserMeta,
      bool fieldRouteDstUserMeta,
      bool fieldNeighborDstUserMeta)
      : stage(stage),
        bindPointTypeList(bindPointTypeList),
        actionTypeList(actionTypeList),
        fieldSrcIpV6(fieldSrcIpV6),
        fieldDstIpV6(fieldDstIpV6),
        fieldSrcIpV4(fieldSrcIpV4),
        fieldDstIpV4(fieldDstIpV4),
        fieldL4SrcPort(fieldL4SrcPort),
        fieldL4DstPort(fieldL4DstPort),
        fieldIpProtocol(fieldIpProtocol),
        fieldTcpFlags(fieldTcpFlags),
        fieldSrcPort(fieldSrcPort),
        fieldOutPort(fieldOutPort),
        fieldIpFrag(fieldIpFrag),
        fieldIcmpV4Type(fieldIcmpV4Type),
        fieldIcmpV4Code(fieldIcmpV4Code),
        fieldIcmpV6Type(fieldIcmpV6Type),
        fieldIcmpV6Code(fieldIcmpV6Code),
        fieldDscp(fieldDscp),
        fieldDstMac(fieldDstMac),
        fieldIpType(fieldIpType),
        fieldTtl(fieldTtl),
        fieldFdbDstUserMeta(fieldFdbDstUserMeta),
        fieldRouteDstUserMeta(fieldRouteDstUserMeta),
        fieldNeighborDstUserMeta(fieldNeighborDstUserMeta) {}

  static sai_acl_api_t* kApi();

  sai_object_id_t id;

  sai_int32_t stage;
  std::vector<sai_int32_t> bindPointTypeList;
  std::vector<sai_int32_t> actionTypeList;
  bool fieldSrcIpV6;
  bool fieldDstIpV6;
  bool fieldSrcIpV4;
  bool fieldDstIpV4;
  bool fieldL4SrcPort;
  bool fieldL4DstPort;
  bool fieldIpProtocol;
  bool fieldTcpFlags;
  bool fieldSrcPort;
  bool fieldOutPort;
  bool fieldIpFrag;
  bool fieldIcmpV4Type;
  bool fieldIcmpV4Code;
  bool fieldIcmpV6Type;
  bool fieldIcmpV6Code;
  bool fieldDscp;
  bool fieldDstMac;
  bool fieldIpType;
  bool fieldTtl;
  bool fieldFdbDstUserMeta;
  bool fieldRouteDstUserMeta;
  bool fieldNeighborDstUserMeta;
};

class FakeAclCounter {
 public:
  explicit FakeAclCounter(sai_object_id_t tableId) : tableId(tableId) {}

  sai_object_id_t tableId;

  bool enablePacketCount{false};
  bool enableByteCount{false};

  sai_uint64_t counterPackets{0};
  sai_uint64_t counterBytes{0};

  sai_object_id_t id;
};

using FakeAclEntryManager = FakeManager<sai_object_id_t, FakeAclEntry>;
using FakeAclCounterManager = FakeManager<sai_object_id_t, FakeAclCounter>;
using FakeAclTableManager = FakeManager<sai_object_id_t, FakeAclTable>;

void populate_acl_api(sai_acl_api_t** acl_api);

} // namespace facebook::fboss
