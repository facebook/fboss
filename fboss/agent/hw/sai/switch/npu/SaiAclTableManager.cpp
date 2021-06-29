/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiMirrorManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiAclTableManager::SaiAclTableManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore),
      managerTable_(managerTable),
      platform_(platform),
      aclEntryMinimumPriority_(
          SaiApiTable::getInstance()->switchApi().getAttribute(
              managerTable_->switchManager().getSwitchSaiId(),
              SaiSwitchTraits::Attributes::AclEntryMinimumPriority())),
      aclEntryMaximumPriority_(
          SaiApiTable::getInstance()->switchApi().getAttribute(
              managerTable_->switchManager().getSwitchSaiId(),
              SaiSwitchTraits::Attributes::AclEntryMaximumPriority())),
      fdbDstUserMetaDataRangeMin_(getFdbDstUserMetaDataRange().min),
      fdbDstUserMetaDataRangeMax_(getFdbDstUserMetaDataRange().max),
      fdbDstUserMetaDataMask_(getMetaDataMask(fdbDstUserMetaDataRangeMax_)),
      routeDstUserMetaDataRangeMin_(getRouteDstUserMetaDataRange().min),
      routeDstUserMetaDataRangeMax_(getRouteDstUserMetaDataRange().max),
      routeDstUserMetaDataMask_(getMetaDataMask(routeDstUserMetaDataRangeMax_)),
      neighborDstUserMetaDataRangeMin_(getNeighborDstUserMetaDataRange().min),
      neighborDstUserMetaDataRangeMax_(getNeighborDstUserMetaDataRange().max),
      neighborDstUserMetaDataMask_(
          getMetaDataMask(neighborDstUserMetaDataRangeMax_)) {}

std::
    pair<SaiAclTableTraits::AdapterHostKey, SaiAclTableTraits::CreateAttributes>
    SaiAclTableManager::aclTableCreateAttributes(sai_acl_stage_t aclStage) {
  std::vector<sai_int32_t> bindPointList{SAI_ACL_BIND_POINT_TYPE_SWITCH};
  SaiAclTableTraits::Attributes::Stage tableStage = aclStage;
  /*
   * Tajo either does not support following qualifier or enabling those
   * overflows max key width. Thus, disable those on Tajo for now.
   */
  bool isTajo =
      platform_->getAsic()->getAsicType() == HwAsic::AsicType::ASIC_TYPE_TAJO;

  std::vector<sai_int32_t> actionTypeList{
      SAI_ACL_ACTION_TYPE_PACKET_ACTION,
      SAI_ACL_ACTION_TYPE_COUNTER,
      SAI_ACL_ACTION_TYPE_SET_TC,
      SAI_ACL_ACTION_TYPE_SET_DSCP,
      SAI_ACL_ACTION_TYPE_MIRROR_INGRESS};
  if (!isTajo) {
    actionTypeList.push_back(SAI_ACL_ACTION_TYPE_MIRROR_EGRESS);
  }

  auto fieldL4SrcPort = isTajo ? false : true;
  auto fieldL4DstPort = isTajo ? false : true;
  auto fieldTcpFlags = isTajo ? false : true;
  auto fieldSrcPort = isTajo ? false : true;
  auto fieldOutPort = isTajo ? false : true;
  auto fieldIpFrag = isTajo ? false : true;
  auto fieldIcmpV4Type = isTajo ? false : true;
  auto fieldIcmpV4Code = isTajo ? false : true;
  auto fieldIcmpV6Type = isTajo ? false : true;
  auto fieldIcmpV6Code = isTajo ? false : true;
  auto fieldDstMac = isTajo ? false : true;

  /*
   * FdbDstUserMetaData is required only for MH-NIC queue-per-host solution.
   * However, the solution is not applicable for Trident2 as FBOSS does not
   * implement queues on Trident2.
   * Furthermore, Trident2 supports fewer ACL qualifiers than other
   * hardwares. Thus, avoid programming unncessary qualifiers (or else we run
   * out resources).
   */
  auto fieldFdbDstUserMeta = platform_->getAsic()->getAsicType() !=
          HwAsic::AsicType::ASIC_TYPE_TRIDENT2
      ? true
      : false;

  SaiAclTableTraits::AdapterHostKey adapterHostKey{
      tableStage,
      bindPointList,
      actionTypeList,
      true, // srcIpv6
      true, // dstIpv6
      true, // srcIpV4
      true, // dstIpV4
      fieldL4SrcPort,
      fieldL4DstPort,
      true, // ipProtocol
      fieldTcpFlags,
      fieldSrcPort,
      fieldOutPort,
      fieldIpFrag,
      fieldIcmpV4Type,
      fieldIcmpV4Code,
      fieldIcmpV6Type,
      fieldIcmpV6Code,
      true, // dscp
      fieldDstMac,
      true, // ipType
      true, // ttl
      fieldFdbDstUserMeta,
      true, // route meta
      true, // neighbor meta
      false, // ether type
  };

  SaiAclTableTraits::CreateAttributes attributes{adapterHostKey};

  return std::make_pair(adapterHostKey, attributes);
}

} // namespace facebook::fboss
