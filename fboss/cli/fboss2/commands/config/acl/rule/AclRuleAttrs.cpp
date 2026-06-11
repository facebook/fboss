/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/acl/rule/AclRuleAttrs.h"

#include <fmt/format.h>
#include <algorithm>
#include <stdexcept>

namespace facebook::fboss {

const std::vector<std::string_view>& aclRuleAttrsOrdered() {
  static const std::vector<std::string_view> kOrdered = {
      kAclRuleAttrSourceIp,
      kAclRuleAttrDestinationIp,
      kAclRuleAttrProtocol,
      kAclRuleAttrSourcePort,
      kAclRuleAttrDestinationPort,
      kAclRuleAttrDscp,
      kAclRuleAttrTcpFlags,
      kAclRuleAttrIcmpType,
      kAclRuleAttrIcmpCode,
      kAclRuleAttrIpFragment,
      kAclRuleAttrTtl,
      kAclRuleAttrDestinationMac,
      kAclRuleAttrEtherType,
      kAclRuleAttrVlan,
      kAclRuleAttrIpType,
      kAclRuleAttrPacketLookupResult,
      kAclRuleAttrAction,
  };
  return kOrdered;
}

const std::set<std::string_view>& aclRuleValidAttrs() {
  static const std::set<std::string_view> kSet(
      aclRuleAttrsOrdered().begin(), aclRuleAttrsOrdered().end());
  return kSet;
}

const std::vector<std::string_view>& aclRuleActionsOrdered() {
  static const std::vector<std::string_view> kOrdered = {
      kAclRuleActionPermit,
      kAclRuleActionDeny,
      kAclRuleActionSendToQueue,
      kAclRuleActionSetDscp,
      kAclRuleActionSetTc,
      kAclRuleActionMirrorIngress,
      kAclRuleActionMirrorEgress,
      kAclRuleActionCounter,
      kAclRuleActionTrapToCpu,
      kAclRuleActionCopyToCpu,
      kAclRuleActionRedirect,
  };
  return kOrdered;
}

const std::set<std::string_view>& aclRuleValidActions() {
  static const std::set<std::string_view> kSet(
      aclRuleActionsOrdered().begin(), aclRuleActionsOrdered().end());
  return kSet;
}

namespace {
std::string joinCsv(const std::vector<std::string_view>& v) {
  std::string out;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i != 0) {
      out += ", ";
    }
    out += v[i];
  }
  return out;
}
} // namespace

std::string aclRuleAttrsCsv() {
  return joinCsv(aclRuleAttrsOrdered());
}

std::string aclRuleActionsCsv() {
  return joinCsv(aclRuleActionsOrdered());
}

AclRuleArity aclRuleAttrArity(std::string_view attr) {
  if (attr == kAclRuleAttrTtl) {
    return {4, 5}; // optional <mask>
  }
  if (attr == kAclRuleAttrAction) {
    return {4, 6}; // refined by aclRuleActionArity()
  }
  return {4, 4};
}

AclRuleArity aclRuleActionArity(std::string_view sub) {
  if (sub == kAclRuleActionPermit || sub == kAclRuleActionDeny ||
      sub == kAclRuleActionTrapToCpu || sub == kAclRuleActionCopyToCpu) {
    return {4, 4}; // no value
  }
  if (sub == kAclRuleActionRedirect) {
    return {6, 6}; // redirect nexthop <ip>
  }
  return {5, 5}; // single value
}

std::pair<cfg::AclTable*, std::string> findAclTable(
    cfg::SwitchConfig& swConfig,
    const std::string& tableName) {
  if (!swConfig.aclTableGroups() || swConfig.aclTableGroups()->empty()) {
    throw std::runtime_error(
        "No aclTableGroups found in config. "
        "Only field-56 (aclTableGroups) is supported; "
        "the deprecated aclTableGroup (field 45) is not.");
  }

  for (auto& group : *swConfig.aclTableGroups()) {
    auto& tables = *group.aclTables();
    auto tit =
        std::find_if(tables.begin(), tables.end(), [&](const cfg::AclTable& t) {
          return *t.name() == tableName;
        });
    if (tit != tables.end()) {
      return {&*tit, *group.name()};
    }
  }
  throw std::runtime_error(
      fmt::format("AclTable '{}' not found in any AclTableGroup", tableName));
}

} // namespace facebook::fboss
