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
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiAclTableManager::SaiAclTableManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore),
      managerTable_(managerTable),
      platform_(platform),
      aclEntryMinimumPriority_(0),
      aclEntryMaximumPriority_(0),
      fdbDstUserMetaDataRangeMin_(0),
      fdbDstUserMetaDataRangeMax_(0),
      fdbDstUserMetaDataMask_(0),
      routeDstUserMetaDataRangeMin_(0),
      routeDstUserMetaDataRangeMax_(0),
      routeDstUserMetaDataMask_(0),
      neighborDstUserMetaDataRangeMin_(0),
      neighborDstUserMetaDataRangeMax_(0),
      neighborDstUserMetaDataMask_(0) {
  // TODO(rajank) Implement Elbert PHY acl table logic
}
} // namespace facebook::fboss
