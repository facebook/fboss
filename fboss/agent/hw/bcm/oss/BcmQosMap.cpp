/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmQosMap.h"
#include "fboss/agent/state/QosPolicy.h"

namespace facebook {
namespace fboss {

class BcmQosMapEntry {};

BcmQosMap::BcmQosMap(const BcmSwitchIf* hw, Type type)
    : hw_(hw), type_(type), flags_(0) {}
BcmQosMap::BcmQosMap(const BcmSwitchIf* hw, int flags, int mapHandle)
    : hw_(hw), flags_(flags), handle_(mapHandle) {}
BcmQosMap::~BcmQosMap() {}
void BcmQosMap::addRule(
    uint16_t /*internalTrafficClass*/,
    uint8_t /*externalTrafficClass*/) {}
void BcmQosMap::BcmQosMap::clear() {}
void BcmQosMap::removeRule(
    uint16_t /*internalTrafficClass*/,
    uint8_t /*externalTrafficClass*/) {}
bool BcmQosMap::ruleExists(
    uint16_t /*internalTrafficClass*/,
    uint8_t /*externalTrafficClass*/) const {
  return false;
}
int BcmQosMap::getQosMapFlags(BcmQosMap::Type /*type*/) {
  return 0;
}

} // namespace fboss
} // namespace facebook
