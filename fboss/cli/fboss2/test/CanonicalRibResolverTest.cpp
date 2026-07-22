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

#include <gtest/gtest.h>
#include <thrift/lib/cpp/TApplicationException.h>

#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/test/CmdBgpTestUtils.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_route_types_types.h"

namespace facebook::fboss {

using neteng::fboss::bgp::thrift::TBgpAttrDict;
using neteng::fboss::bgp::thrift::TBgpDedupedPath;
using neteng::fboss::bgp::thrift::TBgpPathCanonical;
using neteng::fboss::bgp::thrift::TCanonicalPeer;
using neteng::fboss::bgp::thrift::TCanonicalRibState;
using neteng::fboss::bgp::thrift::TRibEntryCanonical;
using neteng::fboss::bgp_attr::TBgpCommunity;

namespace {
// Mirrors facebook::bgp::kBestPathGroup / kDefaultPathGroup -- the group keys
// bgpd uses; the resolver must reconstruct best_group as kBestGroup.
constexpr auto kBestGroup = "best";

// A canonical state with a single prefix and sparse path/peer IDs, optionally
// marked as the selected best.
TCanonicalRibState makeSingleEntryState(int64_t med, bool markBest) {
  TCanonicalRibState state;

  TBgpDedupedPath deduped;
  deduped.next_hop() = getPrefix(kNextHop);
  deduped.med() = med;
  state.deduped_paths()->emplace(7, deduped);

  TCanonicalPeer peer;
  peer.peer_id() = getPrefix(kPeerId);
  peer.router_id() = 1234;
  peer.peer_description() = kPeerDescription;
  state.peers()->emplace(11, peer);

  TBgpPathCanonical canonPath;
  canonPath.path_idx() = 7;
  canonPath.peer_idx() = 11;
  if (markBest) {
    canonPath.is_best_path() = true;
  }

  TRibEntryCanonical entry;
  entry.prefix() = getPrefix(kIpAddress);
  entry.paths() = {{std::string(kBestGroup), {canonPath}}};
  state.rib_entries() = {{kIpAddress, entry}};

  return state;
}
} // namespace

// A selected best path is expanded with its peer attribution and scalars, and
// the dropped best_group / best_next_hop fields are reconstructed.
TEST(CanonicalRibResolverTest, ResolvesPathPeerAndBestPath) {
  const auto state = makeSingleEntryState(/*med=*/100, /*markBest=*/true);

  const auto entries = resolveCanonicalRibState(state);

  ASSERT_EQ(1, entries.size());
  const auto& entry = entries[0];
  EXPECT_EQ(getPrefix(kIpAddress), entry.prefix().value());
  EXPECT_EQ(kBestGroup, entry.best_group().value());
  EXPECT_EQ(getPrefix(kNextHop), entry.best_next_hop().value());
  ASSERT_TRUE(entry.best_path().has_value());
  EXPECT_TRUE(hasBestPath(entry));

  const auto& groups = entry.paths().value();
  ASSERT_TRUE(groups.contains(kBestGroup));
  ASSERT_EQ(1, groups.at(kBestGroup).size());
  const auto& path = groups.at(kBestGroup)[0];
  EXPECT_EQ(getPrefix(kNextHop), path.next_hop().value());
  EXPECT_EQ(getPrefix(kPeerId), path.peer_id().value());
  EXPECT_EQ(kPeerDescription, path.peer_description().value());
  EXPECT_EQ(1234, path.router_id().value());
  EXPECT_EQ(100, path.med().value());
  EXPECT_TRUE(path.is_best_path().value());
}

// CPS native-criteria violation: the entry is multipath-only -- no path is
// marked best and best_path is unset. best_group must still be reconstructed so
// the ECMP paths render with "*" markers and a correct "Selected N/M" count,
// matching the legacy getRibEntries path.
TEST(CanonicalRibResolverTest, ReconstructsBestGroupWhenNoBestPathSelected) {
  const auto state = makeSingleEntryState(/*med=*/0, /*markBest=*/false);

  const auto entries = resolveCanonicalRibState(state);

  ASSERT_EQ(1, entries.size());
  const auto& entry = entries[0];
  EXPECT_EQ(kBestGroup, entry.best_group().value());
  EXPECT_FALSE(entry.best_path().has_value());
  EXPECT_FALSE(hasBestPath(entry));
  ASSERT_TRUE(entry.paths().value().contains(kBestGroup));
  EXPECT_EQ(1, entry.paths().value().at(kBestGroup).size());
}

// List-valued sub-attributes are resolved through the shared attr_dict by
// index.
TEST(CanonicalRibResolverTest, ResolvesCommunitiesThroughDict) {
  TCanonicalRibState state;

  TBgpCommunity comm;
  comm.asn() = 100;
  comm.value() = 200;
  comm.community() = (static_cast<int64_t>(100) << 16) + 200;
  TBgpAttrDict dict;
  dict.community_lists()->emplace(13, std::vector<TBgpCommunity>{comm});
  state.attr_dict() = dict;

  TBgpDedupedPath deduped;
  deduped.next_hop() = getPrefix(kNextHop);
  deduped.communities_idx() = 13;
  state.deduped_paths()->emplace(17, deduped);

  TBgpPathCanonical canonPath;
  canonPath.path_idx() = 17;
  canonPath.is_best_path() = true;
  TRibEntryCanonical entry;
  entry.prefix() = getPrefix(kIpAddress);
  entry.paths() = {{std::string(kBestGroup), {canonPath}}};
  state.rib_entries() = {{kIpAddress, entry}};

  const auto entries = resolveCanonicalRibState(state);

  ASSERT_EQ(1, entries.size());
  const auto& path = entries[0].paths().value().at(kBestGroup)[0];
  ASSERT_TRUE(path.communities().has_value());
  const std::vector<TBgpCommunity> expected{comm};
  EXPECT_EQ(expected, path.communities().value());
}

TEST(CanonicalRibResolverTest, EmptyStateYieldsNoEntries) {
  const TCanonicalRibState state;
  EXPECT_TRUE(resolveCanonicalRibState(state).empty());
}

// runMethodWithLegacyFallback: the canonical-default + legacy-fallback core
// that each command relies on. Tested directly (no Thrift round-trip needed).

// When the server predates the canonical RPC, Thrift throws
// TApplicationException::UNKNOWN_METHOD and the legacy path runs.
TEST(CanonicalRibResolverTest, FallbackRunsLegacyOnUnknownMethod) {
  bool legacyCalled = false;
  const auto entries = runMethodWithLegacyFallback(
      // NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
      []() -> std::vector<TRibEntry> {
        throw apache::thrift::TApplicationException(
            apache::thrift::TApplicationException::UNKNOWN_METHOD,
            "Method name getRibEntriesCanonical not found");
      },
      [&]() -> std::vector<TRibEntry> {
        legacyCalled = true;
        return std::vector<TRibEntry>(2);
      });
  EXPECT_TRUE(legacyCalled);
  EXPECT_EQ(2, entries.size());
}

#ifndef IS_OSS
TEST(CanonicalRibResolverTest, FallbackRunsLegacyOnServiceRouterUnknownMethod) {
  bool legacyCalled = false;
  const auto entries = runMethodWithLegacyFallback(
      // NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
      []() -> std::vector<TRibEntry> {
        throw facebook::servicerouter::TServiceRouterException(
            facebook::servicerouter::ErrorReason::UNKNOWN_METHOD,
            "getRibEntriesCanonical");
      },
      [&]() -> std::vector<TRibEntry> {
        legacyCalled = true;
        return std::vector<TRibEntry>(2);
      });
  EXPECT_TRUE(legacyCalled);
  EXPECT_EQ(2, entries.size());
}
#endif

// When the canonical RPC succeeds, the legacy path is never invoked.
TEST(CanonicalRibResolverTest, FallbackSkipsLegacyOnCanonicalSuccess) {
  bool legacyCalled = false;
  const auto entries = runMethodWithLegacyFallback(
      []() -> std::vector<TRibEntry> { return std::vector<TRibEntry>(3); },
      [&]() -> std::vector<TRibEntry> {
        legacyCalled = true;
        return std::vector<TRibEntry>(2);
      });
  EXPECT_FALSE(legacyCalled);
  EXPECT_EQ(3, entries.size());
}

TEST(CanonicalRibResolverTest, SupportsNonRibReturnTypes) {
  EXPECT_EQ(
      42, runMethodWithLegacyFallback([] { return 42; }, [] { return 0; }));
}

// Any non-UNKNOWN_METHOD application error must propagate -- a genuine server
// error must not be masked as version skew by silently retrying the legacy RPC.
TEST(CanonicalRibResolverTest, FallbackRethrowsOtherApplicationErrors) {
  bool legacyCalled = false;
  EXPECT_THROW(
      runMethodWithLegacyFallback(
          // NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
          []() -> std::vector<TRibEntry> {
            throw apache::thrift::TApplicationException(
                apache::thrift::TApplicationException::INTERNAL_ERROR, "boom");
          },
          [&]() -> std::vector<TRibEntry> {
            legacyCalled = true;
            return {};
          }),
      apache::thrift::TApplicationException);
  EXPECT_FALSE(legacyCalled);
}

#ifndef IS_OSS
TEST(CanonicalRibResolverTest, FallbackRethrowsOtherServiceRouterErrors) {
  bool legacyCalled = false;
  EXPECT_THROW(
      runMethodWithLegacyFallback(
          // NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
          []() -> std::vector<TRibEntry> {
            throw facebook::servicerouter::TServiceRouterException(
                facebook::servicerouter::ErrorReason::APP_EXCEPTION, "boom");
          },
          [&]() -> std::vector<TRibEntry> {
            legacyCalled = true;
            return {};
          }),
      facebook::servicerouter::TServiceRouterException);
  EXPECT_FALSE(legacyCalled);
}
#endif

} // namespace facebook::fboss
