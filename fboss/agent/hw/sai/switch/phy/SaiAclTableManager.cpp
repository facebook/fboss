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
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
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
      fdbDstUserMetaDataRangeMin_(0),
      fdbDstUserMetaDataRangeMax_(0),
      fdbDstUserMetaDataMask_(0),
      routeDstUserMetaDataRangeMin_(0),
      routeDstUserMetaDataRangeMax_(0),
      routeDstUserMetaDataMask_(0),
      neighborDstUserMetaDataRangeMin_(0),
      neighborDstUserMetaDataRangeMax_(0),
      neighborDstUserMetaDataMask_(0) {}

std::vector<sai_int32_t> SaiAclTableManager::getActionTypeList(
    const std::shared_ptr<AclTable>& /*addedAclTable*/) {
  return {};
}

std::set<cfg::AclTableQualifier> getQualifierSet(
    const std::shared_ptr<AclTable>& /*addedAclTable*/) {
  return {};
}

/*
 * aclTableCreateAttributes
 *
 * This function the ACL table crete attributes for the Phy. The Phy needs only
 * Stage, bindPoint, actionType, DestMac and EthType
 */
std::
    pair<SaiAclTableTraits::AdapterHostKey, SaiAclTableTraits::CreateAttributes>
    SaiAclTableManager::aclTableCreateAttributes(
        sai_acl_stage_t aclStage,
        const std::shared_ptr<AclTable>& /*addedAclTable*/) {
  SaiAclTableTraits::Attributes::Stage tableStage = aclStage;
  std::vector<sai_int32_t> bindPointList{SAI_ACL_BIND_POINT_TYPE_PORT};

  SaiAclTableTraits::AdapterHostKey adapterHostKey{
      tableStage, // stage
      bindPointList,
      std::nullopt, // actionTypeList
      std::nullopt, // srcIpv6
      std::nullopt, // dstIpv6
      std::nullopt, // srcIpV4
      std::nullopt, // dstIpV4
      std::nullopt, // fieldL4SrcPort
      std::nullopt, // fieldL4DstPort
      std::nullopt, // ipProtocol
      std::nullopt, // fieldTcpFlags
      std::nullopt, // fieldSrcPort
      std::nullopt, // fieldOutPort
      std::nullopt, // fieldIpFrag
      std::nullopt, // fieldIcmpV4Type
      std::nullopt, // fieldIcmpV4Code
      std::nullopt, // fieldIcmpV6Type
      std::nullopt, // fieldIcmpV6Code
      std::nullopt, // dscp
      true, // fieldDstMac
      std::nullopt, // ipType
      std::nullopt, // ttl
      std::nullopt, // fieldFdbDstUserMeta
      std::nullopt, // route meta
      std::nullopt, // neighbor meta
      true, // ether type
  };

  SaiAclTableTraits::CreateAttributes attributes{adapterHostKey};

  return std::make_pair(adapterHostKey, attributes);
}

} // namespace facebook::fboss
