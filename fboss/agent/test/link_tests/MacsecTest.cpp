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
#include "fboss/lib/CommonUtils.h"
#include "fboss/mka_service/if/gen-cpp2/mka_config_constants.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"
#include "fboss/qsfp_service/lib/QsfpClient.h"

using namespace ::testing;
using namespace facebook::fboss;
using namespace facebook::fboss::mka;

class MacsecTest : public LinkTest {
 public:
  void SetUp() override;
  void TearDown() override;
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
    portsFut.wait();
    return macsecPorts;
  }
  static std::string getCkn() {
    return "2b7e151628aed2a6abf7158809cf4f3c";
  }
  static std::string getCak() {
    return "135bd758b0ee5c11c55ff6ab19fdb199";
  }
  static std::string getCkn2() {
    return "1b7e151628aed2a6abf7158809cf4f3c";
  }
  static std::string getCak2() {
    return "235bd758b0ee5c11c55ff6ab19fdb199";
  }
  void programCak(
      const std::set<std::pair<PortID, PortID>>& macsecPorts,
      const std::string& cak = getCak(),
      const std::string& ckn = getCkn()) {
#if FOLLY_HAS_COROUTINES
    for (auto portAndNeighbor : macsecPorts) {
      auto port = portAndNeighbor.first;
      auto neighborPort = portAndNeighbor.second;
      XLOG(INFO) << " Port: " << port << " neighbor: " << neighborPort;
      auto priority = mka_config_constants::DEFAULT_KEYSERVER_PRIORITY();
      auto srcMac = sw()->getPlatform()->getLocalMac().u64NBO();
      auto macGen = facebook::fboss::utility::MacAddressGenerator();
      for (auto p : {port, neighborPort}) {
        MKAConfig config;
        config.l2Port_ref() = folly::to<std::string>(p);
        config.transport_ref() = MKATransport::THRIFT_TRANSPORT;
        config.capability_ref() = MACSecCapability::CAPABILITY_INGTY_CONF;
        config.srcMac_ref() = macGen.get(srcMac++).toString();
        config.primaryCak_ref()->key_ref() = cak;
        config.primaryCak_ref()->ckn_ref() = ckn;
        // Different priorities to allow for key server election
        config.priority_ref() = priority--;
        auto result =
            folly::coro::blockingWait(client_->co_provisionCAK(config));
        ASSERT_EQ(result, MKAResponse::SUCCESS);
      }
    }
    checkWithRetry([this, ckn, macsecPorts]() {
      for (auto portAndNeighbor : macsecPorts) {
        auto port = portAndNeighbor.first;
        auto neighborPort = portAndNeighbor.second;
        for (auto p : {port, neighborPort}) {
          auto portCkn = folly::coro::blockingWait(
              client_->co_getActiveCKN(folly::to<std::string>(p)));
          auto sakInstalled = folly::coro::blockingWait(
              client_->co_isSAKInstalled(folly::to<std::string>(p)));
          XLOG(INFO) << " Port: " << p << " ckn: " << portCkn
                     << " SAK installed: " << sakInstalled;
          if (portCkn != ckn || !sakInstalled) {
            return false;
          }
        }
      }
      return true;
    });
#endif
  }
  void programCakAndCheckLoss(
      const std::set<std::pair<PortID, PortID>>& macsecPorts,
      const std::string& cak,
      const std::string& ckn) {
    programCak(macsecPorts, cak, ckn);
    assertNoInDiscards();
  }

  std::vector<std::pair<PortID, PortID>> getConnectedMacsecCapablePairs()
      const {
    auto macsecCapablePorts = getMacsecCapablePorts();
    std::vector<std::pair<PortID, PortID>> connectedMacsecCapablePairs;
    for (auto& connectedPair : getConnectedPairs()) {
      if (macsecCapablePorts.find(connectedPair.first) !=
              macsecCapablePorts.end() &&
          macsecCapablePorts.find(connectedPair.second) !=
              macsecCapablePorts.end()) {
        connectedMacsecCapablePairs.push_back(connectedPair);
      }
    }
    return connectedMacsecCapablePairs;
  }

 protected:
  std::unique_ptr<folly::ScopedEventBaseThread> evbThread_;
  std::unique_ptr<facebook::fboss::mka::MKAServiceAsyncClient> client_;
};

void MacsecTest::SetUp() {
  LinkTest::SetUp();
  checkWithRetry([this] { return lldpNeighborsOnAllCabledPorts(); });
  evbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
  evbThread_->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [&]() {
        folly::SocketAddress addr("::1", 5920);
        auto socket = folly::AsyncSocket::newSocket(
            folly::EventBaseManager::get()->getEventBase(), addr, 5000);
        auto channel =
            apache::thrift::RocketClientChannel::newChannel(std::move(socket));
        client_ = std::make_unique<facebook::fboss::mka::MKAServiceAsyncClient>(
            std::move(channel));
      });
}

void MacsecTest::TearDown() {
  evbThread_->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [&]() { client_.reset(); });
  evbThread_.reset();
  LinkTest::TearDown();
}

TEST_F(MacsecTest, setupMkaSession) {
  auto verify = [this]() {
    auto connectedMacsecCapablePairs = getConnectedMacsecCapablePairs();
    ASSERT_GT(connectedMacsecCapablePairs.size(), 1);
    // TODO - program all pairs simultaneously rather than one by one.
    // Today there is a bug which causes packet mka sessions to flap
    // if we program all sessions back to back. Need to debug this.
    for (auto portAndNeighbor : connectedMacsecCapablePairs) {
      programCak({portAndNeighbor});
    }
  };

  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(MacsecTest, programCakIsHitless) {
  auto macsecCapablePortPair = *getConnectedMacsecCapablePairs().begin();
  auto setup = [this, macsecCapablePortPair]() {
    createL3DataplaneFlood(
        {PortDescriptor(macsecCapablePortPair.first),
         PortDescriptor(macsecCapablePortPair.second)});
    assertNoInDiscards();
  };
  auto verify = [this, macsecCapablePortPair]() {
    for (auto i = 0; i < 10; ++i) {
      // - All except the first program should be noop
      // - All programCAKs should be hitless
      programCakAndCheckLoss({macsecCapablePortPair}, getCak(), getCkn());
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(MacsecTest, rotateCakIsHitless) {
  auto macsecCapablePortPair = *getConnectedMacsecCapablePairs().begin();
  auto setup = [this, macsecCapablePortPair]() {
    createL3DataplaneFlood(
        {PortDescriptor(macsecCapablePortPair.first),
         PortDescriptor(macsecCapablePortPair.second)});
    assertNoInDiscards();
  };
  auto verify = [this, macsecCapablePortPair]() {
    programCakAndCheckLoss({macsecCapablePortPair}, getCak(), getCkn());
    programCakAndCheckLoss({macsecCapablePortPair}, getCak2(), getCkn2());
  };

  verifyAcrossWarmBoots(setup, verify);
}
