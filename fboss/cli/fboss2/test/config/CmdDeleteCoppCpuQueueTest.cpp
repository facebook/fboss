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
#include "fboss/cli/fboss2/commands/delete/copp/cpu_queue/CmdDeleteCoppCpuQueue.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Seed config mirrors the cpuQueues + cpuTrafficPolicy shape of a real
// production FBOSS agent.conf: several queues with per-queue weight, name
// and rate caps. Queue 0 is deliberately unreferenced (deletable), queue 9
// is referenced by reason mappings, queue 4 only by the matchToAction
// send-to-queue action, and queue 6 only by the matchToAction
// user-defined-trap action — covering each side of the referenced-queue
// guard.
class CmdDeleteCoppCpuQueueTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteCoppCpuQueueTestFixture()
      : CmdConfigTestBase(
            "fboss_delete_copp_cpu_queue_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "cpuQueues": [
      { "id": 9, "streamType": 1, "weight": 4, "scheduling": 0,
        "name": "cpuQueue-high" },
      { "id": 2, "streamType": 1, "weight": 2, "scheduling": 0,
        "name": "cpuQueue-mid",
        "portQueueRate": { "kbitsPerSec": { "minimum": 0, "maximum": 2000 } } },
      { "id": 1, "streamType": 1, "weight": 1, "scheduling": 0,
        "name": "cpuQueue-default",
        "portQueueRate": { "pktsPerSec": { "minimum": 0, "maximum": 1000 } } },
      { "id": 4, "streamType": 1, "weight": 1, "scheduling": 0,
        "name": "cpuQueue-hostif" },
      { "id": 6, "streamType": 1, "weight": 1, "scheduling": 0,
        "name": "cpuQueue-trap" },
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
            "action": { "sendToQueue": { "queueId": 4 },
                        "counter": "cpuPolicy-mid-counter" } },
          { "matcher": "cpuPolicy-trap",
            "action": { "userDefinedTrap": { "queueId": 6 } } }
        ]
      }
    }
  }
})") {}

 protected:
  const std::string cmdPrefix_ = "delete copp cpu-queue";

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
};

// =============================================================
// CoppCpuQueueDeleteArgs validation tests
// =============================================================

TEST_F(CmdDeleteCoppCpuQueueTestFixture, argsIdOnly) {
  CoppCpuQueueDeleteArgs a({"9"});
  EXPECT_EQ(a.getQueueId(), 9);
}

TEST_F(CmdDeleteCoppCpuQueueTestFixture, argsInvalid) {
  // Empty
  EXPECT_THROW(CoppCpuQueueDeleteArgs({}), std::invalid_argument);

  // Bad queue id
  EXPECT_THROW(CoppCpuQueueDeleteArgs({"abc"}), std::invalid_argument);
  EXPECT_THROW(CoppCpuQueueDeleteArgs({"-1"}), std::invalid_argument);
  EXPECT_THROW(CoppCpuQueueDeleteArgs({"999"}), std::invalid_argument);

  // No per-attribute sub-commands are accepted
  EXPECT_THROW(CoppCpuQueueDeleteArgs({"9", "name"}), std::invalid_argument);
}

// =============================================================
// Whole-queue delete
// =============================================================

TEST_F(CmdDeleteCoppCpuQueueTestFixture, deleteWholeQueue) {
  setupTestableConfigSession(cmdPrefix_, "0");
  CmdDeleteCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  CoppCpuQueueDeleteArgs args({"0"});

  ASSERT_NE(findQueue(0), nullptr);
  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("Deleted cpu-queue 0"));

  EXPECT_EQ(findQueue(0), nullptr);
  // Siblings untouched.
  EXPECT_NE(findQueue(9), nullptr);
  EXPECT_NE(findQueue(2), nullptr);
  EXPECT_NE(findQueue(1), nullptr);
}

