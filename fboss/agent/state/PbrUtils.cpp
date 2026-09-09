/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/PbrUtils.h"

#include <folly/Conv.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {

std::string makePbrAclEntryName(
    const std::string& policyName,
    ForwardingClass trafficClass) {
  return folly::to<std::string>(
      policyName, "_tc", static_cast<int>(trafficClass));
}

std::string makePbrCounterName(
    ForwardingClass trafficClass,
    const std::string& redirectNhgName) {
  return folly::to<std::string>(
      apache::thrift::util::enumNameSafe(trafficClass), "_", redirectNhgName);
}

} // namespace facebook::fboss
