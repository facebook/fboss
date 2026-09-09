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

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/ClassBasedPolicyNode.h"
#include "fboss/agent/state/MatchAction.h"

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

std::vector<std::shared_ptr<AclEntry>> createAclEntriesFromPolicy(
    const std::shared_ptr<ClassBasedPolicyNode>& policy) {
  std::vector<std::shared_ptr<AclEntry>> entries;
  auto matchNhgId = *policy->getDefaultNextHopGroup().id();
  for (const auto& [trafficClass, nhg] : policy->getClass2NextHopGroup()) {
    auto redirectId = *nhg.id();
    auto entry = std::make_shared<AclEntry>(
        FLAGS_pbr_acl_priority,
        makePbrAclEntryName(policy->getID(), trafficClass));
    entry->setNextHopGroupId(matchNhgId);
    entry->setTrafficClass(static_cast<uint8_t>(trafficClass));
    MatchAction action;
    action.setRedirectNextHopGroupId(redirectId);
    cfg::TrafficCounter counter;
    counter.name() = makePbrCounterName(trafficClass, *nhg.name());
    counter.types() = {cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
    action.setTrafficCounter(counter);
    entry->setAclAction(action);
    entries.push_back(std::move(entry));
  }
  return entries;
}

} // namespace facebook::fboss
