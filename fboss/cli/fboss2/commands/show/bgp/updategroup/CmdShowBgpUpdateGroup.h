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

#include <fmt/format.h>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/updategroup/gen-cpp2/bgp_update_group_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

namespace facebook::fboss {
using facebook::fboss::utils::Table;
using facebook::neteng::fboss::bgp::thrift::TUpdateGroupInfo;
using facebook::neteng::fboss::bgp::thrift::TUpdateGroupKey;
using facebook::neteng::fboss::bgp::thrift::TUpdateGroupPeerInfo;
using facebook::neteng::fboss::bgp::thrift::TUpdateGroupStats;

struct CmdShowBgpUpdateGroupTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowBgpUpdateGroupModel;
};

class CmdShowBgpUpdateGroup
    : public CmdHandler<CmdShowBgpUpdateGroup, CmdShowBgpUpdateGroupTraits> {
 public:
  using RetType = CmdShowBgpUpdateGroupTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& args) {
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

    facebook::neteng::fboss::bgp::thrift::TGetUpdateGroupInfoRequest request;
    bool detailMode = false;

    if (!args.empty()) {
      int64_t targetId;
      try {
        size_t pos = 0;
        targetId = std::stoll(args[0], &pos);
        if (pos != args[0].size()) {
          throw std::invalid_argument("trailing characters");
        }
      } catch (const std::exception& e) {
        throw std::invalid_argument(
            fmt::format("Invalid update-group ID '{}': {}", args[0], e.what()));
      }
      request.group_id() = targetId;
      detailMode = true;
    }

    facebook::neteng::fboss::bgp::thrift::TGetUpdateGroupInfoResponse response;
    client->sync_getUpdateGroupInfo(response, request);

    RetType model;
    model.enable_update_group() = response.enable_update_group().value();
    model.update_groups() = std::move(*response.update_groups());
    model.detail_mode() = detailMode;

    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    const auto& updateGroups = model.update_groups().value();

    out.imbue(std::locale("C"));

    if (!model.enable_update_group().value()) {
      out << "Update group: DISABLED" << std::endl;
      return;
    }

    if (updateGroups.empty()) {
      if (model.detail_mode().value()) {
        out << "Update group not found." << std::endl;
      } else {
        out << "No active update groups." << std::endl;
      }
      return;
    }

    if (model.detail_mode().value()) {
      printDetailView(updateGroups[0], out);
    } else {
      printSummaryView(model, updateGroups, out);
    }
  }

 private:
  static std::string boolStr(bool v) {
    return v ? "Yes" : "No";
  }

  static std::string epochMsToString(int64_t epochMs) {
    auto seconds = epochMs / 1000;
    std::time_t t = static_cast<std::time_t>(seconds);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return std::string(buf);
  }

  void printSummaryView(
      const RetType& /*model*/,
      const std::vector<TUpdateGroupInfo>& groups,
      std::ostream& out) {
    out << "Update group: ENABLED" << std::endl;
    out << "Update Groups: " << groups.size() << std::endl;
    out << std::endl;

    Table table;
    table.setHeader(
        {"ID",
         "Policy Name",
         "State",
         "Members",
         "InSync",
         "Detached",
         "Prefixes(v4/v6)",
         "RIB Ver"});

    for (const auto& group : groups) {
      const auto& key = group.group_key().value();
      const auto& stats = group.stats().value();
      auto prefixes = fmt::format(
          "{}({}/{})",
          stats.post_out_prefix_count().value(),
          stats.post_out_prefix_count_ipv4().value(),
          stats.post_out_prefix_count_ipv6().value());

      auto ribVer = group.last_seen_rib_version().value();
      table.addRow(
          {std::to_string(group.group_id().value()),
           key.egress_policy_name().value(),
           group.group_state().value(),
           std::to_string(group.member_count().value()),
           std::to_string(group.in_sync_peer_count().value()),
           std::to_string(group.detached_peer_count().value()),
           prefixes,
           ribVer > 0 ? std::to_string(ribVer) : "N/A"});
    }

    out << table << std::endl;
  }

  void printDetailView(const TUpdateGroupInfo& group, std::ostream& out) {
    const auto& key = group.group_key().value();

    out << "=== Update Group " << group.group_id().value()
        << " ===" << std::endl;

    out << std::endl;
    out << "Group Key:" << std::endl;
    out << "  Session Type:            " << key.session_type().value()
        << std::endl;
    out << "  Peer Group:              " << key.peer_group_name().value()
        << std::endl;
    out << "  Egress Policy:           " << key.egress_policy_name().value()
        << std::endl;
    out << "  Route Filter:            " << key.route_filter_stmt_name().value()
        << std::endl;

    out << "  AFI:                     "
        << (key.afi_ipv4_negotiated().value() ? "IPv4" : "")
        << (key.afi_ipv4_negotiated().value() &&
                    key.afi_ipv6_negotiated().value()
                ? "+"
                : "")
        << (key.afi_ipv6_negotiated().value() ? "IPv6" : "") << std::endl;

    out << "  Add-Path Send:           " << boolStr(key.send_add_path().value())
        << std::endl;
    out << "  RR Client:               " << boolStr(key.is_rr_client().value())
        << std::endl;
    out << "  Confed Peer:             "
        << boolStr(key.is_confed_peer().value()) << std::endl;
    out << "  Out Delay:               " << key.out_delay_seconds().value()
        << "s" << std::endl;

    out << "  Link Bandwidth:          advertise="
        << (key.advertise_link_bandwidth().has_value()
                ? apache::thrift::util::enumNameSafe(
                      key.advertise_link_bandwidth().value())
                : "-")
        << ", receive="
        << (key.receive_link_bandwidth().has_value()
                ? apache::thrift::util::enumNameSafe(
                      key.receive_link_bandwidth().value())
                : "-")
        << ", bps="
        << (key.link_bandwidth_bps().has_value()
                ? std::to_string(key.link_bandwidth_bps().value())
                : "-")
        << std::endl;

    out << "  Remove Private ASN:      "
        << boolStr(key.remove_private_asn().value()) << std::endl;
    out << "  4-Byte AS Capable:       "
        << boolStr(key.as4_byte_capable().value()) << std::endl;
    out << "  Ext NH Encoding:         "
        << boolStr(key.ext_nh_encoding_capable().value()) << std::endl;
    out << "  Peer Override:           " << boolStr(key.peer_override().value())
        << std::endl;

    out << std::endl;
    out << "Runtime State:" << std::endl;
    out << "  Group State:               " << group.group_state().value()
        << std::endl;
    out << "  Member Count:              " << group.member_count().value()
        << std::endl;
    out << "  Number of In-Sync Members: " << group.in_sync_peer_count().value()
        << std::endl;
    out << "  Number of Detached Members: "
        << group.detached_peer_count().value() << std::endl;
    out << "  Last RIB Version:          "
        << group.last_seen_rib_version().value() << std::endl;

    const auto& stateCounts = group.peer_state_counts().value();
    if (!stateCounts.empty()) {
      out << std::endl;
      out << "Sub-State Breakdown:" << std::endl;
      for (const auto& [state, count] : stateCounts) {
        out << fmt::format("  {:<26} {}", state + ":", count) << std::endl;
      }
    }

    const auto& stats = group.stats().value();

    out << std::endl;
    out << "Stats:" << std::endl;
    out << "  Announcements(v4/v6):    "
        << stats.total_sent_announcements_ipv4().value() << "/"
        << stats.total_sent_announcements_ipv6().value() << std::endl;
    out << "  Withdrawals:             " << stats.group_withdrawals().value()
        << std::endl;
    out << "  Queue Blocks:            "
        << stats.group_total_queue_blocks().value()
        << " (total wait: " << stats.group_total_queue_wait_ms().value()
        << "ms)" << std::endl;
    out << "  Slow Peer Detachments:   "
        << stats.slow_peer_detachments().value() << std::endl;
    out << "  DFP Rejoins:             " << stats.dfp_rejoin_events().value()
        << std::endl;
    out << "  DSP Rejoins:             " << stats.dsp_rejoin_events().value()
        << std::endl;
    out << "  Lazy Clones:             " << stats.lazy_clone_events().value()
        << std::endl;
    out << "  Collapse Corrections:    "
        << stats.collapse_entries_corrected().value() << std::endl;

    out << std::endl;
    out << "Prefixes (post-policy RIB-OUT):" << std::endl;
    out << "  Total:                   "
        << stats.post_out_prefix_count().value() << std::endl;
    out << "  IPv4:                    "
        << stats.post_out_prefix_count_ipv4().value() << std::endl;
    out << "  IPv6:                    "
        << stats.post_out_prefix_count_ipv6().value() << std::endl;

    out << std::endl;
    out << "Diagnostics:" << std::endl;

    if (group.initial_dump_completion_time_ms().has_value()) {
      out << "  Initial Dump Completed:  "
          << epochMsToString(group.initial_dump_completion_time_ms().value())
          << std::endl;
    } else {
      out << "  Initial Dump Completed:  N/A" << std::endl;
    }

    out << "  Total Discrepancies:     " << group.total_discrepancies().value()
        << std::endl;

    const auto& peers = group.peers().value();
    if (!peers.empty()) {
      out << std::endl;
      out << "Peers:" << std::endl;

      Table table;
      table.setHeader(
          {"Peer Address",
           "State",
           "Sync",
           "Blocked",
           "Detached",
           "Type",
           "LastRibVer",
           "DetachRibVer",
           "QueueSize",
           "EntryCount"});

      for (const auto& peer : peers) {
        table.addRow(
            {peer.peer_addr().value(),
             peer.peer_state().value(),
             boolStr(peer.is_in_sync().value()),
             boolStr(peer.is_blocked().value()),
             boolStr(peer.is_detached().value()),
             peer.detach_type().has_value() ? peer.detach_type().value()
                                            : std::string("-"),
             std::to_string(peer.last_seen_rib_version().value()),
             peer.detached_rib_version().has_value()
                 ? std::to_string(peer.detached_rib_version().value())
                 : std::string("-"),
             std::to_string(peer.queue_size().value()),
             std::to_string(peer.entry_count().value())});
      }
      out << table << std::endl;
    }
  }
};

} // namespace facebook::fboss
