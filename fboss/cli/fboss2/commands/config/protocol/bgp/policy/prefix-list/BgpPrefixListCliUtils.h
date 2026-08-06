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
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h"

/**
 * Lookup/create helpers for the prefix-list CLI family, shared between the
 * list-level dispatcher (CmdConfigProtocolBgpPolicyPrefixList), the entry
 * subcommand (CmdConfigProtocolBgpPolicyPrefixListEntry), and the delete
 * counterparts. A PrefixList is keyed by name; a PrefixListEntry (in
 * prefixes[]) is keyed by seq_num.
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

inline bool prefixListEntryExists(
    const bgp::routing_policy::PrefixList& list,
    int32_t seqNum) {
  const auto& entries = *list.prefixes();
  return std::any_of(entries.begin(), entries.end(), [&](const auto& entry) {
    return entry.seq_num().has_value() && *entry.seq_num() == seqNum;
  });
}

// Find the entry keyed by seq_num within a list's prefixes[], creating it if
// absent. seq_num is the entry's identity.
inline bgp::routing_policy::PrefixListEntry& findOrCreatePrefixListEntry(
    bgp::routing_policy::PrefixList& list,
    int32_t seqNum) {
  auto& entries = *list.prefixes();
  for (auto& entry : entries) {
    if (entry.seq_num().has_value() && *entry.seq_num() == seqNum) {
      return entry;
    }
  }
  entries.emplace_back();
  auto& entry = entries.back();
  entry.seq_num() = seqNum;
  return entry;
}

} // namespace facebook::fboss::bgpcli
