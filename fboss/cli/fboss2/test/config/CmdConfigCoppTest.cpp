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
#include <cstdint>
#include <stdexcept>
#include <string>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/copp/CmdConfigCopp.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Seed config mirrors the cpuQueues + cpuTrafficPolicy shape observed in a
// real production FBOSS agent.conf (rsw001_p001_m002_qzr1 near line 11076):
//   - cpuQueues[] holds 4 entries with per-queue weight, name, and an
//     optional portQueueRate.pktsPerSec cap on the low-priority queues.
//   - cpuTrafficPolicy.rxReasonToQueueOrderedList[] maps protocol reasons
//     to queue ids in priority order.
class CmdConfigCoppTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigCoppTestFixture()
      : CmdConfigTestBase(
            "fboss_copp_config_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "cpuQueues": [
      { "id": 9, "streamType": 1, "weight": 4, "scheduling": 0,
        "name": "cpuQueue-high" },
      { "id": 2, "streamType": 1, "weight": 2, "scheduling": 0,
        "name": "cpuQueue-mid" },
      { "id": 1, "streamType": 1, "weight": 1, "scheduling": 0,
        "name": "cpuQueue-default",
        "portQueueRate": { "pktsPerSec": { "minimum": 0, "maximum": 1000 } } },
      { "id": 0, "streamType": 1, "weight": 1, "scheduling": 0,
        "name": "cpuQueue-low",
        "portQueueRate": { "pktsPerSec": { "minimum": 0, "maximum": 500 } } }
    ],
    "cpuTrafficPolicy": {
      "rxReasonToQueueOrderedList": [
        { "rxReason": 8,  "queueId": 9 },
        { "rxReason": 1,  "queueId": 9 },
        { "rxReason": 11, "queueId": 9 },
        { "rxReason": 13, "queueId": 2 },
        { "rxReason": 0,  "queueId": 1 }
      ],
      "trafficPolicy": {
        "matchToAction": [
          { "matcher": "cpuPolicy-mid",
            "action": { "counter": "cpuPolicy-mid-counter" } }
        ]
      }
    }
  }
})") {}

 protected:
  const std::string cpuQueueCmdPrefix_ = "config copp cpu-queue";
  const std::string reasonCmdPrefix_ = "config copp reason";
  const std::string cpuTrafficPolicyCmdPrefix_ =
      "config copp cpu-traffic-policy";

  // Helper: find the cpuQueues entry with the given id, or nullptr.
  const cfg::PortQueue* findQueue(int16_t id) const {
    auto& config = ConfigSession::getInstance().getAgentConfig();
    for (const auto& q : *config.sw()->cpuQueues()) {
      if (*q.id() == id) {
        return &q;
      }
    }
    return nullptr;
  }

  // Helper: find the rxReasonToQueueOrderedList entry for a reason, or nullptr.
  const cfg::PacketRxReasonToQueue* findReason(cfg::PacketRxReason r) const {
    auto& config = ConfigSession::getInstance().getAgentConfig();
    const auto& policy = *config.sw()->cpuTrafficPolicy();
    for (const auto& entry : *policy.rxReasonToQueueOrderedList()) {
      if (*entry.rxReason() == r) {
        return &entry;
      }
    }
    return nullptr;
  }

  // Helper: find the matchToAction entry with the given matcher, or nullptr.
  const cfg::MatchToAction* findMatchToAction(const std::string& name) const {
    auto& config = ConfigSession::getInstance().getAgentConfig();
    const auto& policy = *config.sw()->cpuTrafficPolicy();
    if (!policy.trafficPolicy().has_value()) {
      return nullptr;
    }
    for (const auto& mta : *policy.trafficPolicy()->matchToAction()) {
      if (*mta.matcher() == name) {
        return &mta;
      }
    }
    return nullptr;
  }
};

// =============================================================
// CoppCpuQueueArgs validation tests
// =============================================================

