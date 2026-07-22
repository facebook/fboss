// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/qos/queuing_policy/CmdConfigQosQueuingPolicyQueueId.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>

#include "fboss/cli/fboss2/commands/config/qos/default_queue_config/CmdConfigQosDefaultQueueConfig.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

// Seed JSON mirrors a typical RSW defaultPortQueues shape with 9 unicast
// queues (ids 0-8) at weighted round-robin scheduling with varying weights.
static const std::string kSeedConfig = R"({
  "sw": {
    "defaultPortQueues": [
      {"id": 0, "streamType": 1, "weight": 1, "scheduling": 5},
      {"id": 1, "streamType": 1, "weight": 9, "scheduling": 5},
      {"id": 2, "streamType": 1, "weight": 9, "scheduling": 5},
      {"id": 3, "streamType": 1, "weight": 9, "scheduling": 5},
      {"id": 4, "streamType": 1, "weight": 9, "scheduling": 5},
      {"id": 5, "streamType": 1, "weight": 9, "scheduling": 5},
      {"id": 6, "streamType": 1, "weight": 9, "scheduling": 5},
      {"id": 7, "streamType": 1, "weight": 9, "scheduling": 5},
      {"id": 8, "streamType": 1, "weight": 9, "scheduling": 0}
    ]
  }
})";

class CmdConfigQosDefaultQueueConfigTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigQosDefaultQueueConfigTestFixture()
      : CmdConfigTestBase("fboss_dqc_test_%%%%-%%%%-%%%%-%%%%", kSeedConfig) {}

 protected:
  const std::string cmdPrefix_ = "config qos default-queue-config";

  static const cfg::PortQueue* findQueue(int16_t queueId) {
    const auto& queues = *ConfigSession::getInstance()
                              .getAgentConfig()
                              .sw()
                              ->defaultPortQueues();
    for (const auto& queue : queues) {
      if (*queue.id() == queueId) {
        return &queue;
      }
    }
    return nullptr;
  }

  // Run the command with the given argument string and return the target queue.
  const cfg::PortQueue* runAndFind(const std::string& args, int16_t queueId) {
    setupTestableConfigSession(cmdPrefix_, args);
    auto cmd = CmdConfigQosDefaultQueueConfig();
    QueueConfig config(getCmdArgsList());
    cmd.queryClient(localhost(), config);
    return findQueue(queueId);
  }
};

// Arg validation: minimum valid input
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, validMinimalArgs) {
  EXPECT_NO_THROW(QueueConfig({"0", "weight", "5"}));
  EXPECT_NO_THROW(QueueConfig({"7", "reserved-bytes", "1024"}));
  EXPECT_NO_THROW(QueueConfig({"3", "scheduling", "WRR"}));
}

// Arg validation: multiple attributes
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, validMultipleAttrs) {
  EXPECT_NO_THROW(QueueConfig({"1", "weight", "10", "reserved-bytes", "2048"}));
  EXPECT_NO_THROW(
      QueueConfig({"2", "scheduling", "SP", "shared-bytes", "4096"}));
}

// Arg validation: empty args
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, emptyArgsFails) {
  EXPECT_THROW(QueueConfig({}), std::invalid_argument);
}

// Arg validation: attribute missing value
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, attrMissingValueFails) {
  EXPECT_THROW(QueueConfig({"0", "weight"}), std::invalid_argument);
}

// Arg validation: negative queue id
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, negativeQueueIdFails) {
  EXPECT_THROW(QueueConfig({"-1", "weight", "5"}), std::invalid_argument);
}

// Modify weight on an existing queue in defaultPortQueues
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, setWeightExistingQueue) {
  setupTestableConfigSession(cmdPrefix_, "1 weight 20");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, ::testing::HasSubstr("Successfully configured"));
  EXPECT_THAT(result, ::testing::HasSubstr("1"));

  auto& agentConfig = ConfigSession::getInstance().getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  ASSERT_TRUE(switchConfig.defaultPortQueues().has_value());
  const auto& queues = *switchConfig.defaultPortQueues();

  const cfg::PortQueue* q = nullptr;
  for (const auto& queue : queues) {
    if (*queue.id() == 1) {
      q = &queue;
      break;
    }
  }
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->weight().has_value());
  EXPECT_EQ(*q->weight(), 20);
}

