/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/reflection/testing.h> // NOLINT(misc-include-cleaner)
#include <string_view>

#include "fboss/cli/fboss2/commands/show/bgp/summary/CmdShowBgpSummary.h"
#include "fboss/cli/fboss2/commands/show/bgp/summary/gen-cpp2/bgp_summary_types.h"
#include "fboss/cli/fboss2/test/CmdBgpTestUtils.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#ifndef IS_OSS
// Avoid EXPECT_THRIFT_EQ clash with <thrift/lib/cpp2/reflection/testing.h>
#undef EXPECT_THRIFT_EQ
#include "nettools/common/TestUtils.h"
#endif

using namespace ::testing;
using facebook::neteng::fboss::bgp::thrift::TBgpDrainState;
using facebook::neteng::fboss::bgp::thrift::TBgpLocalConfig;
using facebook::neteng::fboss::bgp::thrift::TBgpSession;

namespace facebook::fboss {
// Constants for TBgpLocalConfig
const int kRouterId = 1234;
const int kLocalAS = 6000;
const int kLocalConfedAs = 700;
const bool kUCMPWeights = false;
const bool kEnableUpdateGroup = true;

// Constants for TBgpDrainState
const std::vector<std::string> kDrainedInterfaces = {"eth1/1/1", "eth1/2/1"};

class CmdShowBgpSummaryTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<TBgpSession> sessions_;
  TBgpLocalConfig localConfig_;
  TBgpDrainState drainState_;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    sessions_ = getQueriedSession();
    localConfig_ = getQueriedConfig();
    drainState_ = getQueriedDrainState();
  }

  std::vector<TBgpSession> getQueriedSession() {
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

    return {establishedSession, idleSession};
  }

  TBgpLocalConfig getQueriedConfig() {
    TBgpLocalConfig config;

    config.my_router_id() = kRouterId;
    config.local_as_4_byte() = kLocalAS;
    config.local_confed_as_4_byte() = kLocalConfedAs;
    config.program_ucmp_weights() = kUCMPWeights;
    config.enable_update_group() = kEnableUpdateGroup;

    return config;
  }

  TBgpDrainState getQueriedDrainState() {
    TBgpDrainState drain_state;

    drain_state.drain_state() =
        facebook::bgp::bgp_policy::DrainState::UNDRAINED;
    drain_state.drained_interfaces() = kDrainedInterfaces;

    return drain_state;
  }

  cli::ShowBgpSummaryModel getModelBgpSummary() {
    cli::ShowBgpSummaryModel model;

    model.sessions() = getQueriedSession();
    model.config() = getQueriedConfig();
    model.drain_state() = getQueriedDrainState();

    return model;
  }

  cli::ShowBgpSummaryModel getModelBgpSummaryOldAsn() {
    // TODO(michaeluw): T113736668 deprecate this when we deprecate i32 asns
    // field. To be safe, build from scratch instead of manipulating result of
    // getModelBgpSummary
    cli::ShowBgpSummaryModel model;

    TBgpLocalConfig config;

    config.my_router_id() = kRouterId;
    config.local_as() = kLocalAS;
    config.local_confed_as() = kLocalConfedAs;
    config.program_ucmp_weights() = kUCMPWeights;
    config.enable_update_group() = kEnableUpdateGroup;

    model.config() = std::move(config);

    TBgpSession establishedSession;

    auto establishedPeer = establishedSession.peer();
    establishedPeer->peer_state() = kEstablishedPeerState;
    establishedPeer->remote_as() = kEstablishedRemoteAS;
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

    TBgpSession idleSession;

    auto idlePeer = idleSession.peer();
    idlePeer->peer_state() = kIdlePeerState;
    idlePeer->remote_as() = kIdleRemoteAS;
    idlePeer->graceful() = kIdleIsGraceful;

    idleSession.prepolicy_rcvd_prefix_count() = kIdlePrePolicyRcvdPrefixCount;
    idleSession.postpolicy_sent_prefix_count() = kIdlePostPolicySentPrefixCount;

    idleSession.peer_addr() = kIdlePeerAddress;
    idleSession.uptime() = kUptime;
    idleSession.reset_time() = kIdleDowntime;
    idleSession.num_resets() = kIdleNumResets;
    idleSession.description() = kIdleDescription;
    idleSession.peer_bgp_id() = kIdleRemoteBgpId;

    model.sessions() = {std::move(establishedSession), std::move(idleSession)};

    TBgpDrainState drainState;

    drainState.drain_state() = facebook::bgp::bgp_policy::DrainState::DRAINED;
    drainState.drained_interfaces() = kDrainedInterfaces;
    model.drain_state() = std::move(drainState);

    return model;
  }
};

TEST_F(CmdShowBgpSummaryTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getBgpSessions(_))
      .WillOnce(Invoke([&](auto& entries) { entries = sessions_; }));

  EXPECT_CALL(getMockBgp(), getBgpLocalConfig(_))
      .WillOnce(Invoke([&](auto& entries) { entries = localConfig_; }));

  EXPECT_CALL(getMockBgp(), getDrainState(_))
      .WillOnce(Invoke([&](auto& entries) { entries = drainState_; }));

  auto results = CmdShowBgpSummary().queryClient(localhost());
  EXPECT_THRIFT_EQ(results, getModelBgpSummary());
}

TEST_F(CmdShowBgpSummaryTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowBgpSummary().printOutput(getModelBgpSummary(), ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP summary information for VRF default\n"
      "Router ID - 210.4.0.0, Local ASN - 6000, Confed ASN - 700\n"
      "BGP Switch Drain State: UNDRAINED\n"
      "UCMP weight programming is DISABLED\n"
      "Update group is ENABLED\n"
      "Peers: UP - 1, TOTAL - 2\n"
      "Paths: Received - 2, Accepted - 0, Sent - 2\n"
      "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent, UG - Update Group, UGPS - Update Group Peer State\n\n"
      " Peer     AS    State  GR  PR  PA  PS  UG  UGPS  Uptime    Downtime  Description           Session ID  Flaps \n"
      "----------------------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4  1     ESTA   Y   2   0   2   -   -     0h 0m 1s            fsw001.p015.f01.prn6  4.3.2.1     9     \n"
      " 2.3.4.5  4651  IDLE   N   0   0   0   -   -               0h 0m 2s  fsw007.p015.f01.prn6  6.7.8.9     33    \n\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowBgpSummaryTestFixture, printOutputWhenConfedAsnIsZero) {
  std::stringstream ss;
  auto model = getModelBgpSummary();
  model.config()->local_confed_as_4_byte() = 0; // zero confed asn
  CmdShowBgpSummary().printOutput(model, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP summary information for VRF default\n"
      "Router ID - 210.4.0.0, Local ASN - 6000\n"
      "BGP Switch Drain State: UNDRAINED\n"
      "UCMP weight programming is DISABLED\n"
      "Update group is ENABLED\n"
      "Peers: UP - 1, TOTAL - 2\n"
      "Paths: Received - 2, Accepted - 0, Sent - 2\n"
      "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent, UG - Update Group, UGPS - Update Group Peer State\n\n"
      " Peer     AS    State  GR  PR  PA  PS  UG  UGPS  Uptime    Downtime  Description           Session ID  Flaps \n"
      "----------------------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4  1     ESTA   Y   2   0   2   -   -     0h 0m 1s            fsw001.p015.f01.prn6  4.3.2.1     9     \n"
      " 2.3.4.5  4651  IDLE   N   0   0   0   -   -               0h 0m 2s  fsw007.p015.f01.prn6  6.7.8.9     33    \n\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowBgpSummaryTestFixture, printOutputWithOldAsnField) {
  // TODO(michaeluw): deprecate this when we deprecate i32 asns field
  // Test with only old field present
  // s T113736668
  std::stringstream ss;
  CmdShowBgpSummary().printOutput(getModelBgpSummaryOldAsn(), ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP summary information for VRF default\n"
      "Router ID - 210.4.0.0, Local ASN - 6000, Confed ASN - 700\n"
      "BGP Switch Drain State: DRAINED\n"
      "UCMP weight programming is DISABLED\n"
      "Update group is ENABLED\n"
      "Peers: UP - 1, TOTAL - 2\n"
      "Paths: Received - 2, Accepted - 0, Sent - 2\n"
      "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent, UG - Update Group, UGPS - Update Group Peer State\n\n"
      " Peer     AS    State  GR  PR  PA  PS  UG  UGPS  Uptime    Downtime  Description           Session ID  Flaps \n"
      "----------------------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4  1     ESTA   Y   2   0   2   -   -     0h 0m 1s            fsw001.p015.f01.prn6  4.3.2.1     9     \n"
      " 2.3.4.5  4651  IDLE   N   0   0   0   -   -               0h 0m 2s  fsw007.p015.f01.prn6  6.7.8.9     33    \n\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowBgpSummaryTestFixture, printOutputBothAsnFieldPresent) {
  // TODO(michaeluw): deprecate this when we deprecate i32 asns field
  // Test with only new and old field present
  // s T113736668
  std::stringstream ss;
  auto model = getModelBgpSummary();
  model.config()->local_as() = 123;
  model.config()->local_confed_as() = 123;
  CmdShowBgpSummary().printOutput(model, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP summary information for VRF default\n"
      "Router ID - 210.4.0.0, Local ASN - 6000, Confed ASN - 700\n"
      "BGP Switch Drain State: UNDRAINED\n"
      "UCMP weight programming is DISABLED\n"
      "Update group is ENABLED\n"
      "Peers: UP - 1, TOTAL - 2\n"
      "Paths: Received - 2, Accepted - 0, Sent - 2\n"
      "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent, UG - Update Group, UGPS - Update Group Peer State\n\n"
      " Peer     AS    State  GR  PR  PA  PS  UG  UGPS  Uptime    Downtime  Description           Session ID  Flaps \n"
      "----------------------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4  1     ESTA   Y   2   0   2   -   -     0h 0m 1s            fsw001.p015.f01.prn6  4.3.2.1     9     \n"
      " 2.3.4.5  4651  IDLE   N   0   0   0   -   -               0h 0m 2s  fsw007.p015.f01.prn6  6.7.8.9     33    \n\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowBgpSummaryTestFixture, printOutputUpdateGroupDisabled) {
  // When update-group is disabled, the UG/UGPS columns and their acronyms are
  // omitted entirely.
  std::stringstream ss;
  auto model = getModelBgpSummary();
  model.config()->enable_update_group() = false;
  CmdShowBgpSummary().printOutput(model, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP summary information for VRF default\n"
      "Router ID - 210.4.0.0, Local ASN - 6000, Confed ASN - 700\n"
      "BGP Switch Drain State: UNDRAINED\n"
      "UCMP weight programming is DISABLED\n"
      "Update group is DISABLED\n"
      "Peers: UP - 1, TOTAL - 2\n"
      "Paths: Received - 2, Accepted - 0, Sent - 2\n"
      "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent\n\n"
      " Peer     AS    State  GR  PR  PA  PS  Uptime    Downtime  Description           Session ID  Flaps \n"
      "----------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4  1     ESTA   Y   2   0   2   0h 0m 1s            fsw001.p015.f01.prn6  4.3.2.1     9     \n"
      " 2.3.4.5  4651  IDLE   N   0   0   0             0h 0m 2s  fsw007.p015.f01.prn6  6.7.8.9     33    \n\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowBgpSummaryTestFixture, printOutputWithUpdateGroupAndPaValues) {
  // Update-group id / peer-state and the accepted-prefix count render their
  // actual values (UG/UGPS columns and PA column).
  std::stringstream ss;
  auto model = getModelBgpSummary();
  model.sessions()[0].update_group_id() = 5;
  model.sessions()[0].peer_state_update_group() = "Converged";
  model.sessions()[0].postpolicy_rcvd_prefix_count() = 2;
  model.sessions()[1].update_group_id() = 7;
  model.sessions()[1].peer_state_update_group() = "Pending";
  CmdShowBgpSummary().printOutput(model, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP summary information for VRF default\n"
      "Router ID - 210.4.0.0, Local ASN - 6000, Confed ASN - 700\n"
      "BGP Switch Drain State: UNDRAINED\n"
      "UCMP weight programming is DISABLED\n"
      "Update group is ENABLED\n"
      "Peers: UP - 1, TOTAL - 2\n"
      "Paths: Received - 2, Accepted - 2, Sent - 2\n"
      "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent, UG - Update Group, UGPS - Update Group Peer State\n\n"
      " Peer     AS    State  GR  PR  PA  PS  UG  UGPS       Uptime    Downtime  Description           Session ID  Flaps \n"
      "---------------------------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4  1     ESTA   Y   2   2   2   5   Converged  0h 0m 1s            fsw001.p015.f01.prn6  4.3.2.1     9     \n"
      " 2.3.4.5  4651  IDLE   N   0   0   0   7   Pending              0h 0m 2s  fsw007.p015.f01.prn6  6.7.8.9     33    \n\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowBgpSummaryTestFixture, printOutputNoDowntimeColumn) {
  // When no peer has a downtime value, the Downtime column is omitted.
  std::stringstream ss;
  auto model = getModelBgpSummary();
  // Idle peer with num_resets == 0 has never been established -> no downtime.
  model.sessions()[1].num_resets() = 0;
  CmdShowBgpSummary().printOutput(model, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP summary information for VRF default\n"
      "Router ID - 210.4.0.0, Local ASN - 6000, Confed ASN - 700\n"
      "BGP Switch Drain State: UNDRAINED\n"
      "UCMP weight programming is DISABLED\n"
      "Update group is ENABLED\n"
      "Peers: UP - 1, TOTAL - 2\n"
      "Paths: Received - 2, Accepted - 0, Sent - 2\n"
      "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent, UG - Update Group, UGPS - Update Group Peer State\n\n"
      " Peer     AS    State  GR  PR  PA  PS  UG  UGPS  Uptime    Description           Session ID  Flaps \n"
      "-----------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4  1     ESTA   Y   2   0   2   -   -     0h 0m 1s  fsw001.p015.f01.prn6  4.3.2.1     9     \n"
      " 2.3.4.5  4651  IDLE   N   0   0   0   -   -               fsw007.p015.f01.prn6  6.7.8.9     0     \n\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowBgpSummaryTestFixture, printOutputDrainStateNotSet) {
  // An unset drain state must print "N/A" rather than crash on .value().
  std::stringstream ss;
  auto model = getModelBgpSummary();
  model.drain_state()->drain_state().reset();
  CmdShowBgpSummary().printOutput(model, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP summary information for VRF default\n"
      "Router ID - 210.4.0.0, Local ASN - 6000, Confed ASN - 700\n"
      "BGP Switch Drain State: N/A\n"
      "UCMP weight programming is DISABLED\n"
      "Update group is ENABLED\n"
      "Peers: UP - 1, TOTAL - 2\n"
      "Paths: Received - 2, Accepted - 0, Sent - 2\n"
      "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent, UG - Update Group, UGPS - Update Group Peer State\n\n"
      " Peer     AS    State  GR  PR  PA  PS  UG  UGPS  Uptime    Downtime  Description           Session ID  Flaps \n"
      "----------------------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4  1     ESTA   Y   2   0   2   -   -     0h 0m 1s            fsw001.p015.f01.prn6  4.3.2.1     9     \n"
      " 2.3.4.5  4651  IDLE   N   0   0   0   -   -               0h 0m 2s  fsw007.p015.f01.prn6  6.7.8.9     33    \n\n";

  EXPECT_EQ(output, expectedOutput);
}

#ifndef IS_OSS
// Meta-internal drain states (WARM_DRAINED, SOFT_DRAINED) not available in OSS
TEST_F(CmdShowBgpSummaryTestFixture, printOutputWithDifferentDrainStates) {
  std::stringstream ss;
  auto model = getModelBgpSummary();
  model.drain_state()->drain_state() =
      bgp::bgp_policy::DrainState::WARM_DRAINED;
  CmdShowBgpSummary().printOutput(model, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP summary information for VRF default\n"
      "Router ID - 210.4.0.0, Local ASN - 6000, Confed ASN - 700\n"
      "BGP Switch Drain State: WARM_DRAINED\n"
      "UCMP weight programming is DISABLED\n"
      "Update group is ENABLED\n"
      "Peers: UP - 1, TOTAL - 2\n"
      "Paths: Received - 2, Accepted - 0, Sent - 2\n"
      "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent, UG - Update Group, UGPS - Update Group Peer State\n\n"
      " Peer     AS    State  GR  PR  PA  PS  UG  UGPS  Uptime    Downtime  Description           Session ID  Flaps \n"
      "----------------------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4  1     ESTA   Y   2   0   2   -   -     0h 0m 1s            fsw001.p015.f01.prn6  4.3.2.1     9     \n"
      " 2.3.4.5  4651  IDLE   N   0   0   0   -   -               0h 0m 2s  fsw007.p015.f01.prn6  6.7.8.9     33    \n\n";

  EXPECT_EQ(output, expectedOutput);

  model.drain_state()->drain_state() =
      bgp::bgp_policy::DrainState::SOFT_DRAINED;
  ss.str("");
  ss.clear();
  CmdShowBgpSummary().printOutput(model, ss);
  output = ss.str();

  expectedOutput =
      "BGP summary information for VRF default\n"
      "Router ID - 210.4.0.0, Local ASN - 6000, Confed ASN - 700\n"
      "BGP Switch Drain State: SOFT_DRAINED\n"
      "UCMP weight programming is DISABLED\n"
      "Update group is ENABLED\n"
      "Peers: UP - 1, TOTAL - 2\n"
      "Paths: Received - 2, Accepted - 0, Sent - 2\n"
      "Acronyms: PR - Prefixes Received, PA - Prefixes Accepted, PS - Prefixes Sent, UG - Update Group, UGPS - Update Group Peer State\n\n"
      " Peer     AS    State  GR  PR  PA  PS  UG  UGPS  Uptime    Downtime  Description           Session ID  Flaps \n"
      "----------------------------------------------------------------------------------------------------------------------------\n"
      " 1.2.3.4  1     ESTA   Y   2   0   2   -   -     0h 0m 1s            fsw001.p015.f01.prn6  4.3.2.1     9     \n"
      " 2.3.4.5  4651  IDLE   N   0   0   0   -   -               0h 0m 2s  fsw007.p015.f01.prn6  6.7.8.9     33    \n\n";

  EXPECT_EQ(output, expectedOutput);
}
#endif // IS_OSS

TEST_F(CmdShowBgpSummaryTestFixture, printOutputPrePolicyPrefixDrops) {
  // A non-zero pre-policy dropped count surfaces the PRD column.
  std::stringstream ss;
  auto model = getModelBgpSummary();
  model.sessions()[0].prepolicy_rcvd_dropped_prefix_count() = 50;
  CmdShowBgpSummary().printOutput(model, ss);
  std::string output = ss.str();

  EXPECT_THAT(output, HasSubstr("PRD"));
  EXPECT_THAT(output, HasSubstr("50"));
  EXPECT_THAT(output, HasSubstr("PRD - Prefixes Received Dropped"));
}

TEST_F(CmdShowBgpSummaryTestFixture, printOutputNoPrefixDrops) {
  // With no dropped routes, the PRD column and legend are omitted.
  std::stringstream ss;
  auto model = getModelBgpSummary();
  CmdShowBgpSummary().printOutput(model, ss);
  std::string output = ss.str();

  EXPECT_THAT(output, Not(HasSubstr("PRD")));
}

} // namespace facebook::fboss
