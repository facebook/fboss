/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/summary/egress/CmdShowBgpSummaryEgress.h"

#include <algorithm>
#include <chrono> // NOLINT(misc-include-cleaner)
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "thrift/lib/cpp/util/EnumUtils.h" // NOLINT(misc-include-cleaner)

namespace facebook::fboss {

CmdShowBgpSummaryEgress::RetType CmdShowBgpSummaryEgress::queryClient(
    const HostInfo& hostInfo) {
  std::vector<TPeerEgressStats> peerEgressStats;

  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  client->sync_getPeerEgressStats(peerEgressStats);

  return createModel(peerEgressStats);
}

void CmdShowBgpSummaryEgress::printOutput(
    const RetType& model,
    std::ostream& out) {
  const auto& peerEgressStats = model.peer_egress_stats().value();

  out.imbue(std::locale("C"));

  printGroupSummary(peerEgressStats, out);
  out << std::endl;
  out << std::endl;
  printPeerSummary(peerEgressStats, out);
}

Table CmdShowBgpSummaryEgress::makePeerTable(
    const std::vector<TPeerEgressStats>& peerEgressStats) {
  Table table;
  table.setHeader({
      "Peer",
      "Description",
      "Group Name",
      "State",
      "Uptime",
      "Downtime",
      "PR",
      "PS",
      "UR",
      "US",
      "SU",
      "IQ Blocks",
      "Total IQ Wait (ms)",
      "Last IQ Block",
      "SQ Blocks",
      "Total SQ Wait (ms)",
      "Last SQ Block",
      "Socket Buffered",
      "Last Socket Buffered",
  });

  for (const auto& stats : peerEgressStats) {
    if (!stats.session().has_value()) {
      continue;
    }

    const auto& session = stats.session().value();
    const auto& peer = session.peer().value();

    std::string uptimeDurationString;
    if (*peer.peer_state() == TBgpPeerState::ESTABLISHED) {
      uptimeDurationString = utils::getPrettyElapsedTime(
          utils::getEpochFromDuration(*session.uptime()));
    }

    std::string downtimeDurationString;
    if (((*peer.peer_state() == TBgpPeerState::IDLE ||
          *peer.peer_state() == TBgpPeerState::IDLE_ADMIN)) &&
        (*session.num_resets() > 0)) {
      downtimeDurationString = utils::getPrettyElapsedTime(
          utils::getEpochFromDuration(*session.reset_time()));
    }

    /* PR, PS, UR, US, SU */
    auto rcvd_prefix = *session.prepolicy_rcvd_prefix_count();
    auto sent_prefix = *session.postpolicy_sent_prefix_count();
    auto recv_update_msgs = *session.recv_update_msgs();
    auto sent_update_msgs = *session.sent_update_msgs();
    auto suppressed_updates =
        stats.transient_route_updates_suppressed().value_or(0);

    /* iqueue (boundedAdjRibOutQueue_) related stats */
    auto iq_blocks = stats.adjribout_queue_blocks().value_or(0);
    auto iq_wait = stats.adjribout_queue_total_block_duration().value_or(0);
    std::string last_iq_block;
    if (stats.last_adjribout_queue_block_time() &&
        *stats.last_adjribout_queue_block_time() > 0) {
      last_iq_block =
          utils::parseTimeToTimeStamp(*stats.last_adjribout_queue_block_time());
    }

    /* sendQueue_ related stats */
    auto sq_blocks = stats.send_queue_blocks().value_or(0);
    auto sq_wait = stats.send_queue_total_block_duration().value_or(0);
    std::string last_sq_block;
    if (stats.last_send_queue_block_time() &&
        *stats.last_send_queue_block_time() > 0) {
      last_sq_block =
          utils::parseTimeToTimeStamp(*stats.last_send_queue_block_time());
    }

    /* async socket stats */
    auto sb = stats.total_async_socket_buffered().value_or(0);
    std::string last_sb_time;
    if (stats.last_socket_buffered_time() &&
        *stats.last_socket_buffered_time() > 0) {
      last_sb_time =
          utils::parseTimeToTimeStamp(*stats.last_socket_buffered_time());
    }

    table.addRow({
        session.peer_addr().value(), /* Peer */
        session.description().value(), /* Description */
        stats.group_name().value_or(kNoGroupName), /* Group Name */
        enumNameSafe(folly::copy(peer.peer_state().value()))
            .substr(0, 4), /* State */
        uptimeDurationString, /* Uptime */
        downtimeDurationString, /* Downtime */
        folly::to<std::string>(rcvd_prefix), /* PR */
        folly::to<std::string>(sent_prefix), /* PS */
        folly::to<std::string>(recv_update_msgs), /* UR */
        folly::to<std::string>(sent_update_msgs), /* US */
        folly::to<std::string>(suppressed_updates), /* SU */
        folly::to<std::string>(iq_blocks), /* IQ Blocks */
        folly::to<std::string>(iq_wait), /* Total IQ Wait */
        last_iq_block, /* Last IQ Block */
        folly::to<std::string>(sq_blocks), /* SQ Blocks */
        folly::to<std::string>(sq_wait), /* Total SQ Wait */
        last_sq_block, /* Last SQ Block */
        folly::to<std::string>(sb), /* Socket Buffered */
        last_sb_time, /* Last Socket Buffered */
    });
  }
  return table;
}

void CmdShowBgpSummaryEgress::printPeerSummary(
    const std::vector<TPeerEgressStats>& peerEgressStats,
    std::ostream& out) {
  out << "BGP Peer Egress Summary" << std::endl;
  out << "Acronyms: PR - Prefixes Received, PS - Prefixes Sent, "
      << "UR - Update Messages Received, US - Update Messages Sent, \n"
      << "SU - Suppressed Updates (updates not sent due to packing state compression), "
      << "IQ - Per-peer Input Queue to BGP I/O thread, "
      << "SQ - Per-peer Send Queue to socket \n"
      << std::endl;

  out << makePeerTable(peerEgressStats) << std::endl;
}

Table CmdShowBgpSummaryEgress::makeGroupTable(
    const std::vector<TPeerEgressStats>& peerEgressStats) {
  Table groupSummaryView;
  groupSummaryView.setHeader(
      {"Percentile",
       "Group Name",
       "PR",
       "PS",
       "UR",
       "US",
       "SU",
       "IQ Blocks",
       "Total IQ Wait (ms)",
       "Last IQ Block",
       "SQ Blocks",
       "Total SQ Wait (ms)",
       "Last SQ Block",
       "Socket Buffered",
       "Last Socket Buffered"});

  std::map<std::string, std::vector<std::vector<int64_t>>> groupedValues;

  /* Group all non-string data by @group_name. */
  for (const auto& stats : peerEgressStats) {
    if (!stats.session().has_value()) {
      continue;
    }

    const auto& session = *stats.session();

    /* Skip IDLE and IDLE_ADMIN peers from percentile calculations. */
    const auto& peerState = session.peer()->peer_state();
    if (*peerState == TBgpPeerState::IDLE ||
        *peerState == TBgpPeerState::IDLE_ADMIN) {
      continue;
    }

    std::string group = stats.group_name().value_or(kNoGroupName);

    /* Get group entry if it exists; otherwise create new one in map. */
    if (groupedValues.find(group) == groupedValues.end()) {
      groupedValues[group] =
          std::vector<std::vector<int64_t>>(PeerMetric::MAX_VALUE);
    }

    auto& metricVectors = groupedValues[group];

    /* PR, PS, UR, US, SU */
    metricVectors[PeerMetric::PrefixesRcvd].push_back(
        *session.prepolicy_rcvd_prefix_count());
    metricVectors[PeerMetric::PrefixesSent].push_back(
        *session.postpolicy_sent_prefix_count());
    metricVectors[PeerMetric::UpdatesRcvd].push_back(
        *session.recv_update_msgs());
    metricVectors[PeerMetric::UpdatesSent].push_back(
        *session.sent_update_msgs());
    metricVectors[PeerMetric::SuppressedUpdates].push_back(
        stats.transient_route_updates_suppressed().value_or(0));

    /* iqueue metrics */
    metricVectors[PeerMetric::TotalAdjRibOutQueueBlocks].push_back(
        stats.adjribout_queue_blocks().value_or(0));
    metricVectors[PeerMetric::TotalAdjRibOutQueueWait].push_back(
        stats.adjribout_queue_total_block_duration().value_or(0));
    metricVectors[PeerMetric::LastAdjRibOutQueueBlock].push_back(
        stats.last_adjribout_queue_block_time().value_or(0));

    /* sendQueue metrics */
    metricVectors[PeerMetric::TotalSendQueueBlocks].push_back(
        stats.send_queue_blocks().value_or(0));
    metricVectors[PeerMetric::TotalSendQueueWait].push_back(
        stats.send_queue_total_block_duration().value_or(0));
    metricVectors[PeerMetric::LastSendQueueBlock].push_back(
        stats.last_send_queue_block_time().value_or(0));

    /* async socket metrics */
    metricVectors[PeerMetric::TotalSocketBuffered].push_back(
        stats.total_async_socket_buffered().value_or(0));
    metricVectors[PeerMetric::LastSocketBuffered].push_back(
        stats.last_socket_buffered_time().value_or(0));
  }

  /* Sort data for percentile computation. */
  for (auto& [group, metricVectors] : groupedValues) {
    for (int m = 0; m < metricVectors.size(); ++m) {
      auto& vec = metricVectors[m];
      std::sort(vec.begin(), vec.end());
    }
  }

  std::vector<std::string> percentiles = {"p50", "p95", "p99"};
  std::vector<double> percentileValues = {0.50, 0.95, 0.99};

  for (const auto& [group, metricVectors] : groupedValues) {
    for (size_t p = 0; p < percentiles.size(); ++p) {
      std::vector<std::string> row;
      row.push_back(percentiles[p]);
      row.push_back(group);

      for (size_t i = 0; i < metricVectors.size(); ++i) {
        const auto& vec = metricVectors[i];
        if (vec.empty()) {
          row.emplace_back("0");
        } else {
          size_t index =
              static_cast<size_t>((vec.size() - 1) * percentileValues[p]);
          auto& val = vec[index];
          if (kTimeMetrics.contains(static_cast<PeerMetric>(i))) {
            row.push_back(
                folly::to<std::string>(
                    val > 0 ? utils::parseTimeToTimeStamp(val) : ""));
          } else {
            row.push_back(folly::to<std::string>(val));
          }
        }
      }

      groupSummaryView.addRow({
          row[0], // Percentile
          row[1], // Group Name
          row[2], // PR
          row[3], // PS
          row[4], // UR
          row[5], // US
          row[6], // SU
          row[7], // IQ Blocks
          row[8], // Total IQ Wait
          row[9], // Last IQ Block
          row[10], // SQ Blocks
          row[11], // Total SQ Wait
          row[12], // Last SQ Block
          row[13], // Total Socket Buffered
          row[14], // Last Socket Buffered
      });
    }
  }
  return groupSummaryView;
}

void CmdShowBgpSummaryEgress::printGroupSummary(
    const std::vector<TPeerEgressStats>& peerEgressStats,
    std::ostream& out) {
  out << "BGP Peer Egress Summary by Group Percentiles" << std::endl;
  out << "Acronyms: PR - Prefixes Received, PS - Prefixes Sent, "
      << "UR - Update Messages Received, US - Update Messages Sent, \n"
      << "SU - Suppressed Updates (updates not sent due to packing state compression), "
      << "IQ - Per-peer Input Queue to BGP I/O thread, "
      << "SQ - Per-peer Send Queue to socket \n"
      << std::endl;
  out << makeGroupTable(peerEgressStats) << std::endl;
}

CmdShowBgpSummaryEgress::RetType CmdShowBgpSummaryEgress::createModel(
    std::vector<TPeerEgressStats>& peerEgressStats) {
  std::sort(
      peerEgressStats.begin(),
      peerEgressStats.end(),
      [](const TPeerEgressStats& a, const TPeerEgressStats& b) {
        if (!a.session().has_value() || !b.session().has_value()) {
          return false;
        }
        return a.session()->peer_addr().value() <
            b.session()->peer_addr().value();
      });

  RetType model;
  model.peer_egress_stats() = peerEgressStats;
  return model;
}

} // namespace facebook::fboss
