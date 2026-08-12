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

#include <fmt/core.h>
#include <iostream>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

namespace facebook::fboss {
using facebook::neteng::fboss::bgp::thrift::TAttributeStats;

struct CmdShowBgpStatsAttrsTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = TAttributeStats;
};

class CmdShowBgpStatsAttrs
    : public CmdHandler<CmdShowBgpStatsAttrs, CmdShowBgpStatsAttrsTraits> {
 public:
  using RetType = CmdShowBgpStatsAttrsTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    TAttributeStats stats;
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
    client->sync_getAttributeStats(stats);
    return stats;
  }

  void printOutput(const RetType& stats, std::ostream& out = std::cout) {
    out << "BGP attribute statistics:" << std::endl;
    out << " Total number of attributes: "
        << folly::copy(stats.total_num_of_attributes().value()) << std::endl;
    out << " Total number of unique attributes: "
        << folly::copy(stats.total_unique_attributes().value()) << std::endl;
    out << fmt::format(
        " Average attribute reference count: {:.2f}\n",
        folly::copy(stats.avg_attribute_refcount().value()));
    out << fmt::format(
        " Average community list length: {:.2f}\n",
        folly::copy(stats.avg_community_list_len().value()));
    out << fmt::format(
        " Average extended community list length: {:.2f}\n",
        folly::copy(stats.avg_extcommunity_list_len().value()));
    out << fmt::format(
        " Average as path length: {:.2f}\n",
        folly::copy(stats.avg_as_path_len().value()));
    out << fmt::format(
        " Average cluster list length: {:.2f}\n",
        folly::copy(stats.avg_cluster_list_len().value()));
    out << fmt::format(
        " Average topology info length: {:.2f}\n",
        folly::copy(stats.avg_topology_info_len().value()));

    printDeduplicatorSizes(stats, out);
  }

 private:
  /*
   * Live size of each DeDuplicator<T> -- how many DISTINCT values of that type
   * the daemon currently stores. Printed as a separate block because these are
   * NOT a breakdown of the statistics above: those are derived from the RIB's
   * attribute slots, these are the storage behind them.
   *
   * Laid out by NESTING LEVEL, because the flat list invites the reading that
   * L2 is the sum of L3, which it is not (see the header note printed below):
   *
   *   L1  bgp_path        BgpPathC       = attrs ptr + nexthop + topologyInfo
   *   L2    bgp_attributes  BgpAttributesC = the bundle L1 points at
   *   L3      as_path / communities / cluster_list / ext_communities,
   *           held BY the bundle as deduplicated pointers
   *
   * Optional so an older bgpd that predates the fields still renders the block
   * above rather than throwing on a missing value.
   */
  static void printDeduplicatorSizes(const RetType& stats, std::ostream& out) {
    if (!stats.dedup_bgp_path().has_value()) {
      return;
    }

    out << std::endl;
    out << "Deduplicator sizes (distinct values stored at each nesting level):"
        << std::endl;

    utils::Table table;
    table.setHeader({"Level", "Collection", "Contents", "Entries"});
    const auto row =
        [&table](
            const std::string& level,
            const std::string& name,
            const std::string& contents,
            apache::thrift::optional_field_ref<const int64_t&> value) {
          table.addRow(
              {level,
               name,
               contents,
               value.has_value() ? folly::to<std::string>(*value)
                                 : std::string{"n/a"}});
        };

    /*
     * L1. Ingress pre-policy and post-policy paths reach this deduplicator.
     * AdjRibEntry::setPreOut stores verbatim, but it is handed the RIB
     * best-entry path, which was already interned as a postAttr.
     */
    row("L1",
        "bgp_path",
        "BgpPathC = bgp_attributes + nexthop + topologyInfo",
        stats.dedup_bgp_path());

    /*
     * L2. Published to fb303 as `bgpcpp.deduplicated_attributes.total`; that
     * counter name is kept for continuity but is a misnomer, so the row is
     * named after the deduplicator instead.
     *
     * The scalars it also holds are named in the trailing note rather than in
     * the Contents cell, which would otherwise push the table past a terminal
     * width. They are the reason L2 can grow while every L3 row stays flat.
     */
    row("L2",
        "bgp_attributes",
        "BgpAttributesC = the 4 L3 sub-attributes + scalars",
        stats.dedup_bgp_attributes());

    static constexpr auto kL3Contents = "sub-attribute held by bgp_attributes";
    row("L3", "as_path", kL3Contents, stats.dedup_as_path());
    row("L3", "communities", kL3Contents, stats.dedup_communities());
    row("L3", "cluster_list", kL3Contents, stats.dedup_cluster_list());
    row("L3", "ext_communities", kL3Contents, stats.dedup_ext_communities());

    out << table << std::endl;

    /*
     * Stated explicitly because the arithmetic reading is the natural one and
     * it is wrong in BOTH directions: L2 counts distinct COMBINATIONS, so
     * A as_paths x C community sets can reach A*C bundles (far above the L3
     * sum), while pairing them 1:1 gives max(A,C) (below it). BgpAttributesC
     * also carries med / localPref / aggregator / originatorId / weight, none
     * of which are deduplicated -- bundles differing only in MED add L2
     * entries and no L3 entries at all.
     */
    out << "Each level counts distinct values at that level; a level is NOT "
           "the sum of the level below it. bgp_attributes also holds med, "
           "localPref, origin, aggregator, originatorId and weight, which have "
           "no deduplicator -- so it can grow while every L3 row stays flat."
        << std::endl;
  }
};
} // namespace facebook::fboss
