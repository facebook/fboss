// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/bgp/stats/CmdShowBgpStatsPolicy.h"

#include <fmt/core.h>
#include <iostream>

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

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
#ifndef IS_OSS
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
#else
  // OSS: Simplified policy stats - full statement stats not available
  out << "BGP policy statistics:";
  out << fmt::format("\n  Routes matched: {}", *policy_stats.routes_matched());
  out << fmt::format(
      "\n  Routes rejected: {}", *policy_stats.routes_rejected());
  if (policy_stats.policy_counters().has_value()) {
    out << "\n  Policy counters:";
    for (const auto& [name, count] : *policy_stats.policy_counters()) {
      out << fmt::format("\n    {}: {}", name, count);
    }
  }
  out << "\n";
#endif
}

} // namespace facebook::fboss
