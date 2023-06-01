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
#include <memory>
#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/integration_tests/AgentIntegrationTest.h"
#include "fboss/fsdb/common/Flags.h"
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
using namespace facebook::neteng::fboss::bgp_attr;

using StateUpdateFn = facebook::fboss::SwSwitch::StateUpdateFn;

namespace facebook::fboss {
class BgpIntegrationTest : public AgentIntegrationTest {
 protected:
  void setCmdLineFlagOverrides() const override {
    FLAGS_publish_stats_to_fsdb = true;
    FLAGS_publish_state_to_fsdb = true;
    AgentIntegrationTest::setCmdLineFlagOverrides();
  }
  uint64_t bgpAliveSince() {
    uint64_t aliveSince{0};
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kBgpThriftPort);
    checkWithRetry(
        [&aliveSince, &clientParams]() {
          try {
            auto client = servicerouter::cpp2::getClientFactory()
                              .getSRClientUnique<TBgpServiceAsyncClient>(
                                  "", clientParams);
            aliveSince = client->sync_aliveSince();
            return true;
          } catch (const std::exception& e) {
            return false;
          }
        },
        60, /* retries */
        1s,
        "Cannot connect to BGPD service");
    return aliveSince;
  }
  void checkBgpState(
      TBgpPeerState state,
      std::set<std::string> sessionsToCheck,
      int retries = kMaxRetries) {
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kBgpThriftPort);
    auto client =
        servicerouter::cpp2::getClientFactory()
            .getSRClientUnique<TBgpServiceAsyncClient>("", clientParams);

