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
          getMetaDataMask(neighborDstUserMetaDataRangeMax_)),
      hasTableGroups_(
          platform->getAsic()->isSupported(HwAsic::Feature::ACL_TABLE_GROUP)) {}

std::vector<sai_int32_t> SaiAclTableManager::getActionTypeList(
    const std::shared_ptr<AclTable>& addedAclTable) {
  /*
   * The current wedge agent code does the following.
   * 1. The sai code creates a default ACL table group and ACL table using
   * default qualifier and actiontype list to accommodate warmboot transition
   * from non multi acls to multi acls.
   * 2. The software switch state populates only the ACL entries and does not
   * populate the fields for ACL table group and ACL tables.
   * 3. As a consequence, the warmboot state does not contain the fields for ACL
   * table qualifiers and action type list.
   *
   * The following steps are done for when enable_acl_table_group flag is set.
   * 1. The Agent code populates default ACL table group and ACL table rather
   * than directly populating the ACLs.
   * 2. However, when the ACL table is being populated, software switch state
   * code is not aware of what action types and qualifiers are supported by the
   * current hardware.
   * 3. So ACL table is created with empty qualifier and action type list.
   * 4. When delta processing is hit for the ACL table, the newly added tables
   * will have empty lists for both qualifiers and actiontype list.
   * 5. To handle that case, we have a special check here where if the multi acl
   * flag is enabled and the qualifiers and actiontype list is empty, instead of
   * creating the new table with empty qualifiers and actiontypes, we populate
   * the default set of values so the ACLs can be created without issues.
   */

  auto aclActionTypes = addedAclTable->getActionTypes();

  if (FLAGS_enable_acl_table_group && aclActionTypes.size() != 0) {
    return cfgActionTypeListToSaiActionTypeList(aclActionTypes);
  } else {
    bool isTajo = platform_->getAsic()->getAsicVendor() ==
        HwAsic::AsicVendor::ASIC_VENDOR_TAJO;
    bool isIndus = platform_->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_JERICHO2;

    std::vector<sai_int32_t> actionTypeList{
        SAI_ACL_ACTION_TYPE_PACKET_ACTION,
        SAI_ACL_ACTION_TYPE_COUNTER,
        SAI_ACL_ACTION_TYPE_SET_TC,
        SAI_ACL_ACTION_TYPE_SET_DSCP,
        SAI_ACL_ACTION_TYPE_MIRROR_INGRESS};

    if (!(isTajo || isIndus)) {
      actionTypeList.push_back(SAI_ACL_ACTION_TYPE_MIRROR_EGRESS);
    }
    return actionTypeList;
  }
}

std::set<cfg::AclTableQualifier> SaiAclTableManager::getQualifierSet(
    const std::shared_ptr<AclTable>& addedAclTable) {
  auto aclQualifiers = addedAclTable->getQualifiers();
  /*
   * Please refer to the detailed comment under getActionTypeList() to
   * understand why we have the size check
   */
  if (FLAGS_enable_acl_table_group && aclQualifiers.size() != 0) {
    std::set<cfg::AclTableQualifier> qualifiers;
    for (const auto& qualifier : aclQualifiers) {
      qualifiers.insert(qualifier);
    }

    return qualifiers;
  } else {
    return getSupportedQualifierSet();
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

  SaiAclTableTraits::CreateAttributes attributes{
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
      qualifierExistsFn(cfg::AclTableQualifier::OUTER_VLAN),

  };

  SaiAclTableTraits::AdapterHostKey adapterHostKey{addedAclTable->getID()};

  return std::make_pair(adapterHostKey, attributes);
}

} // namespace facebook::fboss
