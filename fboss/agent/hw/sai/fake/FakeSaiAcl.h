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

#include <folly/IPAddressV6.h>

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

  bool fieldSrcIpV6Enable;
  folly::IPAddressV6 fieldSrcIpV6Data;
  folly::IPAddressV6 fieldSrcIpV6Mask;

  bool fieldDstIpV6Enable;
  folly::IPAddressV6 fieldDstIpV6Data;
  folly::IPAddressV6 fieldDstIpV6Mask;

  bool fieldL4SrcPortEnable;
  sai_uint16_t fieldL4SrcPortData;
  sai_uint16_t fieldL4SrcPortMask;

  bool fieldL4DstPortEnable;
  sai_uint16_t fieldL4DstPortData;
  sai_uint16_t fieldL4DstPortMask;

  bool fieldIpProtocolEnable;
  sai_uint8_t fieldIpProtocolData;
  sai_uint8_t fieldIpProtocolMask;

  bool fieldTcpFlagsEnable;
  sai_uint8_t fieldTcpFlagsData;
  sai_uint8_t fieldTcpFlagsMask;

  bool fieldDscpEnable;
  sai_uint8_t fieldDscpData;
  sai_uint8_t fieldDscpMask;

  bool fieldTtlEnable;
  sai_uint8_t fieldTtlData;
  sai_uint8_t fieldTtlMask;

  bool fieldFdbDstUserMetaEnable;
  sai_uint32_t fieldFdbDstUserMetaData;
  sai_uint32_t fieldFdbDstUserMetaMask;

  bool fieldRouteDstUserMetaEnable;
  sai_uint32_t fieldRouteDstUserMetaData;
  sai_uint32_t fieldRouteDstUserMetaMask;

  bool fieldNeighborDstUserMetaEnable;
  sai_uint32_t fieldNeighborDstUserMetaData;
  sai_uint32_t fieldNeighborDstUserMetaMask;

  sai_object_id_t id;
};

class FakeAclTable {
 public:
  FakeAclTable(
      sai_int32_t stage,
      std::vector<sai_int32_t> bindPointTypeList,
      std::vector<sai_int32_t> actionTypeList,
      sai_uint8_t fieldSrcIpV6,
      sai_uint8_t fieldDstIpV6,
      sai_uint8_t fieldL4SrcPort,
      sai_uint8_t fieldL4DstPort,
      sai_uint8_t fieldIpProtocol,
      sai_uint8_t fieldTcpFlags,
      sai_uint8_t fieldInPort,
      sai_uint8_t fieldOutPort,
      sai_uint8_t fieldIpFrag,
      sai_uint8_t fieldDscp,
      sai_uint8_t fieldDstMac,
      sai_uint8_t fieldIpType,
      sai_uint8_t fieldTtl,
      sai_uint8_t fieldFdbDstUserMeta,
      sai_uint8_t fieldRouteDstUserMeta,
      sai_uint8_t fieldNeighborDstUserMeta)
      : stage(stage),
        bindPointTypeList(bindPointTypeList),
        actionTypeList(actionTypeList),
        fieldSrcIpV6(fieldSrcIpV6),
        fieldDstIpV6(fieldDstIpV6),
        fieldL4SrcPort(fieldL4SrcPort),
        fieldL4DstPort(fieldL4DstPort),
        fieldIpProtocol(fieldIpProtocol),
        fieldTcpFlags(fieldTcpFlags),
        fieldInPort(fieldInPort),
        fieldOutPort(fieldOutPort),
        fieldIpFrag(fieldIpFrag),
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
  sai_uint8_t fieldSrcIpV6;
  sai_uint8_t fieldDstIpV6;
  sai_uint8_t fieldL4SrcPort;
  sai_uint8_t fieldL4DstPort;
  sai_uint8_t fieldIpProtocol;
  sai_uint8_t fieldTcpFlags;
  sai_uint8_t fieldInPort;
  sai_uint8_t fieldOutPort;
  sai_uint8_t fieldIpFrag;
  sai_uint8_t fieldDscp;
  sai_uint8_t fieldDstMac;
  sai_uint8_t fieldIpType;
  sai_uint8_t fieldTtl;
  sai_uint8_t fieldFdbDstUserMeta;
  sai_uint8_t fieldRouteDstUserMeta;
  sai_uint8_t fieldNeighborDstUserMeta;

  FakeManager<sai_object_id_t, FakeAclEntry>& fm() {
    return fm_;
  }
  const FakeManager<sai_object_id_t, FakeAclEntry>& fm() const {
    return fm_;
  }

 private:
  FakeManager<sai_object_id_t, FakeAclEntry> fm_;
};

using FakeAclTableManager = FakeManagerWithMembers<FakeAclTable, FakeAclEntry>;

void populate_acl_api(sai_acl_api_t** acl_api);

} // namespace facebook::fboss