TEST_F(CmdDeleteCoppCpuQueueTestFixture, deleteQueueReferencedByReasonThrows) {
  setupTestableConfigSession(cmdPrefix_, "9");
  CmdDeleteCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  // Queue 9 is referenced by the NDP, ARP, and BGP reason mappings.
  CoppCpuQueueDeleteArgs args({"9"});

  try {
    cmd.queryClient(hostInfo, args);
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    EXPECT_THAT(e.what(), HasSubstr("Cannot delete cpu-queue 9"));
    EXPECT_THAT(e.what(), HasSubstr("reason NDP"));
    EXPECT_THAT(e.what(), HasSubstr("reason ARP"));
    EXPECT_THAT(e.what(), HasSubstr("reason BGP"));
  }
  // Config untouched on error.
  EXPECT_NE(findQueue(9), nullptr);
}

TEST_F(CmdDeleteCoppCpuQueueTestFixture, deleteQueueReferencedByMatcherThrows) {
  setupTestableConfigSession(cmdPrefix_, "4");
  CmdDeleteCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  // Queue 4 is referenced only by the cpuPolicy-mid send-to-queue action.
  CoppCpuQueueDeleteArgs args({"4"});

  try {
    cmd.queryClient(hostInfo, args);
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    EXPECT_THAT(e.what(), HasSubstr("Cannot delete cpu-queue 4"));
    EXPECT_THAT(e.what(), HasSubstr("matcher 'cpuPolicy-mid' send-to-queue"));
  }
  EXPECT_NE(findQueue(4), nullptr);
}

TEST_F(
    CmdDeleteCoppCpuQueueTestFixture,
    deleteQueueReferencedByUserDefinedTrapThrows) {
  setupTestableConfigSession(cmdPrefix_, "6");
  CmdDeleteCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  // Queue 6 is referenced only by the cpuPolicy-trap user-defined-trap
  // action.
  CoppCpuQueueDeleteArgs args({"6"});

  try {
    cmd.queryClient(hostInfo, args);
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    EXPECT_THAT(e.what(), HasSubstr("Cannot delete cpu-queue 6"));
    EXPECT_THAT(
        e.what(), HasSubstr("matcher 'cpuPolicy-trap' user-defined-trap"));
  }
  EXPECT_NE(findQueue(6), nullptr);
}

TEST_F(CmdDeleteCoppCpuQueueTestFixture, deleteQueueNotFound) {
  setupTestableConfigSession(cmdPrefix_, "5");
  CmdDeleteCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  CoppCpuQueueDeleteArgs args({"5"});

  EXPECT_THROW(cmd.queryClient(hostInfo, args), std::runtime_error);
}

// The deprecated rxReasonToCPUQueue map form is still read by the agent as
// a fallback when rxReasonToQueueOrderedList is unset, so a queue referenced
// only there must also refuse deletion. The CLI never writes this map; it
// only appears in configs authored outside the CLI.
class CmdDeleteCoppCpuQueueLegacyMapFixture : public CmdConfigTestBase {
 public:
  CmdDeleteCoppCpuQueueLegacyMapFixture()
      : CmdConfigTestBase(
            "fboss_delete_copp_cpu_queue_legacy_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "cpuQueues": [
      { "id": 9, "streamType": 1, "weight": 4, "scheduling": 0,
        "name": "cpuQueue-high" }
    ],
    "cpuTrafficPolicy": {
      "rxReasonToCPUQueue": { "1": 9 }
    }
  }
})") {}
};

TEST_F(CmdDeleteCoppCpuQueueLegacyMapFixture, deleteQueueInLegacyMapThrows) {
  setupTestableConfigSession("delete copp cpu-queue", "9");
  CmdDeleteCoppCpuQueue cmd;
  HostInfo hostInfo("testhost");
  // Queue 9 is referenced only via the deprecated rxReasonToCPUQueue map
  // (reason 1 = ARP).
  CoppCpuQueueDeleteArgs args({"9"});

  try {
    cmd.queryClient(hostInfo, args);
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    EXPECT_THAT(e.what(), HasSubstr("Cannot delete cpu-queue 9"));
    EXPECT_THAT(
        e.what(), HasSubstr("reason ARP (deprecated rxReasonToCPUQueue map)"));
  }
}

} // namespace facebook::fboss
