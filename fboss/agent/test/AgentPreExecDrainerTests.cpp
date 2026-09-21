// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/facebook/AgentPreExecDrainer.h"

#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/test/MockAgentNetWhoAmI.h"
#include "fboss/lib/CommonFileUtils.h"

#include <folly/Conv.h>
#include <folly/FileUtil.h>
#include <cstdint>

using namespace ::testing;
using namespace facebook::fboss;

// @lint-ignore CLANGTIDY
DECLARE_int32(bgp_drain_sleep_time);

namespace {
class TestAgentPreExecDrainer : public AgentPreExecDrainer {
 public:
  explicit TestAgentPreExecDrainer(
      const AgentDirectoryUtil* dirUtil,
      bool throwException = false)
      : AgentPreExecDrainer(dirUtil) {
    throwException_ = throwException;
  }

  virtual void drainImpl(uint32_t seconds) override {
    drainTime = seconds;
    if (throwException_) {
      throw std::runtime_error("Test exception");
    }
  }

  uint32_t getDrainTime() const {
    return *drainTime;
  }

 private:
  bool throwException_{false};
  std::optional<uint32_t> drainTime{std::nullopt};
};
} // namespace

TEST(AgentPreExecDrainerTests, CanDrain) {
  AgentDirectoryUtil util("/tmp", "/tmp");
  SCOPE_EXIT {
    removeFile(util.getVolatileStateDir() + "/UNDRAINED");
  };
  MockAgentNetWhoAmI mockWhoAmI;
  ON_CALL(mockWhoAmI, isNotDrainable).WillByDefault(Return(true));
  TestAgentPreExecDrainer drainer(&util);
  EXPECT_FALSE(drainer.canDrain(mockWhoAmI));
  ON_CALL(mockWhoAmI, isNotDrainable).WillByDefault(Return(false));
  ON_CALL(mockWhoAmI, isFdsw).WillByDefault(Return(true));
  EXPECT_FALSE(drainer.canDrain(mockWhoAmI));

  ON_CALL(mockWhoAmI, isFdsw).WillByDefault(Return(false));
  touchFile(util.getVolatileStateDir() + "/UNDRAINED");
  EXPECT_TRUE(drainer.canDrain(mockWhoAmI));
  removeFile(util.getVolatileStateDir() + "/UNDRAINED");
  EXPECT_FALSE(drainer.canDrain(mockWhoAmI));
}

TEST(AgentPreExecDrainerTests, GetBgpDrainSleepTime) {
  AgentDirectoryUtil util("/tmp", "/tmp");
  SCOPE_EXIT {
    removeFile(util.getRoutingProtocolColdBootDrainTimeFile());
  };
  MockAgentNetWhoAmI mockWhoAmI;
  ON_CALL(mockWhoAmI, isNotDrainable).WillByDefault(Return(true));
  TestAgentPreExecDrainer drainer(&util);
  EXPECT_EQ(drainer.getBgpDrainSleepTime(), FLAGS_bgp_drain_sleep_time);
  touchFile(util.getRoutingProtocolColdBootDrainTimeFile());
  folly::writeFile(
      folly::to<std::string>(100),
      util.getRoutingProtocolColdBootDrainTimeFile().c_str());
  EXPECT_EQ(drainer.getBgpDrainSleepTime(), 100);
}

TEST(AgentPreExecDrainerTests, ExceptionInDrain) {
  AgentDirectoryUtil util("/tmp", "/tmp");
  SCOPE_EXIT {
    removeFile(util.getVolatileStateDir() + "/UNDRAINED");
    removeFile(util.getRoutingProtocolColdBootDrainTimeFile());
  };
  MockAgentNetWhoAmI mockWhoAmI;
  ON_CALL(mockWhoAmI, isNotDrainable).WillByDefault(Return(false));
  ON_CALL(mockWhoAmI, isFdsw).WillByDefault(Return(false));
  touchFile(util.getVolatileStateDir() + "/UNDRAINED");
  TestAgentPreExecDrainer drainer(&util, true /* throw exception */);
  EXPECT_TRUE(drainer.canDrain(mockWhoAmI));
  touchFile(util.getRoutingProtocolColdBootDrainTimeFile());
  folly::writeFile(
      folly::to<std::string>(100),
      util.getRoutingProtocolColdBootDrainTimeFile().c_str());
  EXPECT_NO_THROW(drainer.drain(mockWhoAmI));
  EXPECT_EQ(drainer.getDrainTime(), 100);
}
