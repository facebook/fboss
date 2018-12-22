/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmQosPolicy.h"

namespace facebook {
namespace fboss {

class BcmQosMap {};

BcmQosPolicy::BcmQosPolicy(
    BcmSwitch* /*hw*/,
    const std::shared_ptr<QosPolicy>& /*qosPolicy*/) {}
BcmQosPolicy::~BcmQosPolicy() {}

void BcmQosPolicy::BcmQosPolicy::update(
    const std::shared_ptr<QosPolicy>& /*oldQosPolicy*/,
    const std::shared_ptr<QosPolicy>& /*newQosPolicy*/) {}
BcmQosPolicyHandle BcmQosPolicy::getHandle() const {
  return static_cast<BcmQosPolicyHandle>(0);
}
bool BcmQosPolicy::policyMatches(
    const std::shared_ptr<QosPolicy>& /*qosPolicy*/) const {
  return false;
}

} // namespace fboss
} // namespace facebook
