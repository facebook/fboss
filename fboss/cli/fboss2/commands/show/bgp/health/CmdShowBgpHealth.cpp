/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/health/CmdShowBgpHealth.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"

namespace facebook::fboss {

using namespace facebook::neteng::fboss::bgp::thrift;

namespace {

std::string categoryToString(HealthCheckCategory category) {
  switch (category) {
    case HealthCheckCategory::GLOBAL_SYSTEM:
      return "Global: System";
    case HealthCheckCategory::GLOBAL_TASK_THREAD:
      return "Global: Task/Thread";
    case HealthCheckCategory::GLOBAL_CONVERGENCE:
      return "Global: Convergence";
    case HealthCheckCategory::SESSION_MANAGER:
      return "Session Manager";
    case HealthCheckCategory::PEER_MANAGER:
      return "Peer Manager";
    case HealthCheckCategory::RIB:
      return "RIB";
    case HealthCheckCategory::NETLINK_WRAPPER:
      return "Netlink Wrapper";
    case HealthCheckCategory::NEXTHOP_TRACKER:
      return "Nexthop Tracker";
    case HealthCheckCategory::FIB_AGENT:
      return "FIB Agent";
    case HealthCheckCategory::THRIFT_ENDPOINT:
      return "Thrift Endpoint";
    case HealthCheckCategory::UNKNOWN:
      return "Unknown";
  }
  return "Unknown";
}

std::string moduleDisplayStatus(const TModuleHealthReport& mod) {
  return apache::thrift::util::enumNameSafe(*mod.overallStatus());
}

} // namespace

CmdShowBgpHealth::RetType CmdShowBgpHealth::queryClient(
    const HostInfo& hostInfo) {
  THealthReport report;
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
  client->sync_getHealthReport(report);
  return report;
}

void CmdShowBgpHealth::printOutput(const RetType& report, std::ostream& out) {
  int32_t total = *report.passCount() + *report.warnCount() +
      *report.failCount() + *report.skipCount();

  out << std::endl;
  out << "BGP++ Health Report" << std::endl;
  out << "===================" << std::endl;

  out << fmt::format(
             "Overall: {}",
             apache::thrift::util::enumNameSafe(*report.overallStatus()))
      << std::endl;

  out << fmt::format(
             "Checks: {} total | {} PASS | {} WARN | {} FAIL | {} SKIP",
             total,
             *report.passCount(),
             *report.warnCount(),
             *report.failCount(),
             *report.skipCount())
      << std::endl;

  out << std::endl;
  out << "Legend:" << std::endl;
  out << "  PASS = Check ran, value within healthy range" << std::endl;
  out << "  WARN = Check ran, value degraded but not critical" << std::endl;
  out << "  FAIL = Check is in unhealthy state (bad value or could not execute)"
      << std::endl;
  out << "  SKIP = Check not applicable or deferred" << std::endl;

  for (const auto& mod : *report.modules()) {
    if (mod.checks()->empty()) {
      continue;
    }

    out << std::endl;
    out << fmt::format(
               "--- {} ({}) ---",
               categoryToString(*mod.category()),
               moduleDisplayStatus(mod))
        << std::endl;

    utils::Table table;
    table.setHeader({"Status", "Check", "Details"});

    for (const auto& check : *mod.checks()) {
      auto status = *check.status();
      auto statusName = apache::thrift::util::enumNameSafe(status);
      std::string statusTag = fmt::format("[{}]", statusName);

      const auto& detail = *check.message();
      auto checkName = apache::thrift::util::enumNameSafe(*check.checkId());
      table.addRow({statusTag, checkName, detail});
    }

    out << table;
  }

  out << std::endl;
}

template void CmdHandler<CmdShowBgpHealth, CmdShowBgpHealthTraits>::run();

} // namespace facebook::fboss
