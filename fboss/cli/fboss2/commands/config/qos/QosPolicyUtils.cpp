/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/qos/QosPolicyUtils.h"

#include <fmt/format.h>
#include <algorithm>
#include <stdexcept>

namespace facebook::fboss::utils {

std::vector<cfg::QosPolicy>::iterator findQosPolicy(
    std::vector<cfg::QosPolicy>& qosPolicies,
    const std::string& name) {
  return std::find_if(
      qosPolicies.begin(), qosPolicies.end(), [&name](const auto& policy) {
        return *policy.name() == name;
      });
}

std::vector<cfg::QosPolicy>::iterator findQosPolicyOrThrow(
    std::vector<cfg::QosPolicy>& qosPolicies,
    const std::string& name) {
  auto it = findQosPolicy(qosPolicies, name);
  if (it == qosPolicies.end()) {
    throw std::runtime_error(
        fmt::format("No QoS policy named '{}' exists", name));
  }
  return it;
}

} // namespace facebook::fboss::utils
