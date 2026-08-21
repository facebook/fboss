// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/TestDataFactory.h"

#include <gtest/gtest.h>

#include "fboss/thrift_cow/nodes/Serializer.h"

namespace facebook::fboss::test_data {

namespace {

fsdb::FsdbOperStateRoot getFsdbRoot(BgpRibMapDataGenerator& gen, int version) {
  auto state = gen.getStateUpdate(version, /*minimal=*/false);
  return facebook::fboss::thrift_cow::deserialize<
      apache::thrift::type_class::structure,
      fsdb::FsdbOperStateRoot>(
      *state.state()->protocol(), folly::fbstring(*state.state()->contents()));
}

} // namespace

// Verifies makeGtswScale(true, 144, 240) emits a canonicalRib whose object
// counts match a captured GTSW canonicalRib.
TEST(TestDataFactoryTest, FpfCanonicalRibObjectCounts) {
  constexpr int kNumPods = 144;
  constexpr int kNumPrefixesPerPod = 240;
  constexpr int kExpectedEntries = kNumPods * kNumPrefixesPerPod; // 34560

  auto scale = BgpRibMapDataGenerator::makeGtswScale(
      /*isFPF=*/true, kNumPods, kNumPrefixesPerPod);
  BgpRibMapDataGenerator gen(RoleSelector::GTSW, scale);

  auto root = getFsdbRoot(gen, /*version=*/0);
  // bgp is non-optional, so there is no presence check to make.
  const auto& bgp = *root.bgp();

  // FPF path emits canonicalRib, not ribMap.
  ASSERT_TRUE(bgp.canonicalRib().has_value());
  EXPECT_FALSE(bgp.ribMap().has_value());

  const auto& canonicalRib = *bgp.canonicalRib();
  EXPECT_EQ(canonicalRib.rib_entries()->size(), kExpectedEntries);

  // Best-path-only view: no shared deduped-path / peer pools.
  EXPECT_EQ(canonicalRib.deduped_paths()->size(), 0);
  EXPECT_EQ(canonicalRib.peers()->size(), 0);

  const auto& attrDict = *canonicalRib.attr_dict();
  EXPECT_EQ(
      attrDict.community_lists()->size(), 1); // one shared 13-community list
  EXPECT_EQ(
      attrDict.as_path_lists()->size(), kNumPods); // one 2-ASN list per pod
  EXPECT_EQ(attrDict.ext_community_lists()->size(), 1);
  EXPECT_EQ(attrDict.cluster_lists()->size(), 0);

  // The single community list carries 13 communities.
  ASSERT_TRUE(attrDict.community_lists()->contains(0));
  EXPECT_EQ(attrDict.community_lists()->at(0).size(), 13);

  // Each AS_PATH list is one AS_SEQUENCE segment with 2 ASNs, in both `asns`
  // and `asns_4_byte`.
  ASSERT_TRUE(attrDict.as_path_lists()->contains(0));
  const auto& asPath = attrDict.as_path_lists()->at(0);
  ASSERT_EQ(asPath.size(), 1);
  EXPECT_EQ(asPath[0].asns()->size(), 2);
  EXPECT_EQ(asPath[0].asns_4_byte()->size(), 2);

  // Every entry: empty paths map + a fully-populated best_path.
  for (const auto& [key, entry] : *canonicalRib.rib_entries()) {
    EXPECT_TRUE(entry.paths()->empty()) << "prefix " << key;
    ASSERT_TRUE(entry.best_path().has_value()) << "prefix " << key;
    const auto& bestPath = *entry.best_path();
    EXPECT_TRUE(bestPath.communities_idx().has_value());
    EXPECT_TRUE(bestPath.as_path_idx().has_value());
    EXPECT_TRUE(bestPath.ext_communities_idx().has_value());
    EXPECT_TRUE(bestPath.med().has_value());
    EXPECT_TRUE(bestPath.atomic_aggregate().has_value());
    // topology_info dominates canonicalRib memory (~416 B/entry).
    ASSERT_TRUE(bestPath.topology_info().has_value()) << "prefix " << key;
    EXPECT_EQ(bestPath.topology_info()->size(), 3);

    // prefix_bin / next_hop hold raw address bytes, not base64 text.
    EXPECT_EQ(entry.prefix()->prefix_bin()->size(), 16) << "prefix " << key;
    EXPECT_EQ(bestPath.next_hop()->prefix_bin()->size(), 16)
        << "prefix " << key;
  }
}

// createFpfPrefix spreads the index over two hextets, so topologies larger than
// a single hextet's 65536 prefixes still yield one distinct prefix per entry.
TEST(TestDataFactoryTest, FpfCanonicalRibExceedsSingleHextet) {
  constexpr int kNumPods = 300;
  constexpr int kNumPrefixesPerPod = 240;
  constexpr int kExpectedEntries = kNumPods * kNumPrefixesPerPod; // 72000
  static_assert(kExpectedEntries > 65536, "must cross the single-hextet bound");

  auto scale = BgpRibMapDataGenerator::makeGtswScale(
      /*isFPF=*/true, kNumPods, kNumPrefixesPerPod);
  BgpRibMapDataGenerator gen(RoleSelector::GTSW, scale);

  auto root = getFsdbRoot(gen, /*version=*/0);
  const auto& canonicalRib = *root.bgp()->canonicalRib();

  // No collisions: one map entry per requested prefix.
  EXPECT_EQ(canonicalRib.rib_entries()->size(), kExpectedEntries);
  EXPECT_EQ(canonicalRib.attr_dict()->as_path_lists()->size(), kNumPods);
}

} // namespace facebook::fboss::test_data
