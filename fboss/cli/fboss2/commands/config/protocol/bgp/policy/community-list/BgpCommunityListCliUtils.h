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
 * list-level dispatcher (CmdConfigProtocolBgpPolicyCommunityList) and its
 * delete counterpart. A CommunityList is keyed by name. Its values live in the
 * flat `communities` string list, which is the field bgpd matches against; the
 * structured members[] are never read by bgpd and have no CLI.
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

} // namespace facebook::fboss::bgpcli
