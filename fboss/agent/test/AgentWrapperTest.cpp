// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentWrapperTest.h"

#include "fboss/agent/AgentCommandExecutor.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "tupperware/agent/system/systemd/Service.h"

#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>

#include "fboss/lib/CommonUtils.h"

DEFINE_int32(num_retries, 5, "number of retries for agent to start");
DEFINE_int32(wait_timeout, 5, "number of seconds to wait before retry");

namespace facebook::fboss {

void AgentWrapperTest::SetUp() {}

void AgentWrapperTest::TearDown() {}

void AgentWrapperTest::start() {
  AgentCommandExecutor executor;
  executor.startService("wedge_agent");
}

void AgentWrapperTest::stop() {
  AgentCommandExecutor executor;
  executor.stopService("wedge_agent");
}

void AgentWrapperTest::wait(bool started) {
  if (started) {
    waitForStart();
  } else {
    waitForStop();
  }
}

void AgentWrapperTest::waitForStart() {
  WITH_RETRIES_N_TIMED(
      FLAGS_num_retries, std::chrono::seconds(FLAGS_wait_timeout), {
        facebook::tupperware::systemd::Service service{"wedge_agent.service"};
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

void AgentWrapperTest::waitForStop() {
  WITH_RETRIES_N_TIMED(
      FLAGS_num_retries, std::chrono::seconds(FLAGS_wait_timeout), {
        facebook::tupperware::systemd::Service service{"wedge_agent.service"};
        auto status = service.waitForExit(
            std::chrono::microseconds(FLAGS_wait_timeout * 1000000));
        EXPECT_EVENTUALLY_EQ(
            status.value().serviceState,
            facebook::tupperware::systemd::ProcessStatus::ServiceState::EXITED);
      });
}

TEST_F(AgentWrapperTest, StartAndStop) {
  start();
  waitForStart();
  stop();
  waitForStop();
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