// Set reserved-bytes on an existing queue
TEST_F(
    CmdConfigQosDefaultQueueConfigTestFixture,
    setReservedBytesExistingQueue) {
  setupTestableConfigSession(cmdPrefix_, "0 reserved-bytes 1024");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  cmd.queryClient(localhost(), config);

  auto& agentConfig = ConfigSession::getInstance().getAgentConfig();
  const auto& queues = *agentConfig.sw()->defaultPortQueues();

  const cfg::PortQueue* q = nullptr;
  for (const auto& queue : queues) {
    if (*queue.id() == 0) {
      q = &queue;
      break;
    }
  }
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->reservedBytes().has_value());
  EXPECT_EQ(*q->reservedBytes(), 1024);
}

// Create a new queue entry when queue-id is not in defaultPortQueues
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, createsNewQueueEntry) {
  setupTestableConfigSession(cmdPrefix_, "15 weight 3");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  cmd.queryClient(localhost(), config);

  const auto& queues =
      *ConfigSession::getInstance().getAgentConfig().sw()->defaultPortQueues();

  const cfg::PortQueue* q = nullptr;
  for (const auto& queue : queues) {
    if (*queue.id() == 15) {
      q = &queue;
      break;
    }
  }
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->weight().has_value());
  EXPECT_EQ(*q->weight(), 3);
}

// Set scheduling to STRICT_PRIORITY via short name
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, setSchedulingShortName) {
  setupTestableConfigSession(cmdPrefix_, "8 scheduling SP");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  cmd.queryClient(localhost(), config);

  const auto& queues =
      *ConfigSession::getInstance().getAgentConfig().sw()->defaultPortQueues();

  const cfg::PortQueue* q = nullptr;
  for (const auto& queue : queues) {
    if (*queue.id() == 8) {
      q = &queue;
      break;
    }
  }
  ASSERT_NE(q, nullptr);
  EXPECT_EQ(*q->scheduling(), cfg::QueueScheduling::STRICT_PRIORITY);
}

// Full AQM grammar: ECN behavior + linear detection thresholds
TEST_F(
    CmdConfigQosDefaultQueueConfigTestFixture,
    setAqmEcnWithLinearDetection) {
  setupTestableConfigSession(
      cmdPrefix_,
      "6 active-queue-management detection linear minimum-length 40000 "
      "maximum-length 40000 congestion-behavior ECN");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  cmd.queryClient(localhost(), config);

  const auto& queues =
      *ConfigSession::getInstance().getAgentConfig().sw()->defaultPortQueues();
  const cfg::PortQueue* q = nullptr;
  for (const auto& queue : queues) {
    if (*queue.id() == 6) {
      q = &queue;
      break;
    }
  }
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->aqms().has_value());
  ASSERT_EQ(q->aqms()->size(), 1);
  const auto& aqm = q->aqms()->front();
  EXPECT_EQ(*aqm.behavior(), cfg::QueueCongestionBehavior::ECN);
  ASSERT_TRUE(aqm.detection()->linear().has_value());
  EXPECT_EQ(*aqm.detection()->linear()->minimumLength(), 40000);
  EXPECT_EQ(*aqm.detection()->linear()->maximumLength(), 40000);
}

// Unknown linear detection attribute error enumerates the valid attributes
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, unknownLinearAttrFails) {
  setupTestableConfigSession(
      cmdPrefix_,
      "6 active-queue-management congestion-behavior ECN "
      "detection linear bogus-attr 5");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), ::testing::HasSubstr("bogus-attr"));
    EXPECT_THAT(e.what(), ::testing::HasSubstr("minimum-length"));
    EXPECT_THAT(e.what(), ::testing::HasSubstr("maximum-length"));
    EXPECT_THAT(e.what(), ::testing::HasSubstr("probability"));
  }
}

// Linear detection attribute with a missing trailing value fails
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, linearAttrMissingValueFails) {
  setupTestableConfigSession(
      cmdPrefix_, "6 active-queue-management detection linear minimum-length");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  EXPECT_THROW(cmd.queryClient(localhost(), config), std::invalid_argument);
}

