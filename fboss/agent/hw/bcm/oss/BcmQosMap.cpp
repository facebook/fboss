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

BcmQosMap::BcmQosMap(const BcmSwitchIf* hw) : hw_(hw), flags_(0) {}
BcmQosMap::BcmQosMap(const BcmSwitchIf* hw, int flags, int mapHandle)
    : hw_(hw), flags_(flags), handle_(mapHandle) {}
BcmQosMap::~BcmQosMap() {}
void BcmQosMap::addRule(const QosRule& /*qosRule*/) {}
void BcmQosMap::clear() {}
void BcmQosMap::removeRule(const QosRule& /*qosRule*/) {}

} // namespace fboss
} // namespace facebook
