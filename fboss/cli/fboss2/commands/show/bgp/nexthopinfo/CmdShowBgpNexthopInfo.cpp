/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/nexthopinfo/CmdShowBgpNexthopInfo.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

namespace {
// Render an age in seconds as a compact "…ago" string.
std::string humanizeAge(int64_t seconds) {
  if (seconds < 0) {
    seconds = 0;
  }
  const int64_t days = seconds / 86400;
  seconds %= 86400;
  const int64_t hours = seconds / 3600;
  seconds %= 3600;
  const int64_t minutes = seconds / 60;
  const int64_t secs = seconds % 60;

  std::string out;
  if (days > 0) {
    out += std::to_string(days) + "d";
  }
  if (hours > 0) {
    out += std::to_string(hours) + "h";
  }
  if (minutes > 0) {
    out += std::to_string(minutes) + "m";
  }
  if (out.empty()) {
    out += std::to_string(secs) + "s";
  }
  return out + " ago";
}

// Unset age => "-" (the value has never been resolved by the underlying
// system); otherwise a compact "…ago" string.
std::string ageOrDash(const std::optional<int64_t>& ageSeconds) {
  return ageSeconds.has_value() ? humanizeAge(*ageSeconds) : "-";
}

std::string nexthopIpStr(const TNexthopInfo& entry) {
  const auto& prefixBin = *entry.next_hop()->prefix_bin();
  return folly::IPAddress::fromBinary(
             folly::ByteRange(
                 reinterpret_cast<const unsigned char*>(prefixBin.data()),
                 prefixBin.size()))
      .str();
}

// Detailed, per-nexthop "zoom" view shown when a specific IP is queried.
// Callers must handle cache misses (empty next_hop) before calling this.
void printDetailEntry(const TNexthopInfo& entry, std::ostream& out) {
  utils::Table table;
  table.setHeader({"Field", "Value"});
  table.addRow({"Nexthop", nexthopIpStr(entry)});
  table.addRow({"Reachable", *entry.is_reachable() ? "Yes" : "No"});
  table.addRow(
      {"IGP Cost",
       entry.igp_cost().has_value() ? folly::to<std::string>(*entry.igp_cost())
                                    : "N/A"});
  table.addRow(
      {"Directly Connected",
       entry.is_connected().has_value() ? (*entry.is_connected() ? "Yes" : "No")
                                        : "Unknown"});
  table.addRow(
      {"Resolved For Selection",
       *entry.is_resolved_for_selection() ? "Yes" : "No"});
  table.addRow(
      {"Dependent Routes", folly::to<std::string>(*entry.route_count())});
  table.addRow(
      {"Last Reachability Change",
       ageOrDash(entry.last_reachability_change_age_s().to_optional())});
  table.addRow(
      {"Last IGP Cost Change",
       ageOrDash(entry.last_igp_cost_change_age_s().to_optional())});
  out << table << std::endl;
}
} // namespace

CmdShowBgpNexthopInfo::RetType CmdShowBgpNexthopInfo::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedIps) {
  RetType result;

  // Specific IP(s) => detailed per-nexthop view; no arg => compact list of all.
  result.detailed() = !queriedIps.empty();

  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  if (queriedIps.empty()) {
    // No IP specified: list every entry in the nexthop cache.
    std::vector<TNexthopInfo> entries;
    client->sync_getNexthopInfos(entries, std::vector<std::string>{});
    result.entries() = std::move(entries);
    return result;
  }

  // Specific IP(s): keep the existing per-nexthop detailed lookup. Record the
  // queried address index-parallel to each entry so a cache miss can be named.
  for (const auto& ip : queriedIps) {
    TNexthopInfo nexthopInfo;
    client->sync_getNexthopInfoForNexthop(nexthopInfo, ip);
    result.entries()->push_back(std::move(nexthopInfo));
    result.queried_nexthops()->push_back(ip);
  }
  return result;
}

void CmdShowBgpNexthopInfo::printOutput(
    const RetType& data,
    std::ostream& out) {
  if (*data.detailed()) {
    const auto& entries = *data.entries();
    const auto& queried = *data.queried_nexthops();
    for (size_t i = 0; i < entries.size(); ++i) {
      const auto& prefixBin = *entries[i].next_hop()->prefix_bin();
      if (prefixBin.empty()) {
        // Miss: name the exact address the operator queried.
        const std::string ip = i < queried.size() ? queried[i] : std::string{};
        out << "Nexthop " << ip << " not found in the nexthop cache"
            << std::endl;
        continue;
      }
      printDetailEntry(entries[i], out);
    }
    return;
  }

  utils::Table table;
  table.setHeader({"Nexthop", "Reachable", "IGP Cost"});

  size_t rows = 0;
  for (const auto& entry : *data.entries()) {
    const auto& prefixBin = *entry.next_hop()->prefix_bin();
    if (prefixBin.empty()) {
      continue;
    }

    const auto reachable = *entry.is_reachable() ? "Yes" : "No";
    const auto igpCost = entry.igp_cost().has_value()
        ? folly::to<std::string>(*entry.igp_cost())
        : "N/A";

    table.addRow({nexthopIpStr(entry), reachable, igpCost});
    ++rows;
  }

  if (rows == 0) {
    out << "No nexthop cache entries found" << std::endl;
    return;
  }

  out << table << std::endl;
}

template void
CmdHandler<CmdShowBgpNexthopInfo, CmdShowBgpNexthopInfoTraits>::run();

} // namespace facebook::fboss
