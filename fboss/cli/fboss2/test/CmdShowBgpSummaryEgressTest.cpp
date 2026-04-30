// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/reflection/testing.h> // NOLINT(misc-include-cleaner)
#include <string_view>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/commands/show/bgp/summary/egress/CmdShowBgpSummaryEgress.h"
#include "fboss/cli/fboss2/commands/show/bgp/summary/gen-cpp2/bgp_summary_types.h"
#include "fboss/cli/fboss2/test/CmdBgpTestUtils.h"
#ifndef IS_OSS
#include "nettools/common/TestUtils.h"
#endif

using namespace ::testing;
using facebook::neteng::fboss::bgp::thrift::TBgpSession;
using facebook::neteng::fboss::bgp::thrift::TPeerEgressStats;

namespace facebook::fboss {

class CmdShowBgpSummaryEgressTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<TPeerEgressStats> peerEgressStats_;
  std::vector<TPeerEgressStats> largeDataset_;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    peerEgressStats_ = getBasicTestData();
    largeDataset_ = getLargeTestData();
  }

  std::vector<TPeerEgressStats> getBasicTestData() {
    TBgpSession establishedSession;

    auto establishedPeer = establishedSession.peer();
    establishedPeer->peer_state() = kEstablishedPeerState;
    establishedPeer->remote_as_4_byte() = kEstablishedRemoteAS;
    establishedPeer->graceful() = kEstablishedIsGraceful;

    establishedSession.prepolicy_rcvd_prefix_count() =
        kEstablishedPrePolicyRcvdPrefixCount;
    establishedSession.postpolicy_sent_prefix_count() =
        kEstablishedPostPolicySentPrefixCount;

    establishedSession.peer_addr() = kEstablishedPeerAddress;
    establishedSession.uptime() = kUptime;
    establishedSession.reset_time() = kEstablishedDowntime;
    establishedSession.num_resets() = kEstablishedNumResets;
    establishedSession.description() = kEstablishedDescription;
    establishedSession.peer_bgp_id() = kEstablishedRemoteBgpId;
    establishedSession.recv_update_msgs() = kEstablishedRecvUpdateMsgs;
    establishedSession.sent_update_msgs() = kEstablishedSentUpdateMsgs;

    TPeerEgressStats establishedStats;
    establishedStats.session() = establishedSession;
    establishedStats.group_name() = kEstablishedGroupName;
    establishedStats.last_adjribout_queue_block_time() =
        kEstablishedLastSocketWrite;
    establishedStats.adjribout_queue_blocks() = kEstablishedBackpressureEvents;
    establishedStats.transient_route_updates_suppressed() =
        kEstablishedSuppressedUpdates;

    TBgpSession idleSession;

    auto idlePeer = idleSession.peer();
    idlePeer->peer_state() = kIdlePeerState;
    idlePeer->remote_as_4_byte() = kIdleRemoteAS;
    idlePeer->graceful() = kIdleIsGraceful;

    idleSession.prepolicy_rcvd_prefix_count() = kIdlePrePolicyRcvdPrefixCount;
    idleSession.postpolicy_sent_prefix_count() = kIdlePostPolicySentPrefixCount;

    idleSession.peer_addr() = kIdlePeerAddress;
    idleSession.uptime() = kUptime;
    idleSession.reset_time() = kIdleDowntime;
    idleSession.num_resets() = kIdleNumResets;
    idleSession.description() = kIdleDescription;
    idleSession.peer_bgp_id() = kIdleRemoteBgpId;
    idleSession.recv_update_msgs() = kIdleRecvUpdateMsgs;
    idleSession.sent_update_msgs() = kIdleSentUpdateMsgs;

    TPeerEgressStats idleStats;
    idleStats.session() = idleSession;
    idleStats.group_name() = kIdleGroupName;
    idleStats.adjribout_queue_blocks() = kIdleBackpressureEvents;
    idleStats.transient_route_updates_suppressed() = kIdleSuppressedUpdates;

    return {establishedStats, idleStats};
  }

  std::vector<TPeerEgressStats> getLargeTestData(int numStats = 100) {
    std::vector<TPeerEgressStats> result;

    for (int i = 1; i <= numStats; ++i) {
      TBgpSession session;
      TBgpPeer peer;
      peer.peer_state() = kEstablishedPeerState;
      peer.remote_as_4_byte() = i;
      peer.graceful() = true;
      session.peer() = peer;

      session.peer_addr() = fmt::format("{}.{}.{}.{}", i, i + 1, i + 2, i + 3);
      session.uptime() = i * 10;
      session.reset_time() = 0;
      session.description() = kIbgpDescription;
      session.peer_bgp_id() =
          fmt::format("{}.{}.{}.{}", i + 3, i + 2, i + 1, i);
      session.prepolicy_rcvd_prefix_count() = 0;
      session.postpolicy_sent_prefix_count() = 0;
      session.recv_update_msgs() = i * 5;
      session.sent_update_msgs() = i * 10;

      TPeerEgressStats stats;
      stats.group_name() = kIbgpGroupName;
      stats.session() = session;
      stats.transient_route_updates_suppressed() = i;
      stats.adjribout_queue_blocks() = i;
      stats.adjribout_queue_total_block_duration() = i;
      stats.send_queue_blocks() = i;
      stats.send_queue_total_block_duration() = i;
      stats.total_async_socket_buffered() = i;

      auto iMinutes = i * 60 * 1000;
      stats.last_adjribout_queue_block_time() = iMinutes;
      stats.last_send_queue_block_time() = iMinutes;
      stats.last_socket_buffered_time() = iMinutes;

      result.push_back(stats);
    }

    TBgpSession idleSession;
    TBgpPeer idlePeer;
    idlePeer.peer_state() = kIdlePeerState;
    idlePeer.remote_as_4_byte() = kIdleRemoteAS;
    idlePeer.graceful() = kIdleIsGraceful;
    idleSession.peer() = idlePeer;

    idleSession.prepolicy_rcvd_prefix_count() = kIdlePrePolicyRcvdPrefixCount;
    idleSession.postpolicy_sent_prefix_count() = kIdlePostPolicySentPrefixCount;
    idleSession.peer_addr() = "200.201.202.203";
    idleSession.uptime() = 0;
    idleSession.reset_time() = kIdleDowntime;
    idleSession.num_resets() = kIdleNumResets;
    idleSession.description() = kIdleDescription;
    idleSession.peer_bgp_id() = "203.202.201.200";
    idleSession.recv_update_msgs() = kIdleRecvUpdateMsgs;
    idleSession.sent_update_msgs() = kIdleSentUpdateMsgs;

    TPeerEgressStats idleStats;
    idleStats.session() = idleSession;
    idleStats.group_name() = kIdleGroupName;
    idleStats.last_adjribout_queue_block_time() = kIdleLastSocketWrite;
    idleStats.adjribout_queue_blocks() = kIdleBackpressureEvents;
    idleStats.transient_route_updates_suppressed() = kIdleSuppressedUpdates;
    idleStats.send_queue_blocks() = 0;
    idleStats.send_queue_total_block_duration() = 0;
    idleStats.last_send_queue_block_time() = 0;
    idleStats.total_async_socket_buffered() = 0;
    idleStats.last_socket_buffered_time() = 0;
    idleStats.adjribout_queue_total_block_duration() = 0;

    result.push_back(idleStats);

    return result;
  }

  cli::ShowBgpSummaryModel getBasicModel() {
    cli::ShowBgpSummaryModel model;
    model.peer_egress_stats() = getBasicTestData();
    return model;
  }

  cli::ShowBgpSummaryModel getLargeModel(int numStats = 100) {
    cli::ShowBgpSummaryModel model;
    auto stats = getLargeTestData(numStats);
    std::sort(
        stats.begin(),
        stats.end(),
        [](const TPeerEgressStats& a, const TPeerEgressStats& b) {
          if (!a.session().has_value() || !b.session().has_value()) {
            return false;
          }
          return a.session()->peer_addr().value() <
              b.session()->peer_addr().value();
        });
    model.peer_egress_stats() = stats;
    return model;
  }
};

