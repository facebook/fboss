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

DECLARE_bool(enable_acl_table_group);

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

std::vector<sai_int32_t> SaiAclTableManager::getActionTypeList(
    const std::shared_ptr<AclTable>& addedAclTable) {
  if (FLAGS_enable_acl_table_group) {
    return cfgActionTypeListToSaiActionTypeList(
        addedAclTable->getActionTypes());
  } else {
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
    return actionTypeList;
  }
}

std::set<cfg::AclTableQualifier> SaiAclTableManager::getQualifierSet(
    const std::shared_ptr<AclTable>& addedAclTable) {
  if (FLAGS_enable_acl_table_group) {
    std::set<cfg::AclTableQualifier> qualifiers;
    for (const auto& qualifier : addedAclTable->getQualifiers()) {
      qualifiers.insert(qualifier);
    }

    return qualifiers;
  } else {
    /*
     * Tajo either does not support following qualifier or enabling those
     * overflows max key width. Thus, disable those on Tajo for now.
     */
    bool isTajo =
        platform_->getAsic()->getAsicType() == HwAsic::AsicType::ASIC_TYPE_TAJO;
    bool isTrident2 = platform_->getAsic()->getAsicType() ==
        HwAsic::AsicType::ASIC_TYPE_TRIDENT2;

    if (isTajo) {
      return {
          cfg::AclTableQualifier::SRC_IPV6,
          cfg::AclTableQualifier::DST_IPV6,
          cfg::AclTableQualifier::SRC_IPV4,
          cfg::AclTableQualifier::DST_IPV4,
          cfg::AclTableQualifier::IP_PROTOCOL,
          cfg::AclTableQualifier::DSCP,
          cfg::AclTableQualifier::IP_TYPE,
          cfg::AclTableQualifier::TTL,
          cfg::AclTableQualifier::LOOKUP_CLASS_L2,
          cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
          cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE};
    } else {
      std::set<cfg::AclTableQualifier> qualifiers = {
          cfg::AclTableQualifier::SRC_IPV6,
          cfg::AclTableQualifier::DST_IPV6,
          cfg::AclTableQualifier::SRC_IPV4,
          cfg::AclTableQualifier::DST_IPV4,
          cfg::AclTableQualifier::L4_SRC_PORT,
          cfg::AclTableQualifier::L4_DST_PORT,
          cfg::AclTableQualifier::IP_PROTOCOL,
          cfg::AclTableQualifier::TCP_FLAGS,
          cfg::AclTableQualifier::SRC_PORT,
          cfg::AclTableQualifier::OUT_PORT,
          cfg::AclTableQualifier::IP_FRAG,
          cfg::AclTableQualifier::ICMPV4_TYPE,
          cfg::AclTableQualifier::ICMPV4_CODE,
          cfg::AclTableQualifier::ICMPV6_TYPE,
          cfg::AclTableQualifier::ICMPV6_CODE,
          cfg::AclTableQualifier::DSCP,
          cfg::AclTableQualifier::DST_MAC,
          cfg::AclTableQualifier::IP_TYPE,
          cfg::AclTableQualifier::TTL,
          cfg::AclTableQualifier::LOOKUP_CLASS_L2,
          cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
          cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE};

      /*
       * FdbDstUserMetaData is required only for MH-NIC queue-per-host solution.
       * However, the solution is not applicable for Trident2 as FBOSS does not
       * implement queues on Trident2.
       * Furthermore, Trident2 supports fewer ACL qualifiers than other
       * hardwares. Thus, avoid programming unncessary qualifiers (or else we
       * run out resources).
       */
      if (isTrident2) {
        qualifiers.erase(cfg::AclTableQualifier::LOOKUP_CLASS_L2);
      }

      return qualifiers;
    }
  }
}

std::
    pair<SaiAclTableTraits::AdapterHostKey, SaiAclTableTraits::CreateAttributes>
    SaiAclTableManager::aclTableCreateAttributes(
        sai_acl_stage_t aclStage,
        const std::shared_ptr<AclTable>& addedAclTable) {
  std::vector<sai_int32_t> bindPointList{SAI_ACL_BIND_POINT_TYPE_SWITCH};
  SaiAclTableTraits::Attributes::Stage tableStage = aclStage;

  auto actionTypeList = getActionTypeList(addedAclTable);

  auto qualifierSet = getQualifierSet(addedAclTable);
  auto qualifierExistsFn = [qualifierSet](cfg::AclTableQualifier qualifier) {
    return qualifierSet.find(qualifier) != qualifierSet.end();
  };

  SaiAclTableTraits::AdapterHostKey adapterHostKey{
      tableStage,
      bindPointList,
      actionTypeList,
      qualifierExistsFn(cfg::AclTableQualifier::SRC_IPV6),
      qualifierExistsFn(cfg::AclTableQualifier::DST_IPV6),
      qualifierExistsFn(cfg::AclTableQualifier::SRC_IPV4),
      qualifierExistsFn(cfg::AclTableQualifier::DST_IPV4),
      qualifierExistsFn(cfg::AclTableQualifier::L4_SRC_PORT),
      qualifierExistsFn(cfg::AclTableQualifier::L4_DST_PORT),
      qualifierExistsFn(cfg::AclTableQualifier::IP_PROTOCOL),
      qualifierExistsFn(cfg::AclTableQualifier::TCP_FLAGS),
      qualifierExistsFn(cfg::AclTableQualifier::SRC_PORT),
      qualifierExistsFn(cfg::AclTableQualifier::OUT_PORT),
      qualifierExistsFn(cfg::AclTableQualifier::IP_FRAG),
      qualifierExistsFn(cfg::AclTableQualifier::ICMPV4_TYPE),
      qualifierExistsFn(cfg::AclTableQualifier::ICMPV4_CODE),
      qualifierExistsFn(cfg::AclTableQualifier::ICMPV6_TYPE),
      qualifierExistsFn(cfg::AclTableQualifier::ICMPV6_CODE),
      qualifierExistsFn(cfg::AclTableQualifier::DSCP),
      qualifierExistsFn(cfg::AclTableQualifier::DST_MAC),
      qualifierExistsFn(cfg::AclTableQualifier::IP_TYPE),
      qualifierExistsFn(cfg::AclTableQualifier::TTL),
      qualifierExistsFn(cfg::AclTableQualifier::LOOKUP_CLASS_L2),
      qualifierExistsFn(cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR),
      qualifierExistsFn(cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE),
      qualifierExistsFn(cfg::AclTableQualifier::ETHER_TYPE),
  };

  SaiAclTableTraits::CreateAttributes attributes{adapterHostKey};

  return std::make_pair(adapterHostKey, attributes);
}

} // namespace facebook::fboss
