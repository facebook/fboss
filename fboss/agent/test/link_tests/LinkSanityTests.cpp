// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fboss/mka_service/if/gen-cpp2/MKAService.h>
#include <folly/dynamic.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/json.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/mka_service/if/gen-cpp2/mka_config_constants.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

using namespace ::testing;
using namespace facebook::fboss;
using namespace facebook::fboss::mka;

// Tests that the link comes up after a flap on the ASIC
TEST_F(LinkTest, asicLinkFlap) {
  auto verify = [this]() {
    auto ports = getCabledPorts();
    // Set the port status on all cabled ports to false. The link should go down
    for (const auto& port : ports) {
      setPortStatus(port, false);
    }
    EXPECT_NO_THROW(waitForAllCabledPorts(false));

    // Set the port status on all cabled ports to true. The link should come
    // back up
    for (const auto& port : ports) {
      setPortStatus(port, true);
    }
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
  };

  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(LinkTest, getTransceivers) {
  auto allTransceiversPresent = [this]() {
    auto ports = getCabledPorts();
    // Set the port status on all cabled ports to false. The link should go
    // down
    for (const auto& port : ports) {
      auto transceiverId =
          platform()->getPlatformPort(port)->getTransceiverID().value();
      if (!platform()->getQsfpCache()->getIf(transceiverId)) {
        return false;
      }
    }
    return true;
  };

  verifyAcrossWarmBoots(
      []() {},
      [allTransceiversPresent, this]() {
        checkWithRetry(allTransceiversPresent);
      });
}

TEST_F(LinkTest, trafficRxTx) {
  auto packetsFlowingOnAllPorts = [this]() {
    sw()->getLldpMgr()->sendLldpOnAllPorts();
    return lldpNeighborsOnAllCabledPorts();
  };

  verifyAcrossWarmBoots(
      []() {},
      [packetsFlowingOnAllPorts, this]() {
        checkWithRetry(packetsFlowingOnAllPorts);
      });
}

TEST_F(LinkTest, warmbootIsHitLess) {
  // Create a L3 data plane flood and then assert that none of the
  // traffic bearing ports loss traffic.
  // TODO: Assert that all (non downlink) cabled ports get traffic.
  verifyAcrossWarmBoots(
      [this]() { createL3DataplaneFlood(); },
      [this]() {
        // Assert no traffic loss and no ecmp shrink. If ports flap
        // these conditions will not be true
        assertNoInDiscards();
        auto ecmpSizeInSw = getVlanOwningCabledPorts().size();
        EXPECT_EQ(
            utility::getEcmpSizeInHw(
                sw()->getHw(),
                {folly::IPAddress("::"), 0},
                RouterID(0),
                ecmpSizeInSw),
            ecmpSizeInSw);
      });
}

TEST_F(LinkTest, ptpEnableIsHitless) {
  createL3DataplaneFlood();
  assertNoInDiscards();
  sw()->updateStateBlocking("ptp enable", [](auto state) {
    auto newState = state->clone();
    auto switchSettings = newState->getSwitchSettings()->modify(&newState);
    EXPECT_FALSE(switchSettings->isPtpTcEnable());
    switchSettings->setPtpTcEnable(true);
    return newState;
  });

  // Assert no traffic loss and no ecmp shrink. If ports flap
  // these conditions will not be true
  assertNoInDiscards();
  auto ecmpSizeInSw = getVlanOwningCabledPorts().size();
  EXPECT_EQ(
      utility::getEcmpSizeInHw(
          sw()->getHw(),
          {folly::IPAddress("::"), 0},
          RouterID(0),
          ecmpSizeInSw),
      ecmpSizeInSw);
}

TEST_F(LinkTest, setupMkaSession) {
  auto verify = [this]() {
    checkWithRetry([this] { return lldpNeighborsOnAllCabledPorts(); });
    auto port = getCabledPorts()[0];
    std::optional<PortID> neighborPort;
    auto lldpNeighbor = sw()->getLldpMgr()->getDB()->getNeighbors(port)[0];
    for (auto port : *sw()->getState()->getPorts()) {
      if (port->getName() == lldpNeighbor.getPortId()) {
        neighborPort = port->getID();
        break;
      }
    }
    CHECK(neighborPort);
    XLOG(INFO) << " Port: " << port << " neighbor: " << *neighborPort;
#if FOLLY_HAS_COROUTINES
    auto evbThread = std::make_unique<folly::ScopedEventBaseThread>();
    std::unique_ptr<facebook::fboss::mka::MKAServiceAsyncClient> client;
    evbThread->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
        [&]() {
          folly::SocketAddress addr("::1", 5920);
          auto socket = folly::AsyncSocket::newSocket(
              folly::EventBaseManager::get()->getEventBase(), addr, 5000);
          auto channel = apache::thrift::RocketClientChannel::newChannel(
              std::move(socket));
          client =
              std::make_unique<facebook::fboss::mka::MKAServiceAsyncClient>(
                  std::move(channel));
        });

    SCOPE_EXIT {
      evbThread->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
          [&]() { client.reset(); });
      evbThread.reset();
    };
    auto ckn = "2b7e151628aed2a6abf7158809cf4f3c";
    auto priority = mka_config_constants::DEFAULT_KEYSERVER_PRIORITY();
    for (auto port : {port, *neighborPort}) {
      MKAConfig config;
      config.l2Port_ref() = folly::to<std::string>(port);
      config.transport_ref() = MKATransport::THRIFT_TRANSPORT;
      config.capability_ref() = MACSecCapability::CAPABILITY_INGTY_CONF;
      config.srcMac_ref() = sw()->getPlatform()->getLocalMac().toString();
      config.primaryCak_ref()->key_ref() = "135bd758b0ee5c11c55ff6ab19fdb199";
      config.primaryCak_ref()->ckn_ref() = ckn;
      // Different priorities to allow for key server election
      config.priority_ref() = priority--;
      auto result = folly::coro::blockingWait(client->co_provisionCAK(config));
      ASSERT_EQ(result, MKAResponse::SUCCESS);
    }
    checkWithRetry([&client, port, neighborPort, ckn]() {
      auto activeSessions =
          folly::coro::blockingWait(client->co_numActiveMKASessions());
      auto portCkn = folly::coro::blockingWait(
          client->co_getActiveCKN(folly::to<std::string>(port)));
      auto nbrCkn = folly::coro::blockingWait(
          client->co_getActiveCKN(folly::to<std::string>(*neighborPort)));
      XLOG(INFO) << " Active sessions: " << activeSessions
                 << " port ckn: " << portCkn << " nbr ckn: " << nbrCkn;
      return activeSessions == 2 && portCkn == nbrCkn && portCkn == ckn;
    });
#endif
  };

  verifyAcrossWarmBoots([]() {}, verify);
}
