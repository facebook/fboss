// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/rib/ClassBasedPolicyUpdater.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/ClassBasedPolicyMap.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

namespace {
struct PolicyDelta {
  std::vector<std::shared_ptr<ClassBasedPolicyNode>> toUpdate;
  std::vector<std::string> toDel;
};

int64_t resolveNhgOrThrow(
    const ClassBasedPolicyNhgs& policy,
    const std::string& name,
    const std::string& nhgName) {
  if (auto it = policy.normalizedIds.find(nhgName);
      it != policy.normalizedIds.end()) {
    return static_cast<int64_t>(it->second);
  }
  XLOG(ERR) << "Unallocated next-hop group '" << nhgName << "' in policy '"
            << name << "'";
  throw FbossError("Unallocated next-hop group ", nhgName, " in ", name);
}

[[maybe_unused]] state::ClassBasedPolicyFields resolvePolicy(
    const NextHopIDManager& manager,
    const std::string& name,
    const ClassBasedPolicyNhgs& policy) {
  state::ClassBasedPolicyFields fields;
  fields.name() = name;
  fields.defaultNextHopGroup()->name() = policy.defaultNexthopGroup;
  fields.defaultNextHopGroup()->id() =
      resolveNhgOrThrow(policy, name, policy.defaultNexthopGroup);
  for (const auto& [fc, nhgName] : policy.class2NextHopGroup) {
    auto& entry = fields.class2NextHopGroup()[fc];
    entry.name() = nhgName;
    entry.id() = resolveNhgOrThrow(policy, name, nhgName);
  }
  fields.referenced() =
      manager.hasRoutesForNamedNhg(policy.defaultNexthopGroup);
  return fields;
}

[[maybe_unused]] bool samePolicy(
    const ClassBasedPolicyNode& node,
    const state::ClassBasedPolicyFields& fields) {
  return node.getDefaultNextHopGroup() == *fields.defaultNextHopGroup() &&
      node.getClass2NextHopGroup() == *fields.class2NextHopGroup() &&
      node.isReferenced() == *fields.referenced();
}
} // namespace

ClassBasedPolicyUpdater::ClassBasedPolicyUpdater(
    const NextHopIDManager* nextHopIDManager)
    : nextHopIDManager_(nextHopIDManager) {}

std::shared_ptr<SwitchState> ClassBasedPolicyUpdater::operator()(
    const std::shared_ptr<SwitchState>& state) {
  return state;
}

} // namespace facebook::fboss
