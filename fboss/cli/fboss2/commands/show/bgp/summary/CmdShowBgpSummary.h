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
#include <string>

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/summary/gen-cpp2/bgp_summary_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/Table.h"
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

    return createModel(sessions, config, drain_state);
  }

  void printOutput(const RetType& bgpSummary, std::ostream& out = std::cout) {
    const auto& config = bgpSummary.config().value();
    const auto& sessions = bgpSummary.sessions().value();
    const auto& drain_state = bgpSummary.drain_state().value();

    // prevent printing number with comma, e.g: 65,000
    out.imbue(std::locale("C"));

    out << "BGP summary information for VRF default" << std::endl;
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
    const auto& drain_state_value = drain_state.drain_state().value();
    out << fmt::format(
               "BGP Switch Drain State: {}", enumNameSafe(drain_state_value))
        << std::endl;

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
    uint64_t up_neighbors = 0, paths_rcvd = 0, paths_sent = 0;

    Table table;
    table.setHeader(
        {"Peer",
         "AS",
         "State",
         "GR",
         "PR",
         "PS",
         "Uptime",
         "Downtime",
         "Description",
         "Session ID",
         "Flaps"});
    for (const auto& bgpSession : sessions) {
      const auto& peer = bgpSession.peer().value();
      up_neighbors +=
          (folly::copy(peer.peer_state().value()) ==
           TBgpPeerState::ESTABLISHED);

      auto rcvd_prefix = *bgpSession.prepolicy_rcvd_prefix_count();
      auto sent_prefix = *bgpSession.postpolicy_sent_prefix_count();

      paths_rcvd += rcvd_prefix;
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
        // If the peer is IDLE but the num_resets() == 0, then the peer has
        // never been in ESTABLISHED state and hence does not have the
        // downtime yet.
        downtimeDurationString = utils::getPrettyElapsedTime(
            utils::getEpochFromDuration(*bgpSession.reset_time()));
      }

      table.addRow({
          bgpSession.peer_addr().value(),
          folly::to<std::string>(peer_remote_as),
          enumNameSafe(folly::copy(peer.peer_state().value())).substr(0, 4),
          folly::copy(peer.graceful().value()) ? "Y" : "N",
          folly::to<std::string>(rcvd_prefix),
          folly::to<std::string>(sent_prefix),
          uptimeDurationString,
          downtimeDurationString,
          bgpSession.description().value(),
          bgpSession.peer_bgp_id().value(),
          folly::to<std::string>(folly::copy(bgpSession.num_resets().value())),
      });
    }

    out << "Peers: UP - " << up_neighbors << ", TOTAL - " << total_neighbors
        << std::endl;
    out << "Paths: Received - " << paths_rcvd << ", Sent - " << paths_sent
        << std::endl;
    out << "Acronyms: PS - Prefixes Sent, PR - Prefixes Received, HT - Hold Time\n"
        << std::endl;
    out << table << std::endl;
  }

  // Move the data from bgp_thrift:TBgpSession to
  // bgp_summary_thrift:TBgpSummary
  RetType createModel(
      std::vector<TBgpSession>& sessions,
      TBgpLocalConfig& config,
      TBgpDrainState& drain_state) {
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
    return model;
  }
};

} // namespace facebook::fboss
