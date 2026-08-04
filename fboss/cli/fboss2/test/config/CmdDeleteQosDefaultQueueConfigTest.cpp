// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>

#include "fboss/cli/fboss2/commands/delete/qos/default_queue_config/CmdDeleteQosDefaultQueueConfig.h"
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

class CmdDeleteQosDefaultQueueConfigTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteQosDefaultQueueConfigTestFixture()
      : CmdConfigTestBase(
            "fboss_del_dqc_test_%%%%-%%%%-%%%%-%%%%",
            kSeedConfig) {}

 protected:
  const std::string cmdPrefix_ = "delete qos default-queue-config";

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
};

// Arg validation: single valid queue-id
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, validQueueId) {
  EXPECT_NO_THROW(DeleteQueueId({"0"}));
  EXPECT_NO_THROW(DeleteQueueId({"8"}));
  EXPECT_EQ(DeleteQueueId({"5"}).getQueueId(), 5);
}

// Arg validation: empty args
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, emptyArgsFails) {
  EXPECT_THROW(DeleteQueueId({}), std::invalid_argument);
}

// Arg validation: more than one queue-id
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, multipleArgsFails) {
  EXPECT_THROW(DeleteQueueId({"0", "1"}), std::invalid_argument);
}

// Arg validation: non-integer queue-id
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, nonIntegerQueueIdFails) {
  EXPECT_THROW(DeleteQueueId({"abc"}), std::invalid_argument);
  EXPECT_THROW(DeleteQueueId({"1.5"}), std::invalid_argument);
}

// Arg validation: queue-id above the shared upper bound (utils::kMaxQueueId)
// is rejected, mirroring the config command's parser.
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, queueIdAboveMaxFails) {
  EXPECT_THROW(DeleteQueueId({"129"}), std::invalid_argument);
  EXPECT_THROW(DeleteQueueId({"200"}), std::invalid_argument);
}

// Arg validation: the maximum queue-id itself (128) is accepted.
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, maxQueueIdAccepted) {
  EXPECT_NO_THROW(DeleteQueueId({"128"}));
  EXPECT_EQ(DeleteQueueId({"128"}).getQueueId(), 128);
}

// Arg validation: negative queue-id
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, negativeQueueIdFails) {
  EXPECT_THROW(DeleteQueueId({"-1"}), std::invalid_argument);
}

// Delete an existing queue entry; other entries are untouched
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, deleteExistingQueue) {
  setupTestableConfigSession(cmdPrefix_, "8");

  auto cmd = CmdDeleteQosDefaultQueueConfig();
  DeleteQueueId queueId(getCmdArgsList());

  auto result = cmd.queryClient(localhost(), queueId);

  EXPECT_THAT(result, ::testing::HasSubstr("Successfully deleted"));
  EXPECT_THAT(result, ::testing::HasSubstr("8"));

  EXPECT_EQ(findQueue(8), nullptr);
  const auto& queues =
      *ConfigSession::getInstance().getAgentConfig().sw()->defaultPortQueues();
  EXPECT_EQ(queues.size(), 8);
  for (int16_t id = 0; id <= 7; ++id) {
    EXPECT_NE(findQueue(id), nullptr);
  }
}

// Delete a queue entry in the middle of the list
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, deleteMiddleQueue) {
  setupTestableConfigSession(cmdPrefix_, "4");

  auto cmd = CmdDeleteQosDefaultQueueConfig();
  DeleteQueueId queueId(getCmdArgsList());

  cmd.queryClient(localhost(), queueId);

  EXPECT_EQ(findQueue(4), nullptr);
  EXPECT_NE(findQueue(3), nullptr);
  EXPECT_NE(findQueue(5), nullptr);
}

// Deleting a queue-id with no entry throws and leaves config unchanged
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, deleteNonExistentQueueFails) {
  setupTestableConfigSession(cmdPrefix_, "15");

  auto cmd = CmdDeleteQosDefaultQueueConfig();
  DeleteQueueId queueId(getCmdArgsList());

  EXPECT_THROW(cmd.queryClient(localhost(), queueId), std::runtime_error);

  const auto& queues =
      *ConfigSession::getInstance().getAgentConfig().sw()->defaultPortQueues();
  EXPECT_EQ(queues.size(), 9);
}

// Deleting the same queue twice: second delete throws
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, doubleDeleteFails) {
  setupTestableConfigSession(cmdPrefix_, "7");

  auto cmd = CmdDeleteQosDefaultQueueConfig();
  DeleteQueueId queueId(getCmdArgsList());

  cmd.queryClient(localhost(), queueId);
  EXPECT_EQ(findQueue(7), nullptr);

  EXPECT_THROW(cmd.queryClient(localhost(), queueId), std::runtime_error);
}

// printOutput emits the message
TEST_F(CmdDeleteQosDefaultQueueConfigTestFixture, printOutput) {
  auto cmd = CmdDeleteQosDefaultQueueConfig();
  std::string msg = "Successfully deleted default-queue-config queue-id 3";

  std::stringstream buf;
  std::streambuf* old = std::cout.rdbuf(buf.rdbuf());
  cmd.printOutput(msg);
  std::cout.rdbuf(old);

  EXPECT_EQ(buf.str(), msg + "\n");
}

} // namespace facebook::fboss