TEST_F(CmdConfigCoppTestFixture, cpuQueueArgs_idOnly) {
  CoppCpuQueueArgs a({"0"});
  EXPECT_EQ(a.getQueueId(), 0);
  EXPECT_EQ(a.getOp(), CoppCpuQueueArgs::Op::NONE);

  CoppCpuQueueArgs b({"9"});
  EXPECT_EQ(b.getQueueId(), 9);
  EXPECT_EQ(b.getOp(), CoppCpuQueueArgs::Op::NONE);
}

TEST_F(CmdConfigCoppTestFixture, cpuQueueArgs_name) {
  CoppCpuQueueArgs a({"2", "name", "cpuQueue-mid"});
  EXPECT_EQ(a.getQueueId(), 2);
  EXPECT_EQ(a.getOp(), CoppCpuQueueArgs::Op::NAME);
  EXPECT_EQ(a.getName(), "cpuQueue-mid");
}

TEST_F(CmdConfigCoppTestFixture, cpuQueueArgs_rateLimitKbps) {
  CoppCpuQueueArgs a({"0", "rate-limit", "kbps", "1500"});
  EXPECT_EQ(a.getQueueId(), 0);
  EXPECT_EQ(a.getOp(), CoppCpuQueueArgs::Op::RATE_LIMIT_KBPS);
  EXPECT_EQ(a.getRateMax(), 1500);
}

TEST_F(CmdConfigCoppTestFixture, cpuQueueArgs_rateLimitPps) {
  CoppCpuQueueArgs a({"1", "rate-limit", "pps", "750"});
  EXPECT_EQ(a.getQueueId(), 1);
  EXPECT_EQ(a.getOp(), CoppCpuQueueArgs::Op::RATE_LIMIT_PPS);
  EXPECT_EQ(a.getRateMax(), 750);
}

TEST_F(CmdConfigCoppTestFixture, cpuQueueArgs_badArity) {
  EXPECT_THROW(CoppCpuQueueArgs({}), std::invalid_argument);
  EXPECT_THROW(CoppCpuQueueArgs({"0", "name"}), std::invalid_argument);
  EXPECT_THROW(
      CoppCpuQueueArgs({"0", "name", "foo", "extra"}), std::invalid_argument);
  EXPECT_THROW(
      CoppCpuQueueArgs({"0", "rate-limit", "kbps"}), std::invalid_argument);
  EXPECT_THROW(CoppCpuQueueArgs({"0", "rate-limit"}), std::invalid_argument);
}

TEST_F(CmdConfigCoppTestFixture, cpuQueueArgs_unknownSubCmd) {
  EXPECT_THROW(CoppCpuQueueArgs({"0", "weight", "4"}), std::invalid_argument);
  EXPECT_THROW(
      CoppCpuQueueArgs({"0", "rate-limit", "mbps", "100"}),
      std::invalid_argument);
}

TEST_F(CmdConfigCoppTestFixture, cpuQueueArgs_badValues) {
  EXPECT_THROW(CoppCpuQueueArgs({"abc"}), std::invalid_argument);
  EXPECT_THROW(CoppCpuQueueArgs({"-1"}), std::invalid_argument);
  EXPECT_THROW(CoppCpuQueueArgs({"999"}), std::invalid_argument);
  EXPECT_THROW(CoppCpuQueueArgs({"0", "name", ""}), std::invalid_argument);
  EXPECT_THROW(
      CoppCpuQueueArgs({"0", "rate-limit", "kbps", "abc"}),
      std::invalid_argument);
  EXPECT_THROW(
      CoppCpuQueueArgs({"0", "rate-limit", "pps", "-5"}),
      std::invalid_argument);
}

// =============================================================
// CoppReasonArgs validation tests
// =============================================================

TEST_F(CmdConfigCoppTestFixture, reasonArgs_valid) {
  CoppReasonArgs a({"arp", "queue", "9"});
  EXPECT_EQ(a.getReason(), cfg::PacketRxReason::ARP);
  EXPECT_EQ(a.getQueueId(), 9);

  // Case-insensitive + dash-to-underscore normalization
  CoppReasonArgs b({"BGP", "queue", "2"});
  EXPECT_EQ(b.getReason(), cfg::PacketRxReason::BGP);
  EXPECT_EQ(b.getQueueId(), 2);

  CoppReasonArgs c({"ttl-1", "queue", "0"});
  EXPECT_EQ(c.getReason(), cfg::PacketRxReason::TTL_1);
  EXPECT_EQ(c.getQueueId(), 0);

  CoppReasonArgs d({"dhcpv6", "queue", "2"});
  EXPECT_EQ(d.getReason(), cfg::PacketRxReason::DHCPV6);
  EXPECT_EQ(d.getQueueId(), 2);
}

