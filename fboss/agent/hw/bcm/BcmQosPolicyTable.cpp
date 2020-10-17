/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmQosPolicyTable.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmPortQueueManager.h"
#include "fboss/agent/hw/bcm/BcmQosPolicy.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/types.h"

namespace facebook::fboss {

BcmQosPolicy* FOLLY_NULLABLE
BcmQosPolicyTable::getQosPolicyIf(const std::string& name) const {
  auto iter = qosPolicyMap_.find(name);
  if (iter == qosPolicyMap_.end()) {
    return nullptr;
  } else {
    return iter->second.get();
  }
}

BcmQosPolicy* BcmQosPolicyTable::getQosPolicy(const std::string& name) const {
  auto bcmQosPolicy = getQosPolicyIf(name);
  if (!bcmQosPolicy) {
    throw FbossError("QoS policy: ", name, " does not exist");
  }
  return bcmQosPolicy;
}

int BcmQosPolicyTable::getNumQosPolicies() const {
  return qosPolicyMap_.size();
}

void BcmQosPolicyTable::processAddedQosPolicy(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (qosPolicyMap_.find(qosPolicy->getName()) != qosPolicyMap_.end()) {
    throw FbossError("BcmQosPolicy=", qosPolicy->getName(), " already exists");
  }
  qosPolicyMap_.emplace(
      qosPolicy->getName(), std::make_unique<BcmQosPolicy>(hw_, qosPolicy));
}

void BcmQosPolicyTable::processChangedQosPolicy(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  if (oldQosPolicy->getName() != newQosPolicy->getName()) {
    processRemovedQosPolicy(oldQosPolicy);
    processAddedQosPolicy(newQosPolicy);
  } else {
    getQosPolicy(newQosPolicy->getName())->update(oldQosPolicy, newQosPolicy);
  }
}

void BcmQosPolicyTable::processRemovedQosPolicy(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  auto iter = qosPolicyMap_.find(qosPolicy->getName());
  if (iter == qosPolicyMap_.end()) {
    throw FbossError("BcmQosPolicy=", qosPolicy->getName(), " does not exist");
  }
  qosPolicyMap_.erase(iter);
}

/*
 * Broadcom requires explicitly configuring QoS map for default queue
 * (queue 0) or else, the packets wll be processed by default queue as
 * expected, but the dscp marking will be removed (dscp set to 0).
 * Thus, QoSPolicy validation verifies if all 64 dscp to queue mappings are
 * configured.
 */
bool BcmQosPolicyTable::isValid(const std::shared_ptr<QosPolicy>& qosPolicy) {
  auto kDscpValueMin = 0;
  auto kDscpValueMax = 63;
  auto kDscpValueMaxCnt = kDscpValueMax - kDscpValueMin + 1;
  std::set<uint8_t> dscpSet;

  for (const auto entry : qosPolicy->getDscpMap().from()) {
    // TODO(pshaikh): validating traffic class is valid, provide separate
    // function for the same
    BcmPortQueueManager::CosQToBcmInternalPriority(entry.trafficClass());
    if (entry.attr() < kDscpValueMin || entry.attr() > kDscpValueMax) {
      return false;
    }
    dscpSet.emplace(entry.attr());
  }

  /*
   * If no qos rules are provided (dscp to queue mappoing), treat it as qos
   * disabled. For example, MH-NIC downwlink ports use queue-per-host and thus
   * don't have an qos rules configured.
   */
  if (dscpSet.size() == 0) {
    return true;
  }

  // we have alredy validated that values are in range [0, 63]
  auto isValid = dscpSet.size() == kDscpValueMaxCnt;
  if (!isValid) {
    XLOG(ERR) << "QosPolicy provides: " << dscpSet.size()
              << " mappings, must provide ALL [0, 64]";
  }

  return isValid;
}

void BcmQosPolicyTable::processAddedDefaultDataPlaneQosPolicy(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  processAddedQosPolicy(qosPolicy);
  defaultDataPlaneQosPolicy_.emplace(qosPolicy->getName());
}
void BcmQosPolicyTable::processChangedDefaultDataPlaneQosPolicy(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  processChangedQosPolicy(oldQosPolicy, newQosPolicy);
}
void BcmQosPolicyTable::processRemovedDefaultDataPlaneQosPolicy(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  defaultDataPlaneQosPolicy_.reset();
  processRemovedQosPolicy(qosPolicy);
}

BcmQosPolicy* BcmQosPolicyTable::getDefaultDataPlaneQosPolicyIf() const {
  if (!defaultDataPlaneQosPolicy_) {
    return nullptr;
  }
  return getQosPolicyIf(defaultDataPlaneQosPolicy_.value());
}

BcmQosPolicy* BcmQosPolicyTable::getDefaultDataPlaneQosPolicy() const {
  auto policy = getDefaultDataPlaneQosPolicyIf();
  if (!policy) {
    if (defaultDataPlaneQosPolicy_) {
      throw FbossError(
          "BcmQosPolicy=",
          defaultDataPlaneQosPolicy_.value(),
          " does not exist");
    } else {
      throw FbossError("Default BcmQosPolicy does not exist");
    }
  }
  return policy;
}

} // namespace facebook::fboss
