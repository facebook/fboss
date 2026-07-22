/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/CanonicalRibResolver.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace facebook::fboss {

using neteng::fboss::bgp::thrift::TBgpAttrDict;
using neteng::fboss::bgp::thrift::TBgpDedupedPath;
using neteng::fboss::bgp::thrift::TBgpPath;
using neteng::fboss::bgp::thrift::TBgpPathCanonical;
using neteng::fboss::bgp::thrift::TCanonicalPeer;
using neteng::fboss::bgp::thrift::TRibEntryCanonical;

namespace {
/*
 * Group key for the best / ECMP path set. Mirrors facebook::bgp::kBestPathGroup
 * ("best") from neteng/fboss/bgp/cpp/BgpServiceUtil.h, hardcoded here so the
 * CLI does not drag the heavyweight bgp_service_lib in for a single string.
 * bgpd (RibBase) always sets TRibEntry.best_group to this value; the canonical
 * schema drops best_group, so we reconstruct it the same way.
 */
constexpr std::string_view kBestPathGroup = "best";

/*
 * Copy a set optional canonical field into the corresponding TBgpPath field.
 * dst is a Thrift field-ref handle taken by value; assigning through it writes
 * the underlying field.
 */
template <typename Dst, typename Src>
void assignIfSet(Dst dst, const Src& src) {
  if (src.has_value()) {
    dst = src.value();
  }
}

/*
 * Bare-.value() contract for this file: the fields read without a has_value()
 * guard -- the top-level pools (attr_dict / deduped_paths / peers /
 * rib_entries), every TBgpAttrDict *_lists, TBgpDedupedPath.next_hop,
 * TCanonicalPeer.peer_id, and TRibEntryCanonical.prefix -- are non-optional in
 * bgp_route_types.thrift, so .value() cannot throw bad_field_access. Only the
 * optional cross-references are guarded: the *_idx sub-attr refs, peer_idx, and
 * router_id via has_value(); the non-optional path_idx is resolved by map key.
 */

/*
 * Inverse of CanonicalRibBuilder::buildDedupedPath + the per-instance fields it
 * sets on TBgpPathCanonical: dereference path_idx -> deduped_paths, the
 * sub-attr
 * *_idx -> attr_dict lists, and peer_idx -> peers. Returns nullopt only when
 * path_idx is absent from the pool map (guarded defensively per the schema's
 * consumer contract).
 */
std::optional<TBgpPath> resolveCanonicalPath(
    const TBgpPathCanonical& canonPath,
    const std::unordered_map<int64_t, TBgpDedupedPath>& dedupedPaths,
    const TBgpAttrDict& attrDict,
    const std::unordered_map<int64_t, TCanonicalPeer>& peers) {
  const auto pathIdx = canonPath.path_idx().value();
  auto dedupedIt = dedupedPaths.find(pathIdx);
  if (dedupedIt == dedupedPaths.end()) {
    return std::nullopt;
  }
  const auto& dedupPath = dedupedIt->second;

  TBgpPath path;
  path.next_hop() = dedupPath.next_hop().value();

  // List-valued sub-attrs resolve through the shared attr_dict by index.
  if (dedupPath.as_path_idx().has_value()) {
    if (auto it = attrDict.as_path_lists()->find(*dedupPath.as_path_idx());
        it != attrDict.as_path_lists()->end()) {
      path.as_path() = it->second;
    }
  }
  if (dedupPath.communities_idx().has_value()) {
    if (auto it =
            attrDict.community_lists()->find(*dedupPath.communities_idx());
        it != attrDict.community_lists()->end()) {
      path.communities() = it->second;
    }
  }
  if (dedupPath.ext_communities_idx().has_value()) {
    if (auto it = attrDict.ext_community_lists()->find(
            *dedupPath.ext_communities_idx());
        it != attrDict.ext_community_lists()->end()) {
      path.extCommunities() = it->second;
    }
  }
  if (dedupPath.cluster_list_idx().has_value()) {
    if (auto it = attrDict.cluster_lists()->find(*dedupPath.cluster_list_idx());
        it != attrDict.cluster_lists()->end()) {
      path.cluster_list() = it->second;
    }
  }

  // Scalars stored inline on the deduped path.
  assignIfSet(path.origin(), dedupPath.origin());
  assignIfSet(path.local_pref(), dedupPath.local_pref());
  assignIfSet(path.med(), dedupPath.med());
  assignIfSet(path.atomic_aggregate(), dedupPath.atomic_aggregate());
  assignIfSet(path.originator_id(), dedupPath.originator_id());
  assignIfSet(path.aggregator(), dedupPath.aggregator());
  assignIfSet(path.topologyInfo(), dedupPath.topology_info());
  assignIfSet(path.weight(), dedupPath.weight());

  // Peer attribution resolves through the shared peers pool by index.
  if (canonPath.peer_idx().has_value()) {
    const auto peerIt = peers.find(*canonPath.peer_idx());
    if (peerIt != peers.end()) {
      const auto& peer = peerIt->second;
      path.peer_id() = peer.peer_id().value();
      if (peer.router_id().has_value()) {
        path.router_id() = peer.router_id().value();
      }
      /*
       * Legacy getRibEntries always sets peer_description (empty string for
       * peers with no description, e.g. self-originated routes), and
       * printRIBEntries renders unset as "--" but set-empty as "". The
       * canonical peer omits an empty description, so default to "" to match
       * the legacy rendering.
       */
      path.peer_description() = peer.peer_description().value_or("");
    }
  }

  // Per-(prefix, path) instance fields carried directly on the canonical path.
  assignIfSet(path.is_best_path(), canonPath.is_best_path());
  assignIfSet(path.next_hop_weight(), canonPath.next_hop_weight());
  assignIfSet(path.path_id(), canonPath.path_id());
  assignIfSet(path.igp_cost(), canonPath.igp_cost());
  assignIfSet(path.last_modified_time(), canonPath.last_modified_time());
  assignIfSet(path.path_id_to_send(), canonPath.path_id_to_send());
  assignIfSet(path.bestpath_filter_descr(), canonPath.bestpath_filter_descr());
  assignIfSet(path.policy_name(), canonPath.policy_name());

  return path;
}
} // namespace

std::vector<TRibEntry> resolveCanonicalRibState(
    const TCanonicalRibState& canonical) {
  const auto& attrDict = canonical.attr_dict().value();
  const auto& dedupedPaths = canonical.deduped_paths().value();
  const auto& peers = canonical.peers().value();

  std::vector<TRibEntry> result;
  result.reserve(canonical.rib_entries().value().size());

  // Map iteration order is not user-visible: printRIBEntries calls
  // sortEntries() by prefix before rendering canonical and legacy results.
  for (const auto& [prefixStr, canonEntry] : canonical.rib_entries().value()) {
    TRibEntry entry;
    entry.prefix() = canonEntry.prefix().value();
    /*
     * The canonical schema drops best_group; bgpd always names the best/ECMP
     * group kBestPathGroup, so mirror that unconditionally. This keeps the
     * "Selected N/M paths" count and "*" markers correct even when no bestpath
     * is currently selected (e.g. CPS native-criteria violation, where the
     * entry is multipath-only), matching the legacy getRibEntries rendering.
     */
    entry.best_group() = std::string(kBestPathGroup);

    std::map<std::string, std::vector<TBgpPath>> resolvedPaths;
    for (const auto& [group, canonPaths] : canonEntry.paths().value()) {
      std::vector<TBgpPath> groupPaths;
      groupPaths.reserve(canonPaths.size());
      for (const auto& canonPath : canonPaths) {
        if (auto resolved = resolveCanonicalPath(
                canonPath, dedupedPaths, attrDict, peers)) {
          groupPaths.push_back(std::move(resolved.value()));
        }
      }
      resolvedPaths[group] = std::move(groupPaths);
    }
    entry.paths() = std::move(resolvedPaths);

    /*
     * The canonical getter leaves the top-level best_path unset (it is
     * FSDB-only); recover best_next_hop / best_path from the is_best_path flag
     * so rendering matches legacy TRibEntry semantics (both unset when no
     * bestpath is currently selected). RIB marks is_best_path on at most one
     * path per entry, so the first match across groups is the unique best and
     * the break stops there.
     */
    for (const auto& [group, groupPaths] : entry.paths().value()) {
      for (const auto& path : groupPaths) {
        if (path.is_best_path().value_or(false)) {
          entry.best_next_hop() = path.next_hop().value();
          entry.best_path() = path;
          break;
        }
      }
      if (entry.best_path().has_value()) {
        break;
      }
    }

    if (canonEntry.rib_version().has_value()) {
      entry.rib_version() = canonEntry.rib_version().value();
    }
    if (canonEntry.path_selection_pending().has_value()) {
      entry.path_selection_pending() =
          canonEntry.path_selection_pending().value();
    }
    if (canonEntry.active_cps_criteria().has_value()) {
      entry.active_cps_criteria() = canonEntry.active_cps_criteria().value();
    }
    if (canonEntry.active_cte_ucmp_action().has_value()) {
      entry.active_cte_ucmp_action() =
          canonEntry.active_cte_ucmp_action().value();
    }

    result.push_back(std::move(entry));
  }

  return result;
}

} // namespace facebook::fboss
