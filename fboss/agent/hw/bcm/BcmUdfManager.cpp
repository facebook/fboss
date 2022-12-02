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

namespace facebook::fboss {
// createUdfPacketMatcher
void BcmUdfManager::createUdfPacketMatcher(
    const std::string& udfPacketMatcherName,
    const std::shared_ptr<UdfPacketMatcher>& udfPacketMatcher) {
  auto bcmUdfPacketMatcher =
      make_shared<BcmUdfPacketMatcher>(hw_, udfPacketMatcher);
  udfPacketMatcherMap_.insert({udfPacketMatcherName, bcmUdfPacketMatcher});
}

// createUdfPacketMatchers adds BcmUdfPacketMatcher to udfPacketMatcherMap_
void BcmUdfManager::createUdfPacketMatchers(
    const std::shared_ptr<UdfPacketMatcherMap>& udfPacketMatcherMap) {
  for (auto udfPacketMatcherElem : *udfPacketMatcherMap) {
    createUdfPacketMatcher(
        udfPacketMatcherElem.first, udfPacketMatcherElem.second);
  }
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

// createUdfGroup
void BcmUdfManager::createUdfGroup(
    const std::string& udfGroupName,
    const std::shared_ptr<UdfGroup>& udfGroup) {
  auto bcmUdfGroup = make_shared<BcmUdfGroup>(hw_, udfGroup);
  for (auto udfPacketMatcherName : udfGroup->getUdfPacketMatcherIds()) {
    attachUdfPacketMatcher(bcmUdfGroup, udfPacketMatcherName);
    XLOG(DBG2) << "udfGroup=" << udfGroupName
               << "attached to udfPacketMatcher=" << udfPacketMatcherName;
  }
  udfGroupsMap_.insert({udfGroupName, bcmUdfGroup});
}

// createUdfGroups associates BcmUdfPacketMatchers to BcmUdfGroup and adds
// BcmUdfGroup to UdfGroupMap
void BcmUdfManager::createUdfGroups(
    const std::shared_ptr<UdfGroupMap>& udfGroupMap) {
  for (auto udfGroupElem : *udfGroupMap) {
    createUdfGroup(udfGroupElem.first, udfGroupElem.second);
  }
}

// deleteUdfPacketMatcher
void BcmUdfManager::deleteUdfPacketMatcher(
    const std::string& udfPacketMatcherName) {
  auto bcmUdfPacketMatcher = udfPacketMatcherMap_.find(udfPacketMatcherName);
  if (bcmUdfPacketMatcher != udfPacketMatcherMap_.end()) {
    bcmUdfPacketMatcher->second.reset();

  } else {
    throw FbossError(
        "bcmUdfPacketMatcher=",
        udfPacketMatcherName,
        " not found in udfPacketMatcherMap_");
  }
  udfPacketMatcherMap_.erase(udfPacketMatcherName);
}

// deleteUdfPacketMatchers removes BcmUdfPacketMatcher from udfPacketMatcherMap_
void BcmUdfManager::deleteUdfPacketMatchers(
    const std::shared_ptr<UdfPacketMatcherMap>& udfPacketMatcherMap) {
  for (auto udfPacketMatcherElem : *udfPacketMatcherMap) {
    deleteUdfPacketMatcher(udfPacketMatcherElem.first);
  }
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
void BcmUdfManager::deleteUdfGroup(
    const std::string& udfGroupName,
    const std::shared_ptr<UdfGroup>& udfGroup) {
  auto bcmUdfGroup = udfGroupsMap_.find(udfGroupName);
  if (bcmUdfGroup != udfGroupsMap_.end()) {
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

// deleteUdfGroups detaches BcmUdfPacketMatchers from BcmUdfGroup and deletes
// BcmUdfGroup from UdfGroupMap
void BcmUdfManager::deleteUdfGroups(
    const std::shared_ptr<UdfGroupMap>& udfGroupMap) {
  for (auto udfGroupElem : *udfGroupMap) {
    deleteUdfGroup(udfGroupElem.first, udfGroupElem.second);
  }
}
BcmUdfManager::~BcmUdfManager() {
  XLOG(DBG2) << "Destroying BcmUdfGroup";
}

// Walk through udfPktMatcherMap and create them using BcmUdfPktMatcher
// Walk through UdfGroupMap and create using BcmUdfGroup and associates
// BcmUdfPacketMatchers to BcmUdfGroup
void BcmUdfManager::addUdfConfig(
    const std::shared_ptr<UdfPacketMatcherMap>& udfPacketMatcherMap,
    const std::shared_ptr<UdfGroupMap>& udfGroupMap) {
  createUdfPacketMatchers(udfPacketMatcherMap);
  createUdfGroups(udfGroupMap);
}

// Walk through UdfGroupMap and delete BcmUdfGroups after detaching
// BcmUdfPacketMatchers from each BcmUdfGroup
// Walk through udfPktMatcherMap and delete BcmUdfPktMatcher
void BcmUdfManager::deleteUdfConfig(
    const std::shared_ptr<UdfPacketMatcherMap>& udfPacketMatcherMap,
    const std::shared_ptr<UdfGroupMap>& udfGroupMap) {
  deleteUdfGroups(udfGroupMap);
  deleteUdfPacketMatchers(udfPacketMatcherMap);
}

} // namespace facebook::fboss
