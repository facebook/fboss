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

#include <functional>
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/acl/CmdConfigAcl.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Returns the description shown by `--help` for `config acl rule`. The
// list of supported <attr> values is generated from the same ordered
// list used for validation, so the three stay in sync.
std::string aclRuleConfigHelpText();

// Argument for `config acl rule <table-name> <rule-name> <attr> <value>
// [<extra>]`.
//
// <table-name> matches an AclTable inside any AclTableGroup in
//   sw.aclTableGroups (field 56). The deprecated aclTableGroup (field 45)
//   is not supported.
// <rule-name>  the AclEntry to upsert. Created with default fields (just
//              `name`) plus the supplied attribute if it doesn't already
//              exist in the matching table; otherwise the attribute is
//              applied to the existing entry.
// <attr>       must be one of the supported attributes (see AclRuleAttrs.h).
// <value>      attribute-dependent — see attribute-specific parsing in the
//              constructor. For `action`, this is the action sub-attribute
//              (permit/deny/send-to-queue/...).
// <extra>      attribute-dependent trailing token. Only valid for
//              `ttl <value> [<mask>]` (mask, defaults 0xFF), the no-arg
//              actions (omit), or `action redirect nexthop <ip>` (where
//              <extra> is the `nexthop` keyword and a sixth token carries
//              the IP).
class AclRuleConfigArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ AclRuleConfigArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getTableName() const {
    return tableName_;
  }

  const std::string& getRuleName() const {
    return ruleName_;
  }

  const std::string& getAttribute() const {
    return attribute_;
  }

  const std::string& getRawValue() const {
    return rawValue_;
  }

  // Apply the parsed attribute value to the given AclEntry. Throws if the
  // rule's existing state is incompatible with the change (none of the
  // current attributes carry such a constraint, but kept as a hook).
  // Only handles match-field attributes plus `action permit | deny`
  // (which lives on AclEntry.actionType). The remaining `action`
  // sub-attrs live on a MatchAction attached via
  // dataPlaneTrafficPolicy.matchToAction — see applyActionTo().
  void applyTo(cfg::AclEntry& rule) const;

  // True if attribute_ == "action" and the sub-attribute targets a
  // MatchAction field (everything other than permit/deny). The handler
  // routes these through applyActionTo() instead of applyTo().
  bool isMatchAction() const;

  // Apply the parsed action to the given MatchAction. Caller is
  // responsible for locating/creating the MatchToAction entry keyed by
  // rule name on dataPlaneTrafficPolicy.matchToAction.
  void applyActionTo(cfg::MatchAction& ma) const;

  const std::string& getActionSubattr() const {
    return actionSubattr_;
  }

 private:
  // Parse a match-field <attr>'s value (v[3], plus v[4] mask for ttl) and
  // populate applyEntryFn_. Arity is validated by the caller.
  void parseMatchField(const std::vector<std::string>& v);

  // Parse `action <sub> [value]` (v[3..5]); permit/deny populate
  // applyEntryFn_, every other action populates applyActionFn_. Validates the
  // sub-attribute name and its arity.
  void parseAction(const std::vector<std::string>& v);

  std::string tableName_;
  std::string ruleName_;
  std::string attribute_;
  std::string rawValue_;

  // For attribute_ == "action": the sub-attribute (permit/send-to-queue/...).
  // Read by isMatchAction(); empty for match-field attributes.
  std::string actionSubattr_;

  // The parsed mutation, captured at parse time and replayed by applyTo() /
  // applyActionTo(). Keeping parse-and-apply together (one lambda per
  // attribute) means each attribute lives in exactly one place instead of a
  // parse chain plus a parallel apply chain. At most one is set: match-field
  // attrs and permit/deny set applyEntryFn_; MatchAction-typed actions set
  // applyActionFn_.
  std::function<void(cfg::AclEntry&)> applyEntryFn_;
  std::function<void(cfg::MatchAction&)> applyActionFn_;
};

struct CmdConfigAclRuleTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigAcl;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // expected_min=4 (table, rule, attr, value) is required to shield
    // attribute names against CLI11's subcommand-priority override at
    // App.hpp:_parse_subcommand: once an option's count reaches its
    // expected_min, CLI11 will route any subsequent token that matches a
    // subcommand-name anywhere up the tree (e.g. `protocol`, `vlan` —
    // siblings of `acl` under `config`) to that subcommand instead of the
    // positional, which strands the attribute value as an extra and
    // teleports the parser into the wrong subtree. expected_max=6
    // accommodates the optional ttl mask, the no-arg actions
    // (permit/deny/trap-to-cpu/copy-to-cpu — 4 tokens), and the
    // `action redirect nexthop <ip>` form (6 tokens). allow_extra_args()
    // is needed so CLI11's positional consume loop keeps eating past
    // expected_min. AclRuleConfigArgs validates exact arity per attr.
    cmd.add_option("acl_rule_config", args, aclRuleConfigHelpText())
        ->required()
        ->expected(4, 6)
        ->allow_extra_args();
  }
  using ObjectArgType = AclRuleConfigArgs;
  using RetType = std::string;
};

class CmdConfigAclRule
    : public CmdHandler<CmdConfigAclRule, CmdConfigAclRuleTraits> {
 public:
  using ObjectArgType = CmdConfigAclRuleTraits::ObjectArgType;
  using RetType = CmdConfigAclRuleTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
