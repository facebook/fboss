/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <ratio> // NOLINT(misc-include-cleaner)

#include <chrono>
#include <exception> // NOLINT(misc-include-cleaner)
#include <iostream>
#include <optional>
#include <string>

#include <folly/logging/xlog.h>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/summary/gen-cpp2/bgp_summary_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {
using apache::thrift::util::enumNameSafe;
using facebook::fboss::utils::Table;
using facebook::neteng::fboss::bgp::thrift::TBgpLocalConfig;
using facebook::neteng::fboss::bgp::thrift::TBgpSession;
using neteng::fboss::bgp::thrift::TBgpDrainState;
using neteng::fboss::bgp::thrift::TBgpPeerState;
using std::chrono::duration_cast;
using std::chrono::system_clock;

struct CmdShowBgpSummaryTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowBgpSummaryModel;
};

class CmdShowBgpSummary
    : public CmdHandler<CmdShowBgpSummary, CmdShowBgpSummaryTraits> {
 public:
  using RetType = CmdShowBgpSummaryTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<TBgpSession> sessions;
    TBgpLocalConfig config;
    TBgpDrainState drain_state;

    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

    client->sync_getBgpSessions(sessions);
    client->sync_getBgpLocalConfig(config);
    client->sync_getDrainState(drain_state);

    const int64_t ribVersion = client->sync_getRibVersion();

    /*
     * getProcessUptimeSeconds() and getNumPrefixes() are newer thrift methods
     * that may not exist on the bgpd binary deployed to prod. Against an older
     * daemon the calls throw (TApplicationException, UNKNOWN_METHOD), so guard
     * them and leave the values unset when unavailable -- printOutput omits the
     * corresponding lines. This self-heals once the new binary is deployed; no
     * code change is needed to re-enable them. getRibVersion() already exists
     * on prod, so it stays outside the guard.
     */
    std::optional<int64_t> processUptimeSeconds;
    std::optional<int64_t> totalPrefixCount;
    try {
      processUptimeSeconds = client->sync_getProcessUptimeSeconds();
      totalPrefixCount = client->sync_getNumPrefixes();
    } catch (const std::exception& ex) {
      XLOG(DBG2) << "process uptime / prefix count unavailable on this bgpd: "
                 << ex.what();
    }

    return createModel(
        sessions,
        config,
        drain_state,
        processUptimeSeconds,
        totalPrefixCount,
        ribVersion);
  }

  // Format process uptime, omitting leading zero units so a short uptime reads
  // naturally: 45 -> "45s", 330 -> "5m 30s", 3661 -> "1h 1m 1s",
  // 90061 -> "1d 1h 1m 1s". Units below the largest non-zero one are kept
  // (e.g. "3h 0m 5s") so the value stays unambiguous.
  static std::string formatUptime(int64_t totalSeconds) {
    if (totalSeconds < 0) {
      totalSeconds = 0;
    }
    const int64_t days = totalSeconds / 86400;
    const int64_t hours = (totalSeconds % 86400) / 3600;
    const int64_t minutes = (totalSeconds % 3600) / 60;
    const int64_t seconds = totalSeconds % 60;
    if (days > 0) {
      return fmt::format("{}d {}h {}m {}s", days, hours, minutes, seconds);
    }
    if (hours > 0) {
      return fmt::format("{}h {}m {}s", hours, minutes, seconds);
    }
    if (minutes > 0) {
      return fmt::format("{}m {}s", minutes, seconds);
    }
    return fmt::format("{}s", seconds);
  }

  void printOutput(const RetType& bgpSummary, std::ostream& out = std::cout) {
    const auto& config = bgpSummary.config().value();
    const auto& sessions = bgpSummary.sessions().value();
    const auto& drain_state = bgpSummary.drain_state().value();

    // prevent printing number with comma, e.g: 65,000
    out.imbue(std::locale("C"));

    out << "BGP summary information for VRF default" << std::endl;

    /*
     * Process uptime one-liner, e.g. "BGP is up for 1d 1h 1m 1s". Omitted when
     * unset (older bgpd without getProcessUptimeSeconds(); see queryClient()).
     */
    if (bgpSummary.process_uptime_seconds().has_value()) {
      out << "BGP is up for "
          << formatUptime(
                 folly::copy(bgpSummary.process_uptime_seconds().value()))
          << std::endl;
    }

    // Retrive the router's Ip Address from its ID.
    char ip_buff[INET_ADDRSTRLEN];
    auto id = folly::copy(config.my_router_id().value());
    inet_ntop(
        AF_INET, &((struct in_addr*)&id)->s_addr, ip_buff, INET_ADDRSTRLEN);

    // TODO(michaeluw): T113736668 deprecate i32 asns field
    const uint32_t local_as = folly::copy(config.local_as_4_byte().value())
        ? static_cast<uint32_t>(folly::copy(config.local_as_4_byte().value()))
        : folly::copy(config.local_as().value());

    out << "Router ID - " << ip_buff << ", Local ASN - " << local_as;

    // TODO(michaeluw): T113736668 deprecate i32 asns field
    if (const auto& confed_asn =
            folly::copy(config.local_confed_as_4_byte().value())) {
      out << ", Confed ASN - " << confed_asn << std::endl;
    } else if (
        const auto& confed_asn_old =
            apache::thrift::get_pointer(config.local_confed_as())) {
      out << ", Confed ASN - " << *confed_asn_old << std::endl;
    } else {
      out << std::endl;
    }
    // Add BGP drain status
    if (drain_state.drain_state().has_value()) {
      out << fmt::format(
                 "BGP Switch Drain State: {}",
                 enumNameSafe(*drain_state.drain_state()))
          << std::endl;
    } else {
      out << "BGP Switch Drain State: N/A" << std::endl;
    }

    // Add UCMP config header
    out << "UCMP weight programming is ";
    if (config.program_ucmp_weights().value()) {
      out << "ENABLED, with normalization width of "
          << folly::copy(config.ucmp_width().value()) << std::endl;
    } else {
      out << "DISABLED" << std::endl;
    }

    // Show if update-group is enabled
    out << "Update group is ";
    if (config.enable_update_group().value()) {
      out << "ENABLED" << std::endl;
    } else {
      out << "DISABLED" << std::endl;
    }

    // Add Summary
    const uint64_t total_neighbors = sessions.size();
    uint64_t up_neighbors = 0, paths_rcvd = 0, paths_accepted = 0,
             paths_sent = 0;

    // First pass: collect row data and detect whether any peer has downtime, so
    // the Downtime column can be omitted entirely when no peer is down.
    struct PeerRowData {
      std::string peerAddr;
      std::string remoteAs;
      std::string state;
      std::string gr;
      std::string pr;
      std::string pa;
      std::string ps;
      std::string prd;
      std::string ug;
      std::string ugps;
      std::string uptime;
      std::string downtime;
      std::string description;
      std::string sessionId;
      std::string flaps;
    };
    std::vector<PeerRowData> rows;
    bool hasAnyDowntime = false;
    bool hasAnyPreDrops = false;

    for (const auto& bgpSession : sessions) {
      const auto& peer = bgpSession.peer().value();
      up_neighbors +=
          (folly::copy(peer.peer_state().value()) ==
           TBgpPeerState::ESTABLISHED);

      auto rcvd_prefix = *bgpSession.prepolicy_rcvd_prefix_count();
      auto accepted_prefix = *bgpSession.postpolicy_rcvd_prefix_count();
      auto sent_prefix = *bgpSession.postpolicy_sent_prefix_count();

      /*
       * Per-peer routes dropped because the peer reached its configured
       * pre-filter prefix limit (server reports this count). Surfaced as the
       * PRD column, shown only when some peer actually dropped routes. A
       * non-zero count is reliable even when the live prefix count has churned
       * back below the limit, so it avoids digging through logs.
       */
      auto pre_dropped =
          bgpSession.prepolicy_rcvd_dropped_prefix_count().value_or(0);
      if (pre_dropped > 0) {
        hasAnyPreDrops = true;
      }

      paths_rcvd += rcvd_prefix;
      paths_accepted += accepted_prefix;
      paths_sent += sent_prefix;

      const uint32_t peer_remote_as =
          folly::copy(peer.remote_as_4_byte().value())
          ? folly::copy(peer.remote_as_4_byte().value())
          : folly::copy(peer.remote_as().value());

      std::string uptimeDurationString;
      if (*peer.peer_state() == TBgpPeerState::ESTABLISHED) {
        uptimeDurationString = utils::getPrettyElapsedTime(
            utils::getEpochFromDuration(*bgpSession.uptime()));
      }

      std::string downtimeDurationString;
      if (((*peer.peer_state() == TBgpPeerState::IDLE ||
            *peer.peer_state() == TBgpPeerState::IDLE_ADMIN)) &&
          (*bgpSession.num_resets() > 0)) {
        // If the peer is IDLE but num_resets() == 0, then the peer has never
        // been in ESTABLISHED state and hence does not have downtime yet.
        downtimeDurationString = utils::getPrettyElapsedTime(
            utils::getEpochFromDuration(*bgpSession.reset_time()));
      }
      if (!downtimeDurationString.empty()) {
        hasAnyDowntime = true;
      }

      // Update-group ID: show value or "-" if not set.
      std::string ugStr = "-";
      if (auto ugId =
              apache::thrift::get_pointer(bgpSession.update_group_id())) {
        ugStr = folly::to<std::string>(*ugId);
      }

      // Peer update-group state: show value or "-" if not set.
      std::string peerStateStr = "-";
      if (auto ps = apache::thrift::get_pointer(
              bgpSession.peer_state_update_group())) {
        peerStateStr = *ps;
      }

      rows.push_back(
          {bgpSession.peer_addr().value(),
           folly::to<std::string>(peer_remote_as),
           enumNameSafe(folly::copy(peer.peer_state().value())).substr(0, 4),
           folly::copy(peer.graceful().value()) ? "Y" : "N",
           folly::to<std::string>(rcvd_prefix),
           folly::to<std::string>(accepted_prefix),
           folly::to<std::string>(sent_prefix),
           folly::to<std::string>(pre_dropped),
           ugStr,
           peerStateStr,
           uptimeDurationString,
           downtimeDurationString,
           bgpSession.description().value(),
           bgpSession.peer_bgp_id().value(),
           folly::to<std::string>(
               folly::copy(bgpSession.num_resets().value()))});
    }

    // Build the table with conditional columns:
    //  - UG/UGPS: only when update-group is enabled
    //  - Downtime: only when at least one peer has downtime
    const bool enableUpdateGroup = config.enable_update_group().value();

    auto buildRow = [&](const PeerRowData& row) {
      std::vector<Table::RowData> fields = {
          row.peerAddr,
          row.remoteAs,
          row.state,
          row.gr,
          row.pr,
          row.pa,
          row.ps};
      // PRD: per-peer dropped count, shown only when some peer actually dropped
      // routes.
      if (hasAnyPreDrops) {
        fields.emplace_back(row.prd);
      }
      if (enableUpdateGroup) {
        fields.emplace_back(row.ug);
        fields.emplace_back(row.ugps);
      }
      fields.emplace_back(row.uptime);
      if (hasAnyDowntime) {
        fields.emplace_back(row.downtime);
      }
      fields.emplace_back(row.description);
      fields.emplace_back(row.sessionId);
      fields.emplace_back(row.flaps);
      return fields;
    };

    std::vector<std::string> header = {
        "Peer", "AS", "State", "GR", "PR", "PA", "PS"};
    if (hasAnyPreDrops) {
      header.emplace_back("PRD");
    }
    if (enableUpdateGroup) {
      header.emplace_back("UG");
      header.emplace_back("UGPS");
    }
    header.emplace_back("Uptime");
    if (hasAnyDowntime) {
      header.emplace_back("Downtime");
    }
    header.emplace_back("Description");
    header.emplace_back("Session ID");
    header.emplace_back("Flaps");

    Table table;
    table.setHeader({header.begin(), header.end()});
    for (const auto& row : rows) {
      auto fields = buildRow(row);
      table.addRow({fields.begin(), fields.end()});
    }

    out << "Peers: UP - " << up_neighbors << ", TOTAL - " << total_neighbors
        << std::endl;
    out << "Paths: Received - " << paths_rcvd << ", Accepted - "
        << paths_accepted << ", Sent - " << paths_sent << std::endl;
    /*
     * Prefix count is omitted when unset (older bgpd without getNumPrefixes();
     * see queryClient()). RIB Version comes from getRibVersion(), which is
     * already deployed to prod, so it is always shown.
     */
    if (bgpSummary.total_prefix_count().has_value()) {
      out << "Prefix count: "
          << folly::copy(bgpSummary.total_prefix_count().value()) << std::endl;
    }
    out << "RIB Version: " << folly::copy(bgpSummary.rib_version().value())
        << std::endl;
    std::string acronyms =
        "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent";
    if (hasAnyPreDrops) {
      acronyms += ", PRD - Prefixes Received Dropped";
    }
    if (enableUpdateGroup) {
      acronyms += ", UG - Update Group, UGPS - Update Group Peer State";
    }
    out << acronyms << "\n" << std::endl;
    out << table << std::endl;
  }

  // Move the data from bgp_thrift:TBgpSession to
  // bgp_summary_thrift:TBgpSummary
  RetType createModel(
      std::vector<TBgpSession>& sessions,
      TBgpLocalConfig& config,
      TBgpDrainState& drain_state,
      std::optional<int64_t> processUptimeSeconds,
      std::optional<int64_t> totalPrefixCount,
      int64_t ribVersion) {
    std::sort(
        sessions.begin(),
        sessions.end(),
        [](const TBgpSession& a, const TBgpSession& b) {
          return a.peer_addr().value() < b.peer_addr().value();
        });

    RetType model;
    model.sessions() = sessions;
    model.config() = config;
    model.drain_state() = drain_state;
    // Left unset when the daemon does not implement the getter, so printOutput
    // omits the corresponding line (see queryClient()).
    if (processUptimeSeconds.has_value()) {
      model.process_uptime_seconds() = *processUptimeSeconds;
    }
    if (totalPrefixCount.has_value()) {
      model.total_prefix_count() = *totalPrefixCount;
    }
    model.rib_version() = ribVersion;
    return model;
  }
};

} // namespace facebook::fboss
