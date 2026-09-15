/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <folly/json/dynamic.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

struct CmdShowBgpPolicyTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;
  using ObjectArgType = std::vector<std::string>;
  using RetType = folly::dynamic;

  // Human-authored guide prose for the CLI reference wiki. Superset of the
  // one-line help string registered in the command tree.
  static std::string_view description() {
    return "Displays the definition of one named BGP policy as pretty-printed JSON, read from the switch's running config. The statement is rendered whole: its name, version and default result, then each term in order with the matches it tests and the actions it applies when they hit, plus the term-miss action that decides what happens when they do not. This is the config side of the policy questions the RIB views raise - when 'show bgp neighbors <peer> received rejected' names a term, this is where you read what that term actually does. Matching a policy name is exact, and an unknown name reports \"Policy '<name>' not found\" on stderr and prints nothing on stdout, so an empty result is a lookup failure rather than an empty policy. Only the first name given is looked up. Note this is the configured intent; use the rib-policy commands to see what the switch is currently enforcing.";
  }
};

class CmdShowBgpPolicy
    : public CmdHandler<CmdShowBgpPolicy, CmdShowBgpPolicyTraits> {
 public:
  using RetType = CmdShowBgpPolicyTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& policyNames);
  void printOutput(RetType& policyConfig, std::ostream& out = std::cout);

  /*
   * Canned, synthetic model (no real switch data). Built as a real
   * BgpPolicyStatement and pushed through the same PORTABLE serialization
   * queryClient uses, so the rendered JSON cannot drift from the encoding a
   * switch actually returns. Defined inline rather than in the .cpp: that TU
   * lives only in the fboss2-routing-protocol target, which the test targets
   * do not link, so an out-of-line definition is invisible to the wiki tests.
   */
  static RetType sampleModel() {
    using facebook::bgp::bgp_policy::BgpPolicyAction;
    using facebook::bgp::bgp_policy::BgpPolicyActionType;
    using facebook::bgp::bgp_policy::BgpPolicyAtomicMatch;
    using facebook::bgp::bgp_policy::BgpPolicyAtomicMatchType;
    using facebook::bgp::bgp_policy::BgpPolicyMatch;
    using facebook::bgp::bgp_policy::BgpPolicyStatement;
    using facebook::bgp::bgp_policy::BgpPolicyTerm;
    using facebook::bgp::bgp_policy::CommunityListType;
    using facebook::bgp::bgp_policy::FlowControlAction;
    using facebook::bgp::bgp_policy::LocalPreference;

    // Match on a named community list, by reference rather than inline - which
    // is how a real policy is written, since the list is defined once and
    // reused.
    BgpPolicyAtomicMatch communityMatch;
    communityMatch.type() = BgpPolicyAtomicMatchType::COMMUNITY_LIST;
    CommunityListType communityListRef;
    communityListRef.community_list_name() = "CL_SAMPLE";
    communityMatch.community_list() = communityListRef;

    BgpPolicyMatch match;
    match.name() = "MATCH_SAMPLE_PREFIXES";
    match.match_entries() = {communityMatch};

    LocalPreference localPref;
    localPref.name() = "LP_SAMPLE";
    localPref.local_pref() = 100;

    BgpPolicyAction setLocalPref;
    setLocalPref.type() = BgpPolicyActionType::SET_LOCAL_PREF;
    setLocalPref.set_local_pref() = localPref;

    BgpPolicyAction permit;
    permit.type() = BgpPolicyActionType::PERMIT;

    BgpPolicyTerm term;
    term.name() = "LOCAL_ACCEPT_RULE_990";
    term.description() =
        "Accept locally originated prefixes tagged with a sample community";
    term.policy_matches() = {match};
    term.policy_action_entries() = {setLocalPref, permit};
    term.term_miss_action() = FlowControlAction::NEXT_TERM;
    term.sequence_number() = 990;

    BgpPolicyStatement statement;
    statement.name() = "PROPAGATE_RSW_FSW_OUT";
    statement.description() = "Egress policy applied to FSW uplinks";
    statement.policy_version() = "1";
    statement.policy_entries() = {term};
    // A term that misses falls through to the statement result, which denies -
    // the default-deny shape the prose points at.
    statement.result() = FlowControlAction::DENY;

    return facebook::thrift::to_dynamic(
        statement, facebook::thrift::dynamic_format::PORTABLE);
  }
};

} // namespace facebook::fboss
