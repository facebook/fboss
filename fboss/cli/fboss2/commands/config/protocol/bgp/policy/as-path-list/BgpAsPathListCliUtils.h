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
#include <cstdint>
#include <string>
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"

/**
 * Lookup/create helpers for the as-path-list CLI family, shared between the
 * list-level dispatcher (CmdConfigProtocolBgpPolicyAsPathList), the entry
 * subcommand (CmdConfigProtocolBgpPolicyAsPathListEntry), and the delete
 * counterparts. An AsPathList is keyed by name; an AsPathListEntry (in
 * as_path_list[]) is keyed by sequence_number.
 */
namespace facebook::fboss::bgpcli {

inline bool asPathListExists(
    const bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  if (!cfg.policies().has_value()) {
    return false;
  }
  const auto& lists = *cfg.policies()->aspath_lists();
  return std::any_of(lists.begin(), lists.end(), [&](const auto& list) {
    return *list.name() == name;
  });
}

// Find the as-path-list keyed by name, creating it if absent. Setting an
// attribute on a not-yet-created list implicitly creates it, so command
// ordering stays forgiving; a bare `as-path-list <name>` creates one
// explicitly. AsPathList's only key field is `name`.
inline bgp::bgp_policy::AsPathList& findOrCreateAsPathList(
    bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  auto& lists = *cfg.policies().ensure().aspath_lists();
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

inline bool asPathListEntryExists(
    const bgp::bgp_policy::AsPathList& list,
    int64_t seqNum) {
  const auto& entries = *list.as_path_list();
  return std::any_of(entries.begin(), entries.end(), [&](const auto& entry) {
    return entry.sequence_number().has_value() &&
        *entry.sequence_number() == seqNum;
  });
}

// Find the entry keyed by sequence_number within a list, creating it if
// absent. sequence_number is the entry's identity.
inline bgp::bgp_policy::AsPathListEntry& findOrCreateAsPathListEntry(
    bgp::bgp_policy::AsPathList& list,
    int64_t seqNum) {
  auto& entries = *list.as_path_list();
  for (auto& entry : entries) {
    if (entry.sequence_number().has_value() &&
        *entry.sequence_number() == seqNum) {
      return entry;
    }
  }
  entries.emplace_back();
  auto& entry = entries.back();
  entry.sequence_number() = seqNum;
  return entry;
}

} // namespace facebook::fboss::bgpcli
