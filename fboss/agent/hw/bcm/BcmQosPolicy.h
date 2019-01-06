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

namespace facebook {
namespace fboss {

class BcmSwitch;
class QosPolicy;

class BcmQosPolicy {
 public:
  BcmQosPolicy(BcmSwitch* hw, const std::shared_ptr<QosPolicy>& qosPolicy);

  void update(
      const std::shared_ptr<QosPolicy>& oldQosPolicy,
      const std::shared_ptr<QosPolicy>& newQosPolicy);
  BcmQosPolicyHandle getHandle() const;
  bool policyMatches(const std::shared_ptr<QosPolicy>& qosPolicy) const;

 private:
  std::unique_ptr<BcmQosMap> qosMap_;
};

} // namespace fboss
} // namespace facebook