// Invalid congestion-behavior error enumerates the thrift enum values
TEST_F(
    CmdConfigQosDefaultQueueConfigTestFixture,
    invalidCongestionBehaviorFails) {
  setupTestableConfigSession(
      cmdPrefix_, "6 active-queue-management congestion-behavior BOGUS");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), ::testing::HasSubstr("EARLY_DROP"));
    EXPECT_THAT(e.what(), ::testing::HasSubstr("ECN"));
  }
}

// Non-numeric values for integer attributes fail with a clean CLI error
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, nonNumericValueFails) {
  setupTestableConfigSession(cmdPrefix_, "0 weight abc");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), ::testing::HasSubstr("must be an integer"));
  }
}

// Unknown attribute fails in queryClient
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, unknownAttrFails) {
  setupTestableConfigSession(cmdPrefix_, "0 unknown-attr 99");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  EXPECT_THROW(cmd.queryClient(localhost(), config), std::invalid_argument);
}

// Negative integer value fails with the non-negative error (distinct from the
// non-numeric branch covered above)
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, negativeValueFails) {
  setupTestableConfigSession(cmdPrefix_, "0 weight -5");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), ::testing::HasSubstr("must be non-negative"));
  }
}

// Set shared-bytes through queryClient and verify the stored field
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, setSharedBytesExistingQueue) {
  const auto* q = runAndFind("0 shared-bytes 4096", 0);
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->sharedBytes().has_value());
  EXPECT_EQ(*q->sharedBytes(), 4096);
}

// Set scaling-factor (enum attribute) and verify the stored field
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, setScalingFactor) {
  const auto* q = runAndFind("0 scaling-factor ONE_HALF", 0);
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->scalingFactor().has_value());
  EXPECT_EQ(*q->scalingFactor(), cfg::MMUScalingFactor::ONE_HALF);
}

// Set stream-type (enum attribute) and verify the stored field
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, setStreamType) {
  const auto* q = runAndFind("0 stream-type UNICAST", 0);
  ASSERT_NE(q, nullptr);
  EXPECT_EQ(*q->streamType(), cfg::StreamType::UNICAST);
}

// Set buffer-pool-name (string attribute) and verify the stored field
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, setBufferPoolName) {
  const auto* q = runAndFind("0 buffer-pool-name egress_pool", 0);
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->bufferPoolName().has_value());
  EXPECT_EQ(*q->bufferPoolName(), "egress_pool");
}

// Set scheduling via full thrift enum name (not just the short SP alias)
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, setSchedulingFullEnumName) {
  const auto* q = runAndFind("0 scheduling DEFICIT_ROUND_ROBIN", 0);
  ASSERT_NE(q, nullptr);
  EXPECT_EQ(*q->scheduling(), cfg::QueueScheduling::DEFICIT_ROUND_ROBIN);
}

// Set active-queue-management with EARLY_DROP behavior and probability, and
// verify all three linear fields plus behavior are stored
TEST_F(
    CmdConfigQosDefaultQueueConfigTestFixture,
    setAqmEarlyDropWithProbability) {
  const auto* q = runAndFind(
      "6 active-queue-management detection linear minimum-length 1000 "
      "maximum-length 5000 probability 80 congestion-behavior EARLY_DROP",
      6);
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->aqms().has_value());
  ASSERT_EQ(q->aqms()->size(), 1);
  const auto& aqm = q->aqms()->front();
  EXPECT_EQ(*aqm.behavior(), cfg::QueueCongestionBehavior::EARLY_DROP);
  ASSERT_TRUE(aqm.detection()->linear().has_value());
  EXPECT_EQ(*aqm.detection()->linear()->minimumLength(), 1000);
  EXPECT_EQ(*aqm.detection()->linear()->maximumLength(), 5000);
  EXPECT_EQ(*aqm.detection()->linear()->probability(), 80);
}

// printOutput emits the message
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, printOutput) {
  auto cmd = CmdConfigQosDefaultQueueConfig();
  std::string msg = "Successfully configured default-queue-config queue-id 3";

  std::stringstream buf;
  std::streambuf* old = std::cout.rdbuf(buf.rdbuf());
  cmd.printOutput(msg);
  std::cout.rdbuf(old);

  EXPECT_EQ(buf.str(), msg + "\n");
}