TEST_F(CmdConfigCoppTestFixture, reasonArgs_badArity) {
  EXPECT_THROW(CoppReasonArgs({}), std::invalid_argument);
  EXPECT_THROW(CoppReasonArgs({"arp"}), std::invalid_argument);
  EXPECT_THROW(CoppReasonArgs({"arp", "queue"}), std::invalid_argument);
  EXPECT_THROW(
      CoppReasonArgs({"arp", "queue", "1", "extra"}), std::invalid_argument);
}

TEST_F(CmdConfigCoppTestFixture, reasonArgs_unknownReason) {
  EXPECT_THROW(
      CoppReasonArgs({"not-a-reason", "queue", "0"}), std::invalid_argument);
  EXPECT_THROW(CoppReasonArgs({"", "queue", "0"}), std::invalid_argument);
}

TEST_F(CmdConfigCoppTestFixture, reasonArgs_missingQueueKeyword) {
  EXPECT_THROW(CoppReasonArgs({"arp", "que", "9"}), std::invalid_argument);
}

TEST_F(CmdConfigCoppTestFixture, reasonArgs_badQueueId) {
  EXPECT_THROW(CoppReasonArgs({"arp", "queue", "abc"}), std::invalid_argument);
  EXPECT_THROW(CoppReasonArgs({"arp", "queue", "-1"}), std::invalid_argument);
  EXPECT_THROW(CoppReasonArgs({"arp", "queue", "999"}), std::invalid_argument);
}

// =============================================================
// queryClient() tests — cpu-queue
// =============================================================

