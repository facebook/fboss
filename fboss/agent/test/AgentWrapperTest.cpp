// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentWrapperTest.h"

#include "fboss/agent/AgentCommandExecutor.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "tupperware/agent/system/systemd/Service.h"

#include <folly/FileUtil.h>
#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>

#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/AgentNetWhoAmI.h"
#include "fboss/lib/CommonFileUtils.h"

#include "fboss/agent/AgentConfig.h"

#include <thread>
#include <type_traits>

DEFINE_int32(num_retries, 5, "number of retries for agent to start");
DEFINE_int32(wait_timeout, 15, "number of seconds to wait before retry");

namespace facebook::fboss {

namespace {
using TestTypes = ::testing::Types<std::false_type, std::true_type>;
}

template <typename T>
void AgentWrapperTest<T>::SetUp() {
  whoami_ = std::make_unique<AgentNetWhoAmI>();
  config_ = AgentConfig::fromFile("/etc/coop/agent/current");
  createDirectoryTree(util_.getWarmBootDir());
  if constexpr (T()) {
    createDirectoryTree(parentDirectoryTree(util_.getWrapperRefactorFlag()));
    touchFile(util_.getWrapperRefactorFlag());
  }
}

template <typename T>
void AgentWrapperTest<T>::TearDown() {
  stop();
  if constexpr (T()) {
    removeFile(util_.getWrapperRefactorFlag());
    removeDir(parentDirectoryTree(util_.getWrapperRefactorFlag()));
  }
  removeDir(util_.getWarmBootDir());
}

template <typename T>
void AgentWrapperTest<T>::start() {
  AgentCommandExecutor executor;
  executor.startService("wedge_agent");
}

template <typename T>
void AgentWrapperTest<T>::stop() {
  AgentCommandExecutor executor;
  executor.stopService("wedge_agent");
}

template <typename T>
void AgentWrapperTest<T>::wait(bool started) {
  if (started) {
    waitForStart();
  } else {
    waitForStop();
  }
}

template <typename T>
void AgentWrapperTest<T>::waitForStart(const std::string& unit) {
  WITH_RETRIES_N_TIMED(
      FLAGS_num_retries, std::chrono::seconds(FLAGS_wait_timeout), {
        facebook::tupperware::systemd::Service service{unit};
        auto status = service.getStatus();
        EXPECT_EVENTUALLY_EQ(
            status.value().serviceState,
            facebook::tupperware::systemd::ProcessStatus::ServiceState::
                RUNNING);
      });

  auto client = utils::createWedgeAgentClient();
  apache::thrift::RpcOptions options;
  options.setTimeout(std::chrono::seconds(1));
  WITH_RETRIES_N_TIMED(
      FLAGS_num_retries, std::chrono::seconds(FLAGS_wait_timeout), {
        SwitchRunState runState = SwitchRunState::UNINITIALIZED;
        try {
          runState = client->sync_getSwitchRunState(options);
        } catch (const std::exception& ex) {
          XLOG(INFO) << "Waiting for wedge agent to start";
          continue;
        }
        EXPECT_EVENTUALLY_EQ(runState, SwitchRunState::CONFIGURED);
      });
}

template <typename T>
void AgentWrapperTest<T>::waitForStart() {
  auto multiSwitch = config_->getRunMode() == cfg::AgentRunMode::MULTI_SWITCH;
  if (multiSwitch) {
    // TODO: wait for hw_agent as well
    waitForStart("fboss_sw_agent.service");
  } else {
    waitForStart("wedge_agent.service");
  }
}

template <typename T>
void AgentWrapperTest<T>::waitForStop(const std::string& unit, bool crash) {
  bool wrapperRefactored =
      checkFileExists(this->util_.getWrapperRefactorFlag());
  WITH_RETRIES_N_TIMED(
      FLAGS_num_retries, std::chrono::seconds(FLAGS_wait_timeout), {
        facebook::tupperware::systemd::Service service{unit};
        auto status = service.waitForExit(
            std::chrono::microseconds(FLAGS_wait_timeout * 1000000));
        EXPECT_EVENTUALLY_EQ(
            status.value().serviceState,
            facebook::tupperware::systemd::ProcessStatus::ServiceState::EXITED);
        if (crash) {
          if (wrapperRefactored) {
            EXPECT_EVENTUALLY_EQ(status.value().exitStatus, 134);
          } else {
            EXPECT_EVENTUALLY_EQ(status.value().exitStatus, 255);
          }
        }
      });
}

template <typename T>
void AgentWrapperTest<T>::waitForStop(bool crash) {
  auto multiSwitch = (config_->getRunMode() == cfg::AgentRunMode::MULTI_SWITCH);
  if (multiSwitch) {
    waitForStop("fboss_sw_agent.service", crash);
    // TODO: wait for hw_agent as well
  } else {
    waitForStop("wedge_agent.service", crash);
  }
}

TYPED_TEST_SUITE(AgentWrapperTest, TestTypes);

TYPED_TEST(AgentWrapperTest, ColdBootStartAndStop) {
  auto drainTimeFile = this->util_.getRoutingProtocolColdBootDrainTimeFile();
  std::vector<char> data = {'0', '5'};
  if (!this->whoami_->isNotDrainable() && !this->whoami_->isFdsw()) {
    touchFile(drainTimeFile);
    folly::writeFile(data, drainTimeFile.c_str());
  }
  touchFile(this->util_.getColdBootOnceFile());
  touchFile(this->util_.getUndrainedFlag());
  this->start();
  this->waitForStart();
  if (!this->whoami_->isNotDrainable() && !this->whoami_->isFdsw()) {
    // @lint-ignore CLANGTIDY
    EXPECT_FALSE(checkFileExists(drainTimeFile));
  }
  this->stop();
  this->waitForStop();
  removeFile(this->util_.getRoutingProtocolColdBootDrainTimeFile());
  removeFile(this->util_.getUndrainedFlag());
}

TYPED_TEST(AgentWrapperTest, StartAndStopAndStart) {
  touchFile(this->util_.getColdBootOnceFile());
  this->start();
  this->waitForStart();
  this->stop();
  this->waitForStop();
  this->start();
  this->waitForStart();
  this->stop();
  this->waitForStop();
}

TYPED_TEST(AgentWrapperTest, StartAndCrash) {
  this->start();
  this->waitForStart();
  touchFile(this->util_.sleepSwSwitchOnSigTermFile());
  std::vector<char> sleepTime = {'3', '0', '0'};
  folly::writeFile(sleepTime, this->util_.sleepSwSwitchOnSigTermFile().c_str());
  auto maxPostSignalWaitTime = this->util_.getMaxPostSignalWaitTimeFile();
  touchFile(maxPostSignalWaitTime);
  std::vector<char> data = {'1'};
  folly::writeFile(data, maxPostSignalWaitTime.c_str());
  this->stop();
  this->waitForStop(true /* expect sigabrt to crash */);
}

} // namespace facebook::fboss

#ifdef IS_OSS
FOLLY_INIT_LOGGING_CONFIG("DBG2; default:async=true");
#else
FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");
#endif

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);
  folly::Init init(&argc, &argv, true);

  // Run the tests
  return RUN_ALL_TESTS();
}
