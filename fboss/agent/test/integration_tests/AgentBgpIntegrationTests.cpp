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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/integration_tests/AgentIntegrationTest.h"
#include "fboss/lib/CommonUtils.h"
#include "neteng/fboss/bgp/cpp/BgpServiceUtil.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "servicerouter/client/cpp2/ClientFactory.h"
#include "servicerouter/client/cpp2/ServiceRouter.h"

namespace {
static constexpr int kNumBgpSessions{4}; // ipv4 and ipv6 with one for each end
static constexpr int kBgpThriftPort{6909};
auto constexpr kMaxRetries = 30;
auto constexpr kTestPrefix = "100::";
auto constexpr kTestPrefixLength = 120;
} // namespace

using namespace facebook::neteng::fboss::bgp::thrift;

using StateUpdateFn = facebook::fboss::SwSwitch::StateUpdateFn;

namespace facebook::fboss {
class BgpIntegrationTest : public AgentIntegrationTest {
 protected:
  void checkBgpState(
      TBgpPeerState state,
      std::set<std::string> sessionsToCheck) {
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kBgpThriftPort);
    auto client =
        servicerouter::cpp2::getClientFactory()
            .getSRClientUnique<TBgpServiceAsyncClient>("", clientParams);

    WITH_RETRIES_N(
        {
          std::vector<TBgpSession> sessions;
          client->sync_getBgpSessions(sessions);
          EXPECT_EQ(sessions.size(), kNumBgpSessions);
          for (const auto& session : sessions) {
            if (sessionsToCheck.count(session.my_addr().value())) {
              EXPECT_EVENTUALLY_EQ(session.peer()->peer_state(), state)
                  << "Peer never reached state "
                  << apache::thrift::util::enumNameSafe(TBgpPeerState(state))
                  << " for " << session.my_addr().value();
            }
          }
        },
        kMaxRetries);
  }

  void checkAgentState() {
    WITH_RETRIES({
      auto syncFibCounterName =
          platform()->getHwSwitch()->getBootType() == BootType::WARM_BOOT
          ? "warm_boot.fib_synced_bgp.stage_duration_ms"
          : "cold_boot.fib_synced_bgp.stage_duration_ms";
      auto syncFibCounter =
          fb303::fbData->getCounterIfExists(syncFibCounterName);
      EXPECT_EVENTUALLY_TRUE(syncFibCounter.has_value());
      EXPECT_NE(syncFibCounter.value(), 0);
    });

    // Check that bgp produced route exists
    const auto& fibContainer =
        sw()->getState()->getFibs()->getFibContainer(RouterID(0));
    auto fib = fibContainer->template getFib<folly::IPAddressV6>();
    auto testRoute =
        fib->getRouteIf({folly::IPAddressV6(kTestPrefix), kTestPrefixLength});
    EXPECT_NE(testRoute, nullptr);
    EXPECT_TRUE(testRoute->getEntryForClient(ClientID::BGPD));
  }
};

TEST_F(BgpIntegrationTest, bgpSesionsEst) {
  auto verify = [&]() {
    checkBgpState(TBgpPeerState::ESTABLISHED, {"1::", "2::"});
    checkAgentState();
  };
  verifyAcrossWarmBoots(verify);
}
} // namespace facebook::fboss
