/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmClassIDUtil.h"

namespace {

using facebook::fboss::cfg::AclLookupClass;

const std::set<AclLookupClass> kQueuePerHostLookupClasses = {
    AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
    AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1,
    AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2,
    AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3,
    AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4,
    AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_5,
    AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_6,
    AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_7,
    AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_8,
    AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_9,
};

std::set<AclLookupClass> mergeLookupClasses(
    const std::set<AclLookupClass>& one,
    const std::set<AclLookupClass>& two) {
  std::set<AclLookupClass> mergedClasses;

  std::set_union(
      one.begin(),
      one.end(),
      two.begin(),
      two.end(),
      std::inserter(mergedClasses, mergedClasses.begin()));

  return mergedClasses;
}

const std::set<AclLookupClass> kAllLookupClasses = mergeLookupClasses(
    {AclLookupClass::DST_CLASS_L3_LOCAL_IP4,
     AclLookupClass::DST_CLASS_L3_LOCAL_IP6},
    kQueuePerHostLookupClasses);

} // namespace

namespace facebook::fboss {

bool BcmClassIDUtil::isValidQueuePerHostClass(cfg::AclLookupClass classID) {
  return kQueuePerHostLookupClasses.find(classID) !=
      kQueuePerHostLookupClasses.end();
}

bool BcmClassIDUtil::isValidLookupClass(cfg::AclLookupClass classID) {
  return kAllLookupClasses.find(classID) != kAllLookupClasses.end();
}

} // namespace facebook::fboss