TEST_F(CmdConfigCoppTestFixture, cpuQueue_ensureExists) {
  setupTestableConfigSession(cpuQueueCmdPrefix_, "9");
  CmdConfigCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  CoppCpuQueueArgs args({"9"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("9"));

  const auto* q = findQueue(9);
  ASSERT_NE(q, nullptr);
  // Existing queue's name must be preserved when no sub-command was given.
  EXPECT_EQ(q->name(), "cpuQueue-high");
}

TEST_F(CmdConfigCoppTestFixture, cpuQueue_createIfMissing) {
  setupTestableConfigSession(cpuQueueCmdPrefix_, "5");
  CmdConfigCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  ASSERT_EQ(findQueue(5), nullptr);

  CoppCpuQueueArgs args({"5"});
  cmd.queryClient(hostInfo, args);

  const auto* q = findQueue(5);
  ASSERT_NE(q, nullptr);
  EXPECT_EQ(*q->id(), 5);
}

TEST_F(CmdConfigCoppTestFixture, cpuQueue_setName) {
  setupTestableConfigSession(cpuQueueCmdPrefix_, "2 name cpuQueue-renamed");
  CmdConfigCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  CoppCpuQueueArgs args({"2", "name", "cpuQueue-renamed"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("cpuQueue-renamed"));

  const auto* q = findQueue(2);
  ASSERT_NE(q, nullptr);
  EXPECT_EQ(q->name(), "cpuQueue-renamed");
}

TEST_F(CmdConfigCoppTestFixture, cpuQueue_setRateLimitKbps) {
  setupTestableConfigSession(cpuQueueCmdPrefix_, "0 rate-limit kbps 1500");
  CmdConfigCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  CoppCpuQueueArgs args({"0", "rate-limit", "kbps", "1500"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("1500"));

  const auto* q = findQueue(0);
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->portQueueRate().has_value());
  // Switching from pps to kbps must clear the pps side of the union.
  ASSERT_TRUE(q->portQueueRate()->kbitsPerSec().has_value());
  EXPECT_EQ(*q->portQueueRate()->kbitsPerSec()->maximum(), 1500);
  EXPECT_EQ(*q->portQueueRate()->kbitsPerSec()->minimum(), 0);
  EXPECT_FALSE(q->portQueueRate()->pktsPerSec().has_value());
}

TEST_F(CmdConfigCoppTestFixture, cpuQueue_setRateLimitPps) {
  setupTestableConfigSession(cpuQueueCmdPrefix_, "1 rate-limit pps 2000");
  CmdConfigCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  CoppCpuQueueArgs args({"1", "rate-limit", "pps", "2000"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("2000"));

  const auto* q = findQueue(1);
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->portQueueRate().has_value());
  ASSERT_TRUE(q->portQueueRate()->pktsPerSec().has_value());
  EXPECT_EQ(*q->portQueueRate()->pktsPerSec()->maximum(), 2000);
}

// =============================================================
// queryClient() tests — reason
// =============================================================

TEST_F(CmdConfigCoppTestFixture, reason_updateExisting) {
  setupTestableConfigSession(reasonCmdPrefix_, "arp queue 2");
  CmdConfigCoppReason cmd;
  HostInfo hostInfo("testhost");

  // Seed maps ARP -> 9; update it to 2.
  CoppReasonArgs args({"arp", "queue", "2"});
  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("ARP"));

  const auto* entry = findReason(cfg::PacketRxReason::ARP);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(*entry->queueId(), 2);
}

TEST_F(CmdConfigCoppTestFixture, reason_appendNew) {
  setupTestableConfigSession(reasonCmdPrefix_, "dhcp queue 2");
  CmdConfigCoppReason cmd;
  HostInfo hostInfo("testhost");

  // DHCP is not in the seed — must be appended.
  ASSERT_EQ(findReason(cfg::PacketRxReason::DHCP), nullptr);
  CoppReasonArgs args({"dhcp", "queue", "2"});
  cmd.queryClient(hostInfo, args);

  const auto* entry = findReason(cfg::PacketRxReason::DHCP);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(*entry->queueId(), 2);
}

TEST_F(CmdConfigCoppTestFixture, reason_preservesOrderingOnUpdate) {
  setupTestableConfigSession(reasonCmdPrefix_, "bgp queue 0");
  CmdConfigCoppReason cmd;
  HostInfo hostInfo("testhost");

  auto& config = ConfigSession::getInstance().getAgentConfig();
  const auto origSize =
      config.sw()->cpuTrafficPolicy()->rxReasonToQueueOrderedList()->size();

  CoppReasonArgs args({"bgp", "queue", "0"});
  cmd.queryClient(hostInfo, args);

  // Updating an existing entry must not grow the list.
  EXPECT_EQ(
      config.sw()->cpuTrafficPolicy()->rxReasonToQueueOrderedList()->size(),
      origSize);
  // And BGP's position in the list must be unchanged (still at index 2).
  const auto& list =
      *config.sw()->cpuTrafficPolicy()->rxReasonToQueueOrderedList();
  EXPECT_EQ(*list[2].rxReason(), cfg::PacketRxReason::BGP);
  EXPECT_EQ(*list[2].queueId(), 0);
}

// =============================================================
// CoppCpuTrafficPolicyArgs validation tests
// =============================================================

TEST_F(CmdConfigCoppTestFixture, cpuTrafficPolicyArgs_valid) {
  CoppCpuTrafficPolicyArgs a(
      {"match", "acl-x", "action", "send-to-queue", "2"});
  EXPECT_EQ(a.getMatcherName(), "acl-x");
  EXPECT_EQ(a.getActionType(), "send-to-queue");
  EXPECT_EQ(a.getActionValue(), "2");

  CoppCpuTrafficPolicyArgs b({"match", "acl-x", "action", "counter", "cnt"});
  EXPECT_EQ(b.getActionType(), "counter");
  EXPECT_EQ(b.getActionValue(), "cnt");

  // set-tc writes a thrift byte; 127 is the max accepted value.
  CoppCpuTrafficPolicyArgs c({"match", "acl-x", "action", "set-tc", "127"});
  EXPECT_EQ(c.getActionValue(), "127");

  CoppCpuTrafficPolicyArgs d(
      {"match", "acl-x", "action", "user-defined-trap", "0"});
  EXPECT_EQ(d.getActionValue(), "0");
}

TEST_F(CmdConfigCoppTestFixture, cpuTrafficPolicyArgs_badArity) {
  EXPECT_THROW(CoppCpuTrafficPolicyArgs({}), std::invalid_argument);
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs({"match", "acl-x", "action", "counter"}),
      std::invalid_argument);
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs(
          {"match", "acl-x", "action", "counter", "cnt", "extra"}),
      std::invalid_argument);
}

