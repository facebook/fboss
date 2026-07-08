/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/stats/CmdShowBgpStatsPolicy.h"

#include <fmt/core.h>
#include <iostream>

#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"

namespace facebook::fboss {

CmdShowBgpStatsPolicy::RetType CmdShowBgpStatsPolicy::queryClient(
    const HostInfo& hostInfo) {
  TPolicyStats policy_stats;
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  client->sync_getPolicyStats(policy_stats);
  return policy_stats;
}

void CmdShowBgpStatsPolicy::printOutput(
    const RetType& policy_stats,
    std::ostream& out) {
  out << "BGP policy statistics:";
  for (const auto& stats : policy_stats.policy_statement_stats().value()) {
    const auto statement_prefix_count =
        folly::copy(stats.prefix_hit_count().value());
    out << fmt::format("\n Policy Name: {}", stats.name().value());
    out << fmt::format(
        "\n\t Number of executions: {}",
        folly::copy(stats.num_of_runs().value()));
    out << fmt::format(
        "\n\t Number of hits: {} prefixes", statement_prefix_count);
    out << fmt::format(
        "\n\t Average time of execution: {}us",
        folly::copy(stats.avg_time().value()));
    out << fmt::format(
        "\n\t Maximum time of execution: {}us",
        folly::copy(stats.max_time().value()));

    out << "\n\t Term  Description                                        Hits/Misses";

    int term_number = 1, hit_count_so_far = 0;
    for (const auto& term_stats : stats.term_stats().value()) {
      const auto& term_prefix_count =
          folly::copy(term_stats.prefix_hit_count().value());
      out << fmt::format(
          "\n\t {:<5} {:<48} {:>6}/{:<6}",
          term_number,
          term_stats.description().value(),
          term_prefix_count,
          (statement_prefix_count - term_prefix_count - hit_count_so_far));

      ++term_number;
      hit_count_so_far += term_prefix_count;
    }
    if (hit_count_so_far <= statement_prefix_count) {
      out << fmt::format(
          "\n\t {:<5} {:<48} {:>6}/0\n",
          term_number,
          "Default deny (Implicit)",
          statement_prefix_count - hit_count_so_far);
    }
  }
}

} // namespace facebook::fboss