// Seed with queue 6 already carrying a fully-populated AQM entry (ECN behavior
// + linear detection), to exercise the merge-from-existing path: editing one
// linear field must preserve behavior and the other linear fields.
static const std::string kSeedConfigWithAqm = R"({
  "sw": {
    "defaultPortQueues": [
      {
        "id": 6, "streamType": 1, "weight": 9, "scheduling": 5,
        "aqms": [
          {
            "behavior": 1,
            "detection": {"linear": {
              "minimumLength": 100, "maximumLength": 200, "probability": 50
            }}
          }
        ]
      }
    ]
  }
})";

class CmdConfigQosDefaultQueueConfigAqmSeedFixture : public CmdConfigTestBase {
 public:
  CmdConfigQosDefaultQueueConfigAqmSeedFixture()
      : CmdConfigTestBase(
            "fboss_dqc_aqm_test_%%%%-%%%%-%%%%-%%%%",
            kSeedConfigWithAqm) {}
};

// Editing one linear threshold of an existing AQM entry (selected by its
// behavior) preserves that entry's behavior and its untouched linear fields.
TEST_F(
    CmdConfigQosDefaultQueueConfigAqmSeedFixture,
    aqmEditPreservesExistingFields) {
  setupTestableConfigSession(
      "config qos default-queue-config",
      "6 active-queue-management congestion-behavior ECN "
      "detection linear minimum-length 1234");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());
  cmd.queryClient(localhost(), config);

  const auto& queues =
      *ConfigSession::getInstance().getAgentConfig().sw()->defaultPortQueues();
  const cfg::PortQueue* q = nullptr;
  for (const auto& queue : queues) {
    if (*queue.id() == 6) {
      q = &queue;
      break;
    }
  }
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->aqms().has_value());
  ASSERT_EQ(q->aqms()->size(), 1);
  const auto& aqm = q->aqms()->front();
  // behavior preserved from the seed
  EXPECT_EQ(*aqm.behavior(), cfg::QueueCongestionBehavior::ECN);
  ASSERT_TRUE(aqm.detection()->linear().has_value());
  // edited field updated
  EXPECT_EQ(*aqm.detection()->linear()->minimumLength(), 1234);
  // untouched fields preserved from the seed
  EXPECT_EQ(*aqm.detection()->linear()->maximumLength(), 200);
  EXPECT_EQ(*aqm.detection()->linear()->probability(), 50);
}

// Look up a defaultPortQueues entry in the live ConfigSession by id (the
// AqmSeed fixture is not derived from the main fixture, so it can't use that
// fixture's protected findQueue helper).
static const cfg::PortQueue* queueInSession(int16_t queueId) {
  const auto& queues =
      *ConfigSession::getInstance().getAgentConfig().sw()->defaultPortQueues();
  for (const auto& queue : queues) {
    if (*queue.id() == queueId) {
      return &queue;
    }
  }
  return nullptr;
}

// Returns the aqms entry with the given behavior, or nullptr. Lets coexistence
// tests assert per-behavior instead of relying on positional aqms[0]/[1].
static const cfg::ActiveQueueManagement* findAqmByBehavior(
    const cfg::PortQueue& q,
    cfg::QueueCongestionBehavior behavior) {
  if (!q.aqms().has_value()) {
    return nullptr;
  }
  for (const auto& aqm : *q.aqms()) {
    if (aqm.behavior().has_value() && *aqm.behavior() == behavior) {
      return &aqm;
    }
  }
  return nullptr;
}