    WITH_RETRIES_N(retries, {
      std::vector<TBgpSession> sessions;
      try {
        client->sync_getBgpSessions(sessions);
      } catch (const std::exception& e) {
        XLOG(DBG3) << "checkBgpState: (transient) exception: " << e.what();
        continue;
      }
      EXPECT_EQ(sessions.size(), kNumBgpSessions);
      for (const auto& session : sessions) {
        if (sessionsToCheck.count(session.my_addr().value())) {
          EXPECT_EVENTUALLY_EQ(session.peer()->peer_state(), state)
              << "Peer never reached state "
              << apache::thrift::util::enumNameSafe(TBgpPeerState(state))
              << " for " << session.my_addr().value();
        }
      }
    });
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
      if (syncFibCounter.has_value()) {
        EXPECT_NE(syncFibCounter.value(), 0);
      }
    });

    // Check that bgp produced route exists
    checkRoute(folly::IPAddressV6(kTestPrefix), kTestPrefixLength, true);
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw()->updateStateBlocking(name, func);
  }

  void setPortState(PortID port, bool up) {
    updateState(up ? "set port up" : "set port down", [&](const auto& state) {
      auto newState = state->clone();
      auto newPort = newState->getPorts()->getNodeIf(port)->modify(&newState);
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

  void addRoute(folly::IPAddress dest, int length, folly::IPAddress nhop) {
    ThriftHandler handler(sw());
    auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
    auto nr = std::make_unique<UnicastRoute>();
    *nr->dest()->ip() = network::toBinaryAddress(dest);
    *nr->dest()->prefixLength() = length;
    nr->nextHopAddrs()->push_back(network::toBinaryAddress(nhop));
    handler.addUnicastRoute(bgpClient, std::move(nr));
  }

  void restartBgpService() {
    auto prevAliveSince = bgpAliveSince();
    auto constexpr kBgpServiceKill =
        "sudo systemctl restart bgpd_service_for_testing";
    folly::Subprocess(kBgpServiceKill).waitChecked();
    auto aliveSince = bgpAliveSince();
    EXPECT_NE(aliveSince, 0);
    EXPECT_NE(aliveSince, prevAliveSince);
  }

  template <typename TIpAddress>
  void checkRoute(TIpAddress prefix, uint8_t length, bool exists) {
    WITH_RETRIES({
      const auto& fibContainer =
          sw()->getState()->getFibs()->getNode(RouterID(0));
      auto fib = fibContainer->template getFib<TIpAddress>();
      auto testRoute = fib->getRouteIf({prefix, length});
      if (exists) {
        EXPECT_EVENTUALLY_NE(testRoute, nullptr);
        if (testRoute) {
          EXPECT_TRUE(testRoute->getEntryForClient(ClientID::BGPD));
        }
      } else {
        EXPECT_EVENTUALLY_EQ(testRoute, nullptr);
      }
    });
  }

  TIpPrefix makePrefix(folly::IPAddress prefix, uint8_t length) {
    TIpPrefix tprefix;
    tprefix.afi_ref() = TBgpAfi::AFI_IPV6;
    tprefix.num_bits_ref() = length;
    tprefix.prefix_bin_ref() =
        network::toBinaryAddress(prefix).addr_ref()->toStdString();
    return tprefix;
  }

  void addBgpUnicastRoutes(std::vector<facebook::fboss::UnicastRoute> toAdd) {
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kBgpThriftPort);

    std::chrono::milliseconds processingTimeout{60000};
    clientParams.setProcessingTimeoutMs(processingTimeout);
    clientParams.setOverallTimeoutMs(processingTimeout);

    auto client = servicerouter::cpp2::getClientFactory()
                      .getSRClientUnique<apache::thrift::Client<TBgpService>>(
                          "", clientParams);

    auto nhPrefix = makePrefix(folly::IPAddress("2::"), 128);
    std::map<TIpPrefix, TBgpAttributes> networks;
    for (const auto& rt : toAdd) {
      TBgpAttributes attributes;
      attributes.communities() = std::vector<TBgpCommunity>();
      attributes.nexthop() = nhPrefix;
      attributes.install_to_fib() = true;

      auto tPrefix = makePrefix(
          facebook::network::toIPAddress(*rt.dest()->ip()),
          *rt.dest()->prefixLength());

      networks[tPrefix] = attributes;
    }
    client->sync_addNetworks(std::move(networks));
  }

  template <typename RouteScaleGeneratorT>
  void runRouteScaleTest() {
    auto swstate = sw()->getState();
    utility::RouteDistributionGenerator::ThriftRouteChunks routeChunks;
    auto routeDistributionGen = RouteScaleGeneratorT(swstate);
    routeChunks = routeDistributionGen.getThriftRoutes();

    auto setup = [&]() {
      checkBgpState(TBgpPeerState::ESTABLISHED, {"1::", "2::"});
      checkAgentState();
      int total = 0;
      XLOG(DBG2) << "routeScaleTest: route add: start";
      for (const auto& routeChunk : routeChunks) {
        addBgpUnicastRoutes(routeChunk);
        total += routeChunk.size();
      }
      XLOG(DBG2) << "routeScaleTest: route add: done, total routes: " << total;
    };
    auto verify = [&]() {
      XLOG(DBG2) << "routeScaleTest: verify: start";
      for (const auto& routeChunk : routeChunks) {
        std::for_each(
            routeChunk.begin(), routeChunk.end(), [&](const auto& route) {
              auto ip = *route.dest()->ip();
              auto addr = folly::IPAddress::fromBinary(folly::ByteRange(
                  reinterpret_cast<const unsigned char*>(ip.addr()->data()),
                  ip.addr()->size()));
              if (addr.isV6()) {
                checkRoute(addr.asV6(), *route.dest()->prefixLength(), true);
              } else {
                checkRoute(addr.asV4(), *route.dest()->prefixLength(), true);
              }
            });
      }
      XLOG(DBG2) << "routeScaleTest: verify: done";
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void addBgpRoutes(std::vector<std::pair<folly::IPAddress, uint8_t>> toAdd) {
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kBgpThriftPort);
    auto client =
        servicerouter::cpp2::getClientFactory()
            .getSRClientUnique<TBgpServiceAsyncClient>("", clientParams);
    auto nhPrefix = makePrefix(folly::IPAddress("2::"), 128);
    std::map<TIpPrefix, TBgpAttributes> networks;
    for (const auto& entry : toAdd) {
      auto tPrefix = makePrefix(entry.first, entry.second);
      TBgpAttributes attributes;
      auto tBgpCommunities = std::vector<TBgpCommunity>();
      attributes.nexthop() = nhPrefix;
      attributes.communities() = tBgpCommunities;
      attributes.install_to_fib() = true;
      networks[tPrefix] = attributes;
    }
    client->sync_addNetworks(std::move(networks));
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

TEST_F(BgpIntegrationTest, bgpRestart) {
  auto verify = [&]() {
    checkBgpState(TBgpPeerState::ESTABLISHED, {"1::", "2::"});
    checkAgentState();
    // add a route with clientID bgp
    addRoute(folly::IPAddress("101::"), 120, folly::IPAddress("2::"));
    checkRoute(folly::IPAddressV6("101::"), 120, true);
    restartBgpService();
    // SyncFib after restart should erase the route
    checkRoute(folly::IPAddressV6("101::"), 120, false);
    // bgp config route should be present
    checkBgpState(TBgpPeerState::ESTABLISHED, {"1::", "2::"});
    checkRoute(folly::IPAddressV6(kTestPrefix), kTestPrefixLength, true);
  };
  verifyAcrossWarmBoots(verify);
}

TEST_F(BgpIntegrationTest, routeScaleTest) {
  Platform* platform = sw()->getPlatform_DEPRECATED();
  auto hw = platform->getAsic();
  auto npu = hw->getAsicType();
  switch (npu) {
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
      runRouteScaleTest<utility::FSWRouteScaleGenerator>();
      break;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
      runRouteScaleTest<utility::HgridUuRouteScaleGenerator>();
      break;
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
      runRouteScaleTest<utility::RSWRouteScaleGenerator>();
      break;
    default:
      break;
  }
}

TEST_F(BgpIntegrationTest, addBgpRoute) {
  auto setup = [&]() {
    checkBgpState(TBgpPeerState::ESTABLISHED, {"1::", "2::"});
    checkAgentState();
    addBgpRoutes({{folly::IPAddress("102::"), 120}});
  };
  auto verify = [&]() { checkRoute(folly::IPAddressV6("102::"), 120, true); };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
