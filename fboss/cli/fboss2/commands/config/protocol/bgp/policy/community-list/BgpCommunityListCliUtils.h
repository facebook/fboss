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

#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <algorithm>
#include <string>
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"

/**
 * Lookup/create helpers for the community-list CLI family, shared between the
 * list-level dispatcher (CmdConfigProtocolBgpPolicyCommunityList), the
 * community subcommand (CmdConfigProtocolBgpPolicyCommunityListCommunity), and
 * the delete counterparts. A CommunityList is keyed by name; the inline
 * Community members it holds are keyed by their own name.
 */
namespace facebook::fboss::bgpcli {

inline bool communityListExists(
    const bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  if (!cfg.policies().has_value()) {
    return false;
  }
  const auto& lists = *cfg.policies()->community_lists();
  return std::any_of(lists.begin(), lists.end(), [&](const auto& list) {
    return *list.name() == name;
  });
}

// Find the community-list keyed by name, creating it if absent. Setting an
// attribute on a not-yet-created list implicitly creates it, so command
// ordering stays forgiving; a bare `community-list <name>` creates one
// explicitly. CommunityList's only key field is `name`.
inline bgp::bgp_policy::CommunityList& findOrCreateCommunityList(
    bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  auto& lists = *cfg.policies().ensure().community_lists();
  for (auto& list : lists) {
    if (*list.name() == name) {
      return list;
    }
  }
  lists.emplace_back();
  auto& list = lists.back();
  list.name() = name;
  return list;
}

inline bool communityMemberExists(
    const bgp::bgp_policy::CommunityList& list,
    const std::string& name) {
  if (!list.members().has_value()) {
    return false;
  }
  const auto& members = *list.members();
  return std::any_of(members.begin(), members.end(), [&](const auto& member) {
    return member.community_ref().has_value() &&
        *member.community_ref()->name() == name;
  });
}

// Find the inline Community member keyed by name within a list, creating it
// if absent. The CLI always defines members inline (the CommunityRefType
// union's `community` arm); the member's name is its identity.
inline bgp::bgp_policy::Community& findOrCreateCommunityMember(
    bgp::bgp_policy::CommunityList& list,
    const std::string& name) {
  auto& members = list.members().ensure();
  for (auto& member : members) {
    if (member.community_ref().has_value() &&
        *member.community_ref()->name() == name) {
      return *member.community_ref();
    }
  }
  members.emplace_back();
  auto& community = members.back().set_community();
  community.name() = name;
  return community;
}

} // namespace facebook::fboss::bgpcli
