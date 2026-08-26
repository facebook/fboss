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

#include <iterator>
#include <vector>

#include <thrift/lib/cpp/TApplicationException.h>

#include "neteng/fboss/bgp/if/gen-cpp2/bgp_route_types_types.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#ifndef IS_OSS
#include "servicerouter/common/TServiceRouterException.h"
#endif

namespace facebook::fboss {

using neteng::fboss::bgp::thrift::TCanonicalRibState;
using neteng::fboss::bgp::thrift::TRibEntry;
using neteng::fboss::bgp_attr::TBgpAfi;

/*
 * Expand a canonical (deduplicated) RIB dump back into the flat list<TRibEntry>
 * shape that the legacy getRibEntries() RPCs return, so callers can render it
 * with the existing printRIBEntries() path. Inverse of CanonicalRibBuilder:
 * each TBgpPathCanonical is resolved by dereferencing path_idx ->
 * deduped_paths, the sub-attr *_idx -> attr_dict lists, and peer_idx -> peers.
 */
std::vector<TRibEntry> resolveCanonicalRibState(
    const TCanonicalRibState& canonical);

/*
 * Call the canonical RPC path first; if the server predates it (Thrift replies
 * with TApplicationException::UNKNOWN_METHOD), transparently fall back to the
 * legacy RPC path. Only an unimplemented method triggers the fallback -- any
 * other failure (real application error, transport error, timeout) propagates,
 * so genuine errors are never masked as version skew. The callables must
 * return the same type.
 */
template <typename CanonicalFn, typename LegacyFn>
auto runMethodWithLegacyFallback(
    CanonicalFn&& canonicalFn,
    LegacyFn&& legacyFn) {
  try {
    return canonicalFn();
  } catch (const apache::thrift::TApplicationException& ex) {
    if (ex.getType() != apache::thrift::TApplicationException::UNKNOWN_METHOD) {
      throw;
    }
    return legacyFn();
#ifndef IS_OSS
  } catch (const facebook::servicerouter::TServiceRouterException& ex) {
    if (ex.getErrorReason() !=
        facebook::servicerouter::ErrorReason::UNKNOWN_METHOD) {
      throw;
    }
    return legacyFn();
#endif
  }
}

/*
 * Fetch a full RIB dump across both address families with canonical-first,
 * legacy-fallback semantics. Every "show bgp" table / shadow-rib / changelist
 * command shares this -- they differ only in which RPC pair they pass in. For
 * each AFI the canonical result is resolved back into list<TRibEntry> via
 * resolveCanonicalRibState(); the v4 and v6 results are concatenated. If the
 * server predates the canonical RPC (UNKNOWN_METHOD),
 * runMethodWithLegacyFallback
 * re-runs the whole fetch via the legacy RPC -- canonical support is treated as
 * all-or-nothing, since one server binary serves both AFIs.
 *
 * canonicalRpc(client, TCanonicalRibState&, TBgpAfi) and
 * legacyRpc(client, std::vector<TRibEntry>&, TBgpAfi) each issue one RPC for
 * the given AFI.
 */
template <typename Client, typename CanonicalRpc, typename LegacyRpc>
std::vector<TRibEntry> queryCanonicalRibWithFallback(
    Client& client,
    CanonicalRpc&& canonicalRpc,
    LegacyRpc&& legacyRpc) {
  return runMethodWithLegacyFallback(
      [&]() {
        std::vector<TRibEntry> entries;
        for (const auto afi : {TBgpAfi::AFI_IPV4, TBgpAfi::AFI_IPV6}) {
          TCanonicalRibState canonical;
          canonicalRpc(client, canonical, afi);
          auto resolved = resolveCanonicalRibState(canonical);
          entries.insert(
              entries.end(),
              std::make_move_iterator(resolved.begin()),
              std::make_move_iterator(resolved.end()));
        }
        return entries;
      },
      [&]() {
        std::vector<TRibEntry> entries;
        for (const auto afi : {TBgpAfi::AFI_IPV4, TBgpAfi::AFI_IPV6}) {
          std::vector<TRibEntry> perAfi;
          legacyRpc(client, perAfi, afi);
          entries.insert(
              entries.end(),
              std::make_move_iterator(perAfi.begin()),
              std::make_move_iterator(perAfi.end()));
        }
        return entries;
      });
}

} // namespace facebook::fboss
