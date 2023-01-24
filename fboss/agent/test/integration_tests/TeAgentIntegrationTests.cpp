/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fb303/ServiceData.h>
#include <folly/Subprocess.h>
#include <neteng/ai/te_agent/if/gen-cpp2/TeAgentService.h>
#include <neteng/ai/te_agent/if/gen-cpp2/TeAgent_types.h>
#include "common/thrift/thrift/gen-cpp2/MonitorAsyncClient.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/integration_tests/AgentIntegrationTest.h"
#include "fboss/lib/CommonUtils.h"
#include "servicerouter/client/cpp2/ClientFactory.h"
#include "servicerouter/client/cpp2/ServiceRouter.h"

namespace {
static constexpr int kTeAgentThriftPort{2022};
} // namespace

using namespace facebook::neteng::ai::teagent;

using StateUpdateFn = facebook::fboss::SwSwitch::StateUpdateFn;

namespace facebook::fboss {
class TeAgentIntegrationTest : public AgentIntegrationTest {
 protected:
  uint64_t teAgentAliveSince() {
    uint64_t aliveSince{0};
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kTeAgentThriftPort);
    checkWithRetry(
        [&aliveSince, &clientParams]() {
          try {
            auto client =
                servicerouter::cpp2::getClientFactory()
                    .getSRClientUnique<facebook::thrift::MonitorAsyncClient>(
                        "", clientParams);
            aliveSince = client->sync_aliveSince();
            return true;
          } catch (const std::exception& e) {
            return false;
          }
        },
        60, /* retries */
        1s,
        "Cannot connect to TeAgent service");
    return aliveSince;
  }

  void checkAgentState() {
    auto aliveSince = teAgentAliveSince();
    EXPECT_NE(aliveSince, 0);
  }
};

TEST_F(TeAgentIntegrationTest, teAgentRunning) {
  auto verify = [&]() { checkAgentState(); };
  verifyAcrossWarmBoots(verify);
}

} // namespace facebook::fboss
