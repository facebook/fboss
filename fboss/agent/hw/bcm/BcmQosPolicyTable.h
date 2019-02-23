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

#include "fboss/agent/hw/bcm/BcmQosPolicy.h"
#include "fboss/agent/state/QosPolicy.h"

#include <boost/container/flat_map.hpp>

namespace facebook {
namespace fboss {

class BcmSwitch;

class BcmQosPolicyTable {
 public:
  explicit BcmQosPolicyTable(BcmSwitch* hw) : hw_(hw) {}

  void processAddedQosPolicy(const std::shared_ptr<QosPolicy>& qosPolicy);
  void processChangedQosPolicy(
      const std::shared_ptr<QosPolicy>& oldQosPolicy,
      const std::shared_ptr<QosPolicy>& newQosPolicy);
  void processRemovedQosPolicy(const std::shared_ptr<QosPolicy>& qosPolicy);

  int getNumQosPolicies() const;
  // Throw exception if not found
  BcmQosPolicy* getQosPolicy(const std::string& name) const;
  // return nullptr if not found
  BcmQosPolicy* getQosPolicyIf(const std::string& name) const;

  static bool isValid(const std::shared_ptr<QosPolicy>& qosPolicy);

 private:
  using BcmQosPolicyMap =
      boost::container::flat_map<std::string, std::unique_ptr<BcmQosPolicy>>;

  BcmSwitch* hw_;
  BcmQosPolicyMap qosPolicyMap_;
};

} // namespace fboss
} // namespace facebook
