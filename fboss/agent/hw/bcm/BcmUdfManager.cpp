/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmUdfManager.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmUdfGroup.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"

namespace facebook::fboss {
// createUdfPacketMatcher
void BcmUdfManager::createUdfPacketMatcher(
    const std::shared_ptr<UdfPacketMatcher>& udfPacketMatcher) {
  auto bcmUdfPacketMatcher =
      make_shared<BcmUdfPacketMatcher>(hw_, udfPacketMatcher);
  udfPacketMatcherMap_.insert(
      {udfPacketMatcher->getName(), bcmUdfPacketMatcher});
}

// Attach UdfPacketMatcher to UdfGroup
void BcmUdfManager::attachUdfPacketMatcher(
    std::shared_ptr<BcmUdfGroup>& bcmUdfGroup,
    const std::string& udfPacketMatcherName) {
  auto udfPacketMatcher = udfPacketMatcherMap_.find(udfPacketMatcherName);
  if (udfPacketMatcher != udfPacketMatcherMap_.end()) {
    auto packetMatcherId = udfPacketMatcher->second->getUdfPacketMatcherId();
    bcmUdfGroup->udfPacketMatcherAdd(packetMatcherId, udfPacketMatcherName);
  } else {
    throw FbossError(
        "udfPacketMatcher=",
        udfPacketMatcherName,
        " not found in udfPacketMatcherMap");
  }
}

bool BcmUdfManager::isBcmUdfPacketMatcherCacheMatchesCfg(
    const PacketMatcherIdMap& cachedPacketMatchers,
    const PacketMatcherIds& packetMatcherIds) {
  if (cachedPacketMatchers.size() != packetMatcherIds.size()) {
    return false;
  }

  for (auto packetMatcherId : packetMatcherIds) {
    if (cachedPacketMatchers.find(packetMatcherId) ==
        cachedPacketMatchers.end()) {
      return false;
    }
  }

  return true;
}

// createUdfGroup
void BcmUdfManager::createUdfGroup(const std::shared_ptr<UdfGroup>& udfGroup) {
  auto bcmUdfGroup = make_shared<BcmUdfGroup>(hw_, udfGroup);

  auto warmBootCache = hw_->getWarmBootCache();
  auto name = udfGroup->getID();
  auto udfPacketMatcherItr = warmBootCache->findUdfGroupPacketMatcher(name);

  auto packetMatcherIds = udfGroup->getUdfPacketMatcherIds();
  if (udfPacketMatcherItr !=
      warmBootCache->UdfGroupNameToPacketMatcherMapEnd()) {
    auto cachedPacketMatchers = udfPacketMatcherItr->second;
    if (isBcmUdfPacketMatcherCacheMatchesCfg(
            cachedPacketMatchers, packetMatcherIds)) {
      warmBootCache->programmed(udfPacketMatcherItr);
      for (auto packetMatcher : cachedPacketMatchers) {
        bcmUdfGroup->udfPacketMatcherIdsInsert(
            packetMatcher.second, packetMatcher.first);
        XLOG(DBG2)
            << "Warmboot UdfPacketMatcher cache matches the cfg for UdfGroup"
            << name;
      }
      udfGroupsMap_.insert({udfGroup->getName(), bcmUdfGroup});
      return;
    }
  }

  for (auto udfPacketMatcherName : udfGroup->getUdfPacketMatcherIds()) {
    attachUdfPacketMatcher(bcmUdfGroup, udfPacketMatcherName);
    XLOG(INFO) << "udfGroup=" << udfGroup->getName()
               << "attached to udfPacketMatcher=" << udfPacketMatcherName;
  }
  udfGroupsMap_.insert({udfGroup->getName(), bcmUdfGroup});
}

// deleteUdfPacketMatcher
void BcmUdfManager::deleteUdfPacketMatcher(
    const std::string& udfPacketMatcherName) {
  auto bcmUdfPacketMatcher = udfPacketMatcherMap_.find(udfPacketMatcherName);
  if (bcmUdfPacketMatcher != udfPacketMatcherMap_.end()) {
    auto warmBootCache = hw_->getWarmBootCache();
    auto udfPacketMatcherInfoItr =
        warmBootCache->findUdfPktMatcherInfo(udfPacketMatcherName);
    if (udfPacketMatcherInfoItr !=
        warmBootCache->UdfPktMatcherNameToInfoMapEnd()) {
      warmBootCache->programmed(udfPacketMatcherInfoItr);
      XLOG(DBG2) << "UdfPacketMatcher: " << udfPacketMatcherName
                 << " removed from WarmBootCache";
    }
    bcmUdfPacketMatcher->second.reset();

  } else {
    throw FbossError(
        "bcmUdfPacketMatcher=",
        udfPacketMatcherName,
        " not found in udfPacketMatcherMap_");
  }
  udfPacketMatcherMap_.erase(udfPacketMatcherName);
}

// Detach UdfPacketMatcher to UdfGroup
void BcmUdfManager::detachUdfPacketMatcher(
    std::shared_ptr<BcmUdfGroup>& bcmUdfGroup,
    const std::string& udfPacketMatcherName) {
  auto udfPacketMatcher = udfPacketMatcherMap_.find(udfPacketMatcherName);
  if (udfPacketMatcher != udfPacketMatcherMap_.end()) {
    auto packetMatcherId = udfPacketMatcher->second->getUdfPacketMatcherId();
    bcmUdfGroup->udfPacketMatcherDelete(packetMatcherId, udfPacketMatcherName);
  } else {
    throw FbossError(
        "udfPacketMatcher=",
        udfPacketMatcherName,
        " not found in udfPacketMatcherMap");
  }
}

// deleteUdfGroup
void BcmUdfManager::deleteUdfGroup(const std::shared_ptr<UdfGroup>& udfGroup) {
  auto udfGroupName = udfGroup->getName();

  auto bcmUdfGroup = udfGroupsMap_.find(udfGroupName);
  if (bcmUdfGroup != udfGroupsMap_.end()) {
    auto warmBootCache = hw_->getWarmBootCache();
    auto udfInfoItr = warmBootCache->findUdfGroupInfo(udfGroupName);
    if (udfInfoItr != warmBootCache->UdfGroupNameToInfoMapEnd()) {
      warmBootCache->programmed(udfInfoItr);
      XLOG(DBG2) << "UdfGroup: " << udfGroupName
                 << " removed from warmBootCache ";
    }

    for (auto udfPacketMatcherName : udfGroup->getUdfPacketMatcherIds()) {
      detachUdfPacketMatcher(bcmUdfGroup->second, udfPacketMatcherName);
      XLOG(DBG2) << "udfGroup=" << udfGroupName
                 << "detached from udfPacketMatcher=" << udfPacketMatcherName;
    }
  } else {
    throw FbossError(
        "bcmUdfGroup=", udfGroupName, " not found in udfGroupsMap_");
  }

  bcmUdfGroup->second.reset();
  udfGroupsMap_.erase(udfGroupName);
}

void BcmUdfManager::init() {
  if (udfInitDone_ || hw_->getWarmBootCache()->isUdfInitialized()) {
    // nothing to do,
    // we use this flag to create warm boot cache, hence set it right if not
    // done
    udfInitDone_ = true;
    return;
  }

  // this shouldn't be called at warm boot time if UDF was enabled before
  // We need to set udfInitDone_ = true based on warm boot cache
  int rv = bcm_udf_init(hw_->getUnit());
  bcmCheckError(rv, "Failed to initialize UDF");

  udfInitDone_ = true;
  XLOG(INFO) << "BCM Udf manager initialized";
}

void BcmUdfManager::setUdfInitFlag(bool flag) {
  udfInitDone_ = flag;
}

int BcmUdfManager::getBcmUdfGroupId(const std::string& udfGroupName) const {
  auto iter = udfGroupsMap_.find(udfGroupName);
  if (iter == udfGroupsMap_.end()) {
    throw FbossError("Unable to find : ", udfGroupName, " in the map.");
  }
  XLOG(DBG3) << " For UDF group " << udfGroupName
             << "  get BCM udf group id: " << iter->second->getUdfId();
  return iter->second->getUdfId();
}

int BcmUdfManager::getBcmUdfGroupFieldSize(
    const std::string& udfGroupName) const {
  auto iter = udfGroupsMap_.find(udfGroupName);
  if (iter == udfGroupsMap_.end()) {
    throw FbossError("Unable to find : ", udfGroupName, " in the map.");
  }
  XLOG(DBG3) << " For UDF group " << udfGroupName << "  get match field width: "
             << iter->second->getUdfMatchFieldWidth();
  return iter->second->getUdfMatchFieldWidth();
}

BcmUdfManager::~BcmUdfManager() {
  XLOG(DBG2) << "Destroying BcmUdfGroup";
}
} // namespace facebook::fboss
