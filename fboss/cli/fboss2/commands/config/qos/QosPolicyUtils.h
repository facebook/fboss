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

#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss::utils {

// DSCP is a 6-bit field (RFC 2474): valid codepoints are 0..63. Shared so the
// config and delete qos-policy-map commands validate against one definition
// rather than each carrying their own copy.
constexpr int16_t kMinDscp = 0;
constexpr int16_t kMaxDscp = 63;

// Locates the QosPolicy named `name` in `qosPolicies`, or end() when absent.
// sw.qosPolicies is a list keyed only by the name field, so every command that
// touches a policy has to do this scan; sharing it keeps `config qos policy`
// and `delete qos policy` agreeing on what "the policy called X" means.
//
// Returns an iterator rather than a pointer so callers can erase.
std::vector<cfg::QosPolicy>::iterator findQosPolicy(
    std::vector<cfg::QosPolicy>& qosPolicies,
    const std::string& name);

// Like findQosPolicy, but throws std::runtime_error with a uniform
// "No QoS policy named '<name>' exists" message when absent, so every delete
// command reports the same error instead of repeating the find-then-throw.
std::vector<cfg::QosPolicy>::iterator findQosPolicyOrThrow(
    std::vector<cfg::QosPolicy>& qosPolicies,
    const std::string& name);

} // namespace facebook::fboss::utils
