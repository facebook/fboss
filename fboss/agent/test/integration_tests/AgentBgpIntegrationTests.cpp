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
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/SwitchState.h"
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
auto constexpr kMaxFastNotificationRetries = 2;
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
      std::set<std::string> sessionsToCheck,
      int retries = kMaxRetries) {
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
        retries);
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

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw()->updateStateBlocking(name, func);
  }

  void setPortState(PortID port, bool up) {
    updateState(up ? "set port up" : "set port down", [&](const auto& state) {
      auto newState = state->clone();
      auto newPort = newState->getPorts()->getPort(port)->modify(&newState);
      newPort->setLoopbackMode(
          up ? cfg::PortLoopbackMode::MAC : cfg::PortLoopbackMode::NONE);
      newPort->setAdminState(
          up ? cfg::PortState::ENABLED : cfg::PortState::DISABLED);
      return newState;
    });
  }

  void addNDPEntry(
      folly::IPAddressV6 srcIpV6,
      folly::IPAddressV6 dstIpV6,
      PortDescriptor portDesc,
      VlanID vlan) {
    auto srcMac = utility::kLocalCpuMac(), dstMac = srcMac;
    sw()->getIPv6Handler()->sendNeighborAdvertisement(
        vlan, srcMac, srcIpV6, dstMac, dstIpV6, portDesc);
  }
};

TEST_F(BgpIntegrationTest, bgpSesionsEst) {
  auto verify = [&]() {
    checkBgpState(TBgpPeerState::ESTABLISHED, {"1::", "2::"});
    checkAgentState();
  };
  verifyAcrossWarmBoots(verify);
}

TEST_F(BgpIntegrationTest, bgpNotifyPortDown) {
  auto verify = [&]() {
    // Bring the port up and verify
    setPortState(masterLogicalPortIds()[0], true);
    setPortState(masterLogicalPortIds()[1], true);
    checkBgpState(TBgpPeerState::ESTABLISHED, {"1::", "2::"});
    checkAgentState();
    // add an NDP entres to trigger update to bgp on flush
    addNDPEntry(
        folly::IPAddressV6("1::"),
        folly::IPAddressV6("1::"),
        PortDescriptor(masterLogicalPortIds()[0]),
        VlanID(utility::kBaseVlanId));
    addNDPEntry(
        folly::IPAddressV6("2::"),
        folly::IPAddressV6("2::"),
        PortDescriptor(masterLogicalPortIds()[1]),
        VlanID(utility::kBaseVlanId + 1));
    // Bring down a port and ensure that BGP session goes idle
    setPortState(masterLogicalPortIds()[0], false);
    setPortState(masterLogicalPortIds()[1], false);
    // BGP session should transition to idle through fast notification
    checkBgpState(
        TBgpPeerState::IDLE, {"1::", "2::"}, kMaxFastNotificationRetries);
  };
  verifyAcrossWarmBoots(verify);
}
} // namespace facebook::fboss
