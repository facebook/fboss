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
} // namespace facebook::fboss