TEST_F(CmdShowBgpSummaryEgressTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getPeerEgressStats(_))
      .WillOnce(Invoke([&](auto& entries) { entries = peerEgressStats_; }));

  auto results = CmdShowBgpSummaryEgress().queryClient(localhost());
  EXPECT_THRIFT_EQ(results, getBasicModel());
}

TEST_F(CmdShowBgpSummaryEgressTestFixture, printGroupSummary) {
  std::stringstream ss;
  CmdShowBgpSummaryEgress().printGroupSummary(
      *getLargeModel().peer_egress_stats(), ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP Peer Egress Summary by Group Percentiles\n"
      "Acronyms: PR - Prefixes Received, PS - Prefixes Sent, UR - Update Messages Received, US - Update Messages Sent, \n"
      "SU - Suppressed Updates (updates not sent due to packing state compression), IQ - Per-peer Input Queue to BGP I/O thread, SQ - Per-peer Send Queue to socket \n\n"
      " Percentile  Group Name  PR  PS  UR   US   SU  IQ Blocks  Total IQ Wait (ms)  Last IQ Block              SQ Blocks  Total SQ Wait (ms)  Last SQ Block              Socket Buffered  Last Socket Buffered      \n"
      "------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
      " p50         IBGP_GROUP  0   0   250  500  50  50         50                  1969-12-31 16:50:00.0 PST  50         50                  1969-12-31 16:50:00.0 PST  50               1969-12-31 16:50:00.0 PST \n"
      " p95         IBGP_GROUP  0   0   475  950  95  95         95                  1969-12-31 17:35:00.0 PST  95         95                  1969-12-31 17:35:00.0 PST  95               1969-12-31 17:35:00.0 PST \n"
      " p99         IBGP_GROUP  0   0   495  990  99  99         99                  1969-12-31 17:39:00.0 PST  99         99                  1969-12-31 17:39:00.0 PST  99               1969-12-31 17:39:00.0 PST \n\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowBgpSummaryEgressTestFixture, printPeerSummary) {
  std::stringstream ss;
  CmdShowBgpSummaryEgress().printPeerSummary(
      *getLargeModel(5).peer_egress_stats(), ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP Peer Egress Summary\n"
      "Acronyms: PR - Prefixes Received, PS - Prefixes Sent, UR - Update Messages Received, US - Update Messages Sent, \n"
      "SU - Suppressed Updates (updates not sent due to packing state compression), IQ - Per-peer Input Queue to BGP I/O thread, SQ - Per-peer Send Queue to socket \n\n"
      " Peer             Description           Group Name  State  Uptime    Downtime  PR  PS  UR  US  SU  IQ Blocks  Total IQ Wait (ms)  Last IQ Block              SQ Blocks  Total SQ Wait (ms)  Last SQ Block              Socket Buffered  Last Socket Buffered      \n"
      "--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4          IBGP v4 Peers         IBGP_GROUP  ESTA   0h 0m 0s            0   0   5   10  1   1          1                   1969-12-31 16:01:00.0 PST  1          1                   1969-12-31 16:01:00.0 PST  1                1969-12-31 16:01:00.0 PST \n"
      " 2.3.4.5          IBGP v4 Peers         IBGP_GROUP  ESTA   0h 0m 0s            0   0   10  20  2   2          2                   1969-12-31 16:02:00.0 PST  2          2                   1969-12-31 16:02:00.0 PST  2                1969-12-31 16:02:00.0 PST \n"
      " 200.201.202.203  fsw007.p015.f01.prn6  IDLE_GROUP  IDLE             0h 0m 2s  0   0   50  75  15  20         0                                              0          0                                              0                                          \n"
      " 3.4.5.6          IBGP v4 Peers         IBGP_GROUP  ESTA   0h 0m 0s            0   0   15  30  3   3          3                   1969-12-31 16:03:00.0 PST  3          3                   1969-12-31 16:03:00.0 PST  3                1969-12-31 16:03:00.0 PST \n"
      " 4.5.6.7          IBGP v4 Peers         IBGP_GROUP  ESTA   0h 0m 0s            0   0   20  40  4   4          4                   1969-12-31 16:04:00.0 PST  4          4                   1969-12-31 16:04:00.0 PST  4                1969-12-31 16:04:00.0 PST \n"
      " 5.6.7.8          IBGP v4 Peers         IBGP_GROUP  ESTA   0h 0m 0s            0   0   25  50  5   5          5                   1969-12-31 16:05:00.0 PST  5          5                   1969-12-31 16:05:00.0 PST  5                1969-12-31 16:05:00.0 PST \n\n";

  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