TEST_F(CmdConfigCoppTestFixture, cpuTrafficPolicyArgs_badLiterals) {
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs(
          {"matcher", "acl-x", "action", "counter", "cnt"}),
      std::invalid_argument);
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs({"match", "acl-x", "actions", "counter", "cnt"}),
      std::invalid_argument);
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs({"match", "", "action", "counter", "cnt"}),
      std::invalid_argument);
  // "action" as a matcher name collides with the delete command tree's
  // subcommand token and would be undeletable, so config must reject it.
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs({"match", "action", "action", "counter", "cnt"}),
      std::invalid_argument);
}

TEST_F(CmdConfigCoppTestFixture, cpuTrafficPolicyArgs_badValues) {
  // Unknown action type.
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs({"match", "acl-x", "action", "drop", "1"}),
      std::invalid_argument);
  // send-to-queue: non-numeric, negative, above the 255 queue-id cap.
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs(
          {"match", "acl-x", "action", "send-to-queue", "abc"}),
      std::invalid_argument);
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs(
          {"match", "acl-x", "action", "send-to-queue", "-1"}),
      std::invalid_argument);
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs(
          {"match", "acl-x", "action", "send-to-queue", "256"}),
      std::invalid_argument);
  // set-tc writes a thrift byte: reject negative and anything above 127.
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs({"match", "acl-x", "action", "set-tc", "-1"}),
      std::invalid_argument);
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs({"match", "acl-x", "action", "set-tc", "128"}),
      std::invalid_argument);
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs({"match", "acl-x", "action", "set-tc", "256"}),
      std::invalid_argument);
  // user-defined-trap shares the queue-id validation path.
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs(
          {"match", "acl-x", "action", "user-defined-trap", "abc"}),
      std::invalid_argument);
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs(
          {"match", "acl-x", "action", "user-defined-trap", "-1"}),
      std::invalid_argument);
  // counter name must be non-empty.
  EXPECT_THROW(
      CoppCpuTrafficPolicyArgs({"match", "acl-x", "action", "counter", ""}),
      std::invalid_argument);
}

// =============================================================
// queryClient() tests — cpu-traffic-policy
// =============================================================

