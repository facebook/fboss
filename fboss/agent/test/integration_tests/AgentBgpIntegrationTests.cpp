/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

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
} // namespace

using namespace facebook::neteng::fboss::bgp::thrift;

namespace facebook::fboss {
class BgpIntegrationTest : public AgentIntegrationTest {
 protected:
  void checkBgpState() {
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kBgpThriftPort);
    auto client =
        servicerouter::cpp2::getClientFactory()
            .getSRClientUnique<TBgpServiceAsyncClient>("", clientParams);

    WITH_RETRIES({
      std::vector<TBgpSession> sessions;
      client->sync_getBgpSessions(sessions);
      EXPECT_EQ(sessions.size(), kNumBgpSessions);
      for (const auto& session : sessions) {
        auto peerId = session.peer()->peer_id();
        EXPECT_EVENTUALLY_EQ(
            session.peer()->peer_state(), TBgpPeerState::ESTABLISHED)
            << "Peer never established " << *peerId;
      }
    });
  }
};

TEST_F(BgpIntegrationTest, bgpSesionsEst) {
  auto verify = [&]() { checkBgpState(); };
  verifyAcrossWarmBoots(verify);
}
} // namespace facebook::fboss
