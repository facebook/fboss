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

#include <algorithm>
#include <string>

#ifndef IS_OSS
#include <configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h>
#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#else
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#endif

/**
 * Lookup/create helpers for the prefix-list CLI family, shared between the
 * list-level dispatcher (CmdConfigProtocolBgpPolicyPrefixList) and its delete
 * counterpart. A PrefixList is keyed by name. Entry helpers arrive with the
 * entry subcommand, which decides the entry's identity (bgpd rejects a
 * PrefixListEntry carrying seq_num).
 */
namespace facebook::fboss::bgpcli {

inline bool prefixListExists(
    const bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  if (!cfg.policies().has_value()) {
    return false;
  }
  const auto& lists = *cfg.policies()->prefix_lists();
  return std::any_of(lists.begin(), lists.end(), [&](const auto& list) {
    return *list.name() == name;
  });
}

// Find the prefix-list keyed by name, creating it if absent. Setting an
// attribute on a not-yet-created list implicitly creates it, so command
// ordering stays forgiving; a bare `prefix-list <name>` creates one
// explicitly. PrefixList's only key field is `name`.
inline bgp::routing_policy::PrefixList& findOrCreatePrefixList(
    bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  auto& lists = *cfg.policies().ensure().prefix_lists();
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
