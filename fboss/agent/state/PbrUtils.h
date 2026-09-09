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

#include "fboss/agent/if/gen-cpp2/common_types.h"

#include <string>

namespace facebook::fboss {

// Deterministic, debug-facing PBR ACL entry name: "<policyName>_tc<N>".
std::string makePbrAclEntryName(
    const std::string& policyName,
    ForwardingClass trafficClass);

std::string makePbrCounterName(
    ForwardingClass trafficClass,
    const std::string& redirectNhgName);

} // namespace facebook::fboss