// Adding an EARLY_DROP policy to a queue that already carries an ECN entry must
// APPEND (yielding two entries) rather than overwrite the ECN one — this is the
// core coexistence behavior of selectOrCreateAqm.
TEST_F(
    CmdConfigQosDefaultQueueConfigAqmSeedFixture,
    aqmCoexistEcnAndEarlyDrop) {
  setupTestableConfigSession(
      "config qos default-queue-config",
      "6 active-queue-management congestion-behavior EARLY_DROP "
      "detection linear minimum-length 300 maximum-length 400 probability 25");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());
  cmd.queryClient(localhost(), config);

  const cfg::PortQueue* q = queueInSession(6);
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->aqms().has_value());
  ASSERT_EQ(q->aqms()->size(), 2)
      << "EARLY_DROP should append, not clobber ECN";

  // The pre-existing ECN entry is untouched.
  const auto* ecn = findAqmByBehavior(*q, cfg::QueueCongestionBehavior::ECN);
  ASSERT_NE(ecn, nullptr) << "seeded ECN entry was lost";
  ASSERT_TRUE(ecn->detection()->linear().has_value());
  EXPECT_EQ(*ecn->detection()->linear()->minimumLength(), 100);
  EXPECT_EQ(*ecn->detection()->linear()->maximumLength(), 200);
  EXPECT_EQ(*ecn->detection()->linear()->probability(), 50);

  // The new EARLY_DROP entry carries its own detection.
  const auto* ed =
      findAqmByBehavior(*q, cfg::QueueCongestionBehavior::EARLY_DROP);
  ASSERT_NE(ed, nullptr) << "EARLY_DROP entry was not created";
  ASSERT_TRUE(ed->detection()->linear().has_value());
  EXPECT_EQ(*ed->detection()->linear()->minimumLength(), 300);
  EXPECT_EQ(*ed->detection()->linear()->maximumLength(), 400);
  EXPECT_EQ(*ed->detection()->linear()->probability(), 25);
}

// A bare `congestion-behavior <x>` (no detection args) must leave the selected
// entry's existing detection intact — the sawDetectionArgs no-clobber guard.
TEST_F(
    CmdConfigQosDefaultQueueConfigAqmSeedFixture,
    aqmBareBehaviorPreservesDetection) {
  setupTestableConfigSession(
      "config qos default-queue-config",
      "6 active-queue-management congestion-behavior ECN");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());
  cmd.queryClient(localhost(), config);

  const cfg::PortQueue* q = queueInSession(6);
  ASSERT_NE(q, nullptr);
  ASSERT_TRUE(q->aqms().has_value());
  ASSERT_EQ(q->aqms()->size(), 1);
  const auto& aqm = q->aqms()->front();
  EXPECT_EQ(*aqm.behavior(), cfg::QueueCongestionBehavior::ECN);
  ASSERT_TRUE(aqm.detection()->linear().has_value())
      << "existing detection was clobbered by a bare congestion-behavior";
  EXPECT_EQ(*aqm.detection()->linear()->minimumLength(), 100);
  EXPECT_EQ(*aqm.detection()->linear()->maximumLength(), 200);
  EXPECT_EQ(*aqm.detection()->linear()->probability(), 50);
}

// An AQM edit that names no congestion-behavior is ambiguous (a queue can hold
// both an ECN and an EARLY_DROP entry) and must be rejected, not silently
// applied to aqms.front() or committed as a phantom EARLY_DROP entry.
TEST_F(
    CmdConfigQosDefaultQueueConfigTestFixture,
    aqmRequiresCongestionBehavior) {
  setupTestableConfigSession(
      cmdPrefix_,
      "6 active-queue-management detection linear minimum-length 100");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), ::testing::HasSubstr("congestion-behavior"));
  }
}

// Two congestion-behavior tokens in one edit are ambiguous (selection keys on
// the first, assignment on the last) and must be rejected.
TEST_F(
    CmdConfigQosDefaultQueueConfigTestFixture,
    aqmDuplicateCongestionBehaviorFails) {
  setupTestableConfigSession(
      cmdPrefix_,
      "6 active-queue-management congestion-behavior ECN "
      "congestion-behavior EARLY_DROP");

  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());
  EXPECT_THROW(cmd.queryClient(localhost(), config), std::invalid_argument);
}

// A queue-id with no attributes at all is rejected by queryClient rather than
// staging an empty edit.
TEST_F(CmdConfigQosDefaultQueueConfigTestFixture, noAttributesFails) {
  setupTestableConfigSession(cmdPrefix_, "0");
  auto cmd = CmdConfigQosDefaultQueueConfig();
  QueueConfig config(getCmdArgsList());
  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), ::testing::HasSubstr("At least one attribute"));
  }
}

} // namespace facebook::fboss
