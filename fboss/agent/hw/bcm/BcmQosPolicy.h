/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmQosMap.h"
#include "fboss/agent/hw/bcm/types.h"

namespace facebook::fboss {

class BcmSwitch;
class QosPolicy;

class BcmQosPolicy {
 public:
  BcmQosPolicy(BcmSwitch* hw, const std::shared_ptr<QosPolicy>& qosPolicy);

  void update(
      const std::shared_ptr<QosPolicy>& oldQosPolicy,
      const std::shared_ptr<QosPolicy>& newQosPolicy);
  BcmQosPolicyHandle getHandle(BcmQosMap::Type type) const;
  bool policyMatches(const std::shared_ptr<QosPolicy>& qosPolicy) const;

  const BcmQosMap* getIngressDscpQosMap() const;
  const BcmQosMap* getIngressExpQosMap() const;
  const BcmQosMap* getEgressExpQosMap() const;

 private:
  void updateIngressDscpQosMap(
      const std::shared_ptr<QosPolicy>& oldQosPolicy,
      const std::shared_ptr<QosPolicy>& newQosPolicy);
  void updateIngressExpQosMap(
      const std::shared_ptr<QosPolicy>& oldQosPolicy,
      const std::shared_ptr<QosPolicy>& newQosPolicy);
  void updateEgressExpQosMap(
      const std::shared_ptr<QosPolicy>& oldQosPolicy,
      const std::shared_ptr<QosPolicy>& newQosPolicy);

  void programIngressDscpQosMap(const std::shared_ptr<QosPolicy>& qosPolicy);
  void programIngressExpQosMap(const std::shared_ptr<QosPolicy>& qosPolicy);
  void programEgressExpQosMap(const std::shared_ptr<QosPolicy>& qosPolicy);

  BcmSwitch* hw_;
  std::unique_ptr<BcmQosMap> ingressDscpQosMap_;
  std::unique_ptr<BcmQosMap> ingressExpQosMap_;
  std::unique_ptr<BcmQosMap> egressExpQosMap_;
};

} // namespace facebook::fboss
