/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/profiler/CmdShowBgpProfiler.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdShowBgpProfiler::RetType CmdShowBgpProfiler::queryClient(
    const HostInfo& hostInfo) {
  std::vector<TBgpProfilerStat> stats;
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
  client->sync_getProfilerStats(stats);
  return stats;
}

void CmdShowBgpProfiler::printOutput(const RetType& stats, std::ostream& out) {
  if (stats.empty()) {
    out << "No profiler stats available. Profiler may be disabled or no "
           "profiled functions have been called yet."
        << std::endl;
    out << "Use 'start bgp profiler' to enable or "
           "'--bgp_coro_profiler_enabled=true' at startup."
        << std::endl;
    return;
  }

  out << fmt::format(
             "{:<50} {:>10} {:>10} {:>10} {:>10} {:>10} {:>10}",
             "Function",
             "Count",
             "P50(ms)",
             "P90(ms)",
             "P99(ms)",
             "Max(ms)",
             "Total(ms)")
      << std::endl;
  out << std::string(110, '-') << std::endl;

  for (const auto& stat : stats) {
    out << fmt::format(
               "{:<50} {:>10} {:>10} {:>10} {:>10} {:>10} {:>10}",
               *stat.name(),
               *stat.count(),
               *stat.p50_ms(),
               *stat.p90_ms(),
               *stat.p99_ms(),
               *stat.max_ms(),
               *stat.total_ms())
        << std::endl;
  }
}

template void CmdHandler<CmdShowBgpProfiler, CmdShowBgpProfilerTraits>::run();

} // namespace facebook::fboss
