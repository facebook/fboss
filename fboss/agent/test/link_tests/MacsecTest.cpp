// Copyright 2004-present Facebook. All Rights Reserved.

#include <fboss/mka_service/if/gen-cpp2/MKAService.h>
#include <folly/dynamic.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/json.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/mka_service/if/gen-cpp2/mka_config_constants.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"
#include "fboss/qsfp_service/lib/QsfpClient.h"

using namespace ::testing;
using namespace facebook::fboss;
using namespace facebook::fboss::mka;

class MacsecTest : public LinkTest {
 public:
  std::set<PortID> getMacsecCapablePorts() const {
    folly::ScopedEventBaseThread evbThread;
    std::set<PortID> macsecPorts;
    auto portsFut = QsfpClient::createClient(evbThread.getEventBase())
                        .thenValue([](auto&& client) {
                          return client->future_getMacsecCapablePorts();
                        })
                        .thenValue([&macsecPorts](auto&& portIds) {
                          for (auto port : portIds) {
                            macsecPorts.insert(PortID(port));
                          }
                        });
    return macsecPorts;
  }
};

TEST_F(MacsecTest, setupMkaSession) {
  auto verify = [this]() {
    checkWithRetry([this] { return lldpNeighborsOnAllCabledPorts(); });
    auto macsecCapablePorts = getMacsecCapablePorts();
    ASSERT_GT(macsecCapablePorts.size(), 1);
    std::vector<std::pair<PortID, PortID>> connectedMacsecCapablePairs;
    for (auto& connectedPair : getConnectedPairs()) {
      if (macsecCapablePorts.find(connectedPair.first) !=
              macsecCapablePorts.end() &&
          macsecCapablePorts.find(connectedPair.second) !=
              macsecCapablePorts.end()) {
        connectedMacsecCapablePairs.push_back(connectedPair);
      }
    }
    ASSERT_GT(connectedMacsecCapablePairs.size(), 1);
    auto port = connectedMacsecCapablePairs.begin()->first;
    auto neighborPort = connectedMacsecCapablePairs.begin()->second;
    XLOG(INFO) << " Port: " << port << " neighbor: " << neighborPort;
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
    auto srcMac = sw()->getPlatform()->getLocalMac().u64NBO();
    auto macGen = facebook::fboss::utility::MacAddressGenerator();
    for (auto p : {port, neighborPort}) {
      MKAConfig config;
      config.l2Port_ref() = folly::to<std::string>(p);
      config.transport_ref() = MKATransport::THRIFT_TRANSPORT;
      config.capability_ref() = MACSecCapability::CAPABILITY_INGTY_CONF;
      config.srcMac_ref() = macGen.get(srcMac++).toString();
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
      XLOG(INFO) << " Active sessions: " << activeSessions;
      if (activeSessions != 2) {
        return false;
      }
      for (auto p : {port, neighborPort}) {
        auto portCkn = folly::coro::blockingWait(
            client->co_getActiveCKN(folly::to<std::string>(p)));
        auto sakInstalled = folly::coro::blockingWait(
            client->co_isSAKInstalled(folly::to<std::string>(p)));
        XLOG(INFO) << " port ckn: " << portCkn
                   << " SAK installed: " << sakInstalled;
        if (portCkn != ckn || !sakInstalled) {
          return false;
        }
      }
      return true;
    });
#endif
  };

  verifyAcrossWarmBoots([]() {}, verify);
}