TEST_F(CmdConfigCoppTestFixture, cpuTrafficPolicy_addActionToExistingMatcher) {
  setupTestableConfigSession(
      cpuTrafficPolicyCmdPrefix_, "match cpuPolicy-mid action send-to-queue 2");
  CmdConfigCoppCpuTrafficPolicy cmd;
  HostInfo hostInfo("testhost");
  CoppCpuTrafficPolicyArgs args(
      {"match", "cpuPolicy-mid", "action", "send-to-queue", "2"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("cpuPolicy-mid"));

  const auto* mta = findMatchToAction("cpuPolicy-mid");
  ASSERT_NE(mta, nullptr);
  ASSERT_TRUE(mta->action()->sendToQueue().has_value());
  EXPECT_EQ(*mta->action()->sendToQueue()->queueId(), 2);
  // The pre-existing counter action on the same matcher must be preserved.
  ASSERT_TRUE(mta->action()->counter().has_value());
  EXPECT_EQ(*mta->action()->counter(), "cpuPolicy-mid-counter");
}

TEST_F(CmdConfigCoppTestFixture, cpuTrafficPolicy_createsNewMatcherEntry) {
  setupTestableConfigSession(
      cpuTrafficPolicyCmdPrefix_, "match acl-new action set-tc 3");
  CmdConfigCoppCpuTrafficPolicy cmd;
  HostInfo hostInfo("testhost");
  CoppCpuTrafficPolicyArgs args({"match", "acl-new", "action", "set-tc", "3"});

  cmd.queryClient(hostInfo, args);

  const auto* mta = findMatchToAction("acl-new");
  ASSERT_NE(mta, nullptr);
  ASSERT_TRUE(mta->action()->setTc().has_value());
  EXPECT_EQ(*mta->action()->setTc()->tcValue(), 3);
}

TEST_F(CmdConfigCoppTestFixture, cpuTrafficPolicy_userDefinedTrap) {
  setupTestableConfigSession(
      cpuTrafficPolicyCmdPrefix_,
      "match cpuPolicy-mid action user-defined-trap 9");
  CmdConfigCoppCpuTrafficPolicy cmd;
  HostInfo hostInfo("testhost");
  CoppCpuTrafficPolicyArgs args(
      {"match", "cpuPolicy-mid", "action", "user-defined-trap", "9"});

  cmd.queryClient(hostInfo, args);

  const auto* mta = findMatchToAction("cpuPolicy-mid");
  ASSERT_NE(mta, nullptr);
  ASSERT_TRUE(mta->action()->userDefinedTrap().has_value());
  EXPECT_EQ(*mta->action()->userDefinedTrap()->queueId(), 9);
}

TEST_F(CmdConfigCoppTestFixture, cpuTrafficPolicy_overwritesSameActionType) {
  setupTestableConfigSession(
      cpuTrafficPolicyCmdPrefix_, "match cpuPolicy-mid action counter cnt-new");
  CmdConfigCoppCpuTrafficPolicy cmd;
  HostInfo hostInfo("testhost");
  CoppCpuTrafficPolicyArgs args(
      {"match", "cpuPolicy-mid", "action", "counter", "cnt-new"});

  cmd.queryClient(hostInfo, args);

  const auto* mta = findMatchToAction("cpuPolicy-mid");
  ASSERT_NE(mta, nullptr);
  ASSERT_TRUE(mta->action()->counter().has_value());
  EXPECT_EQ(*mta->action()->counter(), "cnt-new");
}

// Adding an action when the device has neither cpuTrafficPolicy nor
// trafficPolicy must create both containers, not throw.
class CmdConfigCoppNoCpuTrafficPolicyFixture : public CmdConfigTestBase {
 public:
  CmdConfigCoppNoCpuTrafficPolicyFixture()
      : CmdConfigTestBase(
            "fboss_copp_config_no_policy_test_%%%%-%%%%-%%%%-%%%%",
            R"({"sw": {}})") {}

 protected:
  const std::string cpuTrafficPolicyCmdPrefix_ =
      "config copp cpu-traffic-policy";
};

TEST_F(
    CmdConfigCoppNoCpuTrafficPolicyFixture,
    cpuTrafficPolicy_createsPolicyFromAbsent) {
  setupTestableConfigSession(
      cpuTrafficPolicyCmdPrefix_, "match acl-new action send-to-queue 2");
  CmdConfigCoppCpuTrafficPolicy cmd;
  HostInfo hostInfo("testhost");
  CoppCpuTrafficPolicyArgs args(
      {"match", "acl-new", "action", "send-to-queue", "2"});

  cmd.queryClient(hostInfo, args);

  auto& config = ConfigSession::getInstance().getAgentConfig();
  ASSERT_TRUE(config.sw()->cpuTrafficPolicy().has_value());
  const auto& policy = *config.sw()->cpuTrafficPolicy();
  ASSERT_TRUE(policy.trafficPolicy().has_value());
  const auto& matchToActions = *policy.trafficPolicy()->matchToAction();
  ASSERT_EQ(matchToActions.size(), 1);
  EXPECT_EQ(*matchToActions[0].matcher(), "acl-new");
  ASSERT_TRUE(matchToActions[0].action()->sendToQueue().has_value());
  EXPECT_EQ(*matchToActions[0].action()->sendToQueue()->queueId(), 2);
}

} // namespace facebook::fboss
